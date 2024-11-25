/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "service_loop.h"

#include "a2dp_codec.h"
#include "a2dp_control.h"
#include "a2dp_sink.h"
#include "a2dp_source.h"
#include "a2dp_source_audio.h"
#include "audio_transport.h"

#include "bt_utils.h"
#define LOG_TAG "a2dp_control"
#include "utils/log.h"

audio_transport_t* a2dp_transport = NULL;

static const char* audio_transport_path[] = {
    CONFIG_BLUETOOTH_A2DP_SOURCE_CTRL_PATH,
    CONFIG_BLUETOOTH_A2DP_SOURCE_DATA_PATH,
    CONFIG_BLUETOOTH_A2DP_SINK_CTRL_PATH,
    CONFIG_BLUETOOTH_A2DP_SINK_DATA_PATH
};

const char* audio_a2dp_hw_dump_ctrl_cmd(a2dp_ctrl_cmd_t cmd)
{
    switch (cmd) {
        CASE_RETURN_STR(A2DP_CTRL_CMD_START)
        CASE_RETURN_STR(A2DP_CTRL_CMD_STOP)
        CASE_RETURN_STR(A2DP_CTRL_CMD_CONFIG_DONE)
        DEFAULT_BREAK()
    }

    return "UNKNOWN A2DP_CTRL_CMD";
}

static void a2dp_ctrl_event_with_data(uint8_t ch_id, a2dp_ctrl_evt_t event, uint8_t* data, uint8_t data_len)
{
    uint8_t stream[128];
    uint8_t* p = stream;

    /* set event code */
    UINT8_TO_STREAM(p, event);
    if (data_len) {
        /* if data length is not zero, set event data */
        ARRAY_TO_STREAM(p, data, data_len);
    }

    /* send event */
    if (a2dp_transport != NULL) {
        audio_transport_write(a2dp_transport, ch_id, stream, data_len + A2DP_CTRL_EVT_HEADER_LEN, NULL);
    }
}

void a2dp_control_event(uint8_t ch_id, a2dp_ctrl_evt_t evt)
{
    a2dp_ctrl_event_with_data(ch_id, evt, NULL, 0);
}

void a2dp_control_update_audio_config(uint8_t ch_id, uint8_t isvalid)
{
    uint8_t buffer[64];
    uint8_t len;
    uint8_t* p = buffer;
    a2dp_codec_config_t* codec_config = a2dp_codec_get_config();

    if (!isvalid) {
        len = 1;
        /* set valid code */
        UINT8_TO_STREAM(p, 0);
    } else {
        len = 29;
        /* set valid code */
        UINT8_TO_STREAM(p, 1);
        /* set codec type*/
        UINT32_TO_STREAM(p, codec_config->codec_type);
        /* set sample rate*/
        UINT32_TO_STREAM(p, codec_config->sample_rate);
        /* set bits_per_sample*/
        UINT32_TO_STREAM(p, codec_config->bits_per_sample);
        /* set channel_mode*/
        UINT32_TO_STREAM(p, codec_config->channel_mode);
        /* set bit rate*/
        UINT32_TO_STREAM(p, codec_config->bit_rate);
        /* set frame size*/
        UINT32_TO_STREAM(p, codec_config->frame_size);
        /* set packet size*/
        UINT32_TO_STREAM(p, codec_config->packet_size);
        if (codec_config->codec_type == BTS_A2DP_TYPE_SBC) {
            len += 20;
            /* set sbc channel mode*/
            UINT32_TO_STREAM(p, codec_config->codec_param.sbc.s16ChannelMode);
            /* set sbc number of blocks*/
            UINT32_TO_STREAM(p, codec_config->codec_param.sbc.s16NumOfBlocks);
            /* set sbc number of subbands*/
            UINT32_TO_STREAM(p, codec_config->codec_param.sbc.s16NumOfSubBands);
            /* set sbc allocation method*/
            UINT32_TO_STREAM(p, codec_config->codec_param.sbc.s16AllocationMethod);
            /* set sbc bitpool*/
            UINT32_TO_STREAM(p, codec_config->codec_param.sbc.s16BitPool);
        } else if (codec_config->codec_type == BTS_A2DP_TYPE_MPEG2_4_AAC) {
            len += 8;
            /* set aac object type*/
            UINT32_TO_STREAM(p, codec_config->codec_param.aac.u16ObjectType);
            /* set aac vbr*/
            UINT32_TO_STREAM(p, codec_config->codec_param.aac.u16VariableBitRate);
        }
    }

    a2dp_ctrl_event_with_data(ch_id, A2DP_CTRL_EVT_UPDATE_CONFIG, buffer, len);
}

static void a2dp_control_on_start(uint8_t ch_id)
{
    a2dp_ctrl_evt_t evt = A2DP_CTRL_EVT_START_FAIL;

    if (ch_id == AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        if (a2dp_source_stream_ready()) {
            if (a2dp_source_prepare_start() == true) {
                a2dp_source_stream_start();
                return; /* A2DP_CTRL_EVT_STARTED is send when A2DP started */
            }
            evt = A2DP_CTRL_EVT_STARTED;
        } else if (a2dp_source_stream_started()) {
            evt = A2DP_CTRL_EVT_STARTED;
        } else {
            BT_LOGW("%s: A2DP command start while source stream is not ready", __func__);
            evt = A2DP_CTRL_EVT_START_FAIL;
        }
#endif
    } else {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        if (a2dp_sink_stream_ready() || a2dp_sink_stream_started()) {
            evt = A2DP_CTRL_EVT_STARTED;
            a2dp_sink_resume();
        } else {
            BT_LOGW("%s: A2DP command start while sink stream is not ready", __func__);
            evt = A2DP_CTRL_EVT_START_FAIL;
        }
#endif
    }

    a2dp_control_event(ch_id, evt);
}

static void a2dp_control_on_stop(uint8_t ch_id)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (ch_id == AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL && a2dp_source_stream_started()) {
        a2dp_source_stream_prepare_suspend();
        /* A2DP_CTRL_EVT_STOPPED is send when offload stopped */
    }
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (ch_id == AUDIO_TRANS_CH_ID_AV_SINK_CTRL) {
        a2dp_sink_mute();
        a2dp_control_event(ch_id, A2DP_CTRL_EVT_STOPPED); /* TODO: send event when flush ends */
    }
#endif
}

static void a2dp_control_on_config_done(uint8_t ch_id)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (ch_id == AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL)
        a2dp_source_codec_state_change();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (ch_id == AUDIO_TRANS_CH_ID_AV_SINK_CTRL)
        a2dp_sink_codec_state_change();
#endif
}

static void a2dp_recv_ctrl_data(uint8_t ch_id, a2dp_ctrl_cmd_t cmd)
{
    BT_LOGD("%s: a2dp-ctrl-cmd : %s", __func__,
        audio_a2dp_hw_dump_ctrl_cmd(cmd));
    // check length
    switch (cmd) {
    case A2DP_CTRL_CMD_START:
        a2dp_control_on_start(ch_id);
        break;

    case A2DP_CTRL_CMD_STOP:
        a2dp_control_on_stop(ch_id);
        break;

    case A2DP_CTRL_CMD_CONFIG_DONE:
        a2dp_control_on_config_done(ch_id);
        break;

    default:
        BT_LOGD("%s: UNSUPPORTED CMD (%d)", __func__, cmd);
        break;
    }

    BT_LOGD("%s: a2dp-ctrl-cmd : %s DONE", __func__,
        audio_a2dp_hw_dump_ctrl_cmd(cmd));
}

static void a2dp_ctrl_buffer_alloc(uint8_t ch_id, uint8_t** buffer, size_t* len)
{
    *len = 128;
    *buffer = malloc(*len);
}

static void a2dp_ctrl_data_received(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    a2dp_ctrl_cmd_t cmd;
    uint8_t* pbuf = buffer;

    if (len <= 0) {
        free(buffer);
        if (len < 0)
            audio_transport_read_stop(a2dp_transport, ch_id);
        return;
    }

    while (len) {
        /* get cmd code*/
        STREAM_TO_UINT8(cmd, pbuf);
        len--;
        /* process cmd*/
        a2dp_recv_ctrl_data(ch_id, cmd);
    }
    // free the buffer alloced by a2dp_ctrl_buffer_alloc
    free(buffer);
}

static void a2dp_ctrl_start(uint8_t ch_id)
{
    audio_transport_read_start(a2dp_transport, ch_id, a2dp_ctrl_buffer_alloc, a2dp_ctrl_data_received);
}

static void a2dp_ctrl_stop(uint8_t ch_id)
{
    audio_transport_read_stop(a2dp_transport, ch_id);
}

static void a2dp_ctrl_cb(uint8_t ch_id, audio_transport_event_t event)
{
    BT_LOGD("%s, path:[%s], event:%s", __func__, audio_transport_path[ch_id], audio_transport_dump_event(event));

    switch (event) {
    case TRANSPORT_OPEN_EVT:
        a2dp_ctrl_start(ch_id);
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        if (ch_id == AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL && a2dp_source_stream_ready())
            a2dp_control_update_audio_config(ch_id, 1);
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        if (ch_id == AUDIO_TRANS_CH_ID_AV_SINK_CTRL && a2dp_sink_stream_ready())
            a2dp_control_update_audio_config(ch_id, 1);
#endif
        break;

    case TRANSPORT_CLOSE_EVT:
        a2dp_ctrl_stop(ch_id);
        break;

    default:
        BT_LOGD("%s: ### A2DP-CTRL-CHANNEL EVENT %d NOT HANDLED ###",
            __func__, event);
        break;
    }
}

static void a2dp_data_cb(uint8_t ch_id, audio_transport_event_t event)
{
    BT_LOGD("%s, path:[%s], event:%s", __func__, audio_transport_path[ch_id], audio_transport_dump_event(event));

    switch (event) {
    case TRANSPORT_OPEN_EVT:
        break;

    case TRANSPORT_CLOSE_EVT:
        BT_LOGD("%s: ## AUDIO PATH DETACHED ##", __func__);
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        if (a2dp_source_is_streaming())
            a2dp_source_stream_prepare_suspend();
#endif
        break;

    default:
        BT_LOGD("%s: ### A2DP-DATA EVENT %d NOT HANDLED ###", __func__,
            event);
        break;
    }
}

void a2dp_control_init(uint8_t ctrl_id, uint8_t data_id)
{
    if (a2dp_transport == NULL) {
        a2dp_transport = audio_transport_init(get_service_uv_loop());
    }

    if (ctrl_id != AUDIO_TRANS_CH_ID_AV_INVALID) {
        audio_transport_open(a2dp_transport, ctrl_id, audio_transport_path[ctrl_id], a2dp_ctrl_cb);
    }

    if (data_id != AUDIO_TRANS_CH_ID_AV_INVALID) {
        audio_transport_open(a2dp_transport, data_id, audio_transport_path[data_id], a2dp_data_cb);
    }
}

transport_conn_state_t a2dp_control_get_state(uint8_t ch_id)
{
    if (a2dp_transport == NULL) {
        return IPC_DISCONNTECTED;
    }

    return audio_transport_get_state(a2dp_transport, ch_id);
}

void a2dp_control_ch_close(uint8_t ctrl_id, uint8_t data_id)
{
    audio_transport_close(a2dp_transport, ctrl_id);
    audio_transport_close(a2dp_transport, data_id);
}

void a2dp_control_cleanup(void)
{
    /* don't close ipc when bt stack disable*/
    if (a2dp_transport) {
        audio_transport_close(a2dp_transport, AUDIO_TRANS_CH_ID_ALL);
    }

    a2dp_transport = NULL;
}
