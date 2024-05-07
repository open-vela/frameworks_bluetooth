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
#define LOG_TAG "aac_src_stream"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "a2dp_codec_aac.h"
#include "a2dp_source_audio.h"
#include "service_loop.h"
#include "utils.h"
#include "utils/log.h"

#define A2DP_AAC_BIT_PER_SAMPLE 16
#define A2DP_AAC_ENCODER_INTERVAL_MS 20
#define A2DP_AAC_MAX_PCM_FRAME_NUM_PER_TICK 3

typedef struct {
    uint32_t flag;
    uint64_t last_frame_us;
    float counter;
    uint32_t bytes_per_tick;
} a2dp_aac_feeding_state_t;

typedef struct {
    uint32_t total_tx_frames;
    uint64_t session_start_us;
} a2dp_aac_session_state_t;

typedef struct {
    uint8_t header[3];
    uint8_t header_length;
} loas_header_t;
typedef struct {
    aac_encoder_param_t* param;
    frame_send_callback send_callback;
    frame_read_callback read_callback;
    uint16_t mtu;
    uint16_t frame_len;
    uint16_t max_tx_length;
    uint32_t media_timestamp;
    a2dp_aac_feeding_state_t feeding_state;
    a2dp_aac_session_state_t state;
    loas_header_t loas;
} a2dp_stream_aac_t;

static a2dp_stream_aac_t aac_stream;
static uint32_t a2dp_aac_encoder_interval_ms = A2DP_AAC_ENCODER_INTERVAL_MS;

int a2dp_source_aac_update_config(uint32_t mtu, aac_encoder_param_t* param, uint8_t* codec_info)
{
    uint16_t mtu_size;

    aac_stream.mtu = mtu;
    if (mtu > MAX_2MBPS_AVDTP_MTU)
        mtu_size = MAX_2MBPS_AVDTP_MTU;
    else
        mtu_size = mtu;

    a2dp_codec_parse_aac_param(param, codec_info, mtu_size);

    return 0;
}

static void a2dp_aac_get_num_frame_iteration(uint8_t* num_of_iterations, uint8_t* num_of_frames,
    uint64_t now_timestamp_us)
{
    a2dp_stream_aac_t* stream = &aac_stream;
    aac_encoder_param_t* params = stream->param;
    uint32_t pcm_bytes_per_frame;
    uint32_t projected_nof = 0;
    uint32_t us_this_tick;
    float ticks;

    pcm_bytes_per_frame = stream->frame_len * params->u16NumOfChannels * A2DP_AAC_BIT_PER_SAMPLE / 8;
    us_this_tick = a2dp_aac_encoder_interval_ms * 1000;
    if (stream->feeding_state.last_frame_us != 0)
        us_this_tick = now_timestamp_us - stream->feeding_state.last_frame_us;
    stream->feeding_state.last_frame_us = now_timestamp_us;
    ticks = (float)us_this_tick / (a2dp_aac_encoder_interval_ms * 1000);
    stream->feeding_state.counter += (float)stream->feeding_state.bytes_per_tick * ticks;
    projected_nof = stream->feeding_state.counter / pcm_bytes_per_frame;
    if (projected_nof > A2DP_AAC_MAX_PCM_FRAME_NUM_PER_TICK) {
        projected_nof = A2DP_AAC_MAX_PCM_FRAME_NUM_PER_TICK;
        stream->feeding_state.counter = projected_nof * pcm_bytes_per_frame;
    }

    *num_of_iterations = 1;
    *num_of_frames = projected_nof;
    stream->feeding_state.counter -= projected_nof * pcm_bytes_per_frame;
}

static void a2dp_aac_send_frames(uint16_t header_reserve, uint8_t frames)
{
    loas_header_t* loas = &aac_stream.loas; //= "\x56\xe0\x00";
    uint16_t len = 0;
    int ret;

    do {
        // try find loas header sync word
        // loas->header_length = 0;
        // if (loas->header_length != 3) {
        // ret = aac_stream.read_callback(&loas->header[loas->header_length], 3 - loas->header_length);
        // loas->header_length = ret;
        ret = aac_stream.read_callback(loas->header, 3);
        if (ret == 0)
            return;
        //}

        if (loas->header[0] == 0x56 && (loas->header[1] & 0xE0) == 0xE0)
            len = (uint16_t)((loas->header[1] & 0x1F) << 8) + (uint16_t)loas->header[2];
        else
            continue;
        uint8_t* buffer = malloc(header_reserve + len);
        if (buffer == NULL)
            return;

        // read AAC data with latm header
        ret = aac_stream.read_callback(buffer + header_reserve, len);
        if (ret == 0) {
            free(buffer);
            aac_stream.feeding_state.counter += aac_stream.frame_len * aac_stream.param->u16NumOfChannels * A2DP_AAC_BIT_PER_SAMPLE / 8 * frames;
            BT_LOGW("%s, underflow :%d, %f", __func__, frames, aac_stream.feeding_state.counter);
            return;
        }
        loas->header_length = 0;
        aac_stream.send_callback(buffer, ret, 1, aac_stream.media_timestamp);
        aac_stream.media_timestamp += aac_stream.frame_len;
        aac_stream.state.total_tx_frames++;
        frames--;
        free(buffer);
    } while (frames);
}

static void a2dp_source_aac_send_frames(uint16_t header_reserve, uint64_t timestamp)
{
    uint8_t num_of_frames;
    uint8_t num_of_iterations;

    a2dp_aac_get_num_frame_iteration(&num_of_iterations, &num_of_frames,
        timestamp);
    if (num_of_frames == 0)
        return;

    for (int i = 0; i < num_of_iterations; i++) {
        a2dp_aac_send_frames(header_reserve, num_of_frames);
    }
}

static void a2dp_source_aac_stream_init(void* param, uint32_t mtu,
    frame_send_callback send_cb,
    frame_read_callback read_cb)
{
    a2dp_stream_aac_t* stream = &aac_stream;

    stream->param = (aac_encoder_param_t*)param;
    stream->mtu = mtu;
    stream->send_callback = send_cb;
    stream->read_callback = read_cb;
    stream->frame_len = 1024;
    stream->media_timestamp = 0;
    stream->state.total_tx_frames = 0;
    stream->state.session_start_us = 0;
    a2dp_aac_encoder_interval_ms = stream->frame_len * 1000 / stream->param->u32SampleRate;
    if (a2dp_aac_encoder_interval_ms < A2DP_AAC_ENCODER_INTERVAL_MS)
        a2dp_aac_encoder_interval_ms = A2DP_AAC_ENCODER_INTERVAL_MS;

    BT_LOGD("%s, a2dp_aac_encoder_interval_ms:%" PRIu32, __func__, a2dp_aac_encoder_interval_ms);
}

static void a2dp_source_aac_stream_reset(void)
{
    a2dp_stream_aac_t* stream = &aac_stream;
    aac_encoder_param_t* param = stream->param;

    stream->state.total_tx_frames = 0;
    stream->state.session_start_us = get_os_timestamp_us();
    stream->media_timestamp = 0;
    a2dp_aac_encoder_interval_ms = stream->frame_len * 1000 / param->u32SampleRate;
    if (a2dp_aac_encoder_interval_ms < A2DP_AAC_ENCODER_INTERVAL_MS)
        a2dp_aac_encoder_interval_ms = A2DP_AAC_ENCODER_INTERVAL_MS;

    stream->loas.header_length = 0;
    stream->feeding_state.counter = 0;
    stream->feeding_state.last_frame_us = 0;
    stream->feeding_state.bytes_per_tick = (param->u32SampleRate * A2DP_AAC_BIT_PER_SAMPLE / 8 * param->u16NumOfChannels * a2dp_aac_encoder_interval_ms) / 1000;
}

int a2dp_source_aac_interval_ms(void)
{
    return a2dp_aac_encoder_interval_ms;
}

static const a2dp_source_stream_interface_t a2dp_source_stream_aac = {
    a2dp_source_aac_stream_init,
    a2dp_source_aac_stream_reset,
    NULL,
    a2dp_source_aac_send_frames,
    a2dp_source_aac_interval_ms,
};

const a2dp_source_stream_interface_t* get_a2dp_source_aac_stream_interface(void)
{
    return &a2dp_source_stream_aac;
}
