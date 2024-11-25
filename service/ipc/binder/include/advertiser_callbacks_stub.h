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

#ifndef __ADVERTISER_CALLBACKS_STUB_H__
#define __ADVERTISER_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_le_advertiser.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const advertiser_callback_t* callbacks;
    void* cookie;
} IBtAdvertiserCallbacks;

typedef enum {
    ICBKS_ON_ADVERTISING_START = FIRST_CALL_TRANSACTION,
    ICBKS_ON_ADVERTISING_STOPPED,
} IBtAdvertiserCallbacks_Call;

AIBinder* BtAdvertiserCallbacks_getBinder(IBtAdvertiserCallbacks* adver);
binder_status_t BtAdvertiserCallbacks_associateClass(AIBinder* binder);
IBtAdvertiserCallbacks* BtAdvertiserCallbacks_new(const advertiser_callback_t* callbacks);
void BtAdvertiserCallbacks_delete(IBtAdvertiserCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __ADVERTISER_CALLBACKS_STUB_H__ */