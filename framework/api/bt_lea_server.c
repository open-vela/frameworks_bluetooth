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
#include "lea_server_service.h"
#include "service_manager.h"

static lea_server_interface_t* get_profile_service(void)
{
    return (lea_server_interface_t*)service_manager_get_profile(
        PROFILE_LEAUDIO_SERVER);
}

void* bt_lea_server_register_callbacks(bt_instance_t* ins,
    const lea_server_callbacks_t* callbacks)
{
    lea_server_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool bt_lea_server_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    lea_server_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_lea_server_start_announce(bt_instance_t* ins, uint8_t adv_id,
    uint8_t announce_type, uint8_t* adv_data, uint16_t adv_size,
    uint8_t* md_data, uint16_t md_size)
{
    lea_server_interface_t* profile = get_profile_service();

    return profile->start_announce(adv_id, announce_type, adv_data,
        adv_size, md_data, md_size);
}

bt_status_t bt_lea_server_stop_announce(bt_instance_t* ins, uint8_t adv_id)
{
    lea_server_interface_t* profile = get_profile_service();

    return profile->stop_announce(adv_id);
}

bt_status_t bt_lea_server_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    lea_server_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}

bt_status_t bt_lea_server_disconnect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    lea_server_interface_t* profile = get_profile_service();

    return profile->disconnect_audio(addr);
}

profile_connection_state_t bt_lea_server_get_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    lea_server_interface_t* profile = get_profile_service();

    return profile->get_connection_state(addr);
}
