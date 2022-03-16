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
#include "bts_service.h"
#include "a2dp_codec_sbc.h"
#include "bts_a2dp_codec.h"
#include "a2dp_codec_sbc.h"
#include "bts_a2dp_control.h"
#include "bts_a2dp_source.h"
#include "bts_a2dp_source_audio.h"
#include "bts_a2dp_source_sbc_stream.h"

#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_service_base.h"
#include "utils/utils.h"
#define LOG_TAG "a2dp_stream"
#include "log.h"

#define MAX_FRAME_NUM_PER_TICK 14
#define STREAM_DELAY_MS 100
#define STREAM_FLUSH_SIZE (1024)
#define STREAM_DATA_RESERVED offsetof(SERVICE_A2DP_SOURCE_PACKET_S, data)

typedef enum {
    STATE_OFF,
    STATE_RUNNING,
    STATE_FLUSHING
} stream_state_t;

typedef struct {
    uint16_t         mtu;
    uint16_t         frames_len;
    stream_state_t   stream_state;
    uint8_t          codec_info[10];
    uint32_t         interval_ms;
    uv_timer_t*      media_alarm;
    uint32_t         sequence_number;
    uint32_t         total_tx_frames;
    uint32_t         max_tx_length;
    struct circbuf_s stream_pool;
    uint8_t          read_congest;
    //a2dp_stream_context_t stream_context;
} a2dp_source_stream_t;

a2dp_source_stream_t a2dp_src_stream;
extern a2dp_ipc_t* a2dp_ipc;

static void bts_a2dp_source_read_congest(uint8_t ch_id);

uint64_t get_os_timestamp_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);

    return (uint64_t)(((uint64_t)ts.tv_sec * 1000000L) + ((uint64_t)ts.tv_nsec / 1000));
}

static void bts_a2dp_source_packet_send(SERVICE_A2DP_SOURCE_PACKET_S* packet)
{
    service_adapter_a2dp_source_send_data(bts_a2dp_source_active_peer()->bd_addr, packet);
}

static void bts_a2dp_audio_data_alloc(uint8_t ch_id, uint8_t** buffer, size_t* len)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    int space, next_to_read;
    uint8_t* alloc_buffer;

    if (stream->stream_state == STATE_FLUSHING) {
        *len = STREAM_FLUSH_SIZE;
        *buffer = (uint8_t*)malloc(STREAM_FLUSH_SIZE);
        return;
    }

    //check pool buffer space enough to read one frame
    space = circbuf_space(&stream->stream_pool);
    if (space < a2dp_src_stream.frames_len) {
        BT_LOGE("%s, space:%d, frames_len:%d", __func__, space, a2dp_src_stream.frames_len);
        *buffer = NULL;
        return;
    }

    next_to_read = space > stream->max_tx_length ?
                   stream->max_tx_length :
                   space /(stream->frames_len) * (stream->frames_len);
    //BT_LOGD("%s, space:%d, next_read:%d", __func__, space, next_to_read);

    alloc_buffer = (void*)malloc(next_to_read);
    if (!alloc_buffer) {
        *buffer = NULL;
        a2dp_ipc_read_stop(a2dp_ipc, ch_id);
        return;
    }

    *buffer = alloc_buffer;
    *len = next_to_read;
}

static void bts_a2dp_audio_data_received(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    int space;

    if (buffer == NULL)
        return;

    if (len <= 0) {
        BT_LOGD("%s, status:%d", __func__, len);
        if (len < 0)
            a2dp_ipc_read_stop(a2dp_ipc, ch_id);

        goto out;
    }

    space = circbuf_space(&stream->stream_pool);
    if (len > space) {
        BT_LOGE("%s, unexpected len:%d", __func__, len);
        goto out;
    }

    circbuf_write(&stream->stream_pool, buffer, len);
    space = circbuf_space(&stream->stream_pool);
    if (space < stream->frames_len) {
        bts_a2dp_source_read_congest(ch_id);
    }

out:
    free(buffer);
}

static void bts_a2dp_audio_data_flush(uint8_t ch_id, uint8_t* buffer, ssize_t len)
{
    free(buffer);
}

static void bts_a2dp_source_start_read(void)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if (stream->stream_state != STATE_OFF &&
        stream->read_congest != 1)
        return;

    if (circbuf_space(&stream->stream_pool) < stream->frames_len)
        return;

    stream->read_congest = 0;
    a2dp_ipc_read_start(a2dp_ipc,
        A2DP_IPC_CH_ID_AV_SOURCE_AUDIO,
        bts_a2dp_audio_data_alloc,
        bts_a2dp_audio_data_received);
}

static void bts_a2dp_source_read_congest(uint8_t ch_id)
{
    a2dp_src_stream.read_congest = 1;
    a2dp_ipc_read_stop(a2dp_ipc, ch_id);
}

static void bts_a2dp_source_send_callback(uint8_t* buf, uint16_t nbytes, uint8_t nb_frames, uint64_t timestamp)
{
    SERVICE_A2DP_SOURCE_PACKET_S* packet;
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if (buf == NULL) {
        BT_LOGE("%s, buffer is null", __func__);
        return;
    }

    packet = (SERVICE_A2DP_SOURCE_PACKET_S*)buf;
    packet->data_length = nbytes;
    packet->header.version = 2;
    packet->header.padding = 0;
    packet->header.csrcCount = 0;
    packet->header.marker = 0;
    packet->header.payloadType = 96;
    packet->header.sequenceNumber = stream->sequence_number++;
    packet->header.ssrc = 1;
    packet->header.timestamp = timestamp;
    stream->total_tx_frames += nb_frames;

    bts_a2dp_source_packet_send(packet);
}

static int bts_a2dp_source_read_callback(uint8_t* buf, uint16_t frame_len)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;
    uint16_t remaining_size;

    remaining_size = circbuf_used(&stream->stream_pool);
    if (remaining_size < frame_len) {
        return 0;
    }

    circbuf_read(&stream->stream_pool, buf, frame_len);

    return frame_len;
}

static void bts_a2dp_source_send_frames(a2dp_source_stream_t* stream)
{
    a2dp_codec_config_t* config;

    config = bts_a2dp_codec_get_config();
    if (config->codec_type == BTS_A2DP_TYPE_SBC) {
        a2dp_source_sbc_send_frames(STREAM_DATA_RESERVED, get_os_timestamp_us());
    }

    bts_a2dp_source_start_read();
}

static void bts_a2dp_source_stream_init(bt_address bd_addr)
{
    a2dp_codec_config_t* config;
    a2dp_peer_t* peer = bts_a2dp_source_find_peer(bd_addr);

    if (peer == NULL) {
        BT_LOGE("%s, can't find peer:%s", __func__, addr_str(bd_addr));
        return;
    }

    config = bts_a2dp_codec_get_config();
    if (config->codec_type == BTS_A2DP_TYPE_SBC) {
        //for sbc codec
        a2dp_source_sbc_stream_init(&config->codec_param.sbc, peer->mtu,
            bts_a2dp_source_send_callback,
            bts_a2dp_source_read_callback);
    } else if (config->codec_type == BTS_A2DP_TYPE_MPEG2_4_AAC) {
        //for aac codec
    }
}

static void bts_a2dp_source_stream_reset(void)
{
    a2dp_codec_config_t* config;

    config = bts_a2dp_codec_get_config();
    if (config->codec_type == BTS_A2DP_TYPE_SBC) {
        a2dp_source_sbc_stream_reset();
    }
}

static int bts_a2dp_source_stream_interval_ms(void)
{
    a2dp_codec_config_t* config;

    config = bts_a2dp_codec_get_config();
    if (config->codec_type == BTS_A2DP_TYPE_SBC) {
        return a2dp_source_sbc_interval_ms();
    }

    return 20;
}

static void bts_a2dp_source_audio_handle_timer(char* arg)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    if (a2dp_src_stream.stream_state != STATE_RUNNING)
        return;

    if (circbuf_used(&stream->stream_pool) == 0)
        return;

    bts_a2dp_source_send_frames(stream);
}

static void bts_a2dp_source_start_flush(void)
{
    if (a2dp_src_stream.stream_state == STATE_OFF) {
        a2dp_src_stream.stream_state = STATE_FLUSHING;
        a2dp_ipc_read_start(a2dp_ipc,
            A2DP_IPC_CH_ID_AV_SOURCE_AUDIO,
            bts_a2dp_audio_data_alloc,
            bts_a2dp_audio_data_flush);
    }
}

static void bts_a2dp_source_stop_flush(void)
{
    a2dp_src_stream.stream_state = STATE_OFF;
    a2dp_ipc_read_stop(a2dp_ipc, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
}

static void bts_a2dp_source_start_delay(char* arg)
{
    a2dp_source_stream_t* stream = &a2dp_src_stream;

    stop_timer(a2dp_src_stream.media_alarm);
    stream->media_alarm = start_timer(stream->interval_ms,
                          stream->interval_ms,
                          bts_a2dp_source_audio_handle_timer,
                          NULL);
    bts_a2dp_source_stream_reset();
}

static void bts_a2dp_source_start_audio_req(void)
{
    BT_LOGD("%s", __func__);

    a2dp_source_stream_t* stream = &a2dp_src_stream;
    a2dp_peer_t* peer = bts_a2dp_source_active_peer();

    if (stream->stream_state == STATE_FLUSHING) {
        bts_a2dp_source_stop_flush();
    }

    circbuf_reset(&stream->stream_pool);
    stream->mtu = peer->mtu;
    stream->read_congest = 0;
    stream->interval_ms = bts_a2dp_source_stream_interval_ms();
    stream->frames_len = bts_a2dp_codec_get_frame_length();
    stream->max_tx_length = MAX_FRAME_NUM_PER_TICK * stream->frames_len;
    bts_a2dp_source_start_read();
    /*delay start, wait stream pool filling*/
    stream->media_alarm = start_timer(STREAM_DELAY_MS,
        0,
        bts_a2dp_source_start_delay,
        NULL);
    stream->stream_state = STATE_RUNNING;
}

static void bts_a2dp_source_stop_audio_req(void)
{
    BT_LOGD("%s", __func__);

    if (a2dp_src_stream.stream_state != STATE_RUNNING)
        return;

    a2dp_ipc_read_stop(a2dp_ipc, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
    circbuf_reset(&a2dp_src_stream.stream_pool);
    stop_timer(a2dp_src_stream.media_alarm);
    a2dp_src_stream.media_alarm = NULL;
    a2dp_src_stream.total_tx_frames = 0;
    a2dp_src_stream.sequence_number = 0;
    a2dp_src_stream.stream_state = STATE_OFF;
    bts_a2dp_source_start_flush();
    bts_a2dp_source_stream_reset();
}

static void bts_a2dp_source_close_audio(void)
{
    bts_a2dp_source_stop_audio_req();
    a2dp_ipc_read_stop(a2dp_ipc, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
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
    BT_LOGD("%s: %d", __func__, started);

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

void bts_a2dp_source_setup_codec(bt_address bd_addr)
{
    circbuf_reset(&a2dp_src_stream.stream_pool);
    bts_a2dp_source_stream_init(bd_addr);
    bts_a2dp_source_start_flush();
}

void bts_a2dp_source_audio_init(void)
{
    a2dp_src_stream.media_alarm = NULL;
    a2dp_src_stream.stream_state = STATE_OFF;
    a2dp_src_stream.sequence_number = 0;
    a2dp_src_stream.total_tx_frames = 0;
    circbuf_init(&a2dp_src_stream.stream_pool, NULL, 4096);
    bts_a2dp_control_init(A2DP_IPC_CH_ID_AV_SOURCE_CTRL, A2DP_IPC_CH_ID_AV_SOURCE_AUDIO);
}

void bts_a2dp_source_audio_cleanup(void)
{
    bts_a2dp_source_close_audio();
    circbuf_uninit(&a2dp_src_stream.stream_pool);
    bts_a2dp_control_cleanup();
}
