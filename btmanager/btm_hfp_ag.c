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

#include "btm_hfp_ag.h"
#include "btm_manager.h"
#include "bts_service.h"
#include "bts_service_interface.h"

#define LOG_TAG "btm_hfp_ag"
#include "log.h"

static ag_server_interface_t* get_service(void)
{
    return (ag_server_interface_t*)get_bluetooth_service_interface()->get_profile_interface(BT_PROFILE_HANDSFREE_AG);
}

static bt_result_code ag_start(void)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    return service->server_start();
}

static bt_result_code ag_stop(void)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    return service->server_stop();
}

static bool ag_is_connected(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    BT_LOGD("PERFORMANCE-AG-BTM-ag_is_connected");
    return service->is_connected(handle, addr);
}

static bool ag_is_audio_connected(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    BT_LOGD("PERFORMANCE-AG-BTM-ag_is_audio_connected");
    return service->is_audio_connected(handle, addr);
}

static ag_server_state_t ag_get_connection_state(void* handle,
    bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    BT_LOGD("PERFORMANCE-AG-BTM-ag_get_connection_state");
    return service->get_connection_state(handle, addr);
}

static bt_result_code ag_connect(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    BT_LOGD("PERFORMANCE-AG-BTM-CONNECT_START");
    return service->connect(handle, addr);
}

static bt_result_code ag_disconnect(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->disconnect(handle, addr);
}

static bt_result_code ag_connect_audio(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->connect_audio(handle, addr);
}

static bt_result_code ag_disconnect_audio(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->disconnect_audio(handle, addr);
}

static bt_result_code ag_start_voice_recognition(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->start_voice_recognition(handle, addr);
}

static bt_result_code ag_stop_voice_recognition(void* handle, bt_address addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->stop_voice_recognition(handle, addr);
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
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->phone_state_change(
        bd_addr,
        num_active, num_held,
        call_state,
        type,
        number,
        name);
}

static bt_result_code ag_device_status_changed(
    bt_address bd_addr,
    hfp_network_state_t network,
    hfp_roaming_state_t roam,
    uint8_t signal,
    uint8_t battery)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->device_status_changed(bd_addr, network, roam, signal, battery);
}

static bt_result_code ag_set_inband_ring_enable(bt_address bd_addr)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->set_inband_ring_enable(bd_addr);
}

static bt_result_code ag_send_at_command(
    bt_address bd_addr,
    char* at_command)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->send_at_command(bd_addr, at_command);
}

static bt_result_code ag_dial_result(bt_address bd_addr, uint8_t result)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->dial_result(bd_addr, result);
}

static bt_result_code ag_cind_response(bt_address bd_addr,
    hfp_network_state_t network,
    uint8_t signal,
    hfp_roaming_state_t roam,
    uint8_t battery,
    hfp_call_t call,
    hfp_callsetup_t call_setup,
    hfp_callheld_t call_held)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->cind_response(bd_addr,
        network,
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
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->clcc_response(bd_addr,
        index,
        dir,
        status,
        mode,
        mpty,
        number);
}

static bt_result_code ag_cops_response(bt_address bd_addr, char* operator_name, uint16_t length)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->cops_response(bd_addr, operator_name, length);
}

static void set_callbacks(void* handle, ag_server_callbacks_t* callbacks)
{
    ag_server_interface_t* service = get_service();
    if (!service)
        return;

    return service->set_callbacks(handle, callbacks);
}

static const ag_server_interface_t agInterface = {
    sizeof(ag_server_interface_t),
    ag_start,
    ag_stop,
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

const ag_server_interface_t* get_ag_server_interface(void)
{
    return &agInterface;
}
