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
#ifndef __A2DP_CODEC_SBC_H__
#define __A2DP_CODEC_SBC_H__

#include "sbc_encoder.h"
/* the length of the SBC Media Payload header. */
#define A2DP_SBC_MPL_HDR_LEN 1

/* the LOSC of SBC media codec capabilitiy */
#define A2DP_SBC_INFO_LEN 6

/* for Codec Specific Information Element */
#define A2DP_SBC_SAMP_FREQ_MSK 0xF0 /* b7-b4 sampling frequency */
#define A2DP_SBC_SAMP_FREQ_16 0x80 /* b7:16  kHz */
#define A2DP_SBC_SAMP_FREQ_32 0x40 /* b6:32  kHz */
#define A2DP_SBC_SAMP_FREQ_44 0x20 /* b5:44.1kHz */
#define A2DP_SBC_SAMP_FREQ_48 0x10 /* b4:48  kHz */

#define A2DP_SBC_CH_MD_MSK 0x0F /* b3-b0 channel mode */
#define A2DP_SBC_CH_MD_MONO 0x08 /* b3: mono */
#define A2DP_SBC_CH_MD_DUAL 0x04 /* b2: dual */
#define A2DP_SBC_CH_MD_STEREO 0x02 /* b1: stereo */
#define A2DP_SBC_CH_MD_JOINT 0x01 /* b0: joint stereo */

#define A2DP_SBC_BLOCKS_MSK 0xF0 /* b7-b4 number of blocks */
#define A2DP_SBC_BLOCKS_4 0x80 /* 4 blocks */
#define A2DP_SBC_BLOCKS_8 0x40 /* 8 blocks */
#define A2DP_SBC_BLOCKS_12 0x20 /* 12blocks */
#define A2DP_SBC_BLOCKS_16 0x10 /* 16blocks */

#define A2DP_SBC_SUBBAND_MSK 0x0C /* b3-b2 number of subbands */
#define A2DP_SBC_SUBBAND_4 0x08 /* b3: 4 */
#define A2DP_SBC_SUBBAND_8 0x04 /* b2: 8 */

#define A2DP_SBC_ALLOC_MD_MSK 0x03 /* b1-b0 allocation mode */
#define A2DP_SBC_ALLOC_MD_S 0x02 /* b1: SNR */
#define A2DP_SBC_ALLOC_MD_L 0x01 /* b0: loundess */

#define A2DP_SBC_MIN_BITPOOL 2
#define A2DP_SBC_MAX_BITPOOL 250
#define A2DP_SBC_BITPOOL_MIDDLE_QUALITY 35

/* for media payload header */
#define A2DP_SBC_HDR_F_MSK 0x80
#define A2DP_SBC_HDR_S_MSK 0x40
#define A2DP_SBC_HDR_L_MSK 0x20
#define A2DP_SBC_HDR_NUM_MSK 0x0F

void a2dp_codec_parse_sbc_param(sbc_param_t* param, uint8_t* codec_info);
uint16_t a2dp_sbc_sample_frequency(uint16_t sample_frequency);
uint32_t a2dp_sbc_frame_length(sbc_param_t* param);
uint32_t a2dp_sbc_bit_rate(sbc_param_t* param);
uint16_t a2dp_sbc_max_latency(sbc_param_t* param);
uint8_t a2dp_sbc_bits_per_sample(sbc_param_t* param);
uint32_t a2dp_sbc_encoded_audio_bitrate(sbc_param_t* param);
uint16_t a2dp_sbc_frame_sample(sbc_param_t* param);
uint8_t a2dp_get_sbc_ch_mode(sbc_param_t* param);

#endif
