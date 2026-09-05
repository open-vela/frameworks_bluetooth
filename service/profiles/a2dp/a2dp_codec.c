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
#include "audio_control.h"
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

static uint32_t a2dp_codec_get_frame_size()
{
    a2dp_codec_config_t* codec = &g_current_config;
    switch (codec->codec_type) {
    case BTS_A2DP_TYPE_SBC:
        return a2dp_sbc_frame_length((sbc_param_t*)(&codec->codec_param.sbc));
    case BTS_A2DP_TYPE_MPEG2_4_AAC:
        return A2DP_AAC_FRAME_SIZE;
    default:
        return 0;
    }
}

// frames indicates the number of frame, frame * frames determines the length of a buffer.
static uint32_t a2dp_codec_get_frames()
{
    a2dp_codec_config_t* codec = &g_current_config;
    switch (codec->codec_type) {
    case BTS_A2DP_TYPE_SBC:
        return a2dp_sbc_frames((sbc_param_t*)(&codec->codec_param.sbc));
    case BTS_A2DP_TYPE_MPEG2_4_AAC:
        return A2DP_AAC_FRAMES;
    default:
        return 0;
    }
}

bt_audio_config_t* a2dp_codec_create_media_config(uint8_t profile_id)
{
    a2dp_codec_config_t* codec_config = a2dp_codec_get_config();
    bt_audio_config_t* config = (bt_audio_config_t*)zalloc(sizeof(bt_audio_config_t));

    if (!config) {
        BT_LOGE("%s: allocate memory failed", __func__);
        return NULL;
    }

    config->codec = (bt_audio_codec_t*)zalloc(sizeof(bt_audio_codec_t));
    if (!config->codec) {
        BT_LOGE("%s: allocate memory failed", __func__);
        free(config);
        return NULL;
    }

    config->codec->id = codec_config->codec_type;
    config->codec->sample_rate = codec_config->sample_rate;
    config->codec->bit_rate = codec_config->bit_rate;
    switch (codec_config->bits_per_sample) {
    case BTS_A2DP_CODEC_BITS_PER_SAMPLE_8:
        config->codec->pcm_format = BT_AUDIO_SUBFMT_PCM_U8;
        break;
    case BTS_A2DP_CODEC_BITS_PER_SAMPLE_16:
        config->codec->pcm_format = BT_AUDIO_SUBFMT_PCM_S16_LE;
        break;
    default:
        break;
    }

    config->fragments = BT_AUDIO_FRAGMENTS;
    if (codec_config->codec_type == BTS_A2DP_TYPE_SBC) {
        config->codec->ch_in = codec_config->codec_param.sbc.s16NumOfChannels;
        config->codec->options.sbc.blocks = codec_config->codec_param.sbc.s16NumOfBlocks;
        config->codec->options.sbc.subbands = codec_config->codec_param.sbc.s16NumOfSubBands;
        config->codec->options.sbc.alloc_method = codec_config->codec_param.sbc.s16AllocationMethod;
        config->codec->options.sbc.bitpool = codec_config->codec_param.sbc.s16BitPool;
        config->codec->ch_mode = codec_config->codec_param.sbc.s16ChannelMode;
        switch (profile_id) {
        case A2DP_SINK_PROFILE_ID:
            config->fragment_size = 1024;
            config->codec->format = BT_AUDIO_STREAMFORMAT_SBC_PACKED;
            break;
        case A2DP_SOURCE_PROFILE_ID:
            config->fragment_size = a2dp_codec_get_frame_size() * a2dp_codec_get_frames();
            break;
        default:
            BT_LOGE("%s, error profile_id: %d", __func__, profile_id);
            free(config->codec);
            free(config);
            return NULL;
        }
    } else if (codec_config->codec_type == BTS_A2DP_TYPE_MPEG2_4_AAC) {
        config->codec->ch_in = codec_config->codec_param.aac.u16NumOfChannels;
        config->codec->profile = 1;
        config->codec->rate_control = codec_config->codec_param.aac.u16VariableBitRate;
        switch (profile_id) {
        case A2DP_SINK_PROFILE_ID:
            config->codec->format = BT_AUDIO_STREAMFORMAT_AAC_LATM;
            config->fragment_size = a2dp_codec_get_frame_size() * a2dp_codec_get_frames();
            break;
        case A2DP_SOURCE_PROFILE_ID:
            config->fragment_size = a2dp_codec_get_frame_size() * a2dp_codec_get_frames();
            break;
        default:
            BT_LOGE("%s, error profile_id: %d", __func__, profile_id);
            free(config->codec);
            free(config);
            return NULL;
        }
    } else {
        // TODO: other codec
    }

    return config;
}

void a2dp_codec_delete_media_config(bt_audio_config_t* config)
{
    if (!config) {
        return;
    }

    if (config->codec != NULL) {
        free(config->codec);
        config->codec = NULL;
    }

    free(config);
    config = NULL;
}