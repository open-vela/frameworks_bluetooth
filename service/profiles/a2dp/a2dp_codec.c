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
#include <stdio.h>
#include <stdlib.h>

#include "a2dp_codec.h"
#include "a2dp_device.h"
#include "a2dp_source_audio.h"
#include "bt_vendor.h"

#define LOG_TAG "a2dp_codec"
#include "utils/log.h"

a2dp_codec_config_t g_current_config;

static void a2dp_codec_config_set(uint8_t peer_sep, a2dp_codec_config_t* config, uint16_t mtu)
{
    if (config->codec_type == BTS_A2DP_TYPE_SBC) {
        if (peer_sep == SEP_SNK) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            a2dp_source_sbc_update_config(mtu, &config->codec_param.sbc, config->specific_info);
#endif
        } else {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            a2dp_codec_parse_sbc_param(&config->codec_param.sbc, config->specific_info);
#endif
        }
        config->bit_rate = config->codec_param.sbc.u32BitRate;
#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
    } else if (config->codec_type == BTS_A2DP_TYPE_MPEG2_4_AAC) {
        if (peer_sep == SEP_SNK) {
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
            a2dp_source_aac_update_config(mtu, &config->codec_param.aac, config->specific_info);
#endif
        } else {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
            a2dp_codec_parse_aac_param(&config->codec_param.aac, config->specific_info, 0);
#endif
        }
        config->bit_rate = config->codec_param.aac.u32BitRate;
#endif
    } else {
        BT_LOGE("%s Unkonw Codec", __func__);
    }

    memcpy(&g_current_config, config, sizeof(g_current_config));
}

void a2dp_codec_set_config(uint8_t peer_sep, a2dp_codec_config_t* config)
{
    a2dp_codec_config_set(peer_sep, config, 0);
}

void a2dp_codec_update_config(uint8_t peer_sep, a2dp_codec_config_t* config, uint16_t mtu)
{
    a2dp_codec_config_set(peer_sep, config, mtu);
}

a2dp_codec_config_t* a2dp_codec_get_config(void)
{
    return &g_current_config;
}

bool a2dp_codec_get_offload_config(a2dp_offload_config_t* offload)
{
    a2dp_codec_config_t* codec = &g_current_config;

    switch (codec->codec_type) {
    case BTS_A2DP_TYPE_SBC:
        return a2dp_source_sbc_get_offload_config(codec, offload);

    case BTS_A2DP_TYPE_MPEG2_4_AAC:
    default:
        return false;
    }
}