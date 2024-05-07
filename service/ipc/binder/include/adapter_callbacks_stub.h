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

#ifndef __ADAPTER_CALLBACKS_STUB_H__
#define __ADAPTER_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_adapter.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const adapter_callbacks_t* callbacks;
    void* cookie;
} IBtAdapterCallbacks;

#if 0
typedef struct {
    AIBinder* Binder;
} BpBtAdapterCallbacks;
#endif

typedef enum {
    /* local adapter */
    ICBKS_ADAPTER_STATE_CHANGED = FIRST_CALL_TRANSACTION,
    ICBKS_DISCOVERY_STATE_CHANGED,
    ICBKS_DISCOVERY_RESULT,
    ICBKS_SCAN_MODE_CHANGED,
    ICBKS_DEVICE_NAME_CHANGED,
    ICBKS_PAIR_REQUEST,
    ICBKS_PAIR_DISPLAY,
    ICBKS_CONNECTION_STATE_CHANGED,
    ICBKS_BOND_STATE_CHANGED,
    ICBKS_REMOTE_NAME_CHANGED,
    ICBKS_REMOTE_ALIAS_CHANGED,
    ICBKS_REMOTE_COD_CHANGED,
    ICBKS_REMOTE_UUIDS_CHANGED,
} IBtAdapterCallbacks_Call;

AIBinder* BtAdapterCallbacks_getBinder(IBtAdapterCallbacks* adapter);
binder_status_t BtAdapterCallbacks_associateClass(AIBinder* binder);
IBtAdapterCallbacks* BtAdapterCallbacks_new(const adapter_callbacks_t* callbacks);
void BtAdapterCallbacks_delete(IBtAdapterCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __ADAPTER_CALLBACKS_STUB_H__ */