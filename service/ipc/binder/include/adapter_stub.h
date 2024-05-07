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

#ifndef __ADAPTER_STUB_H__
#define __ADAPTER_STUB_H__

#include <stdbool.h>
#include <stdint.h>
#include <uchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <android/binder_manager.h>
// #include <android/binder_auto_utils.h>

typedef struct {
    AIBinder_Class* clazz;
    AIBinder_Weak* WeakBinder;
    void* usr_data;
} IBtAdapter;

typedef struct {
    AIBinder_Class* clazz;
    AIBinder* binder;
} BpBtAdapter;

#define ADAPTER_BINDER_INSTANCE "Vela.Bluetooth.Adapter"

typedef enum {
    /* local adapter */
    IBTADAPTER_REGISTER_CALLBACK = FIRST_CALL_TRANSACTION,
    IBTADAPTER_UNREGISTER_CALLBACK,
    IBTADAPTER_ENABLE,
    IBTADAPTER_DISABLE,
    IBTADAPTER_ENABLE_LE,
    IBTADAPTER_DISABLE_LE,
    IBTADAPTER_GET_STATE,
    IBTADAPTER_IS_LE_ENABLED,
    IBTADAPTER_GET_TYPE,
    IBTADAPTER_SET_DISCOVERY_FILTER,
    IBTADAPTER_START_DISCOVERY,
    IBTADAPTER_CANCEL_DISCOVERY,
    IBTADAPTER_IS_DISCOVERING,
    IBTADAPTER_GET_ADDR,
    IBTADAPTER_SET_NAME,
    IBTADAPTER_GET_NAME,
    IBTADAPTER_GET_UUIDS,
    IBTADAPTER_SET_SCAN_MODE,
    IBTADAPTER_GET_SCAN_MODE,
    IBTADAPTER_SET_DEVICE_CLASS,
    IBTADAPTER_GET_DEVICE_CLASS,
    IBTADAPTER_SET_IO_CAP,
    IBTADAPTER_GET_IO_CAP,
    IBTADAPTER_GET_LE_IO_CAP,
    IBTADAPTER_SET_LE_IO_CAP,
    IBTADAPTER_GET_LE_ADDR,
    IBTADAPTER_SET_LE_ADDR,
    IBTADAPTER_SET_LE_ID,
    IBTADAPTER_SET_LE_APPEARANCE,
    IBTADAPTER_GET_LE_APPEARANCE,
    IBTADAPTER_ENABLE_KEY_DERIVATION,
    IBTADAPTER_GET_BONDED_DEVICES,
    IBTADAPTER_GET_CONNECTED_DEVICES,
    /* LE ADV */
    IBTADAPTER_START_ADVERTISING,
    IBTADAPTER_STOP_ADVERTISING,
    IBTADAPTER_STOP_ADVERTISING_ID,
    /* LE SCAN */
    IBTADAPTER_START_SCAN,
    IBTADAPTER_START_SCAN_SETTINGS,
    IBTADAPTER_STOP_SCAN,
    /* remote adapter */
    IREMOTE_GET_ADDR_TYPE,
    IREMOTE_GET_DEVICE_TYPE,
    IREMOTE_GET_NAME,
    IREMOTE_GET_DEVICE_CLASS,
    IREMOTE_GET_UUIDS,
    IREMOTE_GET_APPEARANCE,
    IREMOTE_GET_RSSI,
    IREMOTE_GET_ALIAS,
    IREMOTE_SET_ALIAS,
    IREMOTE_IS_CONNECTED,
    IREMOTE_IS_ENCRYPTED,
    IREMOTE_IS_BOND_INIT_LOCAL,
    IREMOTE_GET_BOND_STATE,
    IREMOTE_IS_BONDED,
    IREMOTE_CREATE_BOND,
    IREMOTE_REMOVE_BOND,
    IREMOTE_CANCEL_BOND,
    IREMOTE_PAIR_REQUEST_REPLY,
    IREMOTE_SET_PAIRING_CONFIRM,
    IREMOTE_SET_PIN_CODE,
    IREMOTE_SET_PASSKEY,
    IREMOTE_CONNECT,
    IREMOTE_DISCONNECT,
    IREMOTE_CONNECT_LE,
    IREMOTE_DISCONNECT_LE,
    IREMOTE_SET_LE_PHY,
} IBtAdapter_Call;

binder_status_t BtAdapter_addService(IBtAdapter* adapter, const char* instance);
#ifdef __cplusplus
}
#endif

#endif /* __ADAPTER_STUB_H__ */