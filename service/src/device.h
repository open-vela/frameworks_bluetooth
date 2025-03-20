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
#ifndef __REMOTE_DEVICE_H__
#define __REMOTE_DEVICE_H__

#include "bluetooth_define.h"
#include "bt_list.h"

#define DFLAG_NAME_SET (1 << 0)
#define DFLAG_GET_RMT_NAME (1 << 1)
#define DFLAG_ALIAS_SET (1 << 2)
#define DFLAG_LINKKEY_SET (1 << 3)
#define DFLAG_WHITELIST_ADDED (1 << 4)
#define DFLAG_CONNECTED (1 << 5)
#define DFLAG_BONDED (1 << 6)
#define DFLAG_LE_KEY_SET (1 << 7)

typedef struct bt_device bt_device_t;

bt_device_t* br_device_create(bt_address_t* addr);
bt_device_t* le_device_create(bt_address_t* addr, ble_addr_type_t addr_type);
void device_delete(bt_device_t* device);
bt_transport_t device_get_transport(bt_device_t* device);
bt_address_t* device_get_address(bt_device_t* device);
bt_address_t* device_get_identity_address(bt_device_t* device);
void device_set_identity_address(bt_device_t* device, bt_address_t* addr);
ble_addr_type_t device_get_address_type(bt_device_t* device);
void device_set_address_type(bt_device_t* device, ble_addr_type_t type);
void device_set_device_type(bt_device_t* device, bt_device_type_t type);
bt_device_type_t device_get_device_type(bt_device_t* device);
const char* device_get_name(bt_device_t* device);
bool device_set_name(bt_device_t* device, const char* name);
uint32_t device_get_device_class(bt_device_t* device);
bool device_set_device_class(bt_device_t* device, uint32_t cod);
uint16_t device_get_uuids_size(bt_device_t* device);
uint16_t device_get_uuids(bt_device_t* device, bt_uuid_t* uuids, uint16_t size);
bool device_set_uuids(bt_device_t* device, bt_uuid_t* uuids, uint16_t size);
uint16_t device_get_appearance(bt_device_t* device);
void device_set_appearance(bt_device_t* device, uint16_t appearance);
int8_t device_get_rssi(bt_device_t* device);
void device_set_rssi(bt_device_t* device, int8_t rssi);
const char* device_get_alias(bt_device_t* device);
bool device_set_alias(bt_device_t* device, const char* alias);
connection_state_t device_get_connection_state(bt_device_t* device);
void device_set_connection_state(bt_device_t* device, connection_state_t state);
bool device_is_connected(bt_device_t* device);
bool device_is_encrypted(bt_device_t* device);
uint16_t device_get_acl_handle(bt_device_t* device);
void device_set_acl_handle(bt_device_t* device, uint16_t handle);
bt_link_role_t device_get_local_role(bt_device_t* device);
void device_set_local_role(bt_device_t* device, bt_link_role_t role);
void device_set_bond_initiate_local(bt_device_t* device, bool initiate_local);
bool device_is_bond_initiate_local(bt_device_t* device);
bond_state_t device_get_bond_state(bt_device_t* device);
void device_set_bond_state(bt_device_t* device, bond_state_t state, bool is_ctkd, void* notify_change);
bool device_is_bonded(bt_device_t* device);
uint8_t* device_get_link_key(bt_device_t* device);
void device_set_link_key(bt_device_t* device, bt_128key_t link_key);
void device_delete_link_key(bt_device_t* device);
bt_link_key_type_t device_get_link_key_type(bt_device_t* device);
void device_set_link_key_type(bt_device_t* device, bt_link_key_type_t type);
bt_link_policy_t device_get_link_policy(bt_device_t* device);
void device_set_link_policy(bt_device_t* device, bt_link_policy_t policy);
void device_set_le_phy(bt_device_t* device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
void device_get_le_phy(bt_device_t* device, ble_phy_type_t* tx_phy, ble_phy_type_t* rx_phy);
void device_get_property(bt_device_t* device, remote_device_properties_t* prop);
void device_set_flags(bt_device_t* device, uint32_t flags);
void device_clear_flag(bt_device_t* device, uint32_t flag);
bool device_check_flag(bt_device_t* device, uint32_t flag);
uint8_t* device_get_smp_key(bt_device_t* device);
void device_set_smp_key(bt_device_t* device, uint8_t* smp_key);
void device_delete_smp_key(bt_device_t* device);
void device_get_le_property(bt_device_t* device, remote_device_le_properties_t* prop);
void device_dump(bt_device_t* device);

#endif /* __REMOTE_DEVICE_H__ */