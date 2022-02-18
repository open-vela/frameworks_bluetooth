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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "bts_service.h"
#include "a2dp_codec_sbc.h"
#include "bts_a2dp_source_audio.h"
#include "bts_a2dp_source_sbc_stream.h"

#define LOG_TAG "sbc_src_stream"
#include "log.h"

#define A2DP_SBC_BIT_PER_SAMPLE 16
#define A2DP_SBC_ENCODER_INTERVAL_MS 20

typedef struct {
    sbc_param_t*        param;
    frame_send_callback send_callback;
    frame_read_callback read_callback;
    uint32_t            total_tx_frames;
    uint16_t            mtu;
    uint16_t            frames_len;
    uint16_t            max_tx_length;
    uint32_t            media_timestamp;
    uint64_t            session_start_us;
} a2dp_stream_sbc_t;

a2dp_stream_sbc_t sbc_stream;

static uint8_t calculate_max_frames_per_packet(void)
{
    uint32_t frame_len = sbc_stream.frames_len;
    if (!sbc_stream.mtu || !frame_len)
        return 7;

    return (sbc_stream.mtu - 1) / frame_len;
}

uint32_t a2dp_sbc_frame_length(sbc_param_t* param)
{
    uint32_t frame_len, frame_len2;

    if (param == NULL)
        return 0;

    if (param->s16ChannelMode == SBC_STEREO ||
        param->s16ChannelMode == SBC_JOINT_STEREO) {
        frame_len = 4 +
                    (4 *
                    param->s16NumOfSubBands *
                    param->s16NumOfChannels) /
                    8 +
                    (((param->s16ChannelMode - 2) *
                    param->s16NumOfSubBands) +
                    (param->s16NumOfBlocks *
                    param->s16BitPool)) /
                    8;
    } else {
        frame_len = 4 +
                    ((4 *
                    param->s16NumOfSubBands *
                    param->s16NumOfChannels) /
                    8) +
                    ((param->s16NumOfBlocks *
                    param->s16NumOfChannels *
                    param->s16BitPool) /
                    8);
    }

    frame_len2 = 4 +
                (4 *
                param->s16NumOfSubBands *
                param->s16NumOfChannels) /
                8 +
                ((param->s16NumOfBlocks *
                param->s16BitPool *
                (1 + (param->s16ChannelMode == SBC_DUAL)) +
                (param->s16ChannelMode == SBC_JOINT_STEREO) *
                param->s16NumOfSubBands)+ 7) /
                8;
    assert(frame_len == frame_len2);

    return frame_len;
}

static void a2dp_sbc_get_num_frame_iteration(uint8_t* noi, uint8_t* nof,
    uint64_t now_timestamp_us)
{
    uint16_t sample_rate;
    sbc_param_t* param = sbc_stream.param;

    sample_rate = a2dp_sbc_sample_frequency(param->s16SamplingFreq);

    /* PCM bytes of per  frame */
    uint16_t per_frame_bytes = param->s16NumOfBlocks *
                               param->s16NumOfSubBands *
                               param->s16NumOfChannels *
                               A2DP_SBC_BIT_PER_SAMPLE / 8;
    /* PCM bytes read each media task tick */
    uint16_t bytes_per_tick = (sample_rate *
                              A2DP_SBC_BIT_PER_SAMPLE / 8 *
                              param->s16NumOfChannels *
                              A2DP_SBC_ENCODER_INTERVAL_MS) /
                              1000;
    /* Calculate the playback time of per PCM frame */
    uint64_t per_frame_time = (uint64_t)per_frame_bytes * 1000000UL /
                              (sample_rate *
                              (A2DP_SBC_BIT_PER_SAMPLE / 8) *
                              param->s16NumOfChannels);
    /* Calculate the actual playback timestamp according to the total PCM frames
     * we send */
    uint64_t actual_timestamp_us =
             sbc_stream.session_start_us +
             sbc_stream.total_tx_frames *
             per_frame_time;

    *noi = 1;
    uint8_t projected_nof = (bytes_per_tick + per_frame_bytes / 2) / per_frame_bytes;
    *nof = projected_nof > calculate_max_frames_per_packet() ?
           calculate_max_frames_per_packet() : projected_nof;

    if (now_timestamp_us >= actual_timestamp_us) {
        uint8_t delta_frames = (now_timestamp_us - actual_timestamp_us) / per_frame_time;
        if (delta_frames / *nof)
            *noi = 2;
    } else {
        *nof = *nof - 1;
    }
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
    if (buffer == NULL)
        return;

    frame_buffer = buffer;
    frame_buffer += header_reserve; //reserved for packet
    frame_buffer += 1; //actual number of frames
    do {
        int ret = sbc_stream.read_callback(frame_buffer, sbc_stream.frames_len);
        if (ret > 0) {
            bytes_read += ret;
            frame_buffer += ret;
            frames--;
            read_frames++;
        } else {
            //BT_LOGW("%s, underflow :%d", __func__, frames);
            break;
        }
    } while (frames);

    if (bytes_read > 0) {
        bytes_read += 1;
        buffer[header_reserve] = read_frames;
        sbc_stream.send_callback(buffer, bytes_read, read_frames, sbc_stream.media_timestamp);
        sbc_stream.media_timestamp += read_frames * blocm_x_subband;
        sbc_stream.total_tx_frames += read_frames;
    }

    // free frame buffer
    free(buffer);
}

uint32_t a2dp_sbc_bit_rate(sbc_param_t* param)
{
    uint16_t samp_freq;
    uint32_t bit_rate;
    uint32_t frame_len;

    frame_len = a2dp_sbc_frame_length(param);
    samp_freq = a2dp_sbc_sample_frequency(param->s16SamplingFreq);
    bit_rate = (8 * frame_len * samp_freq) / (param->s16NumOfSubBands * param->s16NumOfBlocks);
    BT_LOGD("%s, birtate: %lu", __func__, bit_rate);

    return bit_rate;
}

void a2dp_source_sbc_send_frames(uint16_t header_reserve, uint64_t timestamp)
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

void a2dp_source_sbc_stream_init(sbc_param_t* param, uint32_t mtu,
                                 frame_send_callback send_cb,
                                 frame_read_callback read_cb)
{
    sbc_stream.param = param;
    sbc_stream.mtu = mtu;
    sbc_stream.send_callback = send_cb;
    sbc_stream.read_callback = read_cb;
    sbc_stream.total_tx_frames = 0;
    sbc_stream.frames_len = a2dp_sbc_frame_length(sbc_stream.param);
    sbc_stream.media_timestamp = 0;
    sbc_stream.session_start_us = 0;
}

void a2dp_source_sbc_stream_reset(void)
{
    sbc_stream.total_tx_frames = 0;
    sbc_stream.media_timestamp = 0;
    sbc_stream.session_start_us = get_os_timestamp_us();
}

int a2dp_source_sbc_interval_ms(void)
{
    return A2DP_SBC_ENCODER_INTERVAL_MS;
}