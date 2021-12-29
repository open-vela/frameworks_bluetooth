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
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "a2dp_codec_sbc.h"
#include "sbc_encoder.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_source_audio.h"

#define LOG_TAG "a2dp_codec_sbc"
#include "log.h"

#define A2DP_SBC_BIT_PER_SAMPLE  16

extern a2dp_source_stream_t a2dp_src_stream;


typedef struct {
    uint8_t samp_freq;    /* Sampling frequency */
    uint8_t ch_mode;      /* Channel mode */
    uint8_t block_len;    /* Block length */
    uint8_t num_subbands; /* Number of subbands */
    uint8_t alloc_method; /* Allocation method */
    uint8_t min_bitpool;  /* Minimum bitpool */
    uint8_t max_bitpool;  /* Maximum bitpool */
} a2dp_sbc_info_t;

static int a2dp_parse_sbc_info(a2dp_sbc_info_t *info, uint8_t* codec_info)
{
    if (info == NULL || codec_info == NULL) {
        return -1;
    }

    info->samp_freq = *codec_info & A2DP_SBC_SAMP_FREQ_MSK;
    info->ch_mode = *codec_info & A2DP_SBC_CH_MD_MSK;
    codec_info++;
    info->block_len = *codec_info & A2DP_SBC_BLOCKS_MSK;
    info->num_subbands = *codec_info & A2DP_SBC_SUBBAND_MSK;
    info->alloc_method = *codec_info & A2DP_SBC_ALLOC_MD_MSK;
    codec_info++;
    info->min_bitpool = *codec_info++;
    info->max_bitpool = *codec_info++;

    return 0;
}

static int a2dp_get_sbc_allocation_method(a2dp_sbc_info_t *info)
{
    switch (info->alloc_method) {
        case A2DP_SBC_ALLOC_MD_S:
            return SBC_SNR;
        case A2DP_SBC_ALLOC_MD_L:
            return SBC_LOUDNESS;
        default:
            break;
    }

    return -1;
}

static int a2dp_get_sbc_blocks(a2dp_sbc_info_t *info)
{
    switch (info->block_len) {
        case A2DP_SBC_BLOCKS_4:
            return SBC_BLOCK_0;
        case A2DP_SBC_BLOCKS_8:
            return SBC_BLOCK_1;
        case A2DP_SBC_BLOCKS_12:
            return SBC_BLOCK_2;
        case A2DP_SBC_BLOCKS_16:
            return SBC_BLOCK_3;
        default:
            break;
    }

    return -1;
}

static int a2dp_get_sbc_subbands(a2dp_sbc_info_t *info)
{
    switch (info->num_subbands) {
        case A2DP_SBC_SUBBAND_4:
            return SUB_BANDS_4;
        case A2DP_SBC_SUBBAND_8:
            return SUB_BANDS_8;
        default:
            break;
    }

    return -1;
}

static int a2dp_get_sbc_samp_frequency(a2dp_sbc_info_t *info)
{
    switch (info->samp_freq) {
        case A2DP_SBC_SAMP_FREQ_16:
            return SBC_SF_16000;
        case A2DP_SBC_SAMP_FREQ_32:
            return SBC_SF_32000;
        case A2DP_SBC_SAMP_FREQ_44:
            return SBC_SF_44100;
        case A2DP_SBC_SAMP_FREQ_48:
            return SBC_SF_48000;
        default:
            break;
    }

    return -1;
}

static int a2dp_get_sbc_channel_mode(a2dp_sbc_info_t *info)
{
    switch (info->ch_mode) {
        case A2DP_SBC_CH_MD_MONO:
            return SBC_MONO;
        case A2DP_SBC_CH_MD_DUAL:
            return SBC_DUAL;
        case A2DP_SBC_CH_MD_STEREO:
            return SBC_STEREO;
        case A2DP_SBC_CH_MD_JOINT:
            return SBC_JOINT_STEREO;
        default:
            break;
  }

  return -1;
}

static int a2dp_get_sbc_channel_count(a2dp_sbc_info_t *info)
{
    return SBC_MAX_NUM_OF_CHANNELS;
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

uint32_t a2dp_codec_sbc_frame_length(sbc_param_t* param)
{
    uint32_t frame_len, frame_len2;

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

    //BT_LOGD("%s :frame_len:%u, frame_len2:%u", __func__, frame_len, frame_len2);
    assert(frame_len == frame_len2);

    return frame_len;
}

uint32_t a2dp_codec_sbc_bit_rate(sbc_param_t* param)
{
    uint16_t samp_freq;
    uint32_t bit_rate;
    uint32_t frame_len;

    frame_len = a2dp_codec_sbc_frame_length(param);
    samp_freq = a2dp_codec_sample_frequency(param->s16SamplingFreq);
    bit_rate = (8 * frame_len * samp_freq) /
                (param->s16NumOfSubBands * param->s16NumOfBlocks);
    BT_LOGD("%s, birtate: %lu", __func__, bit_rate);

    return bit_rate;
}

void a2dp_codec_parse_sbc_param(sbc_param_t* param, uint8_t* codec_info)
{
    a2dp_sbc_info_t si;

    if (a2dp_parse_sbc_info(&si, codec_info) != 0)
        return;

    param->s16SamplingFreq = a2dp_get_sbc_samp_frequency(&si);
    param->s16ChannelMode = a2dp_get_sbc_channel_mode(&si);
    param->s16NumOfSubBands = a2dp_get_sbc_subbands(&si);
    param->s16NumOfChannels = a2dp_get_sbc_channel_count(&si);
    param->s16NumOfBlocks = a2dp_get_sbc_blocks(&si);
    param->s16AllocationMethod = a2dp_get_sbc_allocation_method(&si);
    param->s16BitPool = si.max_bitpool;
    param->u32BitRate = a2dp_codec_sbc_bit_rate(param);

    BT_LOGD("%s:\n \
                s16SamplingFreq:%d,\n \
                s16ChannelMode:%d,\n \
                s16NumOfSubBands:%d,\n \
                s16NumOfChannels:%d,\n \
                s16NumOfBlocks:%d,\n \
                s16AllocationMethod:%d,\n \
                s16BitPool:%d,\n \
                u32BitRate:%lu", __func__, param->s16SamplingFreq,
                param->s16ChannelMode,
                param->s16NumOfSubBands,
                param->s16NumOfChannels,
                param->s16NumOfBlocks,
                param->s16AllocationMethod,
                param->s16BitPool,
                param->u32BitRate);
}

uint8_t calculate_max_frames_per_packet(void)
{
    uint32_t frame_len = bts_a2dp_codec_get_frame_length();
    if (!a2dp_src_stream.mtu || !frame_len)
        return 0;

    // dynamic process
    return (a2dp_src_stream.mtu - 1) / frame_len;
}

void a2dp_codec_sbc_media_timestamp(sbc_param_t* param)
{
    /*
     * Timestamp of the media packet header represent the TS of the
     * first SBC frame, i.e the timestamp before including this frame.
     */
    uint16_t blocm_x_subband
        = param->s16NumOfSubBands * param->s16NumOfBlocks;
    a2dp_src_stream.media_timestamp
        += blocm_x_subband * a2dp_src_stream.last_tx_frames;
}

void a2dp_codec_sbc_get_num_frame_iteration(sbc_param_t* param, uint8_t* noi, uint8_t* nof,
                                             uint64_t now_timestamp_us)
{
    uint16_t sample_rate;
    switch (param->s16SamplingFreq) {
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
    uint16_t per_frame_bytes = param->s16NumOfBlocks *
                               param->s16NumOfSubBands *
                               param->s16NumOfChannels *
                               A2DP_SBC_BIT_PER_SAMPLE / 8;
    /* PCM bytes read each media task tick */
    uint16_t bytes_per_tick = (sample_rate *
                              A2DP_SBC_BIT_PER_SAMPLE / 8 *
                              param->s16NumOfChannels *
                              a2dp_src_stream.interval_ms) /
                              1000;
    /* Calculate the playback time of per PCM frame */
    uint64_t per_frame_time = per_frame_bytes * 1000000 /
                              (sample_rate *
                              (A2DP_SBC_BIT_PER_SAMPLE / 8) *
                              param->s16NumOfChannels);
    /* Calculate the actual playback timestamp according to the total PCM frames
     * we send */
    uint64_t actual_timestamp_us = 
             a2dp_src_stream.session_start_us +
             a2dp_src_stream.total_tx_frames *
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
