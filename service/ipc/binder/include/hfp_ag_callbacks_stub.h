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

#ifndef __HFP_AG_CALLBACKS_STUB_H__
#define __HFP_AG_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_hfp_ag.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const hfp_ag_callbacks_t* callbacks;
    void* cookie;
} IBtHfpAgCallbacks;

typedef enum {
    ICBKS_HFP_AG_CONNECTION_STATE = FIRST_CALL_TRANSACTION,
    ICBKS_HFP_AG_AUDIO_STATE,
    ICBKS_HFP_AG_VR_STATE,
    ICBKS_HFP_AG_BATTERY_UPDATE
} IBtHfpAgCallbacks_Call;

AIBinder* BtHfpAgCallbacks_getBinder(IBtHfpAgCallbacks* adapter);
binder_status_t BtHfpAgCallbacks_associateClass(AIBinder* binder);
IBtHfpAgCallbacks* BtHfpAgCallbacks_new(const hfp_ag_callbacks_t* callbacks);
void BtHfpAgCallbacks_delete(IBtHfpAgCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __HFP_AG_CALLBACKS_STUB_H__ */