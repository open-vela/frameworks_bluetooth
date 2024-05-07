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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "adapter_callbacks_stub.h"
#include "adapter_internel.h"
#include "adapter_proxy.h"
#include "adapter_stub.h"
#include "binder_utils.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_ADAPTER_CALLBACK_DESC "BluetoothAdapterCallback"
static const AIBinder_Class* kIBtAdapterCallbacks_Class = NULL;

static void* IBtAdapterCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtAdapterCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtAdapterCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtAdapterCallbacks* cbks = AIBinder_getUserData(binder);

    switch (code) {
    case ICBKS_ADAPTER_STATE_CHANGED: {
        uint32_t state;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_adapter_state_changed(cbks, state);
        break;
    }
    case ICBKS_DISCOVERY_STATE_CHANGED: {
        uint32_t state;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_discovery_state_changed(cbks, state);
        break;
    }
    case ICBKS_DISCOVERY_RESULT: {
        bt_discovery_result_t remote = { 0 };
        char* remoteName = NULL;

        stat = AParcel_readAddress(in, &remote.addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &remoteName, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        if (remoteName)
            snprintf(remote.name, sizeof(remote.name), "%s", remoteName);
        free(remoteName);

        stat = AParcel_readUint32(in, &remote.cod);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByte(in, &remote.rssi);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_discovery_result(cbks, &remote);
        break;
    }
    case ICBKS_SCAN_MODE_CHANGED: {
        bt_scan_mode_t mode;

        stat = AParcel_readUint32(in, &mode);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_scan_mode_changed(cbks, mode);
        break;
    }
    case ICBKS_DEVICE_NAME_CHANGED: {
        char* deviceName = NULL;

        stat = AParcel_readString(in, &deviceName, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_device_name_changed(cbks, deviceName);
        if (deviceName)
            free(deviceName);
        break;
    }
    case ICBKS_PAIR_REQUEST: {
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_pair_request(cbks, &addr);
        break;
    }
    case ICBKS_PAIR_DISPLAY: {
        bt_address_t addr;
        bt_transport_t transport;
        bt_pair_type_t type;
        uint32_t pass_key;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &transport);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &pass_key);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_pair_display(cbks, &addr, transport, type, pass_key);
        break;
    }
    case ICBKS_CONNECTION_STATE_CHANGED: {
        bt_address_t addr;
        bt_transport_t transport;
        connection_state_t state;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &transport);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_connection_state_changed(cbks, &addr, transport, state);
        break;
    }
    case ICBKS_BOND_STATE_CHANGED: {
        bt_address_t addr;
        bt_transport_t transport;
        bond_state_t state;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &transport);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;
        cbks->callbacks->on_bond_state_changed(cbks, &addr, transport, state);
        break;
    }
    case ICBKS_REMOTE_NAME_CHANGED: {
        bt_address_t addr;
        char* remoteName = NULL;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &remoteName, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_remote_name_changed(cbks, &addr, remoteName);
        free(remoteName);
        break;
    }
    case ICBKS_REMOTE_ALIAS_CHANGED: {
        bt_address_t addr;
        char* alias = NULL;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &alias, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_remote_alias_changed(cbks, &addr, alias);
        free(alias);
        break;
    }
    case ICBKS_REMOTE_COD_CHANGED: {
        bt_address_t addr;
        uint32_t cod;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &cod);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_remote_cod_changed(cbks, &addr, cod);
        break;
    }
    case ICBKS_REMOTE_UUIDS_CHANGED: {
        bt_address_t addr;
        bt_uuid_t* uuids = NULL;
        int32_t uuidSize = 0;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUuidArray(in, (bt_uuid_t*)&uuids, &uuidSize);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_remote_uuids_changed(cbks, &addr, uuids, (uint16_t)uuidSize);
        free(uuids);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtAdapterCallbacks_getBinder(IBtAdapterCallbacks* cbks)
{
    AIBinder* binder = NULL;

    if (cbks->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(cbks->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(cbks->clazz, (void*)cbks);
        if (cbks->WeakBinder != NULL) {
            AIBinder_Weak_delete(cbks->WeakBinder);
        }

        cbks->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtAdapterCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtAdapterCallbacks_Class) {
        kIBtAdapterCallbacks_Class = AIBinder_Class_define(BT_ADAPTER_CALLBACK_DESC, IBtAdapterCallbacks_Class_onCreate,
            IBtAdapterCallbacks_Class_onDestroy, IBtAdapterCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtAdapterCallbacks_Class);
}

IBtAdapterCallbacks* BtAdapterCallbacks_new(const adapter_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtAdapterCallbacks* cbks = malloc(sizeof(IBtAdapterCallbacks));

    clazz = AIBinder_Class_define(BT_ADAPTER_CALLBACK_DESC, IBtAdapterCallbacks_Class_onCreate,
        IBtAdapterCallbacks_Class_onDestroy, IBtAdapterCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtAdapterCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtAdapterCallbacks_delete(IBtAdapterCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
