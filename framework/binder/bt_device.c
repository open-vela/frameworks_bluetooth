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
#include "device.h"

#include "adapter_proxy.h"
#include "adapter_stub.h"

bt_address_t* bt_device_get_identity_address(bt_instance_t* ins, bt_address_t* addr)
{
    return NULL;
}

ble_addr_type_t bt_device_get_address_type(bt_instance_t* ins, bt_address_t* addr)
{
    return 0;
}

bt_device_type_t bt_device_get_device_type(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_getRemoteDeviceType((BpBtAdapter*)ins->adapter_proxy, addr);
}

bool bt_device_get_name(bt_instance_t* ins, bt_address_t* addr, char* name, uint32_t length)
{
    return BpBtAdapter_getRemoteName((BpBtAdapter*)ins->adapter_proxy, addr, name, length);
}

uint32_t bt_device_get_device_class(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_getRemoteDeviceClass((BpBtAdapter*)ins->adapter_proxy, addr);
}

bt_status_t bt_device_get_uuids(bt_instance_t* ins, bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator)
{
    return BpBtAdapter_getRemoteUuids((BpBtAdapter*)ins->adapter_proxy, addr, uuids, size, allocator);
}

uint16_t bt_device_get_appearance(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_getRemoteAppearance((BpBtAdapter*)ins->adapter_proxy, addr);
}

int8_t bt_device_get_rssi(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_getRemoteRssi((BpBtAdapter*)ins->adapter_proxy, addr);
}

bool bt_device_get_alias(bt_instance_t* ins, bt_address_t* addr, char* alias, uint32_t length)
{
    return BpBtAdapter_getRemoteAlias((BpBtAdapter*)ins->adapter_proxy, addr, alias, length);
}

bt_status_t bt_device_set_alias(bt_instance_t* ins, bt_address_t* addr, const char* alias)
{
    return BpBtAdapter_setRemoteAlias((BpBtAdapter*)ins->adapter_proxy, addr, alias);
}

bool bt_device_is_connected(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_isRemoteConnected((BpBtAdapter*)ins->adapter_proxy, addr);
}

bool bt_device_is_encrypted(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_isRemoteEncrypted((BpBtAdapter*)ins->adapter_proxy, addr);
}

bool bt_device_is_bond_initiate_local(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_isBondInitiateLocal((BpBtAdapter*)ins->adapter_proxy, addr);
}

bond_state_t bt_device_get_bond_state(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_getRemoteBondState((BpBtAdapter*)ins->adapter_proxy, addr);
}

bool bt_device_is_bonded(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_isRemoteBonded((BpBtAdapter*)ins->adapter_proxy, addr);
}

bt_status_t bt_device_connect(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_connect((BpBtAdapter*)ins->adapter_proxy, addr);
}

bt_status_t bt_device_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_disconnect((BpBtAdapter*)ins->adapter_proxy, addr);
}

bt_status_t bt_device_connect_le(bt_instance_t* ins,
    bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param)
{
    return BpBtAdapter_leConnect((BpBtAdapter*)ins->adapter_proxy, addr, type, param);
}

bt_status_t bt_device_disconnect_le(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_leDisconnect((BpBtAdapter*)ins->adapter_proxy, addr);
}

void bt_device_connect_all_profile(bt_instance_t* ins, bt_address_t* addr)
{
}

void bt_device_disconnect_all_profile(bt_instance_t* ins, bt_address_t* addr)
{
}

bt_status_t bt_device_set_le_phy(bt_instance_t* ins,
    bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy)
{
    return BpBtAdapter_leSetPhy((BpBtAdapter*)ins->adapter_proxy, addr, tx_phy, rx_phy);
}

bt_status_t bt_device_create_bond(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    return BpBtAdapter_createBond((BpBtAdapter*)ins->adapter_proxy, addr, transport);
}

bt_status_t bt_device_remove_bond(bt_instance_t* ins, bt_address_t* addr, uint8_t transport)
{
    return BpBtAdapter_removeBond((BpBtAdapter*)ins->adapter_proxy, addr, transport);
}

bt_status_t bt_device_cancel_bond(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_cancelBond((BpBtAdapter*)ins->adapter_proxy, addr);
}

bt_status_t bt_device_pair_request_reply(bt_instance_t* ins, bt_address_t* addr, bool accept)
{
    return BpBtAdapter_pairRequestReply((BpBtAdapter*)ins->adapter_proxy, addr, accept);
}

bt_status_t bt_device_set_pairing_confirmation(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept)
{
    return BpBtAdapter_setPairingConfirmation((BpBtAdapter*)ins->adapter_proxy, addr, transport, accept);
}

bt_status_t bt_device_set_pin_code(bt_instance_t* ins, bt_address_t* addr, bool accept,
    char* pincode, int len)
{
    return BpBtAdapter_setPinCode((BpBtAdapter*)ins->adapter_proxy, addr, accept, pincode, len);
}

bt_status_t bt_device_set_pass_key(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept, uint32_t passkey)
{
    return BpBtAdapter_setPassKey((BpBtAdapter*)ins->adapter_proxy, addr, transport, accept, passkey);
}
