/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#ifndef __ADAPTER_PROXY_H__
#define __ADAPTER_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include "adapter_stub.h"
#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_le_advertiser.h"
#include "bt_le_scan.h"

// #include <android/binder_auto_utils.h>

BpBtAdapter* BpBtAdapter_new(const char* instance);
void BpBtAdapter_delete(BpBtAdapter* bpAdapter);
AIBinder* BtAdapter_getService(BpBtAdapter** bpAdapter, const char* instance);

void* BpBtAdapter_registerCallback(BpBtAdapter* bpBinder, AIBinder* cbksBinder);
bool BpBtAdapter_unRegisterCallback(BpBtAdapter* bpBinder, void* cookie);
bt_status_t BpBtAdapter_enable(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_disable(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_enableLe(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_disableLe(BpBtAdapter* bpBinder);
bt_adapter_state_t BpBtAdapter_getState(BpBtAdapter* bpBinder);
bool BpBtAdapter_isLeEnabled(BpBtAdapter* bpBinder);
bt_device_type_t BpBtAdapter_getType(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_setDiscoveryFilter(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_startDiscovery(BpBtAdapter* bpBinder, uint32_t timeout);
bt_status_t BpBtAdapter_cancelDiscovery(BpBtAdapter* bpBinder);
bool BpBtAdapter_isDiscovering(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_getAddress(BpBtAdapter* bpBinder, bt_address_t* addr);
bt_status_t BpBtAdapter_setName(BpBtAdapter* bpBinder, const char* name);
bt_status_t BpBtAdapter_getName(BpBtAdapter* bpBinder, char* name, int length);
bt_status_t BpBtAdapter_getUuids(BpBtAdapter* bpBinder, bt_uuid_t* uuids, uint16_t* size);
bt_status_t BpBtAdapter_setScanMode(BpBtAdapter* bpBinder, bt_scan_mode_t mode, bool bondable);
bt_scan_mode_t BpBtAdapter_getScanMode(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_setDeviceClass(BpBtAdapter* bpBinder, uint32_t cod);
uint32_t BpBtAdapter_getDeviceClass(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_setIOCapability(BpBtAdapter* bpBinder, bt_io_capability_t cap);
bt_io_capability_t BpBtAdapter_getIOCapability(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_SetLeIOCapability(BpBtAdapter* bpBinder, uint32_t le_io_cap);
uint32_t BpBtAdapter_getLeIOCapability(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_getLeAddress(BpBtAdapter* bpBinder, bt_address_t* addr, ble_addr_type_t* type);
bt_status_t BpBtAdapter_setLeAddress(BpBtAdapter* bpBinder, bt_address_t* addr);
bt_status_t BpBtAdapter_setLeIdentityAddress(BpBtAdapter* bpBinder, bt_address_t* addr, bool public);
bt_status_t BpBtAdapter_setLeAppearance(BpBtAdapter* bpBinder, uint16_t appearance);
uint16_t BpBtAdapter_getLeAppearance(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_getBondedDevices(BpBtAdapter* bpBinder, bt_address_t** addr, int* num, bt_allocator_t allocator);
bt_status_t BpBtAdapter_getConnectedDevices(BpBtAdapter* bpBinder, bt_address_t** addr, int* num, bt_allocator_t allocator);
void BpBtAdapter_disconnectAllDevices(BpBtAdapter* bpBinder);
bool BpBtAdapter_isSupportBredr(BpBtAdapter* bpBinder);
bool BpBtAdapter_isSupportLe(BpBtAdapter* bpBinder);
bool BpBtAdapter_isSupportLeaudio(BpBtAdapter* bpBinder);
bt_status_t BpBtAdapter_leEnableKeyDerivation(BpBtAdapter* bpBinder,
    bool brkey_to_lekey,
    bool lekey_to_brkey);
bt_advertiser_t* BpBtAdapter_startAdvertising(BpBtAdapter* bpBinder,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    AIBinder* cbksBinder);
bt_status_t BpBtAdapter_stopAdvertising(BpBtAdapter* bpBinder, bt_advertiser_t* adver);
bt_status_t BpBtAdapter_stopAdvertisingId(BpBtAdapter* bpBinder, uint8_t adver_id);
bt_scanner_t* BpBtAdapter_startScan(BpBtAdapter* bpBinder, AIBinder* cbksBinder);
bt_scanner_t* BpBtAdapter_startScanSettings(BpBtAdapter* bpBinder,
    ble_scan_settings_t* settings,
    AIBinder* cbksBinder);
bt_status_t BpBtAdapter_stopScan(BpBtAdapter* bpBinder, bt_scanner_t* scanner);
bt_device_type_t BpBtAdapter_getRemoteDeviceType(BpBtAdapter* bpBinder, bt_address_t* addr);
bool BpBtAdapter_getRemoteName(BpBtAdapter* bpBinder, bt_address_t* addr, char* name, uint32_t length);
uint32_t BpBtAdapter_getRemoteDeviceClass(BpBtAdapter* bpBinder, bt_address_t* addr);
bt_status_t BpBtAdapter_getRemoteUuids(BpBtAdapter* bpBinder, bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator);
uint16_t BpBtAdapter_getRemoteAppearance(BpBtAdapter* bpBinder, bt_address_t* addr);
int8_t BpBtAdapter_getRemoteRssi(BpBtAdapter* bpBinder, bt_address_t* addr);
bool BpBtAdapter_getRemoteAlias(BpBtAdapter* bpBinder, bt_address_t* addr, char* alias, uint32_t length);
bt_status_t BpBtAdapter_setRemoteAlias(BpBtAdapter* bpBinder, bt_address_t* addr, const char* alias);
bool BpBtAdapter_isRemoteConnected(BpBtAdapter* bpBinder, bt_address_t* addr);
bool BpBtAdapter_isRemoteEncrypted(BpBtAdapter* bpBinder, bt_address_t* addr);
bool BpBtAdapter_isBondInitiateLocal(BpBtAdapter* bpBinder, bt_address_t* addr);
bond_state_t BpBtAdapter_getRemoteBondState(BpBtAdapter* bpBinder, bt_address_t* addr);
bool BpBtAdapter_isRemoteBonded(BpBtAdapter* bpBinder, bt_address_t* addr);
bt_status_t BpBtAdapter_connect(BpBtAdapter* bpBinder, bt_address_t* addr);
bt_status_t BpBtAdapter_disconnect(BpBtAdapter* bpBinder, bt_address_t* addr);

bt_status_t BpBtAdapter_leConnect(BpBtAdapter* bpBinder, bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param);
bt_status_t BpBtAdapter_leDisconnect(BpBtAdapter* bpBinder, bt_address_t* addr);
bt_status_t BpBtAdapter_leSetPhy(BpBtAdapter* bpBinder, bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy);
bt_status_t BpBtAdapter_createBond(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport);
bt_status_t BpBtAdapter_removeBond(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport);
bt_status_t BpBtAdapter_cancelBond(BpBtAdapter* bpBinder, bt_address_t* addr);
bt_status_t BpBtAdapter_pairRequestReply(BpBtAdapter* bpBinder, bt_address_t* addr, bool accept);
bt_status_t BpBtAdapter_setPairingConfirmation(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport, bool accept);
bt_status_t BpBtAdapter_setPinCode(BpBtAdapter* bpBinder, bt_address_t* addr, bool accept,
    char* pincode, int len);
bt_status_t BpBtAdapter_setPassKey(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport, bool accept, uint32_t passkey);
#endif /* __ADAPTER_PROXY_H__ */