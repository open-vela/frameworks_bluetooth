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
#define LOG_TAG "hfp_hf_api"

#include "bt_hfp_hf.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "hfp_hf_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static hfp_hf_interface_t* get_profile_service(void)
{
    return (hfp_hf_interface_t*)service_manager_get_profile(PROFILE_HFP_HF);
}

void* BTSYMBOLS(bt_hfp_hf_register_callbacks)(bt_instance_t* ins, const hfp_hf_callbacks_t* callbacks)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, (hfp_hf_callbacks_t*)callbacks);
}

bool BTSYMBOLS(bt_hfp_hf_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bool BTSYMBOLS(bt_hfp_hf_is_connected)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->is_connected(addr);
}

bool BTSYMBOLS(bt_hfp_hf_is_audio_connected)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->is_audio_connected(addr);
}

profile_connection_state_t BTSYMBOLS(bt_hfp_hf_get_connection_state)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->get_connection_state(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_connect)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->connect(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_disconnect)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_set_connection_policy)(bt_instance_t* ins, bt_address_t* addr, connection_policy_t policy)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->set_connection_policy(addr, policy);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_connect_audio)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->connect_audio(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_disconnect_audio)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->disconnect_audio(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_start_voice_recognition)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->start_voice_recognition(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_stop_voice_recognition)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->stop_voice_recognition(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_dial)(bt_instance_t* ins, bt_address_t* addr, const char* number)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->dial(addr, number);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_dial_memory)(bt_instance_t* ins, bt_address_t* addr, uint32_t memory)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->dial_memory(addr, memory);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_redial)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->redial(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_accept_call)(bt_instance_t* ins, bt_address_t* addr, hfp_call_accept_t flag)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->accept_call(addr, flag);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_reject_call)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->reject_call(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_hold_call)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->hold_call(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_terminate_call)(bt_instance_t* ins, bt_address_t* addr)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->terminate_call(addr);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_control_call)(bt_instance_t* ins, bt_address_t* addr, hfp_call_control_t chld, uint8_t index)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->control_call(addr, chld, index);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_query_current_calls)(bt_instance_t* ins, bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator)
{
    hfp_hf_interface_t* profile = get_profile_service();

    return profile->query_current_calls(addr, calls, num, allocator);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_send_at_cmd)(bt_instance_t* ins, bt_address_t* addr, const char* cmd)
{
    hfp_hf_interface_t* profile = get_profile_service();
    return profile->send_at_cmd(addr, cmd);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_update_battery_level)(bt_instance_t* ins, bt_address_t* addr, uint8_t level)
{
    hfp_hf_interface_t* profile = get_profile_service();
    return profile->update_battery_level(addr, level);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_volume_control)(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    hfp_hf_interface_t* profile = get_profile_service();
    return profile->volume_control(addr, type, volume);
}

bt_status_t BTSYMBOLS(bt_hfp_hf_send_dtmf)(bt_instance_t* ins, bt_address_t* addr, char dtmf)
{
    hfp_hf_interface_t* profile = get_profile_service();
    return profile->send_dtmf(addr, dtmf);
}
