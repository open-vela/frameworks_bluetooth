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
#include "hfp_hf_callbacks_stub.h"
#include "hfp_hf_proxy.h"
#include "hfp_hf_stub.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_HFP_HF_CALLBACK_DESC "BluetoothHfpHfCallback"

static const AIBinder_Class* kIBtHfpHfCallbacks_Class = NULL;

static void* IBtHfpHfCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtHfpHfCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtHfpHfCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtHfpHfCallbacks* cbks = AIBinder_getUserData(binder);
    bt_address_t addr;

    switch (code) {
    case ICBKS_HFP_HF_CONNECTION_STATE: {
        uint32_t state;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->connection_state_cb(cbks, &addr, state);
        break;
    }
    case ICBKS_HFP_HF_AUDIO_STATE: {
        uint32_t state;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->audio_state_cb(cbks, &addr, state);
        break;
    }
    case ICBKS_HFP_HF_VR_STATE: {
        bool start;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &start);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->vr_cmd_cb(cbks, &addr, start);
        break;
    }
    case ICBKS_HFP_HF_CALL_STATE_CHANGE: {
        hfp_current_call_t call;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readCall(in, &call);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->call_state_changed_cb(cbks, &addr, &call);
        break;
    }
    case ICBKS_HFP_HF_CMD_COMPLETE: {
        char* resp = NULL;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &resp, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;
        cbks->callbacks->cmd_complete_cb(cbks, &addr, resp);
        free(resp);
        break;
    }
    case ICBKS_HFP_HF_RING_INDICATION: {
        bool inBandRing;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &inBandRing);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->ring_indication_cb(cbks, &addr, inBandRing);
        break;
    }
    /* not support */
    case ICBKS_HFP_HF_ROAMING_CHANGED: {
        break;
    }
    case ICBKS_HFP_HF_NETWOEK_STATE_CHANGED: {
        break;
    }
    case ICBKS_HFP_HF_SIGNAL_STRENGTH_CHANGED: {
        break;
    }
    case ICBKS_HFP_HF_OPERATOR_CHANGED: {
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtHfpHfCallbacks_getBinder(IBtHfpHfCallbacks* cbks)
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

binder_status_t BtHfpHfCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtHfpHfCallbacks_Class) {
        kIBtHfpHfCallbacks_Class = AIBinder_Class_define(BT_HFP_HF_CALLBACK_DESC,
            IBtHfpHfCallbacks_Class_onCreate,
            IBtHfpHfCallbacks_Class_onDestroy,
            IBtHfpHfCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtHfpHfCallbacks_Class);
}

IBtHfpHfCallbacks* BtHfpHfCallbacks_new(const hfp_hf_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtHfpHfCallbacks* cbks = malloc(sizeof(IBtHfpHfCallbacks));

    clazz = AIBinder_Class_define(BT_HFP_HF_CALLBACK_DESC,
        IBtHfpHfCallbacks_Class_onCreate,
        IBtHfpHfCallbacks_Class_onDestroy,
        IBtHfpHfCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtHfpHfCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtHfpHfCallbacks_delete(IBtHfpHfCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
