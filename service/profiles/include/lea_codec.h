/****************************************************************************
 *
 *   Copyright (C) 2024 Xiaomi InC. All rights reserved.
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
#ifndef __LEA_CODEC_H__
#define __LEA_CODEC_H__

#include <sys/types.h>

#include "bt_vendor.h"
#include "lea_audio_common.h"

enum {
    LEA_CODEC_SINK,
    LEA_CODEC_SOURCE,
    LEA_CODEC_MAX,
};

typedef struct {
    bool initiator;
    bool active;
    uint8_t codec_type;
    uint8_t stream_num;
    uint8_t bits_per_sample;
    uint8_t channel_mode;
    uint32_t sample_rate;
    uint32_t bit_rate;
    uint32_t frame_size;
    uint32_t packet_size;
    lea_stream_info_t streams_info[CONFIG_LEA_STREAM_MAX_NUM];
} lea_audio_config_t;

lea_audio_config_t* lea_codec_get_config(bool is_source);
void lea_codec_set_config(lea_audio_stream_t* audio_stream);
void lea_codec_unset_config(bool is_source);
bool lea_codec_get_offload_config(lea_offload_config_t* offload);

#endif
