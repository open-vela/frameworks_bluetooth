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
#ifndef __A2DP_SOURCE_AUDIO_H__
#define __A2DP_SOURCE_AUDIO_H__

#include "a2dp_codec.h"
#include "bluetooth_define.h"
#include "bt_vendor.h"

#define AVDT_MEDIA_OFFSET 23
#define MAX_2MBPS_AVDTP_MTU 663 // 2DH5 MTU=679, -12 for AVDTP, -4 for L2CAP
#define MAX_3MBPS_AVDTP_MTU 1005 // 3DH5 MTU=1021, -12 for AVDTP, -4 for L2CAP

typedef void (*frame_send_callback)(uint8_t* buf, uint16_t nbytes, uint8_t nb_frames, uint64_t timestamp);
typedef int (*frame_read_callback)(uint8_t* buf, uint16_t frame_len);

typedef struct {
    void (*init)(void* param, uint32_t mtu,
        frame_send_callback send_cb,
        frame_read_callback read_cb);
    void (*reset)(void);
    void (*cleanup)(void);
    void (*send_frames)(uint16_t header_reserve, uint64_t timestamp);
    int (*get_interval_ms)(void);
} a2dp_source_stream_interface_t;

void a2dp_source_audio_init(bool offloading);
void a2dp_source_audio_cleanup(void);
bool a2dp_source_on_connection_changed(bool connected);
void a2dp_source_on_started(bool started);
void a2dp_source_on_stopped(void);
bool a2dp_source_prepare_start(void);
void a2dp_source_prepare_suspend(void);
bool a2dp_source_is_streaming(void);
void a2dp_source_setup_codec(bt_address_t* bd_addr);
int a2dp_source_sbc_update_config(uint32_t mtu, sbc_param_t* param, uint8_t* codec_info);
int a2dp_source_aac_update_config(uint32_t mtu, aac_encoder_param_t* param, uint8_t* codec_info);
bool a2dp_source_sbc_get_offload_config(a2dp_codec_config_t* codec, a2dp_offload_config_t* offload);

extern const a2dp_source_stream_interface_t* get_a2dp_source_sbc_stream_interface(void);
extern const a2dp_source_stream_interface_t* get_a2dp_source_aac_stream_interface(void);

#endif
