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

#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_service_base.h"
#include "a2dp_codec_sbc.h"

#define LOG_TAG "a2dp_stream"
#include "log.h"

#define MAX_SBC_FRAME_NUM_PER_TICK 14
#define STREAM_DATA_OFFSET (offsetof(SERVICE_A2DP_SOURCE_PACKET_S, data) + 1)

a2dp_source_stream_t a2dp_src_stream;
extern a2dp_ipc_t*          a2dp_ipc;

static uint64_t get_os_timestamp_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);

    return (ts.tv_sec * 1000000 + (ts.tv_nsec / 1000));
}

static SERVICE_A2DP_SOURCE_PACKET_S* bts_a2dp_source_build_packet(stream_buf_t *sbuf, uint16_t len, uint8_t nof)
{
    SERVICE_A2DP_SOURCE_PACKET_S* packet;
    struct circbuf_s *fragmente = &a2dp_src_stream.fragmente;
    int fragment_size;

    fragment_size = circbuf_used(fragmente);
    if (fragment_size) {
        circbuf_read(fragmente, sbuf->buffer + STREAM_DATA_OFFSET, fragment_size);
        len += fragment_size;
    }

    packet = (SERVICE_A2DP_SOURCE_PACKET_S *)sbuf->buffer;
    if ((len / a2dp_src_stream.frames_len) > nof) {
        fragment_size = len - nof * a2dp_src_stream.frames_len;
        packet->data_length = len - fragment_size + 1;
        if (fragment_size && circbuf_space(fragmente) > fragment_size) {
            circbuf_write(fragmente, &packet->data[packet->data_length], fragment_size);
        }
        packet->data[0] = nof;
        packet->header.version = 2;
        packet->header.padding = 0;
        packet->header.csrcCount = 0;
        packet->header.marker = 0;
        packet->header.payloadType = 96;
        packet->header.sequenceNumber = a2dp_src_stream.sequence_number++;
        packet->header.ssrc = 1;
        packet->header.timestamp = a2dp_src_stream.media_timestamp;
        a2dp_src_stream.last_tx_frames = nof;
        a2dp_src_stream.total_tx_frames += nof;
    } else {
        packet = NULL;
    }
    return packet;
}

static void bts_a2dp_source_packet_send(SERVICE_A2DP_SOURCE_PACKET_S* packet)
{
    service_adapter_a2dp_source_send_data(bts_a2dp_source_active_peer(), packet);
}

static void bts_a2dp_audio_data_alloc(uint8_t ch_id, uint8_t** buffer, size_t *len)
{
    int fragment_size;
    stream_buf_t *sbuf = &a2dp_src_stream.stream_buf;

    fragment_size = circbuf_used(&a2dp_src_stream.fragmente);
    if (fragment_size > a2dp_src_stream.max_tx_length) {
        *buffer = NULL;
        return;
    }
    *len = a2dp_src_stream.max_tx_length - fragment_size;

    sbuf->offset = STREAM_DATA_OFFSET + fragment_size;
    sbuf->length = *len + sbuf->offset;
    sbuf->buffer = (void *)malloc(sbuf->length);
    if (!sbuf->buffer) {
        *buffer = NULL;
        a2dp_ipc_read_stop(a2dp_ipc, ch_id);
        return;
    }
    memset(sbuf->buffer, 0, sbuf->length);
    *buffer = ((uint8_t *)sbuf->buffer) + sbuf->offset;
}

static void bts_a2dp_audio_data_received(uint8_t ch_id, uint8_t* buffer, size_t len)
{
    stream_buf_t *sbuf = &a2dp_src_stream.stream_buf;
    SERVICE_A2DP_SOURCE_PACKET_S* packet;
    uint8_t num_of_frames;
    uint8_t num_of_iterations;

    if ((sbuf->buffer + sbuf->offset) != buffer) {
        goto out;
    }

    if (len <= 0) {
        BT_LOGD("%s, status:%d", __func__, len);
        if (len < 0)
            a2dp_ipc_read_stop(a2dp_ipc, ch_id);
        goto out;
    }

    a2dp_codec_sbc_get_num_frame_interation(&num_of_iterations, &num_of_frames,
                                            get_os_timestamp_us());
    for (size_t i = 0; i < num_of_iterations; i++) {
      packet = bts_a2dp_source_build_packet(sbuf, len, num_of_frames);
      if (packet != NULL) {
        bts_a2dp_source_packet_send(packet);
        a2dp_codec_sbc_media_timestamp();
        len = 0;
      }
    }
    a2dp_ipc_read_stop(a2dp_ipc, ch_id);
    a2dp_src_stream.stream_state = STATE_START_UP;

out:
    free(sbuf->buffer);
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

    circbuf_reset(&a2dp_src_stream.fragmente);
    a2dp_src_stream.interval_ms = bts_a2dp_codec_interval_ms();
    a2dp_src_stream.frames_len = bts_a2dp_codec_get_frame_length();
    a2dp_src_stream.max_tx_length = MAX_SBC_FRAME_NUM_PER_TICK * a2dp_src_stream.frames_len;
    a2dp_src_stream.media_alarm = start_timer(a2dp_src_stream.interval_ms,
        a2dp_src_stream.interval_ms,
        bts_a2dp_source_audio_handle_timer,
        NULL);
    a2dp_src_stream.stream_state = STATE_START_UP;
    a2dp_src_stream.session_start_us = get_os_timestamp_us();
}

static void bts_a2dp_source_stop_audio_req(void)
{
    BT_LOGD("%s", __func__);

    if (a2dp_src_stream.stream_state == STATE_RUNNING) {
        a2dp_ipc_read_stop(a2dp_ipc, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
    }

    circbuf_reset(&a2dp_src_stream.fragmente);
    //TODO: need flush socket
    stop_timer(a2dp_src_stream.media_alarm);
    a2dp_src_stream.media_alarm = NULL;
    a2dp_src_stream.stream_state = STATE_OFF;
    a2dp_src_stream.session_start_us = 0;
    a2dp_src_stream.last_tx_frames = 0;
    a2dp_src_stream.total_tx_frames = 0;
}

bool bts_a2dp_source_is_streaming(void)
{
    return a2dp_src_stream.media_alarm ? true : false;
}

void bts_a2dp_source_on_connection_changed(bool connected)
{
    BT_LOGD("%s", __func__);
    if (connected) {
        bts_a2dp_control_update_audio_config(1);
    } else {
        bts_a2dp_control_update_audio_config(0);
    }
}

void bts_a2dp_source_on_started(bool started)
{
    BT_LOGD("%s", __func__);

    if (started) {
        bts_a2dp_control_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_CTRL_EVT_STARTED);
        if (a2dp_src_stream.stream_state == STATE_OFF)
            bts_a2dp_source_start_audio_req();
    } else {
        bts_a2dp_control_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_CTRL_EVT_START_FAIL);
    }
}

void bts_a2dp_source_on_stopped(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.stream_state == STATE_OFF)
        return;

    bts_a2dp_source_stop_audio_req();
}

void bts_a2dp_source_on_suspended(void)
{
    BT_LOGD("%s", __func__);
    if (a2dp_src_stream.stream_state == STATE_OFF)
        return;

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
    a2dp_src_stream.media_timestamp = 0;
    a2dp_src_stream.session_start_us = 0;
    a2dp_src_stream.last_tx_frames = 0;
    a2dp_src_stream.total_tx_frames = 0;
    bts_a2dp_codec_init();
    circbuf_init(&a2dp_src_stream.fragmente, NULL, 2048);
    bts_a2dp_control_init(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
}

void bts_a2dp_source_audio_cleanup(void)
{
    circbuf_uninit(&a2dp_src_stream.fragmente);
    bts_a2dp_control_cleanup();
}
