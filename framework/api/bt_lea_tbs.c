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
#include "bt_lea_tbs.h"
#include "bt_profile.h"
#include "lea_tbs_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static lea_tbs_interface_t* get_profile_service(void)
{
    return (lea_tbs_interface_t*)service_manager_get_profile(PROFILE_LEAUDIO_TBS);
}

void* bt_lea_tbs_register_callbacks(bt_instance_t* ins, const lea_tbs_callbacks_t* callbacks)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, (lea_tbs_callbacks_t*)callbacks);
}

bool bt_lea_tbs_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_lea_tbs_service_add(bt_instance_t* ins)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->tbs_add();
}

bt_status_t bt_lea_tbs_service_remove(bt_instance_t* ins)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->tbs_remove();
}

bt_status_t bt_lea_tbs_set_telephone_bearer_info(bt_instance_t* ins,
    lea_tbs_telephone_bearer_t* bearer)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->set_telephone_bearer(bearer);
}

bt_status_t bt_lea_tbs_add_call(bt_instance_t* ins, lea_tbs_calls_t* call_s)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->add_call(call_s);
}

bt_status_t bt_lea_tbs_remove_call(bt_instance_t* ins, uint8_t call_index)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->remove_call(call_index);
}

bt_status_t bt_lea_tbs_provider_name_changed(bt_instance_t* ins, uint8_t* name)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->provider_name_changed(name);
}

bt_status_t bt_lea_tbs_bearer_technology_changed(bt_instance_t* ins,
    lea_adpt_bearer_technology_t technology)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->bearer_technology_changed(technology);
}

bt_status_t bt_lea_tbs_uri_schemes_supported_list_changed(bt_instance_t* ins,
    uint8_t* uri_schemes)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->uri_schemes_supported_list_changed(uri_schemes);
}

bt_status_t bt_lea_tbs_rssi_value_changed(bt_instance_t* ins, uint8_t strength)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->rssi_value_changed(strength);
}

bt_status_t bt_lea_tbs_rssi_interval_changed(bt_instance_t* ins,
    uint8_t interval)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->rssi_interval_changed(interval);
}

bt_status_t bt_lea_tbs_status_flags_changed(bt_instance_t* ins, uint16_t status_flags)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->status_flags_changed(status_flags);
}

bt_status_t bt_lea_tbs_call_state_changed(bt_instance_t* ins, uint8_t number,
    lea_tbs_call_state_t* state_s)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->call_state_changed(number, state_s);
}

bt_status_t bt_lea_tbs_notify_termination_reason(bt_instance_t* ins, uint8_t call_index,
    lea_adpt_termination_reason_t reason)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->notify_termination_reason(call_index, reason);
}

bt_status_t bt_lea_tbs_call_control_response(bt_instance_t* ins, uint8_t call_index,
    lea_adpt_call_control_result_t result)
{
    lea_tbs_interface_t* profile = get_profile_service();

    return profile->call_control_response(call_index, result);
}
