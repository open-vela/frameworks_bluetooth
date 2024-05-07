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
#define LOG_TAG "pan_api"

#include <stdint.h>

#include "bt_internal.h"
#include "bt_pan.h"
#include "bt_profile.h"
#include "pan_service.h"
#include "service_manager.h"
#include "utils/log.h"

static pan_interface_t* get_profile_service(void)
{
    return (pan_interface_t*)service_manager_get_profile(PROFILE_PANU);
}

void* BTSYMBOLS(bt_pan_register_callbacks)(bt_instance_t* ins, const pan_callbacks_t* callbacks)
{
    pan_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool BTSYMBOLS(bt_pan_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    pan_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t BTSYMBOLS(bt_pan_connect)(bt_instance_t* ins, bt_address_t* addr, uint8_t dst_role, uint8_t src_role)
{
    pan_interface_t* profile = get_profile_service();

    return profile->connect(addr, dst_role, src_role);
}

bt_status_t BTSYMBOLS(bt_pan_disconnect)(bt_instance_t* ins, bt_address_t* addr)
{
    pan_interface_t* profile = get_profile_service();

    return profile->disconnect(addr);
}