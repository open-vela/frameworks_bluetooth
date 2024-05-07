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
#ifndef __BT_LEA_SERVER_H__
#define __BT_LEA_SERVER_H__

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"

typedef enum {
    LEA_SERVER_STATE_DISABLED = 0,
    LEA_SERVER_STATE_ENABLED = 1,
} lea_server_stack_state_t;

typedef void (*lea_server_stack_state_callback)(void* cookie,
    lea_server_stack_state_t enabled);

typedef void (*lea_server_connection_state_callback)(void* cookie,
    profile_connection_state_t state, bt_address_t* bd_addr);

typedef struct {
    size_t size;
    lea_server_stack_state_callback server_stack_state_cb;
    lea_server_connection_state_callback server_connection_state_cb;
} lea_server_callbacks_t;

void* bt_lea_server_register_callbacks(bt_instance_t* ins,
    const lea_server_callbacks_t* callbacks);

bool bt_lea_server_unregister_callbacks(bt_instance_t* ins, void* cookie);

bt_status_t bt_lea_server_start_announce(bt_instance_t* ins, uint8_t adv_id,
    uint8_t announce_type, uint8_t* adv_data, uint16_t adv_size,
    uint8_t* md_data, uint16_t md_size);

bt_status_t bt_lea_server_stop_announce(bt_instance_t* ins, uint8_t adv_id);

bt_status_t bt_lea_server_disconnect(bt_instance_t* ins, bt_address_t* addr);

profile_connection_state_t bt_lea_server_get_connection_state(bt_instance_t* ins, bt_address_t* addr);

bt_status_t bt_lea_server_disconnect_audio(bt_instance_t* ins, bt_address_t* addr);

#endif