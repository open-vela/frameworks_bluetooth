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

#ifndef __BLE_GATTC_CALLBACKS_STUB_H__
#define __BLE_GATTC_CALLBACKS_STUB_H__

#include <nuttx/list.h>
#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ble_gattc.h"
#include <android/binder_manager.h>

typedef struct {
    struct list_node node;
    uint16_t attr_handle;
    gattc_notify_cb_t on_notify;
} notify_callback_t;

typedef struct {
    AIBinder_Class *clazz;
    AIBinder_Weak *WeakBinder;
    const gattc_callbacks_t *callbacks;
    struct list_node notify_list;
    void *proxy;
    void *cookie;
} IBleGattClientCallbacks;

typedef enum {
    ICBKS_GATT_CLIENT_CONNECTED = FIRST_CALL_TRANSACTION,
    ICBKS_GATT_CLIENT_DISCONNECTED,
    ICBKS_GATT_CLIENT_DISCOVER,
    ICBKS_GATT_CLIENT_MTU_EXCHANGE,
    ICBKS_GATT_CLIENT_READ,
    ICBKS_GATT_CLIENT_WRITE,
    ICBKS_GATT_CLIENT_NOTIFY
} IBleGattClientCallbacks_Call;

AIBinder *BleGattClientCallbacks_getBinder(IBleGattClientCallbacks *adapter);
binder_status_t BleGattClientCallbacks_associateClass(AIBinder *binder);
IBleGattClientCallbacks *BleGattClientCallbacks_new(const gattc_callbacks_t *callbacks);
void BleGattClientCallbacks_delete(IBleGattClientCallbacks *cbks);
void BleGattClientCallbacks_registerNotify(IBleGattClientCallbacks *cbks, uint16_t value_handle, gattc_notify_cb_t notify_cb);
void BleGattClientCallbacks_unregisterNotify(IBleGattClientCallbacks *cbks, uint16_t value_handle);

#ifdef __cplusplus
}
#endif
#endif /* __BLE_GATTC_CALLBACKS_STUB_H__ */