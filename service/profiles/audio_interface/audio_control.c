/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#define LOG_TAG "audio_control"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "audio_control.h"
#include "audio_transport.h"
#include "bt_config.h"
#include "bt_profile.h"
#include "bt_utils.h"
#include "hfp_ag_service.h"
#include "hfp_hf_service.h"
#include "service_loop.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_A2DP
#include "a2dp_control.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CTRL_EVT_HEADER_LEN 1

/****************************************************************************
 * Private Data
 ****************************************************************************/
static audio_transport_t* g_audio_ctrl_transport;

static const char* audio_cmd_to_string(audio_ctrl_cmd_t cmd)
{
    switch (cmd) {
        CASE_RETURN_STR(AUDIO_CTRL_CMD_START)
        CASE_RETURN_STR(AUDIO_CTRL_CMD_STOP)
        CASE_RETURN_STR(AUDIO_CTRL_CMD_CONFIG_DONE)
    default:
        return "UNKNOWN_CMD";
    }
}

static const char* audio_event_to_string(audio_ctrl_evt_t event)
{
    switch (event) {
        CASE_RETURN_STR(AUDIO_CTRL_EVT_STARTED)
        CASE_RETURN_STR(AUDIO_CTRL_EVT_START_FAIL)
        CASE_RETURN_STR(AUDIO_CTRL_EVT_STOPPED)
        CASE_RETURN_STR(AUDIO_CTRL_EVT_UPDATE_CONFIG)
    default:
        return "UNKNOWN_EVENT";
    }
}

static void audio_ctrl_event_with_data(uint8_t ch_id, audio_ctrl_evt_t event, uint8_t* data, uint8_t data_len)
{
    uint8_t stream[128];
    uint8_t* p = stream;

    BT_LOGD("%s, event:%s", __func__, audio_event_to_string(event));

    UINT8_TO_STREAM(p, event);
    if (data_len) {
        ARRAY_TO_STREAM(p, data, data_len);
    }

    if (g_audio_ctrl_transport != NULL) {
        audio_transport_write(g_audio_ctrl_transport, ch_id, stream, data_len + CTRL_EVT_HEADER_LEN, NULL);
    }
}

void audio_ctrl_send_control_event(uint8_t profile_id, audio_ctrl_evt_t evt)
{
    switch (profile_id) {
    case PROFILE_HFP_AG:
    case PROFILE_HFP_HF:
        audio_ctrl_event_with_data(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_HFP_CTRL, evt, NULL, 0);
        break;
    case PROFILE_A2DP:
#ifdef CONFIG_BLUETOOTH_A2DP
        a2dp_control_event(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_SOURCE_CTRL, evt);
#endif
        /**
         * TODO: replace a2dp_control_event() by audio_ctrl_event_with_data().
         *
         * Currently the A2DP control channel is recorded by a2dp_transport rather than g_audio_ctrl_transport.
         * It's ineffective to send the message via audio_ctrl_event_with_data().
         */
        break;
    default:
        BT_LOGW("%s, unknown profile id: %d", __func__, profile_id);
        /* Use a default control channel, currently via CONFIG_BLUETOOTH_AUDIO_TRANS_ID_HFP_CTRL */
        audio_ctrl_event_with_data(CONFIG_BLUETOOTH_AUDIO_TRANS_ID_HFP_CTRL, evt, NULL, 0);
        break;
    }
}

static void audio_recv_ctrl_data(uint8_t ch_id, audio_ctrl_cmd_t cmd)
{
    BT_LOGD("%s: audio-ctrl-cmd : %s", __func__, audio_cmd_to_string(cmd));

    switch (cmd) {
    case AUDIO_CTRL_CMD_START:
#ifdef CONFIG_BLUETOOTH_HFP_HF
        if (hfp_hf_on_sco_start())
            break;
#endif
#ifdef CONFIG_BLUETOOTH_HFP_AG
        if (hfp_ag_on_sco_start())
            break;
#endif
        /* TODO: Parse the payload to determine an active profile */
        BT_LOGD("%s: active profile not found", __func__);
        audio_ctrl_send_control_event(PROFILE_MAX, AUDIO_CTRL_EVT_START_FAIL);
        break;
    case AUDIO_CTRL_CMD_STOP:
#ifdef CONFIG_BLUETOOTH_HFP_HF
        if (hfp_hf_on_sco_stop())
            break;
#endif
#ifdef CONFIG_BLUETOOTH_HFP_AG
        if (hfp_ag_on_sco_stop())
            break;
#endif
        /* TODO: Parse the payload to determine an active profile */
        BT_LOGD("%s: active profile not found", __func__);
        audio_ctrl_send_control_event(PROFILE_MAX, AUDIO_CTRL_EVT_STOPPED);
        break;
    default:
        BT_LOGD("%s: UNSUPPORTED CMD (%d)", __func__, cmd);
        break;
    }
}

static void audio_ctrl_buffer_alloc(uint8_t ch_id, uint8_t** buffer, size_t* len)
{
    *len = 128;
    *buffer = malloc(*len);
}

static void audio_ctrl_data_received(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    audio_ctrl_cmd_t cmd;
    uint8_t* pbuf = buffer;

    if (!g_audio_ctrl_transport)
        goto free_out;

    if (len < 0) {
        audio_transport_read_stop(g_audio_ctrl_transport, ch_id);
    }

    if (len <= 0)
        goto free_out;

    while (len) {
        /* get cmd code*/
        STREAM_TO_UINT8(cmd, pbuf);
        len--;
        /* process cmd*/
        audio_recv_ctrl_data(ch_id, cmd);
    }

free_out:
    /* free the buffer alloced by a2dp_ctrl_buffer_alloc */
    free(buffer);
}

static void audio_ctrl_start(void)
{
    if (!g_audio_ctrl_transport)
        return;

    /* TODO: check which profile to start */
    audio_transport_read_start(g_audio_ctrl_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_HFP_CTRL,
        audio_ctrl_buffer_alloc, audio_ctrl_data_received);
}

static void audio_ctrl_stop(void)
{
    if (!g_audio_ctrl_transport)
        return;

    /* TODO: check which profile to stop */
    audio_transport_read_stop(g_audio_ctrl_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_HFP_CTRL);
}

static void audio_ctrl_cb(uint8_t ch_id, audio_transport_event_t event)
{
    BT_LOGD("%s, ch_id:%d, event:%s", __func__, ch_id, audio_transport_dump_event(event));

    if (ch_id != CONFIG_BLUETOOTH_AUDIO_TRANS_ID_HFP_CTRL) {
        /* TODO: support other profiles */
        BT_LOGE("fail, ch_id:%d", ch_id);
        return;
    }

    switch (event) {
    case TRANSPORT_OPEN_EVT:
        audio_ctrl_start();
        break;

    case TRANSPORT_CLOSE_EVT:
        audio_ctrl_stop();
        break;

    default:
        BT_LOGD("%s: ### EVENT %d NOT HANDLED ###", __func__, event);
        break;
    }
}

bt_status_t audio_ctrl_init(uint8_t profile_id)
{
    switch (profile_id) {
    case PROFILE_HFP_AG:
    case PROFILE_HFP_HF:
        if (g_audio_ctrl_transport == NULL) {
            g_audio_ctrl_transport = audio_transport_init(get_service_uv_loop());
        }
        if (!audio_transport_open(g_audio_ctrl_transport, CONFIG_BLUETOOTH_AUDIO_TRANS_ID_HFP_CTRL,
                CONFIG_BLUETOOTH_SCO_CTRL_PATH, audio_ctrl_cb)) {
            BT_LOGE("fail to open audio transport");
            return BT_STATUS_FAIL;
        }
        break;
    default:
        BT_LOGW("%s, unknown profile id: %d", __func__, profile_id);
        break;
    }

    return BT_STATUS_SUCCESS;
}

void audio_ctrl_cleanup(uint8_t profile_id)
{
    switch (profile_id) {
    case PROFILE_HFP_AG:
    case PROFILE_HFP_HF:
        if (g_audio_ctrl_transport) {
            audio_transport_close(g_audio_ctrl_transport, AUDIO_TRANS_CH_ID_ALL);
        }
        break;
    default:
        BT_LOGW("%s, unknown profile id: %d", __func__, profile_id);
        break;
    }

    /* TODO: close audio transport when all profile closed */
    g_audio_ctrl_transport = NULL;
}
