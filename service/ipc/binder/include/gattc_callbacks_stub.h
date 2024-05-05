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

#ifndef __BT_GATTC_CALLBACKS_STUB_H__
#define __BT_GATTC_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#include "bt_list.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_gattc.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const gattc_callbacks_t* callbacks;
    void* proxy;
    void* cookie;
} IBtGattClientCallbacks;

typedef enum {
    ICBKS_GATT_CLIENT_CONNECTED = FIRST_CALL_TRANSACTION,
    ICBKS_GATT_CLIENT_DISCONNECTED,
    ICBKS_GATT_CLIENT_DISCOVERED,
    ICBKS_GATT_CLIENT_MTU_EXCHANGE,
    ICBKS_GATT_CLIENT_READ,
    ICBKS_GATT_CLIENT_WRITTEN,
    ICBKS_GATT_CLIENT_NOTIFIED
} IBtGattClientCallbacks_Call;

AIBinder* BtGattClientCallbacks_getBinder(IBtGattClientCallbacks* adapter);
binder_status_t BtGattClientCallbacks_associateClass(AIBinder* binder);
IBtGattClientCallbacks* BtGattClientCallbacks_new(const gattc_callbacks_t* callbacks);
void BtGattClientCallbacks_delete(IBtGattClientCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __BT_GATTC_CALLBACKS_STUB_H__ */
