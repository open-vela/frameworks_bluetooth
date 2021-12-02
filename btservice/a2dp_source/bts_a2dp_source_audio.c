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

#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_service_base.h"

#define LOG_TAG "a2dp_stream"
#include "log.h"

typedef enum {
    STATE_OFF,
    STATE_START_UP,
    STATE_RUNNING,
} a2dp_stream_state_t;

typedef struct
{
    uint8_t tx_frames;
    uint16_t frames_len;
    uint16_t max_tx_length;
    uint16_t mtu;
    uint32_t sequence_number;
    a2dp_stream_state_t stream_state;
    uint8_t codec_info[10];
    uint32_t interval_ms;

    uv_timer_t* media_alarm;
} a2dp_source_stream_t;

static a2dp_source_stream_t a2dp_src_stream;
extern a2dp_ipc_t* a2dp_ipc;

static uint32_t get_os_timestamp_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + (ts.tv_nsec / 1000000));
}

static uint32_t calculate_max_frames_per_packet(void)
{
    uint32_t frame_len = bts_a2dp_codec_get_frame_length();
    if (!a2dp_src_stream.mtu || !frame_len)
        return 0;

    return (a2dp_src_stream.mtu - 1) / frame_len;
}

static SERVICE_A2DP_SOURCE_PACKET_S* bts_a2dp_source_build_packet(uint8_t* buffer, uint16_t len)
{
    SERVICE_A2DP_SOURCE_PACKET_S* packet;
    uint8_t num_frames;
    uint32_t offset = sizeof(SERVICE_A2DP_SOURCE_PACKET_S) + 1;

    packet = (SERVICE_A2DP_SOURCE_PACKET_S *)(buffer - offset);
    num_frames = len / a2dp_src_stream.frames_len;
    packet->data[0] = num_frames;
    packet->header.version = 2;
    packet->header.padding = 0;
    packet->header.csrcCount = 0;
    packet->header.marker = 0;
    packet->header.payloadType = 96;
    packet->header.sequenceNumber = a2dp_src_stream.sequence_number++;
    packet->header.ssrc = 1;
    packet->header.timestamp = get_os_timestamp_ms();
    packet->data_length = len + 1; //frame data + num_frames

    return packet;
}

static void bts_a2dp_source_release_packet(SERVICE_A2DP_SOURCE_PACKET_S* packet)
{
    free(packet);
}

static void bts_a2dp_source_packet_send(SERVICE_A2DP_SOURCE_PACKET_S* packet)
{
    service_adapter_a2dp_source_send_data(bts_a2dp_source_active_peer(), packet);
}

static void bts_a2dp_audio_data_alloc(uint8_t ch_id, uint8_t** buffer, size_t *len)
{
    uint8_t *pbuf;
    uint32_t offset = sizeof(SERVICE_A2DP_SOURCE_PACKET_S) + 1;//header + num_frames

    *len = a2dp_src_stream.max_tx_length;
    pbuf = malloc(*len + offset);
    if (!pbuf) {
        a2dp_ipc_read_stop(a2dp_ipc, ch_id);
        return;
    }
    *buffer = pbuf + offset;
}

static void bts_a2dp_audio_data_received(uint8_t ch_id, uint8_t* buffer, size_t len)
{
    SERVICE_A2DP_SOURCE_PACKET_S* packet;

    if (len <= 0) {
        BT_LOGD("%s, status:%d", __func__, len);
        uint32_t offset = sizeof(SERVICE_A2DP_SOURCE_PACKET_S) + 1;
        buffer -= offset;
        free(buffer);
        if (len < 0)
            a2dp_ipc_read_stop(a2dp_ipc, ch_id);
        return;
    }

    packet = bts_a2dp_source_build_packet(buffer, len);
    bts_a2dp_source_packet_send(packet);
    bts_a2dp_source_release_packet(packet);
    a2dp_ipc_read_stop(a2dp_ipc, ch_id);
    a2dp_src_stream.stream_state = STATE_START_UP;
}

static void bts_a2dp_source_audio_handle_timer(char* arg)
{
    if (a2dp_src_stream.stream_state == STATE_START_UP) {
        a2dp_src_stream.stream_state = STATE_RUNNING;
        a2dp_ipc_read_start(a2dp_ipc,
            A2DP_IPC_CH_ID_AV_SOURCE_AUDIO,
            bts_a2dp_audio_data_alloc,
            bts_a2dp_audio_data_received);
    }
}

static void bts_a2dp_source_start_audio_req(void)
{
    BT_LOGD("%s", __func__);
    a2dp_src_stream.interval_ms = bts_a2dp_codec_interval_ms();
    a2dp_src_stream.frames_len = bts_a2dp_codec_get_frame_length();
    a2dp_src_stream.tx_frames = calculate_max_frames_per_packet();
    a2dp_src_stream.max_tx_length = a2dp_src_stream.tx_frames * a2dp_src_stream.frames_len;
    a2dp_src_stream.media_alarm = start_timer(a2dp_src_stream.interval_ms,
        a2dp_src_stream.interval_ms,
        bts_a2dp_source_audio_handle_timer,
        NULL);
    a2dp_src_stream.stream_state = STATE_START_UP;
}

static void bts_a2dp_source_stop_audio_req(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.stream_state == STATE_RUNNING) {
        a2dp_ipc_read_stop(a2dp_ipc, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
    }

    stop_timer(a2dp_src_stream.media_alarm);
    a2dp_src_stream.media_alarm = NULL;
    a2dp_src_stream.stream_state = STATE_OFF;
}

bool bts_a2dp_source_is_streaming(void)
{
    return a2dp_src_stream.media_alarm ? true : false;
}

void bts_a2dp_source_on_idle(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.stream_state == STATE_OFF)
        return;

    bts_a2dp_source_stop_audio_req();
}

void bts_a2dp_source_on_connection_changed(bool connected)
{
    BT_LOGD("%s", __func__);
    if (connected) {
        bts_a2dp_ctrl_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL,
            A2DP_CTRL_EVT_CONNECTED);
    } else {
        bts_a2dp_ctrl_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL,
            A2DP_CTRL_EVT_DISCONNECTED);
    }
}

void bts_a2dp_source_on_started(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.stream_state == STATE_START_UP)
        return;

    bts_a2dp_ctrl_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL,
        A2DP_CTRL_EVT_STARTED);
    bts_a2dp_source_start_audio_req();
}

void bts_a2dp_source_on_stopped(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.stream_state == STATE_OFF)
        return;

    bts_a2dp_ctrl_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL,
        A2DP_CTRL_EVT_STOPPED);
    bts_a2dp_source_stop_audio_req();
}

void bts_a2dp_source_on_suspended(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.stream_state == STATE_OFF)
        return;

    bts_a2dp_ctrl_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL,
        A2DP_CTRL_EVT_STOPPED);
    bts_a2dp_source_stop_audio_req();
}

void bts_a2dp_source_set_mtu(uint16_t mtu)
{
    a2dp_src_stream.mtu = mtu;
}

void bts_a2dp_source_audio_init(void)
{
    a2dp_src_stream.media_alarm = NULL;
    a2dp_src_stream.stream_state = STATE_OFF;
    a2dp_src_stream.sequence_number = 0;
    bts_a2dp_codec_init();
    bts_a2dp_control_init(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
}

void bts_a2dp_source_audio_cleanup(void)
{
    bts_a2dp_control_cleanup();
}
