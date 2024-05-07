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

#include "a2dp_codec.h"
#include "a2dp_source_audio.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "service_loop.h"

#include "utils.h"
#define LOG_TAG "src_sbc"
#include "utils/log.h"

#define A2DP_SBC_BIT_PER_SAMPLE 16
#define A2DP_SBC_ENCODER_INTERVAL_MS 20
#define MAX_PCM_FRAME_NUM_PER_TICK 14
#define A2DP_SBC_MAX_PCM_ITER_NUM_PER_TICK 3

typedef struct {
    uint64_t last_frame_us;
    float counter;
    uint32_t bytes_per_tick;
} a2dp_sbc_feeding_state_t;

typedef struct {
    uint32_t total_tx_frames;
    uint64_t session_start_us;
} a2dp_sbc_session_state_t;

typedef struct {
    sbc_param_t* param;
    frame_send_callback send_callback;
    frame_read_callback read_callback;
    uint16_t mtu;
    uint16_t frames_len;
    uint16_t max_tx_length;
    uint32_t media_timestamp;
    a2dp_sbc_feeding_state_t feeding_state;
    a2dp_sbc_session_state_t state;
} a2dp_stream_sbc_t;

a2dp_stream_sbc_t sbc_stream;

int a2dp_source_sbc_update_config(uint32_t mtu, sbc_param_t* param, uint8_t* codec_info)
{
    a2dp_codec_parse_sbc_param(param, codec_info);

    return 0;
}

static uint8_t calculate_max_frames_per_packet(void)
{
    uint32_t frame_len = sbc_stream.frames_len;

    assert(frame_len);
    if (!sbc_stream.mtu)
        sbc_stream.mtu = MAX_2MBPS_AVDTP_MTU;

    return (sbc_stream.mtu - 1) / frame_len;
}

static void a2dp_sbc_get_num_frame_iteration(uint8_t* num_of_iterations, uint8_t* num_of_frames,
    uint64_t now_timestamp_us)
{
    a2dp_stream_sbc_t* stream = &sbc_stream;
    sbc_param_t* param = stream->param;
    uint32_t us_this_tick, frames_per_tick;
    uint32_t pcm_bytes_per_frame;
    uint32_t projected_nof = 0;
    uint8_t noi, nof;
    uint32_t delta;
    float ticks;

    pcm_bytes_per_frame = param->s16NumOfSubBands * param->s16NumOfBlocks * param->s16NumOfChannels * A2DP_SBC_BIT_PER_SAMPLE / 8;
    us_this_tick = A2DP_SBC_ENCODER_INTERVAL_MS * 1000;
    if (stream->feeding_state.last_frame_us != 0)
        us_this_tick = now_timestamp_us - stream->feeding_state.last_frame_us;
    stream->feeding_state.last_frame_us = now_timestamp_us;
    ticks = (float)us_this_tick / (A2DP_SBC_ENCODER_INTERVAL_MS * 1000);
    stream->feeding_state.counter += (float)stream->feeding_state.bytes_per_tick * ticks;
    projected_nof = stream->feeding_state.counter / pcm_bytes_per_frame;
    frames_per_tick = stream->feeding_state.bytes_per_tick / pcm_bytes_per_frame;
    if (projected_nof > MAX_PCM_FRAME_NUM_PER_TICK) {
        delta = projected_nof - MAX_PCM_FRAME_NUM_PER_TICK;
        projected_nof = MAX_PCM_FRAME_NUM_PER_TICK;
        if ((delta / frames_per_tick) > A2DP_SBC_MAX_PCM_ITER_NUM_PER_TICK)
            stream->feeding_state.counter = projected_nof * pcm_bytes_per_frame;
    }

    noi = 1;
    nof = calculate_max_frames_per_packet();
    if (nof < projected_nof) {
        noi = projected_nof / frames_per_tick;
        if (noi > 1) {
            nof = frames_per_tick;
            if (noi > A2DP_SBC_MAX_PCM_ITER_NUM_PER_TICK) {
                noi = A2DP_SBC_MAX_PCM_ITER_NUM_PER_TICK;
                stream->feeding_state.counter = noi * nof * pcm_bytes_per_frame;
            }
        }
    } else
        nof = projected_nof;

    stream->feeding_state.counter -= noi * nof * pcm_bytes_per_frame;
    *num_of_frames = nof;
    *num_of_iterations = noi;
}

static int a2dp_sbc_frame_header_check(uint8_t* frame)
{
    return 0;
}

static void a2dp_sbc_send_frames(uint16_t header_reserve, uint8_t frames)
{
    sbc_param_t* param = sbc_stream.param;
    uint16_t max_frames_len;
    uint16_t bytes_read = 0;
    uint8_t read_frames = 0;
    uint8_t* frame_buffer;
    uint8_t* buffer;
    /*
     * Timestamp of the media packet header represent the TS of the
     * first SBC frame, i.e the timestamp before including this frame.
     */
    uint16_t blocm_x_subband = param->s16NumOfSubBands * param->s16NumOfBlocks;

    max_frames_len = frames * sbc_stream.frames_len;
    buffer = malloc(max_frames_len + header_reserve + 1);
    if (buffer == NULL) {
        BT_LOGW("%s, sbc buffer allocation failure: %d", __func__, frames);
        return;
    }

    frame_buffer = buffer;
    frame_buffer += header_reserve; // reserved for packet
    frame_buffer += 1; // actual number of frames
    do {
        int ret = sbc_stream.read_callback(frame_buffer, sbc_stream.frames_len);
        if (ret > 0) {
            a2dp_sbc_frame_header_check(frame_buffer);
            bytes_read += ret;
            frame_buffer += ret;
            frames--;
            read_frames++;
        } else {
            BT_LOGW("%s, underflow :%d", __func__, frames);
            sbc_stream.feeding_state.counter += param->s16NumOfSubBands * param->s16NumOfBlocks * param->s16NumOfChannels * A2DP_SBC_BIT_PER_SAMPLE / 8 * frames;
            break;
        }
    } while (frames);

    if (bytes_read > 0) {
        bytes_read += 1;
        buffer[header_reserve] = read_frames;
        sbc_stream.send_callback(buffer, bytes_read, read_frames, sbc_stream.media_timestamp);
        sbc_stream.media_timestamp += read_frames * blocm_x_subband;
        sbc_stream.state.total_tx_frames += read_frames;
    }

    // free frame buffer
    free(buffer);
}

static void a2dp_source_sbc_send_frames(uint16_t header_reserve, uint64_t timestamp)
{
    uint8_t num_of_frames;
    uint8_t num_of_iterations;

    a2dp_sbc_get_num_frame_iteration(&num_of_iterations, &num_of_frames,
        timestamp);
    if (num_of_frames == 0)
        return;

    for (int i = 0; i < num_of_iterations; i++) {
        a2dp_sbc_send_frames(header_reserve, num_of_frames);
    }
}

static void a2dp_source_sbc_stream_init(void* param, uint32_t mtu,
    frame_send_callback send_cb,
    frame_read_callback read_cb)
{
    sbc_stream.param = (sbc_param_t*)param;
    sbc_stream.mtu = mtu;
    sbc_stream.send_callback = send_cb;
    sbc_stream.read_callback = read_cb;
    sbc_stream.frames_len = a2dp_sbc_frame_length(sbc_stream.param);
    sbc_stream.media_timestamp = 0;
    sbc_stream.state.total_tx_frames = 0;
    sbc_stream.state.session_start_us = 0;
    sbc_stream.feeding_state.last_frame_us = 0;
}

static void a2dp_source_sbc_stream_reset(void)
{
    sbc_param_t* param = sbc_stream.param;
    uint16_t sample_rate;

    sample_rate = a2dp_sbc_sample_frequency(param->s16SamplingFreq);
    sbc_stream.media_timestamp = 0;
    sbc_stream.state.total_tx_frames = 0;
    sbc_stream.state.session_start_us = get_os_timestamp_us();
    sbc_stream.feeding_state.last_frame_us = 0;
    sbc_stream.feeding_state.counter = 0;
    sbc_stream.feeding_state.bytes_per_tick = (sample_rate * A2DP_SBC_BIT_PER_SAMPLE / 8 * param->s16NumOfChannels * A2DP_SBC_ENCODER_INTERVAL_MS) / 1000;
}

static int a2dp_source_sbc_interval_ms(void)
{
    return A2DP_SBC_ENCODER_INTERVAL_MS;
}

static const a2dp_source_stream_interface_t a2dp_source_stream_sbc = {
    a2dp_source_sbc_stream_init,
    a2dp_source_sbc_stream_reset,
    NULL,
    a2dp_source_sbc_send_frames,
    a2dp_source_sbc_interval_ms,
};

const a2dp_source_stream_interface_t* get_a2dp_source_sbc_stream_interface(void)
{
    return &a2dp_source_stream_sbc;
}

bool a2dp_source_sbc_get_offload_config(a2dp_codec_config_t* codec, a2dp_offload_config_t* offload)
{
    sbc_param_t* param = &codec->codec_param.sbc;

    offload->codec_type = BTS_A2DP_TYPE_SBC;
    offload->max_latency = a2dp_sbc_max_latency(param);
    offload->sample_rate = a2dp_sbc_sample_frequency(param->s16SamplingFreq);
    offload->bits_per_sample = a2dp_sbc_bits_per_sample(param);
    offload->frame_sample = a2dp_sbc_frame_sample(param);
    offload->ch_mode = a2dp_get_sbc_ch_mode(param);
    offload->encoded_audio_bitrate = a2dp_sbc_encoded_audio_bitrate(param);
    offload->mtu = sbc_stream.mtu;
    offload->acl_hdl = codec->acl_hdl;
    offload->l2c_rcid = codec->l2c_rcid;

    return true;
}
