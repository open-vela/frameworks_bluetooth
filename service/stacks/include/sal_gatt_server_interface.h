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

#ifndef __SAL_GATT_SERVER_INTERFACE_H__
#define __SAL_GATT_SERVER_INTERFACE_H__

#include "bt_status.h"
#include "gatt_define.h"
#include "gatts_service.h"

#include <stdint.h>
#include <stdio.h>

#define GATT_ELEMENT_GROUP_MASK 0xFF00
#define GATT_ELEMENT_GROUP_MAX 0xFF00
#define GATT_ELEMENT_GROUP_ID(element_id) (element_id & GATT_ELEMENT_GROUP_MASK)

bt_status_t bt_sal_gatt_server_enable(void);
bt_status_t bt_sal_gatt_server_disable(void);
bt_status_t bt_sal_gatt_server_add_elements(gatt_element_t* elements, uint16_t size);
bt_status_t bt_sal_gatt_server_remove_elements(gatt_element_t* elements, uint16_t size);
bt_status_t bt_sal_gatt_server_connect(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type);
bt_status_t bt_sal_gatt_server_cancel_connection(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_gatt_server_send_response(bt_controller_id_t id, bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length);
bt_status_t bt_sal_gatt_server_set_attr_value(bt_controller_id_t id, bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length);
bt_status_t bt_sal_gatt_server_get_attr_value(bt_controller_id_t id, bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length);
#if defined(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
bt_status_t bt_sal_gatt_server_send_notification(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t element_id, uint8_t* value, uint16_t length);
bt_status_t bt_sal_gatt_server_send_indication(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t element_id, uint8_t* value, uint16_t length);
#else
bt_status_t bt_sal_gatt_server_send_notification(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length);
bt_status_t bt_sal_gatt_server_send_indication(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length);
#endif
bt_status_t bt_sal_gatt_server_read_phy(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_gatt_server_set_phy(bt_controller_id_t id, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
void bt_sal_gatt_server_connection_changed_callback(bt_address_t* addr, uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout);

#endif
