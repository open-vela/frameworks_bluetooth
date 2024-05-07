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

#ifndef __HFP_HF_CALLBACKS_STUB_H__
#define __HFP_HF_CALLBACKS_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_hfp_hf.h"
#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    const hfp_hf_callbacks_t* callbacks;
    void* cookie;
} IBtHfpHfCallbacks;

typedef enum {
    ICBKS_HFP_HF_CONNECTION_STATE = FIRST_CALL_TRANSACTION,
    ICBKS_HFP_HF_AUDIO_STATE,
    ICBKS_HFP_HF_VR_STATE,
    ICBKS_HFP_HF_CALL_STATE_CHANGE,
    ICBKS_HFP_HF_CMD_COMPLETE,
    ICBKS_HFP_HF_RING_INDICATION,
    ICBKS_HFP_HF_ROAMING_CHANGED,
    ICBKS_HFP_HF_NETWOEK_STATE_CHANGED,
    ICBKS_HFP_HF_SIGNAL_STRENGTH_CHANGED,
    ICBKS_HFP_HF_OPERATOR_CHANGED,
} IBtHfpHfCallbacks_Call;

AIBinder* BtHfpHfCallbacks_getBinder(IBtHfpHfCallbacks* adapter);
binder_status_t BtHfpHfCallbacks_associateClass(AIBinder* binder);
IBtHfpHfCallbacks* BtHfpHfCallbacks_new(const hfp_hf_callbacks_t* callbacks);
void BtHfpHfCallbacks_delete(IBtHfpHfCallbacks* cbks);

#ifdef __cplusplus
}
#endif
#endif /* __HFP_HF_CALLBACKS_STUB_H__ */