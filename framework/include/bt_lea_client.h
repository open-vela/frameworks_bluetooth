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
#ifndef __BT_LEA_CLIENT_H__
#define __BT_LEA_CLIENT_H__

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_lea.h"

typedef enum {
    LEA_CLIENT_STATE_DISABLED = 0,
    LEA_CLIENT_STATE_ENABLED = 1,
} lea_client_stack_state_t;

typedef enum {
    LEA_CSIP_LOCK_SUCCESS,
    LEA_CSIP_LOCK_ORDERED_ACCESS_LOCKED,
    LEA_CSIP_LOCK_COORDINATED_SET_NOT_FOUND,
    LEA_CSIP_LOCK_LOCK_DENIED,
    LEA_CSIP_LOCK_UNLOCK_NOT_ALLOWED,
    LEA_CSIP_LOCK_LOCK_NOT_EXIST,
} lea_csip_lock_status;

typedef void (*lea_client_stack_state_callback)(void* cookie, lea_client_stack_state_t enabled);

typedef void (*lea_client_connection_state_callback)(void* cookie, profile_connection_state_t state,
    bt_address_t* bd_addr);
typedef void (*lea_client_audio_state_callback)(void* cookie, lea_audio_state_t state,
    bt_address_t* bd_addr);

typedef void (*lea_client_group_member_discovered_callback)(void* cookie, uint32_t group_id, bt_address_t* bd_addr);

typedef void (*lea_client_group_member_added_callback)(void* cookie, uint32_t group_id, bt_address_t* bd_addr);

typedef void (*lea_client_group_member_removed_callback)(void* cookie, uint32_t group_id, bt_address_t* bd_addr);

typedef void (*lea_client_group_discovery_start_callback)(void* cookie, uint32_t group_id);

typedef void (*lea_client_group_discovery_stop_callback)(void* cookie, uint32_t group_id);

typedef void (*lea_client_group_lock_callback)(void* cookie, uint32_t group_id, lea_csip_lock_status result);

typedef void (*lea_client_group_unlock_callback)(void* cookie, uint32_t group_id, lea_csip_lock_status result);

typedef struct {
    size_t size;
    lea_client_stack_state_callback client_stack_state_cb;
    lea_client_connection_state_callback client_connection_state_cb;
    lea_client_audio_state_callback client_audio_state_cb;
    lea_client_group_member_discovered_callback client_group_member_discovered_cb;
    lea_client_group_member_added_callback client_group_member_added_cb;
    lea_client_group_member_removed_callback client_group_member_removed_cb;
    lea_client_group_discovery_start_callback client_group_discovery_start_cb;
    lea_client_group_discovery_stop_callback client_group_discovery_stop_cb;
    lea_client_group_lock_callback client_group_lock_cb;
    lea_client_group_unlock_callback client_group_unlock_cb;
} lea_client_callbacks_t;

void* bt_lea_client_register_callbacks(bt_instance_t* ins, const lea_client_callbacks_t* callbacks);
bool bt_lea_client_unregister_callbacks(bt_instance_t* ins, void* cookie);

bt_status_t bt_lea_client_connect(bt_instance_t* ins, bt_address_t* addr);
bt_status_t bt_lea_client_connect_audio(bt_instance_t* ins, bt_address_t* addr, uint8_t context);
bt_status_t bt_lea_client_disconnect(bt_instance_t* ins, bt_address_t* addr);
bt_status_t bt_lea_client_disconnect_audio(bt_instance_t* ins, bt_address_t* addr);
profile_connection_state_t bt_lea_client_get_connection_state(bt_instance_t* ins, bt_address_t* addr);
bt_status_t bt_lea_client_get_group_id(bt_instance_t* ins, bt_address_t* addr, uint32_t* group_id);
bt_status_t bt_lea_client_discovery_member_start(bt_instance_t* ins, uint32_t group_id);
bt_status_t bt_lea_client_discovery_member_stop(bt_instance_t* ins, uint32_t group_id);
bt_status_t bt_lea_client_group_add_member(bt_instance_t* ins, uint32_t group_id, bt_address_t* addr);
bt_status_t bt_lea_client_group_remove_member(bt_instance_t* ins, uint32_t group_id, bt_address_t* addr);
bt_status_t bt_lea_client_group_connect_audio(bt_instance_t* ins, uint32_t group_id, uint8_t context);
bt_status_t bt_lea_client_group_disconnect_audio(bt_instance_t* ins, uint32_t group_id);
bt_status_t bt_lea_client_group_lock(bt_instance_t* ins, uint32_t group_id);
bt_status_t bt_lea_client_group_unlock(bt_instance_t* ins, uint32_t group_id);
#endif