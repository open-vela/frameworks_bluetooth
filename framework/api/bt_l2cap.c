/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "L2CAP_api"

#include <stdint.h>

#include "bt_internal.h"
#include "bt_l2cap.h"
#include "l2cap_service.h"
#include "utils/log.h"

void* BTSYMBOLS(bt_l2cap_register_callbacks)(bt_instance_t* ins, const l2cap_callbacks_t* callbacks)
{
    return l2cap_register_callbacks(ins, callbacks);
}

bool BTSYMBOLS(bt_l2cap_unregister_callbacks)(bt_instance_t* ins, void* handle)
{
    return l2cap_unregister_callbacks(NULL, handle);
}

bt_status_t BTSYMBOLS(bt_l2cap_listen)(bt_instance_t* ins, void* handle, l2cap_config_option_t* option)
{
    return l2cap_listen_channel(handle, option);
}

bt_status_t BTSYMBOLS(bt_l2cap_connect)(bt_instance_t* ins, void* handel, bt_address_t* addr, l2cap_config_option_t* option)
{
    return l2cap_connect_channel(handel, addr, option);
}

bt_status_t BTSYMBOLS(bt_l2cap_disconnect)(bt_instance_t* ins, void* handle, uint16_t id)
{
    return l2cap_disconnect_channel(handle, id);
}

bt_status_t BTSYMBOLS(bt_l2cap_stop_listen)(bt_instance_t* ins, void* handle, uint16_t psm)
{
    return l2cap_stop_listen_channel(handle, psm);
}
