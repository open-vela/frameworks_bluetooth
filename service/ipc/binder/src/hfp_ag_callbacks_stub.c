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
#include "hfp_ag_callbacks_stub.h"
#include "hfp_ag_proxy.h"
#include "hfp_ag_stub.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_HFP_AG_CALLBACK_DESC "BluetoothHfpAgCallback"

static const AIBinder_Class* kIBtHfpAgCallbacks_Class = NULL;

static void* IBtHfpAgCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtHfpAgCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtHfpAgCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtHfpAgCallbacks* cbks = AIBinder_getUserData(binder);

    switch (code) {
    case ICBKS_HFP_AG_CONNECTION_STATE: {
        uint32_t state;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->connection_state_cb(cbks, &addr, state);
        break;
    }
    case ICBKS_HFP_AG_AUDIO_STATE: {
        uint32_t state;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->audio_state_cb(cbks, &addr, state);
        break;
    }
    case ICBKS_HFP_AG_VR_STATE: {
        bool start;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &start);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->vr_cmd_cb(cbks, &addr, start);
        break;
    }
    case ICBKS_HFP_AG_BATTERY_UPDATE: {
        uint32_t value;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &value);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->hf_battery_update_cb(cbks, &addr, (uint8_t)value);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtHfpAgCallbacks_getBinder(IBtHfpAgCallbacks* cbks)
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

binder_status_t BtHfpAgCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtHfpAgCallbacks_Class) {
        kIBtHfpAgCallbacks_Class = AIBinder_Class_define(BT_HFP_AG_CALLBACK_DESC, IBtHfpAgCallbacks_Class_onCreate,
            IBtHfpAgCallbacks_Class_onDestroy, IBtHfpAgCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtHfpAgCallbacks_Class);
}

IBtHfpAgCallbacks* BtHfpAgCallbacks_new(const hfp_ag_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtHfpAgCallbacks* cbks = malloc(sizeof(IBtHfpAgCallbacks));

    clazz = AIBinder_Class_define(BT_HFP_AG_CALLBACK_DESC, IBtHfpAgCallbacks_Class_onCreate,
        IBtHfpAgCallbacks_Class_onDestroy, IBtHfpAgCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtHfpAgCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtHfpAgCallbacks_delete(IBtHfpAgCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
