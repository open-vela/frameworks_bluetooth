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

#define INVALID_L2CAP_LISTEN_ID 0xFFFF

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
    uint16_t id; /* L2CAP Service socket id */
    char proxy_name[16]; /* Proxy name */
    uint8_t sec_level; /* security level (0=default, 1=L1, 2=L2, 3=L3, 4=L4) */
} l2cap_config_option_t;

typedef struct {
    bt_address_t addr;
    bt_transport_t transport;
    uint16_t cid; /* Local channel id. */
    uint16_t psm; /* Dynamic Service PSM */
    uint16_t incoming_mtu; /* Incoming transmit MTU. */
    uint16_t outgoing_mtu; /* Outgoing transmit MTU */
    uint16_t id; /* Connected L2CAP Channel socket id */
    // for L2CAP listen only.
    uint16_t listen_id; /* New L2CAP Listen socket id, INVALID_L2CAP_LISTEN_ID indicates invalid */
    char proxy_name[16]; /* Proxy name for server */
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
 * @param id - L2CAP service socket id, used to identify L2CAP service channel resource.
 * @param reason - disconnect reason.
 */
typedef void (*l2cap_disconnected_callback_t)(void* cookie, bt_address_t* addr, uint16_t id, uint32_t reason);

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
 * @return void* - L2CAP APP handle, NULL on failure.
 */
void* BTSYMBOLS(bt_l2cap_register_callbacks)(bt_instance_t* ins, const l2cap_callbacks_t* callbacks);

/**
 * @brief Unregister L2CAP callback functions.
 *
 * @param ins - bluetooth client instance.
 * @param handle - L2CAP APP handle.
 * @return true - on callback unregister success
 * @return false - on callback cookie not found
 */
bool BTSYMBOLS(bt_l2cap_unregister_callbacks)(bt_instance_t* ins, void* handle);

/**
 * @brief Listen for a L2CAP connection request
 * @param ins - bluetooth client instance.
 * @param handle - L2CAP APP handle.
 * @param option - L2CAP config option.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_listen)(bt_instance_t* ins, void* handle, l2cap_config_option_t* option);

/**
 * @brief Request L2CAP connection to remote device
 * @param ins - bluetooth client instance.
 * @param handle - L2CAP APP handle.
 * @param addr - remote addr.
 * @param option - L2CAP config option.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_connect)(bt_instance_t* ins, void* handle, bt_address_t* addr, l2cap_config_option_t* option);

/**
 * @brief Reqeust to disconnect a L2CAP channel
 * @param ins - bluetooth client instance.
 * @param handle - L2CAP APP handle.
 * @param id - Connected L2CAP Channel socket id.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_disconnect)(bt_instance_t* ins, void* handle, uint16_t id);

/**
 * @brief Stop L2CAP listen
 *
 * This function used to stop L2CAP listen rather than disconnect all conected
 * L2CAP channels for a specific PSM.
 *
 * @param ins - bluetooth client instance.
 * @param handle - L2CAP APP handle.
 * @param psm - LE PSM used for listen.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * @note This function is only used for LE transport scenario.
 */
bt_status_t BTSYMBOLS(bt_l2cap_stop_listen)(bt_instance_t* ins, void* handle, uint16_t psm);

/**
 * @brief Stop L2CAP listen with transport
 *
 * This function used to stop L2CAP listen rather than disconnect all conected
 * L2CAP channels for a specific PSM.
 *
 * @param ins - bluetooth client instance.
 * @param handle - L2CAP APP handle.
 * @param transport - bt_transport_t, LE or BR/EDR.
 * @param psm - PSM used for listen.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_stop_listen_with_transport)(bt_instance_t* ins, void* handle, bt_transport_t transport, uint16_t psm);

/**
 * @brief Send L2CAP Echo Request over BR/EDR connection.
 *
 * @param ins - bluetooth client instance.
 * @param handle - callbacks cookie, the return value of bt_l2cap_register_callbacks.
 * @param addr - remote device address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_send_echo_req)(bt_instance_t* ins, void* handle, bt_address_t* addr);

/**
 * @brief Send L2CAP Configuration Request on BR/EDR channel.
 *
 * @param ins - bluetooth client instance.
 * @param handle - callbacks cookie.
 * @param addr - remote device address.
 * @param cid - local channel CID.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_send_conf_req)(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t cid);

/**
 * @brief Send data on BR/EDR L2CAP channel by CID.
 *
 * @param ins - bluetooth client instance.
 * @param handle - callbacks cookie.
 * @param addr - remote device address.
 * @param cid - remote channel CID (tx cid).
 * @param data - data buffer to send.
 * @param len - data length.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_send_br_data)(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t cid, uint8_t* data, uint16_t len);

/**
 * @brief Disconnect BR/EDR L2CAP channel by CID.
 *
 * @param ins - bluetooth client instance.
 * @param handle - callbacks cookie.
 * @param addr - remote device address.
 * @param cid - local channel CID.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_l2cap_br_disconnect_channel)(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t cid);

#ifdef __cplusplus
}
#endif

#endif
