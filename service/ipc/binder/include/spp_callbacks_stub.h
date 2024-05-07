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

#ifndef __SPP_CALLBACKS_STUB_H__
#define __SPP_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_spp.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const spp_callbacks_t* callbacks;
    void* cookie;
} IBtSppCallbacks;

typedef enum {
    ICBKS_SPP_CONNECTION_STATE = FIRST_CALL_TRANSACTION,
    ICBKS_PTY_OPEN,
} IBtSppCallbacks_Call;

AIBinder* BtSppCallbacks_getBinder(IBtSppCallbacks* adapter);
binder_status_t BtSppCallbacks_associateClass(AIBinder* binder);
IBtSppCallbacks* BtSppCallbacks_new(const spp_callbacks_t* callbacks);
void BtSppCallbacks_delete(IBtSppCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __SPP_CALLBACKS_STUB_H__ */