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
#ifndef __A2DP_SOURCE_H__
#define __A2DP_SOURCE_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "a2dp_device.h"
#include "bt_a2dp_source.h"
#include "bt_list.h"

void a2dp_source_stream_start(void);
void a2dp_source_stream_prepare_suspend(void);
void a2dp_source_stream_stop(void);
void a2dp_source_codec_state_change(void);
bool a2dp_source_stream_ready(void);
bool a2dp_source_stream_started(void);
a2dp_peer_t* a2dp_source_find_peer(bt_address_t* addr);
a2dp_peer_t* a2dp_source_active_peer(void);

void a2dp_source_service_notify_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);
void a2dp_source_service_notify_audio_state_changed(bt_address_t* addr, a2dp_audio_state_t state);
void a2dp_source_service_notify_audio_source_config_changed(bt_address_t* addr);
#endif
