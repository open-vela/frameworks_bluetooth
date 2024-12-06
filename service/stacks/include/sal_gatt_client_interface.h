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
#ifndef __SAL_GATT_CLIENT_INTERFACE_H__
#define __SAL_GATT_CLIENT_INTERFACE_H__

#include "bt_addr.h"
#include "bt_status.h"
#include "gattc_service.h"
#include <stdint.h>

#define GATT_ELEMENT_GROUP_MASK 0xFF00
#define GATT_ELEMENT_GROUP_MAX 0xFF00
#define GATT_ELEMENT_GROUP_ID(element_id) (element_id & GATT_ELEMENT_GROUP_MASK)

bt_status_t bt_sal_gatt_client_connect(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type);
bt_status_t bt_sal_gatt_client_disconnect(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_gatt_client_discover_all_services(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_gatt_client_discover_service_by_uuid(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t* uuid);
bt_status_t bt_sal_gatt_client_read_element(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id);
bt_status_t bt_sal_gatt_client_write_element(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length, gatt_write_type_t write_type);
bt_status_t bt_sal_gatt_client_register_notifications(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id, uint16_t properties, bool enable);
bt_status_t bt_sal_gatt_client_send_mtu_req(bt_controller_id_t id, bt_address_t* addr, uint32_t mtu);
bt_status_t bt_sal_gatt_client_update_connection_parameter(bt_controller_id_t id, bt_address_t* addr, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
bt_status_t bt_sal_gatt_client_read_remote_rssi(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_gatt_client_read_phy(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_gatt_client_set_phy(bt_controller_id_t id, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
void bt_sal_gatt_client_connection_updated_callback(bt_controller_id_t id, bt_address_t* addr, uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout, bt_status_t status);

#endif /* __SAL_GATT_CLIENT_INTERFACE_H__ */
