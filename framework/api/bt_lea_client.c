/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#include <stdint.h>

#include "bt_lea_server.h"
#include "bt_profile.h"
#include "lea_client_service.h"
#include "service_manager.h"

static lea_client_interface_t* get_profile_service(void)
{
    return (lea_client_interface_t*)service_manager_get_profile(
        PROFILE_LEAUDIO_CLIENT);
}

void* bt_lea_client_register_callbacks(bt_instance_t* ins,
    const lea_client_callbacks_t* callbacks)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool bt_lea_client_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_lea_client_connect(bt_instance_t* ins, bt_address_t* addr)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->connect(addr);
}

bt_status_t bt_lea_client_connect_audio(bt_instance_t* ins, bt_address_t* addr, uint8_t context)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->connect_audio(addr, context);
}

bt_status_t bt_lea_client_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}

bt_status_t bt_lea_client_disconnect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->disconnect_audio(addr);
}

profile_connection_state_t bt_lea_client_get_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->get_connection_state(addr);
}

bt_status_t bt_lea_client_get_group_id(bt_instance_t* ins, bt_address_t* addr, uint32_t* group_id)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->get_group_id(addr, group_id);
}

bt_status_t bt_lea_client_discovery_member_start(bt_instance_t* ins, uint32_t group_id)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->discovery_member_start(group_id);
}

bt_status_t bt_lea_client_discovery_member_stop(bt_instance_t* ins, uint32_t group_id)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->discovery_member_stop(group_id);
}

bt_status_t bt_lea_client_group_add_member(bt_instance_t* ins, uint32_t group_id, bt_address_t* addr)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->group_add_member(group_id, addr);
}

bt_status_t bt_lea_client_group_remove_member(bt_instance_t* ins, uint32_t group_id, bt_address_t* addr)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->group_remove_member(group_id, addr);
}

bt_status_t bt_lea_client_group_connect_audio(bt_instance_t* ins, uint32_t group_id, uint8_t context)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->group_connect_audio(group_id, context);
}

bt_status_t bt_lea_client_group_disconnect_audio(bt_instance_t* ins, uint32_t group_id)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->group_disconnect_audio(group_id);
}

bt_status_t bt_lea_client_group_lock(bt_instance_t* ins, uint32_t group_id)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->group_lock(group_id);
}

bt_status_t bt_lea_client_group_unlock(bt_instance_t* ins, uint32_t group_id)
{
    lea_client_interface_t* profile = get_profile_service();

    return profile->group_unlock(group_id);
}
