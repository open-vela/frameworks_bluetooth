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

#ifndef __BLE_GATTS_CALLBACKS_PROXY_H__
#define __BLE_GATTS_CALLBACKS_PROXY_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "ble_gatts.h"

#include <android/binder_manager.h>

const gatts_callbacks_t *BpBleGattServerCallbacks_getStatic(void);
uint16_t BpBleGattServerCallbacks_onRead(void *handle, uint16_t attr_handle, uint32_t req_handle);
uint16_t BpBleGattServerCallbacks_onWrite(void *handle, uint16_t attr_handle, const uint8_t *value, uint16_t length, uint16_t offset);
void BpBleGattServerCallbacks_onComplete(void *handle, gatt_status_t status, uint16_t attr_handle);

#ifdef __cplusplus
}
#endif
#endif /* __BLE_GATTS_CALLBACKS_PROXY_H__ */