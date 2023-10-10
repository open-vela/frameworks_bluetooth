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

#include "bt_profile.h"
#include "bt_spp.h"
#include "service_manager.h"
#include "spp_service.h"
#include "utils/log.h"

void *bt_spp_register_app(bt_instance_t *ins, const spp_callbacks_t *callbacks)
{
    return NULL;
}

bt_status_t bt_spp_unregister_app(bt_instance_t *ins, void *handle)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_spp_server_start(bt_instance_t *ins, void *handle, uint16_t scn, bt_uuid_t *uuid, uint8_t max_connection)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_spp_server_stop(bt_instance_t *ins, void *handle, uint16_t scn)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_spp_connect(bt_instance_t *ins, void *handle, bt_address_t *addr, int16_t scn, bt_uuid_t *uuid, uint16_t *port)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_spp_disconnect(bt_instance_t *ins, void *handle, bt_address_t *addr, uint16_t port)
{
    return BT_STATUS_SUCCESS;
}
