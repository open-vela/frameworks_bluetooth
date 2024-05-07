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
#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "a2dp_codec_sbc.h"
#include "sbc_encoder.h"

#define LOG_TAG "a2dp_codec_sbc"
#include "utils/log.h"

typedef struct {
    uint8_t samp_freq; /* Sampling frequency */
    uint8_t ch_mode; /* Channel mode */
    uint8_t block_len; /* Block length */
    uint8_t num_subbands; /* Number of subbands */
    uint8_t alloc_method; /* Allocation method */
    uint8_t min_bitpool; /* Minimum bitpool */
    uint8_t max_bitpool; /* Maximum bitpool */
} a2dp_sbc_info_t;

static int a2dp_parse_sbc_info(a2dp_sbc_info_t* info, uint8_t* codec_info)
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

static int a2dp_get_sbc_allocation_method(a2dp_sbc_info_t* info)
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

static int a2dp_get_sbc_blocks(a2dp_sbc_info_t* info)
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

static int a2dp_get_sbc_subbands(a2dp_sbc_info_t* info)
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

static int a2dp_get_sbc_samp_frequency(a2dp_sbc_info_t* info)
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

static int a2dp_get_sbc_channel_mode(a2dp_sbc_info_t* info)
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

static int a2dp_get_sbc_channel_count(a2dp_sbc_info_t* info)
{
    return SBC_MAX_NUM_OF_CHANNELS;
}

uint32_t a2dp_sbc_frame_length(sbc_param_t* param)
{
    uint32_t frame_len;

    if (param == NULL) {
        BT_LOGE("%s, error param", __func__);
        return 0;
    }

    if (param->s16ChannelMode == SBC_MONO) {
        BT_LOGE("%s, not support mono mode", __func__);
        return 0;
    }

    frame_len = 4 + (4 * param->s16NumOfSubBands * param->s16NumOfChannels) / 8 + ((param->s16NumOfBlocks * param->s16BitPool * (1 + (param->s16ChannelMode == SBC_DUAL)) + (param->s16ChannelMode == SBC_JOINT_STEREO) * param->s16NumOfSubBands) + 7) / 8;

    return frame_len;
}

uint16_t a2dp_sbc_sample_frequency(uint16_t sample_frequency)
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

uint32_t a2dp_sbc_bit_rate(sbc_param_t* param)
{
    uint16_t samp_freq;
    uint32_t bit_rate;
    uint32_t frame_len;

    frame_len = a2dp_sbc_frame_length(param);
    samp_freq = a2dp_sbc_sample_frequency(param->s16SamplingFreq);
    bit_rate = (8 * frame_len * samp_freq) / (param->s16NumOfSubBands * param->s16NumOfBlocks);
    BT_LOGD("%s, birtate: %" PRIu32, __func__, bit_rate);

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
    param->u32BitRate = a2dp_sbc_bit_rate(param);

    BT_LOGD("%s:\n \
                s16SamplingFreq:%d,\n \
                s16ChannelMode:%d,\n \
                s16NumOfSubBands:%d,\n \
                s16NumOfChannels:%d,\n \
                s16NumOfBlocks:%d,\n \
                s16AllocationMethod:%d,\n \
                s16BitPool:%d,\n \
                u32BitRate:%" PRIu32,
        __func__, param->s16SamplingFreq,
        param->s16ChannelMode,
        param->s16NumOfSubBands,
        param->s16NumOfChannels,
        param->s16NumOfBlocks,
        param->s16AllocationMethod,
        param->s16BitPool,
        param->u32BitRate);
}

uint16_t a2dp_sbc_frame_sample(sbc_param_t* param)
{
    return param->s16NumOfSubBands * param->s16NumOfBlocks;
}

uint16_t a2dp_sbc_max_latency(sbc_param_t* param)
{
    /* todo: */
    return 0;
}

uint8_t a2dp_sbc_bits_per_sample(sbc_param_t* param)
{
    /* todo: */
    return 0;
}

uint32_t a2dp_sbc_encoded_audio_bitrate(sbc_param_t* param)
{
    return a2dp_sbc_bit_rate(param);
}

uint8_t a2dp_get_sbc_ch_mode(sbc_param_t* param)
{
    /* todo: */
    return 0;
}