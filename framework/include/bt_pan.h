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
 * @cond
 */

/**
 * @brief PAN role type define.
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
 * @endcond
 */

/**
 * @brief Callback for PAN connection state change.
 *
 * When the PAN connection state changes, this callback will be triggered to
 * notify the cookie corresponding to the callback, the latest PAN connection
 * state, the Bluetooth address of the peer device, and the roles of both
 * devices in the PAN.
 *
 * @param cookie - Callbacks cookie, the return value of bt_pan_register_callbacks.
 * @param state - PAN connection state
 * @param bd_addr - The Bluetooth address of the peer device.
 * @param local_role - Local device PAN role, reference type pan_role_t.
 * @param remote_role - Remote device PAN role, reference type pan_role_t.
 *
 * **Example:**
 * @code
void pan_connection_state_cb(void* cookie, profile_connection_state_t state,
    bt_address_t* bd_addr, uint8_t local_role,
    uint8_t remote_role)
{
    printf("pan_connection_state_cb, state: %d, bd_addr: %s, local_role: %d, remote_role: %d\n",
        state, bd_addr->address, local_role, remote_role);
}
 * @endcode
 */
typedef void (*pan_connection_state_callback)(void* cookie, profile_connection_state_t state,
    bt_address_t* bd_addr, uint8_t local_role,
    uint8_t remote_role);

/**
 * @brief Callback for PAN net interface (down/up) state change.
 *
 * When the PAN interface becomes up or down, this callback will be triggered
 * to notify the Cookie corresponding to the callback, the latest status of
 * the interface, the role of the local device in the PAN, and the interface
 * name.
 *
 * @param cookie  - Callbacks cookie, the return value of bt_pan_register_callbacks.
 * @param state - Net interface state.
 * @param local_role - Local device PAN role.
 * @param ifname - Net interface name, default "bt-pan".
 *
 * **Example:**
 * @code
void pan_netif_state_cb(void* cookie, pan_netif_state_t state,
    int local_role, const char* ifname)
{
    printf("pan_netif_state_cb, state: %d, local_role: %d, ifname: %s\n",
        state, local_role, ifname);
}
 * @endcode
 */
typedef void (*pan_netif_state_callback)(void* cookie, pan_netif_state_t state,
    int local_role, const char* ifname);

/**
 * @cond
 */

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
 * @endcond
 */

/**
 * @brief Register callback functions to PAN service.
 *
 * This function is used to register the callback for notifying PAN connection
 * states and the callback for notifying PAN interface states to the PAN service.
 * When the PAN service detects the corresponding event, it will notify the
 * application through the registered callback function. After this function is
 * called, it returns the cookie corresponding to the callback.
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - PAN callback functions.
 * @return void* - Callback cookie, NULL represents fail.
 *
 * **Example:**
 * @code
void* pan_cookie;
const static pan_callbacks_t callbacks = {
    .size = sizeof(pan_callbacks_t),
    .netif_state_cb = pan_netif_state_cb,
    .connection_state_cb = pan_connection_state_cb,
};

void app_init_pan(bt_instance_t* ins)
{
    pan_cookie = bt_pan_register_callbacks(ins, &callbacks);
    if (!pan_cookie)
        printf("register pan callbacks failed\n");
    else
        printf("register pan callbacks success\n");
}
 * @endcode
 */
void* BTSYMBOLS(bt_pan_register_callbacks)(bt_instance_t* ins, const pan_callbacks_t* callbacks);

/**
 * @brief Unregister PAN callback functions.
 *
 * This function is used to unregister callbacks from the PAN service. The caller
 * needs to provide the cookie corresponding to callbacks.
 *
 * @param ins - Bluetooth client instance.
 * @param cookie - Callbacks cookie.
 * @return true - on callback unregister success
 * @return false - on callback cookie not found
 *
 * **Example:**
 * @code
void app_deinit_pan(bt_instance_t* ins)
{
    bt_status_t status;
    if (!pan_cookie)
        return;

    status = bt_pan_unregister_callbacks(ins, pan_cookie);
    if (status != BT_STATUS_SUCCESS)
        printf("unregister pan callbacks failed\n");
    else
        printf("unregister pan callbacks success\n");
}
 * @endcode
 */
bool BTSYMBOLS(bt_pan_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief  Connect to PAN device.
 *
 * This function is used to connect to a PAN device. The caller needs to provide
 * the address of the peer device, the role of the local device, and the role of
 * the peer device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param dst_role - Destination PAN role, reference type pan_role_t.
 *                   Only support PANU connect to NAP server, dst_role must be PAN_ROLE_NAP.
 * @param src_role - Source PAN role, reference type pan_role_t.
 *                   Only support PANU connect to NAP server, src_role must be PAN_ROLE_PANU.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
void app_connect_pan_device(bt_instance_t* ins)
{
    bt_status_t status;
    bt_address_t addr = {
        .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    };

    status = bt_pan_connect(ins, &addr, PAN_ROLE_NAP, PAN_ROLE_PANU);
    if(status != BT_STATUS_SUCCESS)
        printf("connect pan device failed\n");
    else
        printf("connect pan device success\n");
}
 * @endcode

 */
bt_status_t BTSYMBOLS(bt_pan_connect)(bt_instance_t* ins, bt_address_t* addr, uint8_t dst_role, uint8_t src_role);

/**
 * @brief Disconnect from PAN connection.
 *
 * This function is used to disconnect from a PAN device. The caller needs to
 * provide the address of the peer device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
void app_disconnect_pan_device(bt_instance_t* ins)
{
    bt_status_t status;
    bt_address_t addr = {
        .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
    };

    status = bt_pan_disconnect(ins, &addr);
    if(status != BT_STATUS_SUCCESS)
        printf("disconnect pan device failed\n");
    else
        printf("disconnect pan device success\n");
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_pan_disconnect)(bt_instance_t* ins, bt_address_t* addr);

#endif /* __BT_PAN_H__ */