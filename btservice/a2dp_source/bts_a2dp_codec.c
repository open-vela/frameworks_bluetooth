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
#include <stdio.h>
#include <stdlib.h>

#include "btm_manager.h"
#include "bts_a2dp_codec.h"
#include "bts_service.h"
#include "a2dp_codec_sbc.h"

#define LOG_TAG "a2dp_codec"
#include "log.h"

#define A2DP_SBC_ENCODER_INTERVAL_MS 20

a2dp_codec_t g_a2dp_codec;
a2dp_codec_config_t* current_config;

static const a2dp_codec_config_t a2dp_codec_default_config = {
    BTS_A2DP_TYPE_SBC,
    44100,
    BTS_A2DP_CODEC_BITS_PER_SAMPLE_16,
    BTS_A2DP_CODEC_CHANNEL_MODE_STEREO,
    328000,
};

uint32_t bts_a2dp_codec_interval_ms(void)
{
    return A2DP_SBC_ENCODER_INTERVAL_MS;
}

void bts_a2dp_codec_init(void)
{
    current_config = &g_a2dp_codec.current_codec_config;

    memcpy(current_config, &a2dp_codec_default_config, sizeof(a2dp_codec_config_t));
    if (current_config->codec_type == BTS_A2DP_TYPE_SBC)
        a2dp_codec_sbc_init();
}

a2dp_codec_config_t* bts_a2dp_codec_get_config(void)
{
    return current_config;
}

void bts_a2dp_codec_set_config(a2dp_codec_config_t* config)
{
    memcpy(current_config, config, sizeof(a2dp_codec_config_t));
    if (current_config->codec_type == BTS_A2DP_TYPE_SBC)
        current_config->bit_rate = a2dp_codec_sbc_bit_rate(bts_a2dp_codec_get_frame_length());
}

uint32_t bts_a2dp_codec_get_frame_length(void)
{
    if (current_config->codec_type == BTS_A2DP_TYPE_SBC)
        return a2dp_codec_sbc_frame_length();

    return 0; //unknown codec
}
