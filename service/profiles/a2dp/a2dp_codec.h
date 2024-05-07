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
#ifndef __A2DP_CODEC_H__
#define __A2DP_CODEC_H__

#include "a2dp_codec_aac.h"
#include "a2dp_codec_sbc.h"
#include "bt_vendor.h"

#include <sys/types.h>

typedef enum {
    BTS_A2DP_TYPE_SBC,
    BTS_A2DP_TYPE_MPEG1_2_AUDIO,
    BTS_A2DP_TYPE_MPEG2_4_AAC,
    BTS_A2DP_TYPE_ATRAC,
    BTS_A2DP_TYPE_OPUS,
    BTS_A2DP_TYPE_H263,
    BTS_A2DP_TYPE_MPEG4_VSP,
    BTS_A2DP_TYPE_H263_PROF3,
    BTS_A2DP_TYPE_H263_PROF8,
    BTS_A2DP_TYPE_LHDC,
    BTS_A2DP_TYPE_NON_A2DP
} a2dp_codec_index_t;

typedef uint32_t a2dp_codec_sample_rate_t;

typedef enum {
    BTS_A2DP_CODEC_BITS_PER_SAMPLE_8 = 0x0,
    BTS_A2DP_CODEC_BITS_PER_SAMPLE_16 = 0x1,
} a2dp_codec_bits_per_sample_t;

typedef enum {
    BTS_A2DP_CODEC_CHANNEL_MODE_MONO = 0x0,
    BTS_A2DP_CODEC_CHANNEL_MODE_STEREO = 0x1
} a2dp_codec_channel_mode_t;

typedef struct {
    a2dp_codec_index_t codec_type;
    a2dp_codec_sample_rate_t sample_rate;
    a2dp_codec_bits_per_sample_t bits_per_sample;
    a2dp_codec_channel_mode_t channel_mode;
    uint16_t acl_hdl;
    uint16_t l2c_rcid;
    uint32_t bit_rate;
    uint32_t frame_size;
    uint32_t packet_size;
    uint8_t specific_info[20];
    union {
        sbc_param_t sbc;
        aac_encoder_param_t aac;
    } codec_param;
} a2dp_codec_config_t;

a2dp_codec_config_t* a2dp_codec_get_config(void);
void a2dp_codec_set_config(uint8_t peer_sep, a2dp_codec_config_t* config);
void a2dp_codec_update_config(uint8_t peer_sep, a2dp_codec_config_t* config, uint16_t mtu);
bool a2dp_codec_get_offload_config(a2dp_offload_config_t* config);

#endif
