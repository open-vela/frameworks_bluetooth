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
#ifndef __BTS_A2DP_CODEC_H__
#define __BTS_A2DP_CODEC_H__

#include <sys/types.h>

typedef enum {
    BTS_A2DP_CODEC_INDEX_SOURCE_MIN = 0,

    // Add an entry for each source codec here.
    // NOTE: The values should be same as those listed in the following file:
    //   BluetoothCodecConfig.java
    BTS_A2DP_CODEC_INDEX_SOURCE_SBC = 0,
    BTS_A2DP_CODEC_INDEX_SOURCE_AAC,
    BTS_A2DP_CODEC_INDEX_SOURCE_APTX,
    BTS_A2DP_CODEC_INDEX_SOURCE_APTX_HD,
    BTS_A2DP_CODEC_INDEX_SOURCE_LDAC,

    BTS_A2DP_CODEC_INDEX_SOURCE_MAX,

    BTS_A2DP_CODEC_INDEX_SINK_MIN = BTS_A2DP_CODEC_INDEX_SOURCE_MAX,

    // Add an entry for each sink codec here
    BTS_A2DP_CODEC_INDEX_SINK_SBC = BTS_A2DP_CODEC_INDEX_SINK_MIN,
    BTS_A2DP_CODEC_INDEX_SINK_AAC,
    BTS_A2DP_CODEC_INDEX_SINK_LDAC,

    BTS_A2DP_CODEC_INDEX_SINK_MAX,

    BTS_A2DP_CODEC_INDEX_MIN = BTS_A2DP_CODEC_INDEX_SOURCE_MIN,
    BTS_A2DP_CODEC_INDEX_MAX = BTS_A2DP_CODEC_INDEX_SINK_MAX
} bts_a2dp_codec_index_t;

typedef enum {
    BTS_A2DP_CODEC_SAMPLE_RATE_NONE = 0x0,
    BTS_A2DP_CODEC_SAMPLE_RATE_44100 = 0x1 << 0,
    BTS_A2DP_CODEC_SAMPLE_RATE_48000 = 0x1 << 1,
    BTS_A2DP_CODEC_SAMPLE_RATE_88200 = 0x1 << 2,
    BTS_A2DP_CODEC_SAMPLE_RATE_96000 = 0x1 << 3,
    BTS_A2DP_CODEC_SAMPLE_RATE_176400 = 0x1 << 4,
    BTS_A2DP_CODEC_SAMPLE_RATE_192000 = 0x1 << 5,
    BTS_A2DP_CODEC_SAMPLE_RATE_16000 = 0x1 << 6,
    BTS_A2DP_CODEC_SAMPLE_RATE_24000 = 0x1 << 7
} bts_a2dp_codec_sample_rate_t;

typedef enum {
    BTS_A2DP_CODEC_BITS_PER_SAMPLE_NONE = 0x0,
    BTS_A2DP_CODEC_BITS_PER_SAMPLE_16 = 0x1 << 0,
    BTS_A2DP_CODEC_BITS_PER_SAMPLE_24 = 0x1 << 1,
    BTS_A2DP_CODEC_BITS_PER_SAMPLE_32 = 0x1 << 2
} bts_a2dp_codec_bits_per_sample_t;

typedef enum {
    BTS_A2DP_CODEC_CHANNEL_MODE_NONE = 0x0,
    BTS_A2DP_CODEC_CHANNEL_MODE_MONO = 0x1 << 0,
    BTS_A2DP_CODEC_CHANNEL_MODE_STEREO = 0x1 << 1
} bts_a2dp_codec_channel_mode_t;

typedef struct {
    bts_a2dp_codec_index_t codec_type;
    bts_a2dp_codec_sample_rate_t sample_rate;
    bts_a2dp_codec_bits_per_sample_t bits_per_sample;
    bts_a2dp_codec_channel_mode_t channel_mode;
    uint32_t bit_rate;
} a2dp_codec_config_t;

typedef struct {
    a2dp_codec_config_t current_codec_config;
} a2dp_codec_t;

void bts_a2dp_codec_init(void);
a2dp_codec_config_t* bts_a2dp_codec_get_config(void);
void bts_a2dp_codec_set_config(a2dp_codec_config_t* config);
uint32_t bts_a2dp_codec_interval_ms(void);
uint32_t bts_a2dp_codec_get_frame_length(void);

#endif
