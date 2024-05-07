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

#include "binder_utils.h"
#include "pan_callbacks_stub.h"
#include "pan_proxy.h"
#include "pan_stub.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_PAN_CALLBACK_DESC "BluetoothPanCallback"

static const AIBinder_Class* kIBtPanCallbacks_Class = NULL;

static void* IBtPanCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtPanCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtPanCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtPanCallbacks* cbks = AIBinder_getUserData(binder);

    switch (code) {
    case ICBKS_PAN_CONNECTION_STATE: {
        uint32_t state;
        bt_address_t addr;
        uint32_t localRole, remoteRole;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &localRole);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &remoteRole);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->connection_state_cb(cbks, state, &addr, (uint8_t)localRole, (uint8_t)remoteRole);
        break;
    }
    case ICBKS_NETIF_STATE: {
        uint32_t state;
        char* ifName = NULL;
        uint32_t localRole;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &localRole);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &ifName, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->netif_state_cb(cbks, state, localRole, ifName);
        free(ifName);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtPanCallbacks_getBinder(IBtPanCallbacks* cbks)
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

binder_status_t BtPanCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtPanCallbacks_Class) {
        kIBtPanCallbacks_Class = AIBinder_Class_define(BT_PAN_CALLBACK_DESC, IBtPanCallbacks_Class_onCreate,
            IBtPanCallbacks_Class_onDestroy, IBtPanCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtPanCallbacks_Class);
}

IBtPanCallbacks* BtPanCallbacks_new(const pan_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtPanCallbacks* cbks = malloc(sizeof(IBtPanCallbacks));

    clazz = AIBinder_Class_define(BT_PAN_CALLBACK_DESC, IBtPanCallbacks_Class_onCreate,
        IBtPanCallbacks_Class_onDestroy, IBtPanCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtPanCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtPanCallbacks_delete(IBtPanCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
