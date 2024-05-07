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

#ifndef __BLUETOOTH_PARCEL_H__
#define __BLUETOOTH_PARCEL_H__

#include "binder_utils.h"

#include "bluetooth.h"
#include "bt_gatts.h"
#include "bt_hfp_hf.h"
#include "bt_le_advertiser.h"
#include "bt_le_scan.h"
#include "bt_uuid.h"

binder_status_t AParcel_writeAddress(AParcel* parcel, bt_address_t* addr);
binder_status_t AParcel_readAddress(const AParcel* parcel, bt_address_t* addr);
binder_status_t AParcel_writeAddressArray(AParcel* parcel, bt_address_t* addr, int32_t length);
binder_status_t AParcel_readAddressArray(const AParcel* parcel, bt_address_t** addr, int32_t* length, bt_allocator_t allocator);

binder_status_t AParcel_writeUuid(AParcel* parcel, bt_uuid_t* uuid);
binder_status_t AParcel_readUuid(const AParcel* parcel, bt_uuid_t* uuid);
binder_status_t AParcel_writeUuidArray(AParcel* parcel, bt_uuid_t* uuid, int32_t length);
binder_status_t AParcel_readUuidArray(const AParcel* parcel, bt_uuid_t* uuid, int32_t* length);

binder_status_t AParcel_writeBleConnectParam(AParcel* parcel, ble_connect_params_t* param);
binder_status_t AParcel_readBleConnectParam(const AParcel* parcel, ble_connect_params_t* param);

binder_status_t AParcel_writeCall(AParcel* parcel, hfp_current_call_t* call);
binder_status_t AParcel_readCall(const AParcel* parcel, hfp_current_call_t* call);

binder_status_t AParcel_writeCallArray(AParcel* parcel, hfp_current_call_t* calls, int32_t length);
binder_status_t AParcel_readCallArray(const AParcel* parcel, hfp_current_call_t** calls, int32_t* length, bt_allocator_t allocator);

binder_status_t AParcel_writeBleAdvParam(AParcel* parcel, ble_adv_params_t* param);
binder_status_t AParcel_readBleAdvParam(const AParcel* parcel, ble_adv_params_t* param);

binder_status_t AParcel_writeBleScanResult(AParcel* parcel, ble_scan_result_t* result);
binder_status_t AParcel_readBleScanResult(const AParcel* parcel, ble_scan_result_t** outResult);

binder_status_t AParcel_writeServiceTable(AParcel* parcel, gatt_attr_db_t* attribute, int32_t length);
binder_status_t AParcel_readServiceTable(const AParcel* parcel, gatt_attr_db_t** attribute, int32_t* length);

#endif