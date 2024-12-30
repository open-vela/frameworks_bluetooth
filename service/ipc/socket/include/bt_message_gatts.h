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
BT_GATT_SERVER_MESSAGE_START,
    BT_GATT_SERVER_REGISTER_SERVICE,
    BT_GATT_SERVER_UNREGISTER_SERVICE,
    BT_GATT_SERVER_CONNECT,
    BT_GATT_SERVER_CONNECT_BEAR,
    BT_GATT_SERVER_DISCONNECT,
    BT_GATT_SERVER_ADD_ATTR_TABLE,
    BT_GATT_SERVER_REMOVE_ATTR_TABLE,
    BT_GATT_SERVER_SET_ATTR_VALUE,
    BT_GATT_SERVER_GET_ATTR_VALUE,
    BT_GATT_SERVER_RESPONSE,
    BT_GATT_SERVER_NOTIFY,
    BT_GATT_SERVER_INDICATE,
    BT_GATT_SERVER_READ_PHY,
    BT_GATT_SERVER_UPDATE_PHY,
    BT_GATT_SERVER_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_GATT_SERVER_CALLBACK_START,
    BT_GATT_SERVER_ON_CONNECTED,
    BT_GATT_SERVER_ON_DISCONNECTED,
    BT_GATT_SERVER_ON_ATTR_TABLE_ADDED,
    BT_GATT_SERVER_ON_ATTR_TABLE_REMOVED,
    BT_GATT_SERVER_ON_MTU_CHANGED,
    BT_GATT_SERVER_ON_READ_REQUEST,
    BT_GATT_SERVER_ON_WRITE_REQUEST,
    BT_GATT_SERVER_NOTIFY_COMPLETE,
    BT_GATT_SERVER_ON_PHY_READ,
    BT_GATT_SERVER_ON_PHY_UPDATED,
    BT_GATT_SERVER_ON_CONN_PARAM_CHANGED,
    BT_GATT_SERVER_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_GATT_SERVER_H__
#define _BT_MESSAGE_GATT_SERVER_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_gatts.h"

    typedef struct {
        bt_instance_t* ins;
        gatts_callbacks_t* callbacks;
        uint64_t cookie;
        void** user_phandle;
        bt_list_t* db_list;
    } bt_gatts_remote_t;

    typedef struct {
        uint8_t status; /* bt_status_t */
        uint8_t pad[3];
        union {
            uint64_t handle; /* gatts_handle_t */
            struct {
                uint16_t length;
                uint8_t value[32];
            };
        };
    } bt_gatts_result_t;

    typedef union {
        struct {
            uint64_t cookie; /* void* */
        } _bt_gatts_register;

        struct {
            uint64_t handle; /* gatts_handle_t */
        } _bt_gatts_unregister;

        struct {
            uint64_t handle; /* gatts_handle_t */
            bt_address_t addr;
            uint8_t addr_type; /* ble_addr_type_t */
        } _bt_gatts_connect;

        struct {
            uint64_t handle; /* gatts_handle_t */
            bt_address_t addr;
            uint8_t addr_type; /* ble_addr_type_t */
            uint8_t bear_type; /* ble_bear_type_t */
        } _bt_gatts_connect_bear;

        struct {
            uint64_t handle; /* gatts_handle_t */
            bt_address_t addr;
        } _bt_gatts_disconnect;

        struct {
            uint64_t handle; /* gatts_handle_t */
            int32_t attr_num;
            struct {
                bt_uuid_t uuid;
                uint16_t handle;
                uint8_t type; /* gatt_attr_type_t */
                uint8_t rsp_type; /* gatt_attr_rsp_t */
                uint32_t properties;
                uint32_t permissions;
                uint32_t attr_length;
            } attr_db[GATTS_MAX_ATTRIBUTE_NUM];
        } _bt_gatts_add_attr_table;

        struct {
            uint64_t handle; /* gatts_handle_t */
            uint16_t attr_handle;
        } _bt_gatts_remove_attr_table;

        struct {
            uint64_t handle; /* gatts_handle_t */
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[32];
        } _bt_gatts_set_attr_value;

        struct {
            uint64_t handle; /* gatts_handle_t */
            uint16_t attr_handle;
            uint16_t length;
        } _bt_gatts_get_attr_value;

        struct {
            uint64_t handle; /* gatts_handle_t */
            uint32_t req_handle;
            bt_address_t addr;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 1];
        } _bt_gatts_response;

        struct {
            uint64_t handle; /* gatts_handle_t */
            bt_address_t addr;
            uint16_t attr_handle;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 3];
        } _bt_gatts_notify;

        struct {
            uint64_t handle; /* gatts_handle_t */
            bt_address_t addr;
            uint8_t tx_phy; /* ble_phy_type_t */
            uint8_t rx_phy; /* ble_phy_type_t */
        } _bt_gatts_phy;

    } bt_message_gatts_t;

    typedef union {
        struct {
            uint64_t remote; /* void* */
        } _on_callback;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
        } _on_connected;

        struct {
            void* remote;
            bt_address_t addr;
        } _on_disconnected;

        struct {
            uint64_t remote; /* void* */
            uint16_t attr_handle;
            uint8_t status; /* gatt_status_t */
        } _on_attr_table_added;

        struct {
            uint64_t remote; /* void* */
            uint16_t attr_handle;
            uint8_t status; /* gatt_status_t */
        } _on_attr_table_removed;

        struct {
            uint64_t remote; /* void* */
            uint32_t mtu;
            bt_address_t addr;
        } _on_mtu_changed;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
            uint16_t attr_handle;
            uint32_t req_handle;
        } _on_read_request;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
            uint16_t attr_handle;
            uint16_t offset;
            uint16_t length;
            uint8_t value[GATT_MAX_MTU_SIZE - 3];
        } _on_write_request;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
            uint16_t attr_handle;
            uint8_t status; /* gatt_status_t */
        } _on_nofity_complete;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
            uint8_t status; /* gatt_status_t */
            uint8_t tx_phy; /* ble_phy_type_t */
            uint8_t rx_phy; /* ble_phy_type_t */
        } _on_phy_updated;

        struct {
            uint64_t remote; /* void* */
            bt_address_t addr;
            uint16_t interval;
            uint16_t latency;
            uint16_t timeout;
        } _on_conn_param_changed;

    } bt_message_gatts_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_GATT_SERVER_H__ */
