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
#define LOG_TAG "a2dp_source_api"

#include <stdint.h>

#include "a2dp_source_service.h"
#include "bt_a2dp_source.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "service_manager.h"
#include "utils/log.h"

static a2dp_source_interface_t* get_profile_service(void)
{
    return (a2dp_source_interface_t*)service_manager_get_profile(PROFILE_A2DP);
}

void* BTSYMBOLS(bt_a2dp_source_register_callbacks)(bt_instance_t* ins, const a2dp_source_callbacks_t* callbacks)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool BTSYMBOLS(bt_a2dp_source_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bool BTSYMBOLS(bt_a2dp_source_is_connected)(bt_instance_t* ins, bt_address_t* addr)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->is_connected(addr);
}

bool BTSYMBOLS(bt_a2dp_source_is_playing)(bt_instance_t* ins, bt_address_t* addr)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->is_playing(addr);
}

profile_connection_state_t BTSYMBOLS(bt_a2dp_source_get_connection_state)(bt_instance_t* ins, bt_address_t* addr)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->get_connection_state(addr);
}

bt_status_t BTSYMBOLS(bt_a2dp_source_connect)(bt_instance_t* ins, bt_address_t* addr)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->connect(addr);
}

bt_status_t BTSYMBOLS(bt_a2dp_source_disconnect)(bt_instance_t* ins, bt_address_t* addr)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}

bt_status_t BTSYMBOLS(bt_a2dp_source_set_silence_device)(bt_instance_t* ins, bt_address_t* addr, bool silence)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->set_silence_device(addr, silence);
}

bt_status_t BTSYMBOLS(bt_a2dp_source_set_active_device)(bt_instance_t* ins, bt_address_t* addr)
{
    a2dp_source_interface_t* profile = get_profile_service();

    return profile->set_active_device(addr);
}
