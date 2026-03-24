/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#define LOG_TAG "auracast_sink_api"

#include "auracast_sink_service.h"
#include "bt_internal.h"
#include "service_manager.h"

#include "utils/log.h"

static inline auracast_sink_interface_t* get_profile_service(void)
{
    return (auracast_sink_interface_t*)service_manager_get_profile(PROFILE_AURACAST_SINK);
}

void* BTSYMBOLS(bt_auracast_sink_register_callbacks)(bt_instance_t* ins,
    const bt_auracast_sink_callbacks_t* cbs)
{
    auracast_sink_interface_t* profile = get_profile_service();
    if (!profile)
        return NULL;

    return profile->register_callbacks(NULL, cbs);
}

bool BTSYMBOLS(bt_auracast_sink_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    auracast_sink_interface_t* profile = get_profile_service();
    if (!profile)
        return false;

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t BTSYMBOLS(bt_auracast_sink_create_sync)(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, uint32_t bitfield, const uint8_t* broadcast_code)
{
    auracast_sink_interface_t* profile = get_profile_service();
    if (!profile)
        return BT_STATUS_NOT_SUPPORTED;

    return profile->create_sync(addr, sid, bitfield, broadcast_code);
}

bt_status_t BTSYMBOLS(bt_auracast_sink_terminate_sync)(bt_instance_t* ins,
    const bt_le_address_t* addr, uint8_t sid)
{
    auracast_sink_interface_t* profile = get_profile_service();
    if (!profile)
        return BT_STATUS_NOT_SUPPORTED;

    return profile->terminate_sync(addr, sid);
}

bt_status_t BTSYMBOLS(bt_auracast_sink_dump)(bt_instance_t* ins)
{
    auracast_sink_interface_t* profile = get_profile_service();
    if (!profile)
        return BT_STATUS_NOT_SUPPORTED;

    return profile->dump();
}