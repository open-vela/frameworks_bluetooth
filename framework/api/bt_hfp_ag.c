/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#define LOG_TAG "hfp_ag_api"

#include "bt_hfp_ag.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "hfp_ag_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static hfp_ag_interface_t* get_profile_service(void)
{
    return (hfp_ag_interface_t*)service_manager_get_profile(PROFILE_HFP_AG);
}

void* BTSYMBOLS(bt_hfp_ag_register_callbacks)(bt_instance_t* ins, const hfp_ag_callbacks_t* callbacks)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool BTSYMBOLS(bt_hfp_ag_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bool BTSYMBOLS(bt_hfp_ag_is_connected)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->is_connected(addr);
}

bool BTSYMBOLS(bt_hfp_ag_is_audio_connected)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->is_audio_connected(addr);
}

profile_connection_state_t BTSYMBOLS(bt_hfp_ag_get_connection_state)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->get_connection_state(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_connect)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->connect(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_disconnect)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_connect_audio)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->connect_audio(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_disconnect_audio)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->disconnect_audio(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_start_virtual_call)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->start_virtual_call(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_stop_virtual_call)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->stop_virtual_call(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_start_voice_recognition)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->start_voice_recognition(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_stop_voice_recognition)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->stop_voice_recognition(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_phone_state_change)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t num_active, uint8_t num_held,
    hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->phone_state_change(addr, num_active, num_held, call_state, type, number, name);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_notify_device_status)(bt_instance_t* ins, bt_address_t* addr,
    hfp_network_state_t network, hfp_roaming_state_t roam,
    uint8_t signal, uint8_t battery)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->device_status_changed(addr, network, roam, signal, battery);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_volume_control)(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->volume_control(addr, type, volume);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_send_at_command)(bt_instance_t* ins, bt_address_t* addr, const char* at_command)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->send_at_command(addr, at_command);
}

bt_status_t BTSYMBOLS(bt_hfp_ag_send_vendor_specific_at_command)(bt_instance_t* ins, bt_address_t* addr, const char* command, const char* value)
{
    hfp_ag_interface_t* profile = get_profile_service();

    return profile->send_vendor_specific_at_command(addr, command, value);
}