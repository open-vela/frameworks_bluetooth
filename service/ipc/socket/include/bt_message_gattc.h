/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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

#ifdef __BT_MESSAGE_CODE__
BT_GATT_CLIENT_MESSAGE_START,
    BT_GATT_CLIENT_CREATE_CONNECT,
    BT_GATT_CLIENT_DELETE_CONNECT,
    BT_GATT_CLIENT_CONNECT,
    BT_GATT_CLIENT_DISCONNECT,
    BT_GATT_CLIENT_DISCOVER_SERVICE,
    BT_GATT_CLIENT_GET_ATTRIBUTE_BY_HANDLE,
    BT_GATT_CLIENT_GET_ATTRIBUTE_BY_UUID,
    BT_GATT_CLIENT_READ,
    BT_GATT_CLIENT_WRITE,
    BT_GATT_CLIENT_WRITE_NR,
    BT_GATT_CLIENT_SUBSCRIBE,
    BT_GATT_CLIENT_UNSUBSCRIBE,
    BT_GATT_CLIENT_EXCHANGE_MTU,
    BT_GATT_CLIENT_UPDATE_CONNECTION_PARAM,
    BT_GATT_CLIENT_READ_PHY,
    BT_GATT_CLIENT_UPDATE_PHY,
    BT_GATT_CLIENT_READ_RSSI,
    BT_GATT_CLIENT_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_GATT_CLIENT_CALLBACK_START,
    BT_GATT_CLIENT_ON_CONNECTED,
    BT_GATT_CLIENT_ON_DISCONNECTED,
    BT_GATT_CLIENT_ON_DISCOVERED,
    BT_GATT_CLIENT_ON_MTU_UPDATED,
    BT_GATT_CLIENT_ON_READ,
    BT_GATT_CLIENT_ON_WRITTEN,
    BT_GATT_CLIENT_ON_SUBSCRIBED,
    BT_GATT_CLIENT_ON_NOTIFIED,
    BT_GATT_CLIENT_ON_PHY_READ,
    BT_GATT_CLIENT_ON_PHY_UPDATED,
    BT_GATT_CLIENT_ON_RSSI_READ,
    BT_GATT_CLIENT_ON_CONN_PARAM_UPDATED,
    BT_GATT_CLIENT_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_GATT_CLIENT_H__
#define _BT_MESSAGE_GATT_CLIENT_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_gattc.h"

    typedef struct {
        bt_instance_t* ins;
        gattc_callbacks_t* callbacks;
        uint64_t cookie;
        void** user_phandle;
    } bt_gattc_remote_t;

    typedef struct {
        uint8_t status; /* bt_status_t */
        uint8_t pad[3];
        union {
            uint64_t handle; /* gattc_handle_t */
            gatt_attr_desc_t attr_desc;
        };
    } bt_gattc_result_t;

    typedef union {
        struct {
            uint64_t cookie; /* void* */
        } _bt_gattc_create;

        struct {
            uint64_t handle; /* gattc_handle_t */
        } _bt_gattc_delete;

        struct {
            uint64_t handle; /* gattc_handle_t */
            bt_address_t addr;
            uint8_t addr_type; /* ble_addr_type_t */
        } _bt_gattc_connect;

        struct {
            uint64_t handle; /* gattc_handle_t */
        } _bt_gattc_disconnect;

        struct {
            uint64_t handle; /* gattc_handle_t */
            bt_uuid_t filter_uuid;
        } _bt_gattc_discover_service;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint16_t attr_handle;
        } _bt_gattc_get_attr_by_handle;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint16_t start_handle;
            uint16_t end_handle;
            bt_uuid_t attr_uuid;
        } _bt_gattc_get_attr_by_uuid;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint16_t attr_handle;
        } _bt_gattc_read;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 3];
        } _bt_gattc_write;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint16_t attr_handle;
            uint16_t ccc_value;
        } _bt_gattc_subscribe;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint32_t mtu;
        } _bt_gattc_exchange_mtu;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint32_t min_interval;
            uint32_t max_interval;
            uint32_t latency;
            uint32_t timeout;
            uint32_t min_connection_event_length;
            uint32_t max_connection_event_length;
        } _bt_gattc_update_connection_param;

        struct {
            uint64_t handle; /* gattc_handle_t */
            uint8_t tx_phy; /* ble_phy_type_t */
            uint8_t rx_phy; /* ble_phy_type_t */
        } _bt_gattc_phy;

        struct {
            uint64_t handle; /* gattc_handle_t */
        } _bt_gattc_rssi;

    } bt_message_gattc_t;

    typedef union {
        struct {
            uint64_t remote; /* void* */
        } _on_callback;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
        } _on_connected;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
        } _on_disconnected;

        struct {
            uint64_t remote; /* void* */
            uint16_t start_handle;
            uint16_t end_handle;
            bt_uuid_t uuid;
            uint8_t status; /* gatt_status_t */
        } _on_discovered;

        struct {
            uint64_t remote; /* void* */
            uint32_t mtu;
            uint8_t status; /* gatt_status_t */
        } _on_mtu_updated;

        struct {
            uint64_t remote; /* void* */
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 1];
            uint8_t status; /* gatt_status_t */
        } _on_read;

        struct {
            uint64_t remote; /* void* */
            uint16_t attr_handle;
            uint8_t status; /* gatt_status_t */
        } _on_written;

        struct {
            uint64_t remote; /* void* */
            uint16_t attr_handle;
            uint8_t status; /* gatt_status_t */
            uint8_t enable; /* boolean */
        } _on_subscribed;

        struct {
            uint64_t remote; /* void* */
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 3];
        } _on_notified;

        struct {
            uint64_t remote; /* void* */
            uint16_t attr_handle;
            uint8_t status; /* gatt_status_t */
            uint8_t tx_phy; /* ble_phy_type_t */
            uint8_t rx_phy; /* ble_phy_type_t */
        } _on_phy_updated;

        struct {
            uint64_t remote; /* void* */
            int32_t rssi;
            uint8_t status; /* gatt_status_t */
        } _on_rssi_read;

        struct {
            uint64_t remote; /* void* */
            uint16_t interval;
            uint16_t latency;
            uint16_t timeout;
            uint8_t status; /* gatt_status_t */
        } _on_conn_param_updated;

    } bt_message_gattc_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_GATT_CLIENT_H__ */
