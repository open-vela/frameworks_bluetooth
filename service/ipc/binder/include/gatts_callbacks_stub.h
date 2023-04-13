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

#ifndef __BLE_GATTS_CALLBACKS_STUB_H__
#define __BLE_GATTS_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_gatts.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class *clazz;
    AIBinder_Weak *WeakBinder;
    const gatts_callbacks_t *callbacks;
    const gatt_srv_db_t *srv_db;
    void *proxy;
    void *cookie;
} IBleGattServerCallbacks;

typedef enum {
    ICBKS_GATT_SERVER_CONNECTED = FIRST_CALL_TRANSACTION,
    ICBKS_GATT_SERVER_DISCONNECTED,
    ICBKS_GATT_SERVER_STARTED,
    ICBKS_GATT_SERVER_STOPPED,
    ICBKS_GATT_SERVER_MTU_CHANGED,
    ICBKS_GATT_SERVER_READ,
    ICBKS_GATT_SERVER_WRITE,
    ICBKS_GATT_SERVER_COMPLETE
} IBleGattServerCallbacks_Call;

AIBinder *BleGattServerCallbacks_getBinder(IBleGattServerCallbacks *adapter);
binder_status_t BleGattServerCallbacks_associateClass(AIBinder *binder);
IBleGattServerCallbacks *BleGattServerCallbacks_new(const gatts_callbacks_t *callbacks);
void BleGattServerCallbacks_delete(IBleGattServerCallbacks *cbks);

#ifdef __cplusplus
}
#endif
#endif /* __BLE_GATTS_CALLBACKS_STUB_H__ */