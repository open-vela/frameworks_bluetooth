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

#ifndef __BT_GATTC_H__
#define __BT_GATTC_H__

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

typedef void* gattc_handle_t;

typedef struct {
    uint8_t type; /* gatt_attr_type_t */
    uint8_t pad[1];
    uint16_t handle;
    bt_uuid_t uuid;
    uint32_t properties;

} gatt_attr_desc_t;

typedef void (*gattc_connected_cb_t)(gattc_handle_t conn_handle, bt_address_t* addr);
typedef void (*gattc_disconnected_cb_t)(gattc_handle_t conn_handle, bt_address_t* addr);
typedef void (*gattc_discover_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, bt_uuid_t* uuid, uint16_t start_handle, uint16_t end_handle);
typedef void (*gattc_mtu_updated_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint32_t mtu);
typedef void (*gattc_read_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle, uint8_t* value, uint16_t length);
typedef void (*gattc_write_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle);
typedef void (*gattc_subscribe_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle, bool enable);
typedef void (*gattc_notify_cb_t)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
typedef void (*gattc_phy_read_cb_t)(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
typedef void (*gattc_phy_updated_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
typedef void (*gattc_rssi_read_cb_t)(gattc_handle_t conn_handle, gatt_status_t status, int32_t rssi);
typedef void (*gattc_connection_parameter_updated_cb_t)(gattc_handle_t conn_handle, bt_status_t status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout);

typedef struct {
    uint32_t size;
    gattc_connected_cb_t on_connected;
    gattc_disconnected_cb_t on_disconnected;
    gattc_discover_cb_t on_discovered;
    gattc_read_cb_t on_read;
    gattc_write_cb_t on_written;
    gattc_subscribe_cb_t on_subscribed;
    gattc_notify_cb_t on_notified;
    gattc_mtu_updated_cb_t on_mtu_updated;
    gattc_phy_read_cb_t on_phy_read;
    gattc_phy_updated_cb_t on_phy_updated;
    gattc_rssi_read_cb_t on_rssi_read;
    gattc_connection_parameter_updated_cb_t on_conn_param_updated;
} gattc_callbacks_t;

bt_status_t BTSYMBOLS(bt_gattc_create_connect)(bt_instance_t* ins, gattc_handle_t* phandle, gattc_callbacks_t* callbacks);
bt_status_t BTSYMBOLS(bt_gattc_delete_connect)(gattc_handle_t conn_handle);
bt_status_t BTSYMBOLS(bt_gattc_connect)(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type);
bt_status_t BTSYMBOLS(bt_gattc_disconnect)(gattc_handle_t conn_handle);
bt_status_t BTSYMBOLS(bt_gattc_discover_service)(gattc_handle_t conn_handle, bt_uuid_t* filter_uuid);
bt_status_t BTSYMBOLS(bt_gattc_get_attribute_by_handle)(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc);
bt_status_t BTSYMBOLS(bt_gattc_get_attribute_by_uuid)(gattc_handle_t conn_handle, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc);
bt_status_t BTSYMBOLS(bt_gattc_read)(gattc_handle_t conn_handle, uint16_t attr_handle);
bt_status_t BTSYMBOLS(bt_gattc_write)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
bt_status_t BTSYMBOLS(bt_gattc_write_without_response)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
bt_status_t BTSYMBOLS(bt_gattc_subscribe)(gattc_handle_t conn_handle, uint16_t attr_handle, uint16_t ccc_value);
bt_status_t BTSYMBOLS(bt_gattc_unsubscribe)(gattc_handle_t conn_handle, uint16_t attr_handle);
bt_status_t BTSYMBOLS(bt_gattc_exchange_mtu)(gattc_handle_t conn_handle, uint32_t mtu);
bt_status_t BTSYMBOLS(bt_gattc_update_connection_parameter)(gattc_handle_t conn_handle, uint32_t min_interval, uint32_t max_interval,
    uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length,
    uint32_t max_connection_event_length);
bt_status_t BTSYMBOLS(bt_gattc_read_phy)(gattc_handle_t conn_handle);
bt_status_t BTSYMBOLS(bt_gattc_update_phy)(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
bt_status_t BTSYMBOLS(bt_gattc_read_rssi)(gattc_handle_t conn_handle);

#ifdef __cplusplus
}
#endif

#endif /* __BT_GATTC_H__ */
