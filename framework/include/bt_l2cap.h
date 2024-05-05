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
#ifndef __BT_L2CAP_H__
#define __BT_L2CAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

enum {
    LE_PSM_DYNAMIC_MIN = 0x0080,
    LE_PSM_DYNAMIC_MAX = 0x00FF,
    BREDR_PSM_DYNAMIC_MIN = 0x1001,
    BREDR_PSM_DYNAMIC_MAX = 0xFFFF,
};

typedef enum {
    L2CAP_CHANNEL_MODE_BASIC = 0,
    L2CAP_CHANNEL_MODE_RETRANSMISSION,
    L2CAP_CHANNEL_MODE_FLOW_CONTROL,
    L2CAP_CHANNEL_MODE_ENHANCED_RETRANSMISSION,
    L2CAP_CHANNEL_MODE_STREAMING_MODE,
    L2CAP_CHANNEL_MODE_LE_CREDIT_BASED_FLOW_CONTROL,
    L2CAP_CHANNEL_MODE_ENHANCED_CREDIT_BASED_FLOW_CONTROL,
} l2cap_channel_mode_t;

typedef struct {
    uint8_t transport; /* bt_transport_t */
    uint8_t mode; /* l2cap_channel_mode_t, basic or enhanced retransmission mode */
    uint16_t psm; /* Dynamic Service PSM */
    uint16_t mtu; /* Maximum Transmission Unit */
    uint16_t le_mps; /* Maximum PDU payload Size for LE */
    uint16_t init_credits; /* initial credits for LE */
} l2cap_config_option_t;

typedef struct {
    bt_address_t addr;
    bt_transport_t transport;
    uint16_t cid; /* Channel id. */
    uint16_t psm; /* Dynamic Service PSM */
    uint16_t incoming_mtu; /* Incoming transmit MTU. */
    uint16_t outgoing_mtu; /* outgoing transmit MTU */
    const char* pty_name; /* pty device name, like "/dev/pts/0" */
} l2cap_connect_params_t;

/**
 * @brief L2CAP connected event callback
 *
 * @param cookie - callbacks cookie, the return value of bt_l2cap_register_callbacks.
 * @param param - L2CAP connection params.
 */
typedef void (*l2cap_connected_callback_t)(void* cookie, l2cap_connect_params_t* param);

/**
 * @brief L2CAP disconnected event callback
 *
 * @param cookie - callbacks cookie, the return value of bt_l2cap_register_callbacks.
 * @param addr - remote addr.
 * @param cid - channel id.
 * @param reason - disconnect reason.
 */
typedef void (*l2cap_disconnected_callback_t)(void* cookie, bt_address_t* addr, uint16_t cid, uint32_t reason);

/**
 * @brief L2CAP event callback structure
 *
 */
typedef struct {
    size_t size;
    l2cap_connected_callback_t on_connected;
    l2cap_disconnected_callback_t on_disconnected;
} l2cap_callbacks_t;

/**
 * @brief Register callback functions to L2CAP service.
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - L2CAP callback functions.
 * @return void* - callbacks cookie, NULL on failure.
 */
void* BTSYMBOLS(bt_l2cap_register_callbacks)(bt_instance_t* ins, const l2cap_callbacks_t* callbacks);

/**
 * @brief Unregister L2CAP callback functions.
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callbacks cookie.
 * @return true - on callback unregister success
 * @return false - on callback cookie not found
 */
bool BTSYMBOLS(bt_l2cap_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Listen for a L2CAP connection request
 * @param ins - bluetooth client instance.
 * @param option - L2CAP config option.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_listen)(bt_instance_t* ins, l2cap_config_option_t* option);

/**
 * @brief Request L2CAP connection to remote device
 * @param ins - bluetooth client instance.
 * @param addr - remote addr.
 * @param option - L2CAP config option.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_connect)(bt_instance_t* ins, bt_address_t* addr, l2cap_config_option_t* option);

/**
 * @brief Reqeust to disconnect a L2CAP channel
 * @param ins - bluetooth client instance.
 * @param cid - channel id.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_disconnect)(bt_instance_t* ins, uint16_t cid);

#ifdef __cplusplus
}
#endif

#endif
