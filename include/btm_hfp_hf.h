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
#ifndef __BT_HF_CLIENT_H__
#define __BT_HF_CLIENT_H__

#include "btm_manager.h"

typedef enum {
    HF_CLIENT_CONNECTION_STATE_DISCONNECTED = 0,
    HF_CLIENT_CONNECTION_STATE_CONNECTING,
    HF_CLIENT_CONNECTION_STATE_CONNECTED,
    HF_CLIENT_CONNECTION_STATE_DISCONNECTING
} hf_client_connection_state_t;

typedef enum {
    HF_CLIENT_AUDIO_STATE_DISCONNECTED = 0,
    HF_CLIENT_AUDIO_STATE_CONNECTING,
    HF_CLIENT_AUDIO_STATE_CONNECTED,
    HF_CLIENT_AUDIO_STATE_CONNECTED_MSBC,
} hf_client_audio_state_t;

typedef enum {
    HF_CLIENT_VR_STATE_STOPPED = 0,
    HF_CLIENT_VR_STATE_STARTED
} hf_client_vr_state_t;

typedef enum {
    HF_CLIENT_VOLUME_TYPE_SPK = 0,
    HF_CLIENT_VOLUME_TYPE_MIC
} hf_client_volume_type_t;

typedef enum {
    HF_CLIENT_CALL_STATE_ACTIVE = 0,
    HF_CLIENT_CALL_STATE_HELD,
    HF_CLIENT_CALL_STATE_DIALING,
    HF_CLIENT_CALL_STATE_ALERTING,
    HF_CLIENT_CALL_STATE_INCOMING,
    HF_CLIENT_CALL_STATE_WAITING,
    HF_CLIENT_CALL_STATE_HELD_BY_RESP_HOLD,
} hf_client_call_state_t;

typedef enum {
    HF_CLIENT_CALL_NO_CALLS_IN_PROGRESS = 0,
    HF_CLIENT_CALL_CALLS_IN_PROGRESS
} hf_client_call_t;

typedef enum {
    HF_CLIENT_CALLSETUP_NONE = 0,
    HF_CLIENT_CALLSETUP_INCOMING,
    HF_CLIENT_CALLSETUP_OUTGOING,
    HF_CLIENT_CALLSETUP_ALERTING
} hf_client_callsetup_t;

typedef enum {
    HF_CLIENT_CALLHELD_NONE = 0,
    HF_CLIENT_CALLHELD_HELD,
} hf_client_callheld_t;

typedef enum {
    HF_CLIENT_CALL_DIRECTION_OUTGOING = 0,
    HF_CLIENT_CALL_DIRECTION_INCOMING
} hf_client_call_direction_t;

typedef enum {
    HF_CLIENT_CALL_MPTY_TYPE_SINGLE = 0,
    HF_CLIENT_CALL_MPTY_TYPE_MULTI
} hf_client_call_mpty_type_t;

typedef enum {
    HF_CLIENT_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    HF_CLIENT_IN_BAND_RINGTONE_PROVIDED,
} hf_client_in_band_ring_state_t;

typedef void (*hf_client_connection_state_callback)(
    bt_address addr, hf_client_connection_state_t state);
typedef void (*hf_client_audio_state_callback)(
    bt_address addr, hf_client_audio_state_t state);
typedef void (*hf_client_vr_cmd_callback)(bt_address addr,
    hf_client_vr_state_t state);
typedef void (*hf_client_call_callback)(bt_address addr,
    hf_client_call_t call);
typedef void (*hf_client_callsetup_callback)(
    bt_address addr, hf_client_callsetup_t callsetup);
typedef void (*hf_client_callheld_callback)(bt_address addr,
    hf_client_callheld_t callheld);
typedef void (*hf_client_clip_callback)(bt_address addr,
    const char* number, const char* name);
typedef void (*hf_client_current_calls_callback)(bt_address addr, int index,
    hf_client_call_direction_t dir,
    hf_client_call_state_t state,
    hf_client_call_mpty_type_t mpty,
    const char* number);
typedef void (*hf_client_volume_change_callback)(
    bt_address addr, hf_client_volume_type_t type, int volume);
typedef void (*hf_client_cmd_complete_callback)(
    bt_address addr, const char* resp);
typedef void (*hf_client_ring_indication_callback)(bt_address addr,
    hf_client_in_band_ring_state_t state);

typedef struct
{
    size_t size;
    hf_client_connection_state_callback connection_state_cb;
    hf_client_audio_state_callback audio_state_cb;
    hf_client_vr_cmd_callback vr_cmd_cb;
    hf_client_call_callback call_cb;
    hf_client_callsetup_callback callsetup_cb;
    hf_client_callheld_callback callheld_cb;
    hf_client_clip_callback clip_cb;
    hf_client_current_calls_callback current_calls_cb;
    hf_client_volume_change_callback volume_change_cb;
    hf_client_cmd_complete_callback cmd_complete_cb;
    hf_client_ring_indication_callback ring_indication_cb;
} hf_client_callbacks_t;

typedef struct
{
    size_t size;
    BT_RESULT_CODE (*connect)(void* handle, bt_address addr);
    BT_RESULT_CODE (*disconnect)(void* handle, bt_address addr);
    BT_RESULT_CODE (*connect_audio)(void* handle, bt_address addr);
    BT_RESULT_CODE (*disconnect_audio)(void* handle, bt_address addr);
    BT_RESULT_CODE (*start_voice_recognition)(void* handle, bt_address addr);
    BT_RESULT_CODE (*stop_voice_recognition)(void* handle, bt_address addr);
    BT_RESULT_CODE (*volume_control)(void* handle, bt_address addr, hf_client_volume_type_t type, int volume);
    BT_RESULT_CODE (*dial)(void* handle, bt_address addr, const char* number);
    BT_RESULT_CODE (*dial_memory)(void* handle, bt_address addr, uint32_t memory);
    BT_RESULT_CODE (*redial)(void* handle, bt_address addr);
    BT_RESULT_CODE (*accept_call)(void* handle, bt_address addr);
    BT_RESULT_CODE (*reject_call)(void* handle, bt_address addr);
    BT_RESULT_CODE (*hold_call)(void* handle, bt_address addr);
    BT_RESULT_CODE (*terminate_call)(void* handle, bt_address addr);
    BT_RESULT_CODE (*query_current_calls)(void* handle, bt_address addr);
    BT_RESULT_CODE (*send_at_cmd)(void* handle, bt_address addr, const char* cmd);
    BT_RESULT_CODE (*update_battery_level)(void* handle, bt_address addr, uint8_t battery);
    void (*set_callbacks)(void* handle, hf_client_callbacks_t* callbacks);
} hf_client_interface_t;

const hf_client_interface_t* get_hf_client_interface(void);

#endif
