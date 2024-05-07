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

#ifndef __BT_GATTC_PROXY_H__
#define __BT_GATTC_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "bt_gattc.h"
#include "gattc_stub.h"

#ifdef __cplusplus
extern "C" {
#endif

BpBtGattClient* BpBtGattClient_new(const char* instance);
void BpBtGattClient_delete(BpBtGattClient* bpBinder);
void* BpBtGattClient_createConnect(BpBtGattClient* bpBinder, AIBinder* cbksBinder);
bt_status_t BpBtGattClient_deleteConnect(BpBtGattClient* bpBinder, void* handle);
bt_status_t BpBtGattClient_connect(BpBtGattClient* bpBinder, void* handle, bt_address_t* addr, ble_addr_type_t addr_type);
bt_status_t BpBtGattClient_disconnect(BpBtGattClient* bpBinder, void* handle);
bt_status_t BpBtGattClient_discoverService(BpBtGattClient* bpBinder, void* handle, bt_uuid_t* filter_uuid);
bt_status_t BpBtGattClient_getAttributeByHandle(BpBtGattClient* bpBinder, void* handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc);
bt_status_t BpBtGattClient_getAttributeByUUID(BpBtGattClient* bpBinder, void* handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc);
bt_status_t BpBtGattClient_read(BpBtGattClient* bpBinder, void* handle, uint16_t attr_handle);
bt_status_t BpBtGattClient_write(BpBtGattClient* bpBinder, void* handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
bt_status_t BpBtGattClient_writeWithoutResponse(BpBtGattClient* bpBinder, void* handle, uint16_t attr_handle, uint8_t* value, uint16_t length);
bt_status_t BpBtGattClient_subscribe(BpBtGattClient* bpBinder, void* handle, uint16_t value_handle, uint16_t cccd_handle);
bt_status_t BpBtGattClient_unsubscribe(BpBtGattClient* bpBinder, void* handle, uint16_t value_handle, uint16_t cccd_handle);
bt_status_t BpBtGattClient_exchangeMtu(BpBtGattClient* bpBinder, void* handle, uint32_t mtu);
bt_status_t BpBtGattClient_updateConnectionParameter(BpBtGattClient* bpBinder, void* handle, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
#ifdef __cplusplus
}
#endif
#endif /* __BT_GATTC_PROXY_H__ */