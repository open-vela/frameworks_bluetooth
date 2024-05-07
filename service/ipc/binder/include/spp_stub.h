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

#ifndef __BT_SPP_STUB_H__
#define __BT_SPP_STUB_H__

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
} IBtSpp;

typedef struct {
    AIBinder_Class* clazz;
    AIBinder* binder;
} BpBtSpp;

typedef enum {
    ISPP_REGISTER_APP = FIRST_CALL_TRANSACTION,
    ISPP_UNREGISTER_APP,
    ISPP_SERVER_START,
    ISPP_SERVER_STOP,
    ISPP_CONNECT,
    ISPP_DISCONNECT
} IBtSpp_Call;

#define SPP_BINDER_INSTANCE "Vela.Bluetooth.Spp"

binder_status_t BtSpp_addService(IBtSpp* spp, const char* instance);
AIBinder* BtSpp_getService(BpBtSpp** bpSpp, const char* instance);

#ifdef __cplusplus
}
#endif
#endif /* __BT_SPP_STUB_H__ */