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
#ifndef __BTM_A2DP_SOURCE_H__
#define __BTM_A2DP_SOURCE_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_manager.h"

typedef enum {
    A2DP_CONNECTION_STATE_DISCONNECTED = 0,
    A2DP_CONNECTION_STATE_CONNECTING,
    A2DP_CONNECTION_STATE_CONNECTED,
    A2DP_CONNECTION_STATE_DISCONNECTING
} a2dp_connection_state_t;

/* Bluetooth AV datapath states */
typedef enum {
    A2DP_AUDIO_STATE_REMOTE_SUSPEND = 0,
    A2DP_AUDIO_STATE_STOPPED,
    A2DP_AUDIO_STATE_STARTED,
} a2dp_audio_state_t;

typedef void (*a2dp_connection_state_callback)(bt_address addr,
    a2dp_connection_state_t state);

typedef void (*a2dp_audio_state_callback)(bt_address addr,
    a2dp_audio_state_t state);

typedef void (*a2dp_audio_source_config_callback)(bt_address addr);

typedef struct {
    /** set to sizeof(a2dp_source_callbacks_t) */
    size_t size;
    a2dp_connection_state_callback connection_state_cb;
    a2dp_audio_state_callback audio_state_cb;
    a2dp_audio_source_config_callback audio_source_config_cb;
} a2dp_source_callbacks_t;

typedef struct {
    size_t size;

    /** connect to headset */
    bt_result_code (*connect)(void* handle, bt_address addr);

    /** dis-connect from headset */
    bt_result_code (*disconnect)(void* handle, bt_address addr);

    /** sets the connected device silence state */
    bt_result_code (*set_silence_device)(void* handle, bt_address addr, bool silence);

    /** sets the connected device as active */
    bt_result_code (*set_active_device)(void* handle, bt_address addr);

    void (*set_callbacks)(void* handle, a2dp_source_callbacks_t* callbacks);

} a2dp_source_interface_t;

extern const a2dp_source_interface_t* get_a2dp_source_interface(void);

#endif
