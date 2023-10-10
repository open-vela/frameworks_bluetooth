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

#include "bt_pan.h"
#include "bt_profile.h"
#include "pan_service.h"
#include "service_manager.h"
#include "utils/log.h"

void *bt_pan_register_callbacks(bt_instance_t *ins, const pan_callbacks_t *callbacks)
{
    return NULL;
}

bool bt_pan_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    return false;
}

bt_status_t bt_pan_connect(bt_instance_t *ins, bt_address_t *addr, uint8_t dst_role, uint8_t src_role)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_pan_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}
