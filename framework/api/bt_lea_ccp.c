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
#include "bt_lea_ccp.h"
#include "bt_profile.h"
#include "lea_ccp_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static lea_ccp_interface_t* get_profile_service(void)
{
    return (lea_ccp_interface_t*)service_manager_get_profile(PROFILE_LEAUDIO_CCP);
}

void* bt_lea_ccp_register_callbacks(bt_instance_t* ins, const lea_ccp_callbacks_t* callbacks)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, (lea_ccp_callbacks_t*)callbacks);
}

bool bt_lea_ccp_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_lea_ccp_read_bearer_provider_name(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_bearer_provider_name(addr);
}

bt_status_t bt_lea_ccp_read_bearer_uci(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_bearer_uci(addr);
}

bt_status_t bt_lea_ccp_read_bearer_technology(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_bearer_technology(addr);
}

bt_status_t bt_lea_ccp_read_bearer_uri_schemes_supported_list(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_bearer_uri_schemes_supported_list(addr);
}

bt_status_t bt_lea_ccp_read_bearer_signal_strength(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_bearer_signal_strength(addr);
}

bt_status_t bt_lea_ccp_read_bearer_signal_strength_report_interval(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_bearer_signal_strength_report_interval(addr);
}

bt_status_t bt_lea_ccp_read_content_control_id(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_content_control_id(addr);
}

bt_status_t bt_lea_ccp_read_status_flags(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_status_flags(addr);
}

bt_status_t bt_lea_ccp_read_call_control_optional_opcodes(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_call_control_optional_opcodes(addr);
}

bt_status_t bt_lea_ccp_read_incoming_call(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_incoming_call(addr);
}

bt_status_t bt_lea_ccp_read_incoming_call_target_bearer_uri(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_incoming_call_target_bearer_uri(addr);
}

bt_status_t bt_lea_ccp_read_call_state(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_call_state(addr);
}

bt_status_t bt_lea_ccp_read_bearer_list_current_calls(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_bearer_list_current_calls(addr);
}

bt_status_t bt_lea_ccp_read_call_friendly_name(bt_instance_t* ins, bt_address_t* addr)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->read_call_friendly_name(addr);
}

bt_status_t bt_lea_ccp_call_control_by_index(bt_instance_t* ins, bt_address_t* addr, uint8_t opcode)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->call_control_by_index(addr, opcode);
}

bt_status_t bt_lea_ccp_originate_call(bt_instance_t* ins, bt_address_t* addr, uint8_t* uri)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->originate_call(addr, uri);
}

bt_status_t bt_lea_ccp_join_calls(bt_instance_t* ins, bt_address_t* addr, uint8_t number,
    uint8_t* call_indexes)
{
    lea_ccp_interface_t* profile = get_profile_service();

    return profile->join_calls(addr, number, call_indexes);
}
