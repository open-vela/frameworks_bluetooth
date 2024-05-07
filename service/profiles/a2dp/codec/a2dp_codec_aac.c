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

#include "a2dp_codec.h"
#include "a2dp_codec_aac.h"
#include "a2dp_source_audio.h"

#define LOG_TAG "a2dp_codec_aac"
#include "utils/log.h"

typedef struct {
    uint8_t object_type;
    uint16_t sample_rate;
    uint8_t channel_mode;
    uint8_t variable_bit_rate;
    uint32_t bit_rate;
} a2dp_aac_info_t;

static int a2dp_parse_aac_info(a2dp_aac_info_t* info, uint8_t* codec_info)
{
    if (info == NULL || codec_info == NULL)
        return -1;

    info->object_type = *codec_info++;
    info->sample_rate = (*codec_info & A2DP_AAC_SAMPLING_FREQ_MASK0) | (*(codec_info + 1) << 8 & A2DP_AAC_SAMPLING_FREQ_MASK1);
    codec_info++;
    info->channel_mode = *codec_info & A2DP_AAC_CHANNEL_MODE_MASK;
    info->variable_bit_rate = *codec_info & A2DP_AAC_VARIABLE_BIT_RATE_MASK;
    codec_info++;
    info->bit_rate = (*codec_info << 16 & A2DP_AAC_BIT_RATE_MASK0) | (*(codec_info + 1) << 8 & A2DP_AAC_BIT_RATE_MASK1) | (*(codec_info + 2) & A2DP_AAC_BIT_RATE_MASK2);
    if (info->object_type == 0 || info->sample_rate == 0 || info->channel_mode == 0)
        return -1;

    return 0;
}

static int a2dp_get_aac_samplerate(a2dp_aac_info_t* info)
{

    if (info == NULL)
        return -1;

    switch (info->sample_rate) {
    case A2DP_AAC_SAMPLING_FREQ_8000:
        return 8000;
    case A2DP_AAC_SAMPLING_FREQ_11025:
        return 11025;
    case A2DP_AAC_SAMPLING_FREQ_12000:
        return 12000;
    case A2DP_AAC_SAMPLING_FREQ_16000:
        return 16000;
    case A2DP_AAC_SAMPLING_FREQ_22050:
        return 22050;
    case A2DP_AAC_SAMPLING_FREQ_24000:
        return 24000;
    case A2DP_AAC_SAMPLING_FREQ_32000:
        return 32000;
    case A2DP_AAC_SAMPLING_FREQ_44100:
        return 44100;
    case A2DP_AAC_SAMPLING_FREQ_48000:
        return 48000;
    case A2DP_AAC_SAMPLING_FREQ_64000:
        return 64000;
    case A2DP_AAC_SAMPLING_FREQ_88200:
        return 88200;
    case A2DP_AAC_SAMPLING_FREQ_96000:
        return 96000;
    }

    return -1;
}

static int a2dp_get_aac_number_of_channels(a2dp_aac_info_t* info)
{
    if (info == NULL)
        return -1;

    if (info->channel_mode == A2DP_AAC_CHANNEL_MODE_MONO)
        return 1;
    else if (info->channel_mode == A2DP_AAC_CHANNEL_MODE_STEREO)
        return 2;

    return -1;
}

int a2dp_codec_parse_aac_param(aac_encoder_param_t* param, uint8_t* codec_info, uint16_t tx_mtu_size)
{
    a2dp_aac_info_t info;
    uint32_t bitrate;

    if (param == NULL || codec_info == NULL)
        return -1;

    if (a2dp_parse_aac_info(&info, codec_info) != 0)
        return -1;

    param->u16ObjectType = info.object_type;
    param->u16VariableBitRate = info.variable_bit_rate;
    param->u16ChannelMode = info.channel_mode;
    param->u16NumOfChannels = a2dp_get_aac_number_of_channels(&info);
    param->u32SampleRate = a2dp_get_aac_samplerate(&info);
    if (tx_mtu_size > 0) {
        bitrate = (8 * tx_mtu_size * param->u32SampleRate) / 1024;
        param->u32BitRate = bitrate < info.bit_rate ? bitrate : info.bit_rate;
    } else
        param->u32BitRate = info.bit_rate;

    BT_LOGD("MaxTxSize:%" PRIu16 ", BitRate peer: %" PRIu32 ", adjust:%" PRIu32,
        tx_mtu_size, info.bit_rate, param->u32BitRate);
    BT_LOGD("%s:\n \
                u16ObjectType:0x%02x,\n \
                u16VariableBitRate:0x%02x, \n \
                u16ChannelMode:0x%02x,\n \
                u16NumOfChannels:%d,\n \
                u32SampleRate:%" PRIu32 ",\n \
                u32BitRate:%" PRIu32,
        __func__, param->u16ObjectType,
        param->u16VariableBitRate,
        param->u16ChannelMode,
        param->u16NumOfChannels,
        param->u32SampleRate,
        param->u32BitRate);

    return 0;
}
