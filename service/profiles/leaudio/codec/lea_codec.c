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

#include "lea_audio_common.h"
#include "lea_codec.h"

enum {
    LEA_CHANNEL_MODE_MONO = 0x00,
    LEA_CHANNEL_MODE_STEREO,
};

enum {
    LEA_CODEC_BITS_PER_SAMPLE_NONE = 0x0,
    LEA_CODEC_BITS_PER_SAMPLE_16 = 0x01,
    LEA_CODEC_BITS_PER_SAMPLE_24 = 0x02,
    LEA_CODEC_BITS_PER_SAMPLE_32 = 0x04,
};

enum {
    LEA_CODEC_INDEX_RATE_NONE = 0x00,
    LEA_CODEC_INDEX_RATE_8000,
    LEA_CODEC_INDEX_RATE_11025,
    LEA_CODEC_INDEX_RATE_16000,
    LEA_CODEC_INDEX_RATE_22050,
    LEA_CODEC_INDEX_RATE_24000,
    LEA_CODEC_INDEX_RATE_32000,
    LEA_CODEC_INDEX_RATE_44100,
    LEA_CODEC_INDEX_RATE_48000,
    LEA_CODEC_INDEX_RATE_88200,
    LEA_CODEC_INDEX_RATE_96000,
    LEA_CODEC_INDEX_RATE_176400,
    LEA_CODEC_INDEX_RATE_192000,
    LEA_CODEC_INDEX_RATE_384000,
};

enum {
    LEA_CODEC_TYPE_SBC = 0x00,
    LEA_CODEC_TYPE_MPEG1_2_AUDIO,
    LEA_CODEC_TYPE_MPEG2_4_AAC,
    LEA_CODEC_TYPE_ATRAC,
    LEA_CODEC_TYPE_OPUS,
    LEA_CODEC_TYPE_H263,
    LEA_CODEC_TYPE_MPEG4_VSP,
    LEA_CODEC_TYPE_H263_PROF3,
    LEA_CODEC_TYPE_H263_PROF8,
    LEA_CODEC_TYPE_LHDC,
    LEA_CODEC_TYPE_NON_A2DP,
    LEA_CODEC_TYPE_LC3,
};

enum {
    LEA_CODEC_FRAME_DURATION_7500US = 0x00,
    LEA_CODEC_FRAME_DURATION_10000US = 0x01,
};

static lea_audio_config_t g_codec_config[2];

static uint8_t get_channel_count(uint32_t allocation)
{
    uint8_t channels = 0;

    while (allocation) {
        if (allocation & 1) {
            channels++;
        }
        allocation >>= 1;
    }

    return channels;
}

static uint8_t get_channal_mode(uint32_t allocation)
{
    uint8_t channels;

    channels = get_channel_count(allocation);

    switch (channels) {
    case 1:
        return LEA_CHANNEL_MODE_MONO;
    case 2:
        return LEA_CHANNEL_MODE_STEREO;
    }

    return LEA_CHANNEL_MODE_MONO;
}

static uint32_t get_bit_rate(lea_codec_config_t* config)
{
    uint8_t channels;
    uint8_t duration;

    channels = get_channel_count(config->allocation);
    duration = config->duration == 0 ? 134 : 100; // 7.5 ms or 10 ms

    return 8 * channels * config->octets * duration;
}

static uint32_t get_sample_rate(uint8_t index)
{
    switch (index) {
    case LEA_CODEC_INDEX_RATE_NONE:
        return 0;
    case LEA_CODEC_INDEX_RATE_8000:
        return 8000;
    case LEA_CODEC_INDEX_RATE_11025:
        return 11025;
    case LEA_CODEC_INDEX_RATE_16000:
        return 16000;
    case LEA_CODEC_INDEX_RATE_22050:
        return 22050;
    case LEA_CODEC_INDEX_RATE_24000:
        return 24000;
    case LEA_CODEC_INDEX_RATE_32000:
        return 32000;
    case LEA_CODEC_INDEX_RATE_44100:
        return 44100;
    case LEA_CODEC_INDEX_RATE_48000:
        return 48000;
    case LEA_CODEC_INDEX_RATE_88200:
        return 88200;
    case LEA_CODEC_INDEX_RATE_96000:
        return 96000;
    case LEA_CODEC_INDEX_RATE_176400:
        return 176400;
    case LEA_CODEC_INDEX_RATE_192000:
        return 192000;
    case LEA_CODEC_INDEX_RATE_384000:
        return 384000;
    }

    return 0;
}

static uint32_t get_frame_size(lea_codec_config_t* config)
{
    uint32_t sample_rate;
    float scale;

    sample_rate = get_sample_rate(config->frequency);

    switch (config->duration) {
    case LEA_CODEC_FRAME_DURATION_7500US:
        scale = 0.0075;
        break;
    case LEA_CODEC_FRAME_DURATION_10000US:
        scale = 0.01;
        break;
    default:
        scale = 0.01;
        break;
    }

    return sample_rate * scale;
}

static uint32_t get_packet_size(lea_codec_config_t* config)
{
    uint8_t count;

    count = get_channel_count(config->allocation);

    return config->octets * count * config->blocks;
}

static int get_codec_type(void)
{
    int index;

    for (index = 0; index < LEA_CODEC_MAX; index++) {
        if (g_codec_config[index].active) {
            return g_codec_config[index].codec_type;
        }
    }
    return -1;
}

static void lea_codec_lc3_update_config(lea_audio_stream_t* audio_stream)
{
    lea_audio_config_t* audio_config;
    lea_codec_config_t* codec_cfg;
    lea_stream_info_t* streams_info;

    if (audio_stream->is_source) {
        audio_config = &g_codec_config[LEA_CODEC_SOURCE];
    } else {
        audio_config = &g_codec_config[LEA_CODEC_SINK];
    }

    codec_cfg = &audio_stream->codec_cfg;
    audio_config->active = true;
    audio_config->codec_type = LEA_CODEC_TYPE_LC3;
    audio_config->sample_rate = get_sample_rate(codec_cfg->frequency);
    audio_config->bits_per_sample = LEA_CODEC_BITS_PER_SAMPLE_16;
    audio_config->channel_mode = get_channal_mode(codec_cfg->allocation);
    audio_config->bit_rate = get_bit_rate(codec_cfg);
    audio_config->frame_size = get_frame_size(codec_cfg);
    audio_config->packet_size = get_packet_size(codec_cfg);

    streams_info = &audio_config->streams_info[audio_config->stream_num];
    streams_info->stream_handle = audio_stream->iso_handle;
    streams_info->channel_allocation = codec_cfg->allocation;
    audio_config->stream_num++;
}

static bool lea_codec_lc3_get_offload_config(lea_offload_config_t* offload)
{
    lea_audio_config_t* audio_config;
    int index;

    for (index = 0; index < LEA_CODEC_MAX; index++) {
        audio_config = &g_codec_config[index];
        if (audio_config->active) {
            offload->codec[index].active = true;
            offload->codec[index].stream_num = audio_config->stream_num;
            memcpy(offload->codec[index].streams_info, audio_config->streams_info, sizeof(audio_config->streams_info));
        }
    }

    return true;
}

lea_audio_config_t* lea_codec_get_config(bool is_source)
{
    lea_audio_config_t* audio_config;

    if (is_source) {
        audio_config = &g_codec_config[LEA_CODEC_SOURCE];
    } else {
        audio_config = &g_codec_config[LEA_CODEC_SINK];
    }

    return audio_config;
}

void lea_codec_set_config(lea_audio_stream_t* audio_stream)
{
    switch (audio_stream->codec_cfg.codec_id.codec_id) {
    // todo: codec id
    case LEA_CODEC_TYPE_LC3:
    default:
        lea_codec_lc3_update_config(audio_stream);
        break;
    }
}

void lea_codec_unset_config(bool is_source)
{
    lea_audio_config_t* audio_config;

    if (is_source) {
        audio_config = &g_codec_config[LEA_CODEC_SOURCE];
    } else {
        audio_config = &g_codec_config[LEA_CODEC_SINK];
    }

    memset(audio_config, 0, sizeof(lea_audio_config_t));
}

bool lea_codec_get_offload_config(lea_offload_config_t* offload)
{
    uint8_t codec_type;

    codec_type = get_codec_type();
    switch (codec_type) {
    case LEA_CODEC_TYPE_LC3:
        return lea_codec_lc3_get_offload_config(offload);

    default:
        return false;
    }
}
