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
#ifndef __BT_PAN_H__
#define __BT_PAN_H__

#include <stddef.h>

#include "bluetooth.h"
#include "bt_device.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief Pan role type define.
 *
 */
typedef enum {
    /* None */
    PAN_ROLE_NONE = 0,
    /* Network Access Points */
    PAN_ROLE_NAP,
    /* PAN User */
    PAN_ROLE_PANU
} pan_role_t;

/**
 * @brief 'bt-pan' interface state.
 *
 */
typedef enum {
    /* Net up */
    PAN_STATE_ENABLED = 0,
    /* Net down */
    PAN_STATE_DISABLED = 1
} pan_netif_state_t;

/**
 * @brief Pan connection state change callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pan_register_callbacks.
 * @param state - pan connection state
 * @param bd_addr - address of peer device.
 * @param local_role - local device pan role, reference type pan_role_t.
 * @param remote_role - remote device pan role, reference type pan_role_t.
 */
typedef void (*pan_connection_state_callback)(void* cookie, profile_connection_state_t state,
    bt_address_t* bd_addr, uint8_t local_role,
    uint8_t remote_role);

/**
 * @brief Pan net interface (down/up)state change callback.
 *
 * @param cookie  - callbacks cookie, the return value of bt_pan_register_callbacks.
 * @param state - net interface state.
 * @param local_role - local device pan role
 * @param ifname - net interface name, default "bt-pan".
 */
typedef void (*pan_netif_state_callback)(void* cookie, pan_netif_state_t state,
    int local_role, const char* ifname);

/**
 * @brief PAN event callbacks structure
 *
 */
typedef struct {
    size_t size;
    pan_netif_state_callback netif_state_cb;
    pan_connection_state_callback connection_state_cb;
} pan_callbacks_t;

/**
 * @brief Register callback functions to pan service
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - pan callback functions.
 * @return void* - callback cookie, NULL on failure.
 */
void* BTSYMBOLS(bt_pan_register_callbacks)(bt_instance_t* ins, const pan_callbacks_t* callbacks);

/**
 * @brief Unregister pan callback function
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callbacks cookie.
 * @return true - on callback unregister success
 * @return false - on callback cookie not found
 */
bool BTSYMBOLS(bt_pan_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief  Connect to pan device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @param dst_role - dest pan role, reference type pan_role_t.
 * @param src_role - src pan role, reference type pan_role_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pan_connect)(bt_instance_t* ins, bt_address_t* addr, uint8_t dst_role, uint8_t src_role);

/**
 * @brief Disconnect from pan connection
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pan_disconnect)(bt_instance_t* ins, bt_address_t* addr);

#endif /* __BT_PAN_H__ */