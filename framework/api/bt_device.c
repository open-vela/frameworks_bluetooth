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
#include <stdint.h>
#include <stdlib.h>

#include "adapter_internel.h"
#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_device.h"
#include "bt_internal.h"
#include "connection_manager.h"
#include "device.h"

bt_status_t BTSYMBOLS(bt_device_get_identity_address)(bt_instance_t* ins, bt_address_t* bd_addr, bt_address_t* id_addr)
{
    return adapter_get_remote_identity_address(bd_addr, id_addr);
}

ble_addr_type_t BTSYMBOLS(bt_device_get_address_type)(bt_instance_t* ins, bt_address_t* addr)
{
    return 0;
}

bt_device_type_t BTSYMBOLS(bt_device_get_device_type)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_get_remote_device_type(addr);
}

bool BTSYMBOLS(bt_device_get_name)(bt_instance_t* ins, bt_address_t* addr, char* name, uint32_t length)
{
    return adapter_get_remote_name(addr, name);
}

uint32_t BTSYMBOLS(bt_device_get_device_class)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_get_remote_device_class(addr);
}

bt_status_t BTSYMBOLS(bt_device_get_uuids)(bt_instance_t* ins, bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator)
{
    return adapter_get_remote_uuids(addr, uuids, size, allocator);
}

uint16_t BTSYMBOLS(bt_device_get_appearance)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_get_remote_appearance(addr);
}

int8_t BTSYMBOLS(bt_device_get_rssi)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_get_remote_rssi(addr);
}

bool BTSYMBOLS(bt_device_get_alias)(bt_instance_t* ins, bt_address_t* addr, char* alias, uint32_t length)
{
    return adapter_get_remote_alias(addr, alias);
}

bt_status_t BTSYMBOLS(bt_device_set_alias)(bt_instance_t* ins, bt_address_t* addr, const char* alias)
{
    return adapter_set_remote_alias(addr, alias);
}

bool BTSYMBOLS(bt_device_is_connected)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return adapter_is_remote_connected(addr, transport);
}

bool BTSYMBOLS(bt_device_is_encrypted)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return adapter_is_remote_encrypted(addr, transport);
}

bool BTSYMBOLS(bt_device_is_bond_initiate_local)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return adapter_is_bond_initiate_local(addr, transport);
}

bond_state_t BTSYMBOLS(bt_device_get_bond_state)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return adapter_get_remote_bond_state(addr, transport);
}

bool BTSYMBOLS(bt_device_is_bonded)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return adapter_is_remote_bonded(addr, transport);
}

bt_status_t BTSYMBOLS(bt_device_connect)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_connect(addr);
}

bt_status_t BTSYMBOLS(bt_device_disconnect)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_disconnect(addr);
}

bt_status_t BTSYMBOLS(bt_device_background_connect)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return bt_cm_device_connect(addr, transport);
}

bt_status_t BTSYMBOLS(bt_device_background_disconnect)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return bt_cm_device_disconnect(addr, transport);
}

bt_status_t BTSYMBOLS(bt_device_connect_le)(bt_instance_t* ins,
    bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param)
{
    return adapter_le_connect(addr, type, param);
}

bt_status_t BTSYMBOLS(bt_device_disconnect_le)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_le_disconnect(addr);
}

bt_status_t BTSYMBOLS(bt_device_connect_request_reply)(bt_instance_t* ins, bt_address_t* addr, bool accept)
{
    return adapter_connect_request_reply(addr, accept);
}

void BTSYMBOLS(bt_device_connect_all_profile)(bt_instance_t* ins, bt_address_t* addr)
{
}

void BTSYMBOLS(bt_device_disconnect_all_profile)(bt_instance_t* ins, bt_address_t* addr)
{
}

bt_status_t BTSYMBOLS(bt_device_set_le_phy)(bt_instance_t* ins, bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy)
{
    return adapter_le_set_phy(addr, tx_phy, rx_phy);
}

bt_status_t BTSYMBOLS(bt_device_create_bond)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return adapter_create_bond(addr, transport);
}

bt_status_t BTSYMBOLS(bt_device_remove_bond)(bt_instance_t* ins, bt_address_t* addr, uint8_t transport)
{
    return adapter_remove_bond(addr, transport);
}

bt_status_t BTSYMBOLS(bt_device_cancel_bond)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_cancel_bond(addr);
}

bt_status_t BTSYMBOLS(bt_device_pair_request_reply)(bt_instance_t* ins, bt_address_t* addr, bool accept)
{
    return adapter_pair_request_reply(addr, accept);
}

bt_status_t BTSYMBOLS(bt_device_set_pairing_confirmation)(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept)
{
    return adapter_set_pairing_confirmation(addr, transport, accept);
}

bt_status_t BTSYMBOLS(bt_device_set_pin_code)(bt_instance_t* ins, bt_address_t* addr, bool accept,
    char* pincode, int len)
{
    return adapter_set_pin_code(addr, accept, pincode, len);
}

bt_status_t BTSYMBOLS(bt_device_set_pass_key)(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept, uint32_t passkey)
{
    return adapter_set_pass_key(addr, transport, accept, passkey);
}

bt_status_t BTSYMBOLS(bt_device_set_le_legacy_tk)(bt_instance_t* ins, bt_address_t* addr, bt_128key_t tk_val)
{
    return adapter_le_set_legacy_tk(addr, tk_val);
}

bt_status_t BTSYMBOLS(bt_device_set_le_sc_remote_oob_data)(bt_instance_t* ins, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    return adapter_le_set_remote_oob_data(addr, c_val, r_val);
}

bt_status_t BTSYMBOLS(bt_device_get_le_sc_local_oob_data)(bt_instance_t* ins, bt_address_t* addr)
{
    return adapter_le_get_local_oob_data(addr);
}

bt_status_t BTSYMBOLS(bt_device_enable_enhanced_mode)(bt_instance_t* ins, bt_address_t* addr, bt_enhanced_mode_t mode)
{
    return bt_cm_enable_enhanced_mode(addr, mode);
}

bt_status_t BTSYMBOLS(bt_device_disable_enhanced_mode)(bt_instance_t* ins, bt_address_t* addr, bt_enhanced_mode_t mode)
{
    return bt_cm_disable_enhanced_mode(addr, mode);
}