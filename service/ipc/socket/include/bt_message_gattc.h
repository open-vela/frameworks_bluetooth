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
        bt_instance_t *ins;
        gattc_callbacks_t *callbacks;
        void *cookie;
        void **user_phandle;
    } bt_gattc_remote_t;

    typedef struct {
        bt_status_t status;
        union {
            gattc_handle_t handle;
            gatt_attr_desc_t attr_desc;
        };
    } bt_gattc_result_t;

    typedef union {
        struct {
            void *cookie;
        } _bt_gattc_create;

        struct {
            gattc_handle_t handle;
        } _bt_gattc_delete;

        struct {
            gattc_handle_t handle;
            bt_address_t addr;
            ble_addr_type_t addr_type;
        } _bt_gattc_connect;

        struct {
            gattc_handle_t handle;
        } _bt_gattc_disconnect;

        struct {
            gattc_handle_t handle;
            bt_uuid_t filter_uuid;
        } _bt_gattc_discover_service;

        struct {
            gattc_handle_t handle;
            uint16_t attr_handle;
        } _bt_gattc_get_attr_by_handle;

        struct {
            gattc_handle_t handle;
            uint16_t start_handle;
            uint16_t end_handle;
            bt_uuid_t attr_uuid;
        } _bt_gattc_get_attr_by_uuid;

        struct {
            gattc_handle_t handle;
            uint16_t attr_handle;
        } _bt_gattc_read;

        struct {
            gattc_handle_t handle;
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 3];
        } _bt_gattc_write;

        struct {
            gattc_handle_t handle;
            uint16_t attr_handle;
            uint16_t ccc_value;
        } _bt_gattc_subscribe;

        struct {
            gattc_handle_t handle;
            uint32_t mtu;
        } _bt_gattc_exchange_mtu;

        struct {
            gattc_handle_t handle;
            uint32_t min_interval;
            uint32_t max_interval;
            uint32_t latency;
            uint32_t timeout;
            uint32_t min_connection_event_length;
            uint32_t max_connection_event_length;
        } _bt_gattc_update_connection_param;

        struct {
            gattc_handle_t handle;
            ble_phy_type_t tx_phy;
            ble_phy_type_t rx_phy;
        } _bt_gattc_phy;

        struct {
            gattc_handle_t handle;
        } _bt_gattc_rssi;

    } bt_message_gattc_t;

    typedef union {
        struct {
            void *remote;
        } _on_callback;

        struct {
            void *remote;
            bt_address_t addr;
        } _on_connected;

        struct {
            void *remote;
            bt_address_t addr;
        } _on_disconnected;

        struct {
            void *remote;
            gatt_status_t status;
            uint16_t start_handle;
            uint16_t end_handle;
            bt_uuid_t uuid;
        } _on_discovered;

        struct {
            void *remote;
            gatt_status_t status;
            uint32_t mtu;
        } _on_mtu_updated;

        struct {
            void *remote;
            gatt_status_t status;
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 1];
        } _on_read;

        struct {
            void *remote;
            gatt_status_t status;
            uint16_t attr_handle;
        } _on_written;

        struct {
            void *remote;
            gatt_status_t status;
            uint16_t attr_handle;
            bool enable;
        } _on_subscribed;

        struct {
            void *remote;
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 3];
        } _on_notified;

        struct {
            void *remote;
            gatt_status_t status;
            ble_phy_type_t tx_phy;
            ble_phy_type_t rx_phy;
        } _on_phy_updated;

        struct {
            void *remote;
            gatt_status_t status;
            int32_t rssi;
        } _on_rssi_read;

        struct {
            void *remote;
            bt_status_t status;
            uint16_t interval;
            uint16_t latency;
            uint16_t timeout;
        } _on_conn_param_updated;

    } bt_message_gattc_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_GATT_CLIENT_H__ */
