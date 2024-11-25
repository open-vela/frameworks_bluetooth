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
#ifndef __A2DP_CODEC_AAC_H__
#define __A2DP_CODEC_AAC_H__

#include <stdint.h>

// [Octet 0] Object Type
#define A2DP_AAC_OBJECT_TYPE_MPEG2_LC 0x80 /* MPEG-2 Low Complexity */
#define A2DP_AAC_OBJECT_TYPE_MPEG4_LC 0x40 /* MPEG-4 Low Complexity */
#define A2DP_AAC_OBJECT_TYPE_MPEG4_LTP 0x20 /* MPEG-4 Long Term Prediction */
#define A2DP_AAC_OBJECT_TYPE_MPEG4_SCALABLE 0x10

// [Octet 1] Sampling Frequency - 8000 to 44100
#define A2DP_AAC_SAMPLING_FREQ_MASK0 0xFF
#define A2DP_AAC_SAMPLING_FREQ_8000 0x80
#define A2DP_AAC_SAMPLING_FREQ_11025 0x40
#define A2DP_AAC_SAMPLING_FREQ_12000 0x20
#define A2DP_AAC_SAMPLING_FREQ_16000 0x10
#define A2DP_AAC_SAMPLING_FREQ_22050 0x08
#define A2DP_AAC_SAMPLING_FREQ_24000 0x04
#define A2DP_AAC_SAMPLING_FREQ_32000 0x02
#define A2DP_AAC_SAMPLING_FREQ_44100 0x01
// [Octet 2], [Bits 4-7] Sampling Frequency - 48000 to 96000
// NOTE: Bits offset for the higher-order octet 16-bit integer
#define A2DP_AAC_SAMPLING_FREQ_MASK1 (0xF0 << 8)
#define A2DP_AAC_SAMPLING_FREQ_48000 (0x80 << 8)
#define A2DP_AAC_SAMPLING_FREQ_64000 (0x40 << 8)
#define A2DP_AAC_SAMPLING_FREQ_88200 (0x20 << 8)
#define A2DP_AAC_SAMPLING_FREQ_96000 (0x10 << 8)
// [Octet 2], [Bits 2-3] Channel Mode
#define A2DP_AAC_CHANNEL_MODE_MASK 0x0C
#define A2DP_AAC_CHANNEL_MODE_MONO 0x08
#define A2DP_AAC_CHANNEL_MODE_STEREO 0x04
// [Octet 2], [Bits 0-1] RFA
// [Octet 3], [Bit 7] Variable Bit Rate Supported
#define A2DP_AAC_VARIABLE_BIT_RATE_MASK 0x80
#define A2DP_AAC_VARIABLE_BIT_RATE_ENABLED 0x80
#define A2DP_AAC_VARIABLE_BIT_RATE_DISABLED 0x00
// [Octet 3], [Bits 0-6] Bit Rate - Bits 16-22 in the 23-bit UiMsbf
#define A2DP_AAC_BIT_RATE_MASK0 (0x7F << 16)
#define A2DP_AAC_BIT_RATE_MASK1 (0xFF << 8)
#define A2DP_AAC_BIT_RATE_MASK2 0xFF

typedef struct {
    uint16_t u16ObjectType;
    uint16_t u16VariableBitRate;
    uint16_t u16ChannelMode; /* mono, streo */
    uint16_t u16NumOfChannels; /* 1, 2 */
    uint32_t u32SampleRate;
    uint32_t u32BitRate;
} aac_encoder_param_t;

int a2dp_codec_parse_aac_param(aac_encoder_param_t* param, uint8_t* codec_info, uint16_t tx_mtu_size);
#endif
