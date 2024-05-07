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

#ifndef __BT_PAN_STUB_H__
#define __BT_PAN_STUB_H__

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
} IBtPan;

typedef struct {
    AIBinder_Class* clazz;
    AIBinder* binder;
} BpBtPan;

typedef enum {
    IPAN_REGISTER_CALLBACK = FIRST_CALL_TRANSACTION,
    IPAN_UNREGISTER_CALLBACK,
    IPAN_CONNECT,
    IPAN_DISCONNECT
} IBtPan_Call;

#define PAN_BINDER_INSTANCE "Vela.Bluetooth.Pan"

binder_status_t BtPan_addService(IBtPan* pan, const char* instance);
AIBinder* BtPan_getService(BpBtPan** bpPan, const char* instance);

#ifdef __cplusplus
}
#endif
#endif /* __BT_PAN_STUB_H__ */