/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
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

#include "btm_manager.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_control.h"
#include "bts_a2dp_source.h"
#include "bts_a2dp_source_audio.h"
#include "bts_service.h"
#include "a2dp_ipc.h"

#include "utils.h"
#define LOG_TAG "a2dp_control"
#include "log.h"

a2dp_ipc_t* a2dp_ipc;
#define A2DP_CTRL_PATH A2DP_SOURCE_CTRL_PATH
#define A2DP_DATA_PATH A2DP_SOURCE_DATA_PATH

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

static void bts_a2dp_ctrl_event_with_data(uint8_t ch_id, a2dp_ctrl_evt_t event, uint8_t* data, uint8_t data_len)
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
    if (a2dp_ipc != NULL) {
        a2dp_ipc_write(a2dp_ipc, ch_id, stream, data_len + A2DP_CTRL_EVT_HEADER_LEN, NULL);
    }
}

void bts_a2dp_control_event(uint8_t ch_id, a2dp_ctrl_evt_t evt)
{
    bts_a2dp_ctrl_event_with_data(ch_id, evt, NULL, 0);
}

void bts_a2dp_control_update_audio_config(uint8_t isvalid)
{
    uint8_t buffer[64];
    uint8_t len;
    uint8_t* p = buffer;
    uint8_t ch_id = A2DP_IPC_CH_ID_AV_SOURCE_CTRL;
    a2dp_codec_config_t* codec_config = bts_a2dp_codec_get_config();

    if (!isvalid) {
        len = 1;
        /* set valid code */
        UINT8_TO_STREAM(p, 0);
    } else {
        len = 21;
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
    }

    bts_a2dp_ctrl_event_with_data(ch_id, A2DP_CTRL_EVT_UPDATE_CONFIG, buffer, len);
}

static void bts_a2dp_control_on_start(uint8_t ch_id)
{
    a2dp_ctrl_evt_t evt;

    if (bts_a2dp_source_stream_ready()) {
        bts_a2dp_source_stream_start();
        return;
    } else if (bts_a2dp_source_stream_started()) {
        evt = A2DP_CTRL_EVT_STARTED;
    } else {
        BT_LOGW("%s: A2DP command start while AV stream is not ready", __func__);
        evt = A2DP_CTRL_EVT_START_FAIL;
    }

    bts_a2dp_control_event(ch_id, evt);
}

static void bts_a2dp_control_on_stop(uint8_t ch_id)
{
    if (bts_a2dp_source_stream_started()) {
        bts_a2dp_source_stream_stop();
    }

    bts_a2dp_control_event(ch_id, A2DP_CTRL_EVT_STOPPED);
}

static void bts_a2dp_control_on_config_done(uint8_t ch_id)
{
    bts_a2dp_source_codec_state_change();
}

static void bts_a2dp_recv_ctrl_data(uint8_t ch_id, a2dp_ctrl_cmd_t cmd)
{
    BT_LOGD("%s: a2dp-ctrl-cmd : %s", __func__,
        audio_a2dp_hw_dump_ctrl_cmd(cmd));
    //check length
    switch (cmd) {
    case A2DP_CTRL_CMD_START:
        bts_a2dp_control_on_start(ch_id);
        break;

    case A2DP_CTRL_CMD_STOP:
        bts_a2dp_control_on_stop(ch_id);
        break;

    case A2DP_CTRL_CMD_CONFIG_DONE:
        bts_a2dp_control_on_config_done(ch_id);
        break;

    default:
        BT_LOGD("%s: UNSUPPORTED CMD (%d)", __func__, cmd);
        break;
    }

    BT_LOGD("%s: a2dp-ctrl-cmd : %s DONE", __func__,
        audio_a2dp_hw_dump_ctrl_cmd(cmd));
}

static void bts_a2dp_ctrl_buffer_alloc(uint8_t ch_id, uint8_t** buffer, size_t *len)
{
    *len = 128;
    *buffer = malloc(*len);
}

static void bts_a2dp_ctrl_data_received(uint8_t ch_id, uint8_t* buffer, size_t len)
{
    a2dp_ctrl_cmd_t cmd;
    uint8_t* pbuf = buffer;

    if (len <= 0) {
        free(buffer);
        if (len < 0)
            a2dp_ipc_read_stop(a2dp_ipc, ch_id);
        return;
    }

    while(len) {
        /* get cmd code*/
        STREAM_TO_UINT8(cmd, pbuf);
        len--;
        /* process cmd*/
        bts_a2dp_recv_ctrl_data(ch_id, cmd);
    }
    //free the buffer alloced by bts_a2dp_ctrl_buffer_alloc
    free(buffer);
}

static void bts_a2dp_ctrl_start(uint8_t ch_id)
{
    a2dp_ipc_read_start(a2dp_ipc, ch_id, bts_a2dp_ctrl_buffer_alloc, bts_a2dp_ctrl_data_received);
}

static void bts_a2dp_ctrl_stop(uint8_t ch_id)
{
    a2dp_ipc_read_stop(a2dp_ipc, ch_id);
}

static void bts_a2dp_ctrl_cb(uint8_t ch_id, a2dp_ipc_event_t event)
{
    BT_LOGD("%s, event:%s", __func__, dump_a2dp_ipc_event(event));
    switch (event) {
    case IPC_OPEN_EVT:
        bts_a2dp_ctrl_start(ch_id);
        break;

    case IPC_CLOSE_EVT:
        bts_a2dp_ctrl_stop(ch_id);
        break;

    default:
        BT_LOGD("%s: ### A2DP-CTRL-CHANNEL EVENT %d NOT HANDLED ###",
            __func__, event);
        break;
    }
}

static void bts_a2dp_data_cb(uint8_t ch_id, a2dp_ipc_event_t event)
{
    BT_LOGD("%s,event:%s", __func__, dump_a2dp_ipc_event(event));

    switch (event) {
    case IPC_OPEN_EVT:
        break;

    case IPC_CLOSE_EVT:
        BT_LOGD("%s: ## AUDIO PATH DETACHED ##", __func__);
        if (bts_a2dp_source_is_streaming())
            bts_a2dp_source_stream_stop();
        break;

    default:
        BT_LOGD("%s: ### A2DP-DATA EVENT %d NOT HANDLED ###", __func__,
            event);
        break;
    }
}

void bts_a2dp_control_init(uint8_t ctrl_id, uint8_t data_id)
{
    if (a2dp_ipc == NULL)
        a2dp_ipc = a2dp_ipc_init(get_service_loop());

    a2dp_ipc_open(a2dp_ipc, ctrl_id, A2DP_CTRL_PATH, bts_a2dp_ctrl_cb);
    a2dp_ipc_open(a2dp_ipc, data_id, A2DP_DATA_PATH, bts_a2dp_data_cb);
}

void bts_a2dp_control_cleanup(void)
{
    if (a2dp_ipc) {
        a2dp_ipc_close(a2dp_ipc, A2DP_IPC_CH_ID_ALL);
    }
}