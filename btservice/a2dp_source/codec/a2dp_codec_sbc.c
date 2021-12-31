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
#include <stdlib.h>
#include <string.h>

#include "a2dp_codec_sbc.h"
#include "sbc_encoder.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_source_audio.h"
static sbc_param_t* sbc_param;

#define LOG_TAG "a2dp_codec_sbc"
#include "log.h"

#define A2DP_SBC_BIT_PER_SAMPLE  16

extern a2dp_source_stream_t a2dp_src_stream;

void a2dp_codec_sbc_init(void)
{
    const sbc_param_t* defaultparam;
    sbc_param = malloc(sizeof(sbc_param_t));
    defaultparam = sbc_encoder_param_get();
    memcpy(sbc_param, defaultparam, sizeof(sbc_param_t));
}

uint16_t a2dp_codec_sample_frequency(uint16_t sample_frequency)
{
    uint16_t sampling_freq;

    if (sample_frequency == SBC_SF_16000)
        sampling_freq = 16000;
    else if (sample_frequency == SBC_SF_32000)
        sampling_freq = 32000;
    else if (sample_frequency == SBC_SF_44100)
        sampling_freq = 44100;
    else
        sampling_freq = 48000;

    return sampling_freq;
}

uint32_t a2dp_codec_sbc_frame_length(void)
{
    uint32_t frame_len;
#if 1
    if (sbc_param->s16ChannelMode == SBC_JOINT_STEREO || sbc_param->s16ChannelMode == SBC_STEREO) {
        frame_len = 4 + (4 * sbc_param->s16NumOfSubBands * sbc_param->s16NumOfChannels) / 8 + (((sbc_param->s16ChannelMode - 2) * sbc_param->s16NumOfSubBands) + (sbc_param->s16NumOfBlocks * sbc_param->s16BitPool)) / 8;
    } else {
        frame_len = 4 + ((4 * sbc_param->s16NumOfSubBands * sbc_param->s16NumOfChannels) / 8) + ((sbc_param->s16NumOfBlocks * sbc_param->s16NumOfChannels * sbc_param->s16BitPool) / 8);
    }
#else
    frame_len = 4 + (4 * sbc_param->s16NumOfSubBands * sbc_param->s16NumOfChannels) / 8
        + ((sbc_param->s16NumOfBlocks * sbc_param->s16BitPool * (1 + (sbc_param->s16ChannelMode == SBC_DUAL))
               + (sbc_param->s16ChannelMode == SBC_JOINT_STEREO) * sbc_param->s16NumOfSubBands)
              + 7)
            / 8;
#endif
//    BT_LOGD("%s :frame_len:%lu", __func__, frame_len);

    return frame_len;
}

uint32_t a2dp_codec_sbc_bit_rate(uint32_t frame_len)
{
    uint16_t sampling_freq;
    uint32_t bit_rate;

    sampling_freq = a2dp_codec_sample_frequency(sbc_param->s16SamplingFreq);
    bit_rate = (8 * frame_len * sampling_freq) / (sbc_param->s16NumOfSubBands * sbc_param->s16NumOfBlocks);
    BT_LOGD("%s, birtate: %lu", __func__, bit_rate);

    return bit_rate;
}

uint8_t calculate_max_frames_per_packet(void)
{
    uint32_t frame_len = bts_a2dp_codec_get_frame_length();
    if (!a2dp_src_stream.mtu || !frame_len)
        return 0;

    // dynamic process
    return (a2dp_src_stream.mtu - 1) / frame_len;
}

void a2dp_codec_sbc_media_timestamp(void)
{
    /*
     * Timestamp of the media packet header represent the TS of the
     * first SBC frame, i.e the timestamp before including this frame.
     */
    uint16_t blocm_x_subband
        = sbc_param->s16NumOfSubBands * sbc_param->s16NumOfBlocks;
    a2dp_src_stream.media_timestamp
        += blocm_x_subband * a2dp_src_stream.last_tx_frames;
}

void a2dp_codec_sbc_get_num_frame_interation(uint8_t* noi, uint8_t* nof,
                                             uint64_t now_timestamp_us)
{
    uint16_t sample_rate;
    switch (sbc_param->s16SamplingFreq) {
    case SBC_SF_44100:
        sample_rate = 44100;
        break;
    case SBC_SF_48000:
        sample_rate = 48000;
    default:
        sample_rate = 44100;
        break;
    }
    /* PCM bytes of per  frame */
    uint16_t per_frame_bytes = sbc_param->s16NumOfBlocks
        * sbc_param->s16NumOfSubBands * sbc_param->s16NumOfChannels
        * A2DP_SBC_BIT_PER_SAMPLE / 8;
    /* PCM bytes read each media task tick */
    uint16_t bytes_per_tick
        = (sample_rate * A2DP_SBC_BIT_PER_SAMPLE / 8
           * sbc_param->s16NumOfChannels * a2dp_src_stream.interval_ms)
        / 1000;
    /* Calculate the playback time of per PCM frame */
    uint64_t per_frame_time = per_frame_bytes * 1000000
        / (sample_rate * (A2DP_SBC_BIT_PER_SAMPLE / 8)
           * sbc_param->s16NumOfChannels);
    /* Calculate the actual playback timestamp according to the total PCM frames
     * we send */
    uint64_t actual_timestamp_us
        = a2dp_src_stream.session_start_us + a2dp_src_stream.total_tx_frames * per_frame_time;

    *noi = 1;
    uint8_t projected_nof = (bytes_per_tick + per_frame_bytes / 2) / per_frame_bytes;
    *nof = projected_nof > calculate_max_frames_per_packet()
        ? calculate_max_frames_per_packet() : projected_nof;

    if (now_timestamp_us >= actual_timestamp_us) {
        uint8_t delta_frames = (now_timestamp_us - actual_timestamp_us) / per_frame_time;
        if (delta_frames / *nof)  *noi = 2;
    } else {
      *nof = *nof - 1;
    }
}
