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
#include "a2dp_codec_sbc.h"
#include "bts_service.h"

#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_service_base.h"
#include "a2dp_codec_sbc.h"

#define LOG_TAG "a2dp_stream"
#include "log.h"

#define MAX_SBC_FRAME_NUM_PER_TICK 14
#define STREAM_DATA_OFFSET (offsetof(SERVICE_A2DP_SOURCE_PACKET_S, data) + 1)

#define STREAM_FLUSH_SIZE (1024)
#if 0
typedef enum {
    STATE_OFF,
    STATE_START_UP,
    STATE_RUNNING,
    STATE_FLUSHING
} stream_state_t;

typedef struct {
    void*    buffer;
    uint16_t offset;
    uint16_t length;
} stream_buf_t;

typedef struct {
    uint8_t             tx_frames;
    uint16_t            frames_len;
    uint16_t            max_tx_length;
    uint16_t            mtu;
    uint32_t            sequence_number;
    stream_state_t      stream_state;
    uint8_t             codec_info[10];
    uint32_t            interval_ms;
    uv_timer_t*         media_alarm;
    struct circbuf_s    fragmente;
    stream_buf_t        stream_buf;
} a2dp_source_stream_t;
static a2dp_source_stream_t a2dp_src_stream;

#endif
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
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    struct circbuf_s *fragmente = &stream->fragmente;
    int fragment_size;

    fragment_size = circbuf_used(fragmente);
    if (len == 0 && (fragment_size / stream->frames_len) < nof)
        return NULL;
    else if (len > 0 && ((len + fragment_size) / stream->frames_len) < nof)
        nof = (len + fragment_size) / stream->frames_len;

    if (fragment_size) {
        circbuf_read(fragmente, sbuf->buffer + STREAM_DATA_OFFSET, fragment_size);
        len += fragment_size;
    }

    packet = (SERVICE_A2DP_SOURCE_PACKET_S *)sbuf->buffer;
    fragment_size = len - nof * stream->frames_len;
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
    packet->header.sequenceNumber = stream->sequence_number++;
    packet->header.ssrc = 1;
    packet->header.timestamp = stream->media_timestamp;
    stream->last_tx_frames = nof;
    stream->total_tx_frames += nof;

    return packet;
}

static void bts_a2dp_source_packet_send(SERVICE_A2DP_SOURCE_PACKET_S* packet)
{
    service_adapter_a2dp_source_send_data(bts_a2dp_source_active_peer()->bd_addr, packet);
}

static void bts_a2dp_audio_data_alloc(uint8_t ch_id, uint8_t** buffer, size_t *len)
{
    int fragment_size;
    stream_buf_t *sbuf = &a2dp_src_stream.stream_buf;

    if (a2dp_src_stream.stream_state == STATE_FLUSHING) {
        *len = STREAM_FLUSH_SIZE;
        *buffer = (uint8_t*)malloc(STREAM_FLUSH_SIZE);
        return;
    }

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

static void bts_a2dp_audio_data_flush(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    free(buffer);
}

#if 0
static int consume_ms(void)
{
    static struct timespec last = {0};
    struct timespec now;
    int consume_ms = 0;

    clock_gettime(CLOCK_MONOTONIC, &now);

    if ((now.tv_nsec - last.tv_nsec) > 0) {
      consume_ms =  (now.tv_sec - last.tv_sec) * 1000 + \
        (now.tv_nsec - last.tv_nsec)/1000000UL;
    } else {
      consume_ms =  (now.tv_sec - last.tv_sec - 1) * 1000 + \
        (1000000000LL + now.tv_nsec - last.tv_nsec)/1000000UL;
    }

    memcpy(&last, &now, sizeof(struct timespec));
    return consume_ms;
}
#endif

static void bts_a2dp_audio_data_received(uint8_t ch_id, uint8_t* buffer, ssize_t len)
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
    //TODO: first play need cache packet
    a2dp_codec_config_t *config = bts_a2dp_codec_get_config();
    a2dp_codec_sbc_get_num_frame_iteration(&config->codec_param.sbc, &num_of_iterations, &num_of_frames,
                                            get_os_timestamp_us());
    for (size_t i = 0; i < num_of_iterations; i++) {
      packet = bts_a2dp_source_build_packet(sbuf, len, num_of_frames);
      if (packet != NULL) {
        bts_a2dp_source_packet_send(packet);
        a2dp_codec_sbc_media_timestamp(&config->codec_param.sbc);
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


    a2dp_source_stream_t* stream = &a2dp_src_stream;
    a2dp_peer_t *peer = bts_a2dp_source_active_peer();

    if (a2dp_src_stream.stream_state == STATE_FLUSHING)
        a2dp_ipc_read_stop(a2dp_ipc, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);

    circbuf_reset(&stream->fragmente);
    stream->mtu = peer->mtu;
    stream->interval_ms = bts_a2dp_codec_interval_ms();
    stream->frames_len = bts_a2dp_codec_get_frame_length();
    stream->max_tx_length = MAX_SBC_FRAME_NUM_PER_TICK * stream->frames_len;
    stream->media_alarm = start_timer(stream->interval_ms,
        stream->interval_ms,
        bts_a2dp_source_audio_handle_timer,
        NULL);
    stream->stream_state = STATE_START_UP;
    stream->session_start_us = get_os_timestamp_us();
}

static void bts_a2dp_source_stop_audio_req(void)
{
    BT_LOGD("%s", __func__);

    if (a2dp_src_stream.stream_state == STATE_OFF ||
        a2dp_src_stream.stream_state == STATE_FLUSHING)
        return;

    if (a2dp_src_stream.stream_state == STATE_RUNNING) {
        a2dp_ipc_read_stop(a2dp_ipc, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
    }

    circbuf_reset(&a2dp_src_stream.fragmente);
    //TODO: need flush socket
    stop_timer(a2dp_src_stream.media_alarm);
    a2dp_src_stream.media_alarm = NULL;
    a2dp_src_stream.stream_state = STATE_FLUSHING;
    a2dp_src_stream.session_start_us = 0;
    a2dp_src_stream.last_tx_frames = 0;
    a2dp_src_stream.total_tx_frames = 0;
    a2dp_ipc_read_start(a2dp_ipc,
            A2DP_IPC_CH_ID_AV_SOURCE_AUDIO,
            bts_a2dp_audio_data_alloc,
            bts_a2dp_audio_data_flush);
}

bool bts_a2dp_source_is_streaming(void)
{
    return a2dp_src_stream.media_alarm ? true : false;
}

void bts_a2dp_source_on_connection_changed(bool connected)
{
    BT_LOGD("%s, %d", __func__, connected);
    if (connected) {
        bts_a2dp_control_update_audio_config(1);
    } else {
        bts_a2dp_control_update_audio_config(0);
        bts_a2dp_source_stop_audio_req();
    }
}

void bts_a2dp_source_on_started(bool started)
{
    BT_LOGD("%s", __func__);

    if (started) {
        bts_a2dp_control_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_CTRL_EVT_STARTED);
        if (a2dp_src_stream.stream_state == STATE_OFF ||
            a2dp_src_stream.stream_state == STATE_FLUSHING)
            bts_a2dp_source_start_audio_req();
    } else {
        bts_a2dp_control_event(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_CTRL_EVT_START_FAIL);
    }
}

void bts_a2dp_source_on_stopped(void)
{
    BT_LOGD("%s", __func__);

    bts_a2dp_source_stop_audio_req();
}

void bts_a2dp_source_on_suspended(void)
{
    BT_LOGD("%s", __func__);

    bts_a2dp_source_stop_audio_req();
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
    circbuf_init(&a2dp_src_stream.fragmente, NULL, 2048);
    bts_a2dp_control_init(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
}

void bts_a2dp_source_audio_cleanup(void)
{
    circbuf_uninit(&a2dp_src_stream.fragmente);
    bts_a2dp_control_cleanup();
}
