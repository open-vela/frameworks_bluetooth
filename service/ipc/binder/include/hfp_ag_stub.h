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

#ifndef __BT_HFP_AG_STUB_H__
#define __BT_HFP_AG_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <android/binder_manager.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    void* usr_data;
} IBtHfpAg;

typedef struct {
    AIBinder_Class* clazz;
    AIBinder* binder;
} BpBtHfpAg;

typedef enum {
    IHFP_AG_REGISTER_CALLBACK = FIRST_CALL_TRANSACTION,
    IHFP_AG_UNREGISTER_CALLBACK,
    IHFP_AG_IS_CONNECTED,
    IHFP_AG_IS_AUDIO_CONNECTED,
    IHFP_AG_GET_CONNECTION_STATE,
    IHFP_AG_CONNECT,
    IHFP_AG_DISCONNECT,
    IHFP_AG_AUDIO_CONNECT,
    IHFP_AG_AUDIO_DISCONNECT,
    IHFP_AG_START_VOICE_RECOGNITION,
    IHFP_AG_STOP_VOICE_RECOGNITION,
} IBtHfpAg_Call;

#define HFP_AG_BINDER_INSTANCE "Vela.Bluetooth.Hfp.AG"

binder_status_t BtHfpAg_addService(IBtHfpAg* hfpAg, const char* instance);
AIBinder* BtHfpAg_getService(BpBtHfpAg** bpHfpAg, const char* instance);

#ifdef __cplusplus
}
#endif
#endif /* __BT_HFP_AG_STUB_H__ */