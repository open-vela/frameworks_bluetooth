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
#include <sys/types.h>

#include "btm_manager.h"
#include "bts_hf_client.h"
#include "bts_service.h"

#define LOG_TAG "hfp_hf_service"
#include "log.h"

static hf_client_callbacks_t* hfCallbacks = NULL;

void hf_svr_connection_state_callback(
    const bt_address addr, hf_client_connection_state_t state)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->connection_state_cb(addr, state);
}
void hf_svr_audio_state_callback(
    const bt_address addr, hf_client_audio_state_t state)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->audio_state_cb(addr, state);
}
void hf_svr_vr_cmd_callback(const bt_address addr,
    hf_client_vr_state_t state)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->vr_cmd_cb(addr, state);
}
void hf_svr_call_callback(const bt_address addr,
    hf_client_call_t call)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->call_cb(addr, call);
}
void hf_svr_callsetup_callback(
    const bt_address addr, hf_client_callsetup_t callsetup)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->callsetup_cb(addr, callsetup);
}
void hf_svr_callheld_callback(const bt_address addr,
    hf_client_callheld_t callheld)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->callheld_cb(addr, callheld);
}
void hf_svr_clip_callback(const bt_address addr,
    const char* number, const char* name)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->clip_cb(addr, number, name);
}
void hf_svr_current_calls_callback(const bt_address addr, int index,
    hf_client_call_direction_t dir,
    hf_client_call_state_t state,
    hf_client_call_mpty_type_t mpty,
    const char* number)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->current_calls_cb(addr, index, dir, state, mpty, number);
}
void hf_svr_volume_change_callback(
    const bt_address addr, hf_client_volume_type_t type, int volume)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->volume_change_cb(addr, type, volume);
}
void hf_svr_cmd_complete_callback(
    const bt_address addr, const char* resp)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->cmd_complete_cb(addr, resp);
}
void hf_svr_ring_indication_callback(const bt_address addr,
    hf_client_in_band_ring_state_t state)
{
    BT_LOGD("%s", __func__);
    if (hfCallbacks)
        hfCallbacks->ring_indication_cb(addr, state);
}

static bt_result_code hf_connect(void* handle, bt_address addr)
{
    return hf_client_connect(addr);
}

static bt_result_code hf_disconnect(void* handle, bt_address addr)
{
    return hf_client_disconnect(addr);
}

static bt_result_code hf_connect_audio(void* handle, bt_address addr)
{
    return hf_client_connect_audio(addr);
}

static bt_result_code hf_disconnect_audio(void* handle, bt_address addr)
{
    return hf_client_disconnect_audio(addr);
}

static bt_result_code hf_start_voice_recognition(void* handle, bt_address addr)
{
    return hf_client_start_voice_recognition(addr);
}

static bt_result_code hf_stop_voice_recognition(void* handle, bt_address addr)
{
    return hf_client_stop_voice_recognition(addr);
}

static bt_result_code hf_volume_control(void* handle, bt_address addr, hf_client_volume_type_t type, int volume)
{
    return hf_client_volume_control(addr, type, volume);
}

static bt_result_code hf_dial(void* handle, bt_address addr, const char* number)
{
    return hf_client_dial(addr, number);
}

static bt_result_code hf_dial_memory(void* handle, bt_address addr, uint32_t memory)
{
    return hf_client_dial_memory(addr, memory);
}

static bt_result_code hf_redial(void* handle, bt_address addr)
{
    return hf_client_redial(addr);
}

static bt_result_code hf_accept_call(void* handle, bt_address addr)
{
    return hf_client_accept_call(addr);
}

static bt_result_code hf_reject_call(void* handle, bt_address addr)
{
    return hf_client_reject_call(addr);
}

static bt_result_code hf_hold_call(void* handle, bt_address addr)
{
    return hf_client_hold_call(addr);
}

static bt_result_code hf_terminate_call(void* handle, bt_address addr)
{
    return hf_client_terminate_call(addr);
}

static bt_result_code hf_query_current_calls(void* handle, bt_address addr)
{
    return hf_client_query_current_calls(addr);
}

static bt_result_code hf_send_at_cmd(void* handle, bt_address addr, const char* cmd)
{
    return hf_client_send_at_cmd(addr, cmd);
}

static void set_callbacks(void* handle, hf_client_callbacks_t* callbacks)
{
    hfCallbacks = callbacks;
}

static const hf_client_service_callbacks_t hf_client_svr_callbacks = {
    sizeof(hf_client_service_callbacks_t),
    hf_svr_connection_state_callback,
    hf_svr_audio_state_callback,
    hf_svr_vr_cmd_callback,
    hf_svr_call_callback,
    hf_svr_callsetup_callback,
    hf_svr_callheld_callback,
    hf_svr_clip_callback,
    hf_svr_current_calls_callback,
    hf_svr_volume_change_callback,
    hf_svr_cmd_complete_callback,
    hf_svr_ring_indication_callback,
};

static const hf_client_interface_t hfInterface = {
    sizeof(hf_client_interface_t),
    hf_connect,
    hf_disconnect,
    hf_connect_audio,
    hf_disconnect_audio,
    hf_start_voice_recognition,
    hf_stop_voice_recognition,
    hf_volume_control,
    hf_dial,
    hf_dial_memory,
    hf_redial,
    hf_accept_call,
    hf_reject_call,
    hf_hold_call,
    hf_terminate_call,
    hf_query_current_calls,
    hf_send_at_cmd,
    set_callbacks,
};

bt_result_code hf_client_service_start(void)
{
    bt_result_code ret;

    ret = hf_client_init(&hf_client_svr_callbacks);
    if (ret != BT_RESULT_SUCCESS)
        return ret;

    BT_LOGD("hf client service started");
    return ret;
}

void hf_client_service_stop(void)
{
    hf_client_cleanup();
    BT_LOGD("hf client service stoped");
}

const hf_client_interface_t* get_hf_client_service_interface(void)
{
    return &hfInterface;
}