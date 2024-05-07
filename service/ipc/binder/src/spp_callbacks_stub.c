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
#include "parcel.h"
#include "spp_callbacks_stub.h"
#include "spp_proxy.h"
#include "spp_stub.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_SPP_CALLBACK_DESC "BluetoothSppCallback"

static const AIBinder_Class* kIBtSppCallbacks_Class = NULL;

static void* IBtSppCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtSppCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtSppCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtSppCallbacks* cbks = AIBinder_getUserData(binder);
    bt_address_t addr;
    uint32_t scn, port;

    switch (code) {
    case ICBKS_PTY_OPEN: {
        char* name = NULL;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &scn);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &port);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &name, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->pty_open_cb(cbks, &addr, (uint16_t)scn, (uint16_t)port, name);
        free(name);

        break;
    }
    case ICBKS_SPP_CONNECTION_STATE: {

        uint32_t state;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &scn);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &port);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->connection_state_cb(cbks, &addr, (uint16_t)scn, (uint16_t)port, state);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtSppCallbacks_getBinder(IBtSppCallbacks* cbks)
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

binder_status_t BtSppCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtSppCallbacks_Class) {
        kIBtSppCallbacks_Class = AIBinder_Class_define(BT_SPP_CALLBACK_DESC, IBtSppCallbacks_Class_onCreate,
            IBtSppCallbacks_Class_onDestroy, IBtSppCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtSppCallbacks_Class);
}

IBtSppCallbacks* BtSppCallbacks_new(const spp_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtSppCallbacks* cbks = malloc(sizeof(IBtSppCallbacks));

    clazz = AIBinder_Class_define(BT_SPP_CALLBACK_DESC, IBtSppCallbacks_Class_onCreate,
        IBtSppCallbacks_Class_onDestroy, IBtSppCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtSppCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtSppCallbacks_delete(IBtSppCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
