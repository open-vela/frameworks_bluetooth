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

#ifndef __BT_GATTC_STUB_H__
#define __BT_GATTC_STUB_H__

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
} IBtGattClient;

typedef struct {
    AIBinder_Class* clazz;
    AIBinder* binder;
} BpBtGattClient;

typedef enum {
    IGATT_CLIENT_CREATE_CONNECT = FIRST_CALL_TRANSACTION,
    IGATT_CLIENT_DELETE_CONNECT,
    IGATT_CLIENT_CONNECT,
    IGATT_CLIENT_DISCONNECT,
    IGATT_CLIENT_DISCOVER_SERVICE,
    IGATT_CLIENT_GET_ATTRIBUTE_BY_HANDLE,
    IGATT_CLIENT_GET_ATTRIBUTE_BY_UUID,
    IGATT_CLIENT_READ,
    IGATT_CLIENT_WRITE,
    IGATT_CLIENT_WRITE_WITHOUT_RESPONSE,
    IGATT_CLIENT_SUBSCRIBE,
    IGATT_CLIENT_UNSUBSCRIBE,
    IGATT_CLIENT_EXCHANGE_MTU,
    IGATT_CLIENT_UPDATE_CONNECTION_PARAM,
} IBtGattClient_Call;

#define GATT_CLIENT_BINDER_INSTANCE "Vela.Bluetooth.Gatt.Client"

binder_status_t BtGattClient_addService(IBtGattClient* iGattc, const char* instance);
AIBinder* BtGattClient_getService(BpBtGattClient** bpGattc, const char* instance);

#ifdef __cplusplus
}
#endif
#endif /* __BT_GATTC_STUB_H__ */