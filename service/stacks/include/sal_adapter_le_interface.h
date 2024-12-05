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
#ifndef __SAL_ADAPTER_LE_INTERFACE_H_
#define __SAL_ADAPTER_LE_INTERFACE_H_

#include <stdint.h>

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_addr.h"
#include "bt_status.h"
#include "power_manager.h"
#include "vhal/bt_vhal.h"

bt_status_t bt_sal_le_init(const bt_vhal_interface* vhal);
void bt_sal_le_cleanup(void);
bt_status_t bt_sal_le_enable(bt_controller_id_t id);
bt_status_t bt_sal_le_disable(bt_controller_id_t id);
bt_status_t bt_sal_le_set_io_capability(bt_controller_id_t id, bt_io_capability_t cap);
bt_status_t bt_sal_le_set_static_identity(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_le_set_public_identity(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_le_set_address(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_le_get_address(bt_controller_id_t id);
bt_status_t bt_sal_le_set_bonded_devices(bt_controller_id_t id, remote_device_le_properties_t* props, uint16_t prop_cnt);
bt_status_t bt_sal_le_get_bonded_devices(bt_controller_id_t id, remote_device_le_properties_t* props, uint16_t* prop_cnt);
bt_status_t bt_sal_le_connect(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t type, ble_connect_params_t* params);
bt_status_t bt_sal_le_disconnect(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_le_create_bond(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t type);
bt_status_t bt_sal_le_remove_bond(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_le_smp_reply(bt_controller_id_t id, bt_address_t* addr, bool accept, bt_pair_type_t type, uint32_t passkey);
bt_status_t bt_sal_le_set_legacy_tk(bt_controller_id_t id, bt_address_t* addr, bt_128key_t tk_val);
bt_status_t bt_sal_le_set_remote_oob_data(bt_controller_id_t id, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val);
bt_status_t bt_sal_le_get_local_oob_data(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_le_add_white_list(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type);
bt_status_t bt_sal_le_remove_white_list(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type);
bt_status_t bt_sal_le_set_phy(bt_controller_id_t id, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
bt_status_t bt_sal_le_set_appearance(bt_controller_id_t id, uint16_t appearance);
uint16_t bt_sal_le_get_appearance(bt_controller_id_t id);
bt_status_t bt_sal_le_enable_key_derivation(bt_controller_id_t id, bool brkey_to_lekey, bool lekey_to_brkey);

struct bt_conn* get_le_conn_from_addr(bt_address_t* addr);
bt_status_t get_le_addr_from_conn(struct bt_conn* conn, bt_address_t* addr);

#endif /* __SAL_ADAPTER_LE_INTERFACE_H_ */
