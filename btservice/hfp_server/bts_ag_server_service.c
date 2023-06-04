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
#include <sys/types.h>

#include "btm_manager.h"
#include "bts_ag_server.h"
#include "bts_service.h"

#define LOG_TAG "hfp_ag_service"
#include "log.h"

static ag_server_callbacks_t* agCallbacks = NULL;

void ag_svr_connection_state_callback(
    bt_address addr, profile_connection_state_t state)
{
    BT_LOGD("ag_svr_connection_state_callback");

    if (agCallbacks->connection_state_cb) {
        agCallbacks->connection_state_cb(addr, state);
    } else {
        BT_LOGE("ag_server_svr_callbacks connection_state_cb is null");
    }
}

void ag_svr_audio_state_callback(
    bt_address addr, hfp_audio_state_t state)
{
    if (agCallbacks->audio_state_cb) {
        agCallbacks->audio_state_cb(addr, state);
    } else {
        BT_LOGE("ag_server_svr_callbacks audio_state_cb is null");
    }
}

void ag_svr_vr_cmd_callback(
    bt_address addr, bool started)
{
    if (agCallbacks->vr_cmd_cb) {
        agCallbacks->vr_cmd_cb(addr, started);
    } else {
        BT_LOGE("ag_server_svr_callbacks vr_cmd_cb is null");
    }
}

void ag_svr_battery_update_callback(
    bt_address addr, uint8_t value)
{
    if (agCallbacks->ag_battery_update_cb) {
        agCallbacks->ag_battery_update_cb(addr, value);
    } else {
        BT_LOGE("ag_server_svr_callbacks ag_battery_update_cb is null");
    }
}

void ag_svr_answer_call_callback(
    bt_address addr)
{
    if (agCallbacks->answer_call_cb) {
        agCallbacks->answer_call_cb(addr);
    } else {
        BT_LOGE("ag_server_svr_callbacks answer_call_cb is null");
    }
}

void ag_svr_reject_call_callback(
    bt_address addr)
{
    if (agCallbacks->reject_call_cb) {
        agCallbacks->reject_call_cb(addr);
    } else {
        BT_LOGE("ag_server_svr_callbacks reject_call_cb is null");
    }
}

void ag_svr_hangup_call_callback(
    bt_address addr)
{
    if (agCallbacks->hangup_call_cb) {
        agCallbacks->hangup_call_cb(addr);
    } else {
        BT_LOGE("ag_server_svr_callbacks hangup_call_cb is null");
    }
}

void ag_svr_dial_number_callback(
    bt_address addr, char* number)
{
    if (agCallbacks->dial_number_cb) {
        agCallbacks->dial_number_cb(addr, number);
    } else {
        BT_LOGE("ag_server_svr_callbacks dial_number_cb is null");
    }
}

void ag_svr_call_control_callback(
    bt_address addr, uint8_t chld)
{
    if (agCallbacks->call_control_cb) {
        agCallbacks->call_control_cb(addr, chld);
    } else {
        BT_LOGE("ag_server_svr_callbacks call_control_cb is null");
    }
}

void ag_svr_cind_callback(
    bt_address addr)
{
    if (agCallbacks->cind_cb) {
        agCallbacks->cind_cb(addr);
    } else {
        BT_LOGE("ag_server_svr_callbacks cind_cb is null");
    }
}

void ag_svr_clcc_callback(
    bt_address addr)
{
    if (agCallbacks->clcc_cb) {
        agCallbacks->clcc_cb(addr);
    } else {
        BT_LOGE("ag_server_svr_callbacks clcc_cb is null");
    }
}

void ag_svr_cops_callback(
    bt_address addr)
{
    if (agCallbacks->cops_cb) {
        agCallbacks->cops_cb(addr);
    } else {
        BT_LOGE("ag_server_svr_callbacks cops_cb is null");
    }
}

void ag_svr_at_command_callback(
    bt_address addr, char* at_command)
{
    if (agCallbacks->at_command_cb) {
        agCallbacks->at_command_cb(addr, at_command);
    } else {
        BT_LOGE("ag_svr_at_command_callback at_command_cb is null");
    }
}

static bool ag_is_connected(void* handle, bt_address addr)
{
    return bts_ag_server_is_connected(addr);
}

static bool ag_is_audio_connected(void* handle, bt_address addr)
{
    return bts_ag_server_is_audio_connected(addr);
}

static ag_server_state_t ag_get_connection_state(void* handle,
    bt_address addr)
{
    return bts_ag_server_get_connection_state(addr);
}

static bt_result_code ag_connect(void* handle, bt_address addr)
{
    return bts_ag_server_connect(addr);
}

static bt_result_code ag_disconnect(void* handle, bt_address addr)
{
    return bts_ag_server_disconnect(addr);
}

static bt_result_code ag_connect_audio(void* handle, bt_address addr)
{
    BT_LOGD("ag server ag_connect_audio");

    return bts_ag_server_connect_audio(addr);
}

static bt_result_code ag_disconnect_audio(void* handle, bt_address addr)
{
    return bts_ag_server_disconnect_audio(addr);
}

static bt_result_code ag_start_voice_recognition(void* handle, bt_address addr)
{
    return bts_ag_server_start_voice_recognition(addr);
}

static bt_result_code ag_stop_voice_recognition(void* handle, bt_address addr)
{
    return bts_ag_server_stop_voice_recognition(addr);
}

static bt_result_code ag_phone_state_change(
    bt_address bd_addr,
    uint8_t num_active,
    uint8_t num_held,
    ag_server_call_state_t call_state,
    ag_server_call_addrtype_t type,
    const char* number,
    const char* name)
{
    return bts_ag_server_phone_state_change(
        bd_addr,
        num_active, num_held,
        call_state,
        type,
        number,
        name);
}

static bt_result_code ag_device_status_changed(bt_address bd_addr,
    hfp_network_state_t network,
    hfp_roaming_state_t roam,
    uint8_t signal,
    uint8_t battery)
{
    return bts_ag_server_device_status_changed(bd_addr, network, roam, signal, battery);
}

static bt_result_code ag_set_inband_ring_enable(bt_address bd_addr)
{
    return bts_ag_server_set_inband_ring_enable(bd_addr);
}

static bt_result_code ag_send_at_command(
    bt_address bd_addr,
    char* at_command)
{
    return bts_ag_server_send_at_command(bd_addr, at_command);
}

static bt_result_code ag_dial_result(bt_address bd_addr, uint8_t result)
{
    return bts_ag_server_dial_result(bd_addr, result);
}

static bt_result_code ag_cind_response(bt_address bd_addr,
    hfp_network_state_t service,
    uint8_t signal,
    hfp_roaming_state_t roam,
    uint8_t battery,
    hfp_call_t call,
    hfp_callsetup_t call_setup,
    hfp_callheld_t call_held)
{
    return bts_ag_server_cind_response(bd_addr,
        service,
        signal,
        roam,
        battery,
        call,
        call_setup,
        call_held);
}

static bt_result_code ag_clcc_response(bt_address bd_addr,
    uint32_t index,
    uint8_t dir,
    ag_server_call_state_t status,
    uint8_t mode,
    uint8_t mpty,
    const char* number)
{
    return bts_ag_server_clcc_response(bd_addr,
        index,
        dir,
        status,
        mode,
        mpty,
        number);
}

static bt_result_code ag_cops_response(bt_address bd_addr,
    char* operator_name, uint16_t length)
{
    return bts_ag_server_cops_response(bd_addr,
        operator_name, length);
}

static void set_callbacks(void* handle, ag_server_callbacks_t* callbacks)
{
    agCallbacks = callbacks;
}

static const ag_server_service_callbacks_t ag_server_svr_callbacks = {
    sizeof(ag_server_service_callbacks_t),
    ag_svr_connection_state_callback,
    ag_svr_audio_state_callback,
    ag_svr_vr_cmd_callback,
    ag_svr_battery_update_callback,
    ag_svr_answer_call_callback,
    ag_svr_reject_call_callback,
    ag_svr_hangup_call_callback,
    ag_svr_dial_number_callback,
    ag_svr_call_control_callback,
    ag_svr_at_command_callback,
    ag_svr_cind_callback,
    ag_svr_clcc_callback,
    ag_svr_cops_callback
};

bt_result_code ag_service_start()
{
    bt_result_code ret;

    ret = bts_ag_server_init(&ag_server_svr_callbacks);
    if (ret != BT_RESULT_SUCCESS)
        return ret;

    BT_LOGD("ag server Service Started");
    return ret;
}

bt_result_code ag_service_stop()
{
    bts_ag_server_cleanup();
    BT_LOGD("ag server Service Stoped");
    return 0;
}

static const ag_server_interface_t agInterface = {
    sizeof(ag_server_interface_t),
    ag_service_start,
    ag_service_stop,
    ag_is_connected,
    ag_is_audio_connected,
    ag_get_connection_state,
    ag_connect,
    ag_disconnect,
    ag_connect_audio,
    ag_disconnect_audio,
    ag_start_voice_recognition,
    ag_stop_voice_recognition,
    ag_phone_state_change,
    ag_device_status_changed,
    ag_set_inband_ring_enable,
    ag_send_at_command,
    ag_dial_result,
    ag_cind_response,
    ag_clcc_response,
    ag_cops_response,
    set_callbacks,
};

const ag_server_interface_t* get_ag_server_service_interface(void)
{
    return &agInterface;
}
