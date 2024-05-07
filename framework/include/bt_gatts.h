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

#ifndef __BT_GATTS_H__
#define __BT_GATTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_gatt_defs.h"
#include "bt_status.h"
#include "bt_uuid.h"
#include <stddef.h>

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

typedef void* gatts_handle_t;

typedef uint16_t (*attribute_read_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle);
typedef uint16_t (*attribute_written_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);

typedef struct {
    uint16_t handle;
    bt_uuid_t uuid;
    gatt_attr_type_t type;
    uint32_t properties;
    uint32_t permissions;
    gatt_attr_rsp_t rsp_type;
    attribute_read_cb_t read_cb;
    attribute_written_cb_t write_cb;
    uint32_t attr_length;
    uint8_t* attr_value;

} gatt_attr_db_t;

typedef struct {
    int32_t attr_num;
    gatt_attr_db_t* attr_db;
} gatt_srv_db_t;

typedef void (*gatts_connected_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr);
typedef void (*gatts_disconnected_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr);
typedef void (*gatts_attr_table_added_cb_t)(gatts_handle_t srv_handle, gatt_status_t status, uint16_t attr_handle);
typedef void (*gatts_attr_table_removed_cb_t)(gatts_handle_t srv_handle, gatt_status_t status, uint16_t attr_handle);
typedef void (*gatts_mtu_changed_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t mtu);
typedef void (*gatts_nofity_complete_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle);
typedef void (*gatts_phy_read_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
typedef void (*gatts_phy_updated_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy);
typedef void (*gatts_connection_parameter_changed_cb_t)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout);

typedef struct {
    uint32_t size;
    gatts_connected_cb_t on_connected;
    gatts_disconnected_cb_t on_disconnected;
    gatts_attr_table_added_cb_t on_attr_table_added;
    gatts_attr_table_removed_cb_t on_attr_table_removed;
    gatts_nofity_complete_cb_t on_notify_complete;
    gatts_mtu_changed_cb_t on_mtu_changed;
    gatts_phy_read_cb_t on_phy_read;
    gatts_phy_updated_cb_t on_phy_updated;
    gatts_connection_parameter_changed_cb_t on_conn_param_changed;

} gatts_callbacks_t;

bt_status_t BTSYMBOLS(bt_gatts_register_service)(bt_instance_t* ins, gatts_handle_t* phandle, gatts_callbacks_t* callbacks);
bt_status_t BTSYMBOLS(bt_gatts_unregister_service)(gatts_handle_t srv_handle);
bt_status_t BTSYMBOLS(bt_gatts_connect)(gatts_handle_t srv_handle, bt_address_t* addr, ble_addr_type_t addr_type);
bt_status_t BTSYMBOLS(bt_gatts_disconnect)(gatts_handle_t srv_handle, bt_address_t* addr);
bt_status_t BTSYMBOLS(bt_gatts_add_attr_table)(gatts_handle_t srv_handle, gatt_srv_db_t* srv_db);
bt_status_t BTSYMBOLS(bt_gatts_remove_attr_table)(gatts_handle_t srv_handle, uint16_t attr_handle);
bt_status_t BTSYMBOLS(bt_gatts_set_attr_value)(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
bt_status_t BTSYMBOLS(bt_gatts_get_attr_value)(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t* length);
bt_status_t BTSYMBOLS(bt_gatts_response)(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t req_handle, uint8_t* value, uint16_t length);
bt_status_t BTSYMBOLS(bt_gatts_notify)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length);
bt_status_t BTSYMBOLS(bt_gatts_indicate)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length);
bt_status_t BTSYMBOLS(bt_gatts_read_phy)(gatts_handle_t srv_handle, bt_address_t* addr);
bt_status_t BTSYMBOLS(bt_gatts_update_phy)(gatts_handle_t srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);

#ifdef __cplusplus
}
#endif

#endif /* __BT_GATTS_H__ */
