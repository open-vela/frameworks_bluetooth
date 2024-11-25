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
#ifndef __GATTC_SERVICE_H__
#define __GATTC_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_gatt_defs.h"
#include "bt_gattc.h"
#include "gatt_define.h"

/*
 * sal callback
 */
void if_gattc_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);
void if_gattc_on_service_discovered(bt_address_t* addr, gatt_element_t* elements, uint16_t size);
void if_gattc_on_discover_completed(bt_address_t* addr, gatt_status_t status);
void if_gattc_on_element_read(bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length, gatt_status_t status);
void if_gattc_on_element_written(bt_address_t* addr, uint16_t element_id, gatt_status_t status);
void if_gattc_on_element_subscribed(bt_address_t* addr, uint16_t element_id, gatt_status_t status, bool enable);
void if_gattc_on_element_changed(bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length);
void if_gattc_on_mtu_changed(bt_address_t* addr, uint32_t mtu, gatt_status_t status);
void if_gattc_on_phy_read(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
void if_gattc_on_phy_updated(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, gatt_status_t status);
void if_gattc_on_rssi_read(bt_address_t* addr, int32_t rssi, gatt_status_t status);
void if_gattc_on_connection_parameter_updated(bt_address_t* addr, uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout, bt_status_t status);

/*
 * gattc remote
 */
void* if_gattc_get_remote(void* conn_handle);

typedef struct gattc_interface {
    size_t size;
    bt_status_t (*create_connect)(void* remote, void** phandle, gattc_callbacks_t* callbacks);
    bt_status_t (*delete_connect)(void* conn_handle);
    bt_status_t (*connect)(void* conn_handle, bt_address_t* addr, ble_addr_type_t addr_type);
    bt_status_t (*disconnect)(void* conn_handle);
    bt_status_t (*discover_service)(void* conn_handle, bt_uuid_t* filter_uuid);
    bt_status_t (*get_attribute_by_handle)(void* conn_handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc);
    bt_status_t (*get_attribute_by_uuid)(void* conn_handle, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc);
    bt_status_t (*read)(void* conn_handle, uint16_t attr_handle);
    bt_status_t (*write)(void* conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
    bt_status_t (*write_without_response)(void* conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
    bt_status_t (*subscribe)(void* conn_handle, uint16_t attr_handle, uint16_t ccc_value);
    bt_status_t (*unsubscribe)(void* conn_handle, uint16_t attr_handle);
    bt_status_t (*exchange_mtu)(void* conn_handle, uint32_t mtu);
    bt_status_t (*update_connection_parameter)(void* conn_handle, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
        uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
    bt_status_t (*read_phy)(void* conn_handle);
    bt_status_t (*update_phy)(void* conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_status_t (*read_rssi)(void* conn_handle);
} gattc_interface_t;

/*
 * register profile to service manager
 */
void register_gattc_service(void);

#endif /* __GATTC_SERVICE_H__ */