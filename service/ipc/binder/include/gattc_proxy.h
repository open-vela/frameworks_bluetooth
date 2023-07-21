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

#ifndef __BLE_GATTC_PROXY_H__
#define __BLE_GATTC_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "ble_gattc.h"
#include "gattc_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

BpBleGattClient *BpBleGattClient_new(const char *instance);
void BpBleGattClient_delete(BpBleGattClient *bpBinder);
void *BpBleGattClient_createConnect(BpBleGattClient *bpBinder, AIBinder *cbksBinder);
bt_status_t BpBleGattClient_deleteConnect(BpBleGattClient *bpBinder, void *handle);
bt_status_t BpBleGattClient_connect(BpBleGattClient *bpBinder, void *handle, bt_address_t *addr, ble_addr_type_t addr_type);
bt_status_t BpBleGattClient_disconnect(BpBleGattClient *bpBinder, void *handle);
bt_status_t BpBleGattClient_discoverService(BpBleGattClient *bpBinder, void *handle, bt_uuid_t *filter_uuid);
bt_status_t BpBleGattClient_getAttributeByHandle(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle, gatt_attr_desc_t *attr_desc);
bt_status_t BpBleGattClient_getAttributeByUUID(BpBleGattClient *bpBinder, void *handle, bt_uuid_t *attr_uuid, gatt_attr_desc_t *attr_desc);
bt_status_t BpBleGattClient_read(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle);
bt_status_t BpBleGattClient_write(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle, uint8_t *value, uint16_t length, uint16_t offset);
bt_status_t BpBleGattClient_writeWithoutResponse(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle, uint8_t *value, uint16_t length);
bt_status_t BpBleGattClient_subscribe(BpBleGattClient *bpBinder, void *handle, uint16_t value_handle, uint16_t cccd_handle);
bt_status_t BpBleGattClient_unsubscribe(BpBleGattClient *bpBinder, void *handle, uint16_t value_handle, uint16_t cccd_handle);
bt_status_t BpBleGattClient_exchangeMtu(BpBleGattClient *bpBinder, void *handle, uint32_t mtu);
#ifdef __cplusplus
}
#endif
#endif /* __BLE_GATTC_PROXY_H__ */