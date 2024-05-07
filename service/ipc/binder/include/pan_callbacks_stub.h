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

#ifndef __PAN_CALLBACKS_STUB_H__
#define __PAN_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_pan.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const pan_callbacks_t* callbacks;
    void* cookie;
} IBtPanCallbacks;

typedef enum {
    ICBKS_PAN_CONNECTION_STATE = FIRST_CALL_TRANSACTION,
    ICBKS_NETIF_STATE,
} IBtPanCallbacks_Call;

AIBinder* BtPanCallbacks_getBinder(IBtPanCallbacks* adapter);
binder_status_t BtPanCallbacks_associateClass(AIBinder* binder);
IBtPanCallbacks* BtPanCallbacks_new(const pan_callbacks_t* callbacks);
void BtPanCallbacks_delete(IBtPanCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __PAN_CALLBACKS_STUB_H__ */