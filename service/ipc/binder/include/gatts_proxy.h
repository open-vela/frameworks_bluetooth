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

#ifndef __BLE_GATTS_PROXY_H__
#define __BLE_GATTS_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "ble_gatts.h"
#include "gatts_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

BpBleGattServer *BpBleGattServer_new(const char *instance);
void BpBleGattServer_delete(BpBleGattServer *bpBinder);
void *BpBleGattServer_registerService(BpBleGattServer *bpBinder, AIBinder *cbksBinder);
bt_status_t BpBleGattServer_unregisterService(BpBleGattServer *bpBinder, void *handle);
bt_status_t BpBleGattServer_connect(BpBleGattServer *bpBinder, void *handle, bt_address_t *addr, ble_addr_type_t addr_type);
bt_status_t BpBleGattServer_disconnect(BpBleGattServer *bpBinder, void *handle);
bt_status_t BpBleGattServer_createServiceTable(BpBleGattServer *bpBinder, void *handle, gatt_srv_db_t *srv_db);
bt_status_t BpBleGattServer_start(BpBleGattServer *bpBinder, void *handle);
bt_status_t BpBleGattServer_stop(BpBleGattServer *bpBinder, void *handle);
bt_status_t BpBleGattServer_response(BpBleGattServer *bpBinder, void *handle, uint32_t req_handle, uint8_t *value, uint16_t length);
bt_status_t BpBleGattServer_notify(BpBleGattServer *bpBinder, void *handle, uint16_t attr_handle, uint8_t *value, uint16_t length);
bt_status_t BpBleGattServer_indicate(BpBleGattServer *bpBinder, void *handle, uint16_t attr_handle, uint8_t *value, uint16_t length);
#ifdef __cplusplus
}
#endif
#endif /* __BLE_GATTS_PROXY_H__ */