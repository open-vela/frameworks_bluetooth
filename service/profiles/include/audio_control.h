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
#ifndef __AUDIO_CONTROL_H__
#define __AUDIO_CONTROL_H__

#include "audio_codec.h"

#define A2DP_SOURCE_PROFILE_ID 0
#define A2DP_SINK_PROFILE_ID 1
#define HFP_HF_PROFILE_ID 2
#define HFP_AG_PROFILE_ID 3

typedef void (*audio_control_start_callback)();
typedef void (*audio_control_stop_callback)();
typedef struct {
    size_t size;
    audio_control_start_callback start_cb;
    audio_control_stop_callback stop_cb;
} audio_control_callbacks_t;

void audio_control_open(uint8_t profile_id, const bt_audio_config_t* config, const void* ctrl_callback);
void audio_control_start(uint8_t profile_id, bool started);
void audio_control_stop(uint8_t profile_id);
void audio_control_reset(uint8_t profile_id);
int audio_control_read(uint8_t profile_id, void* buf, unsigned int size);
int audio_control_write(uint8_t profile_id, const void* buf, unsigned int size);

#endif
