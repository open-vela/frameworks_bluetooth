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
#define LOG_TAG "spp_api"

#include <stdint.h>

#include "bt_internal.h"
#include "bt_profile.h"
#include "bt_spp.h"
#include "service_manager.h"
#include "spp_service.h"
#include "utils/log.h"

static spp_interface_t* get_profile_service(void)
{
    return (spp_interface_t*)service_manager_get_profile(PROFILE_SPP);
}

void* BTSYMBOLS(bt_spp_register_app_with_name)(bt_instance_t* ins, const char* name, const spp_callbacks_t* callbacks)
{
    spp_interface_t* profile = get_profile_service();

    return profile->register_app(NULL, name, callbacks);
}

void* BTSYMBOLS(bt_spp_register_app)(bt_instance_t* ins, const spp_callbacks_t* callbacks)
{
    return BTSYMBOLS(bt_spp_register_app_with_name)(ins, NULL, callbacks);
}

void* BTSYMBOLS(bt_spp_register_app_ext)(bt_instance_t* ins, const char* name, int port_type, const spp_callbacks_t* callbacks)
{
    return BTSYMBOLS(bt_spp_register_app_with_name)(ins, name, callbacks);
}

bt_status_t BTSYMBOLS(bt_spp_unregister_app)(bt_instance_t* ins, void* handle)
{
    spp_interface_t* profile = get_profile_service();

    return profile->unregister_app(NULL, handle);
}

bt_status_t BTSYMBOLS(bt_spp_server_start)(bt_instance_t* ins, void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t max_connection)
{
    spp_interface_t* profile = get_profile_service();

    return profile->server_start(handle, scn, uuid, max_connection);
}

bt_status_t BTSYMBOLS(bt_spp_server_stop)(bt_instance_t* ins, void* handle, uint16_t scn)
{
    spp_interface_t* profile = get_profile_service();

    return profile->server_stop(handle, scn);
}

bt_status_t BTSYMBOLS(bt_spp_connect)(bt_instance_t* ins, void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port)
{
    spp_interface_t* profile = get_profile_service();

    return profile->connect(handle, addr, scn, uuid, port);
}

bt_status_t BTSYMBOLS(bt_spp_disconnect)(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t port)
{
    spp_interface_t* profile = get_profile_service();

    return profile->disconnect(handle, addr, port);
}