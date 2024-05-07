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

#include "advertiser_callbacks_stub.h"
#include "binder_utils.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_ADVERTISER_CALLBACK_DESC "BluetoothADvertiserCallback"

static const AIBinder_Class* kIBtAdvertiserCallbacks_Class = NULL;

static void* IBtAdvertiserCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtAdvertiserCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtAdvertiserCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtAdvertiserCallbacks* cbks = AIBinder_getUserData(binder);

    switch (code) {
    case ICBKS_ON_ADVERTISING_START: {
        uint32_t adv_id, status;

        stat = AParcel_readUint32(in, &adv_id);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_advertising_start(cbks, (uint8_t)adv_id, (uint8_t)status);
        break;
    }
    case ICBKS_ON_ADVERTISING_STOPPED: {
        uint32_t adv_id;

        stat = AParcel_readUint32(in, &adv_id);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_advertising_stopped(cbks, (uint8_t)adv_id);
        BtAdvertiserCallbacks_delete(cbks);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtAdvertiserCallbacks_getBinder(IBtAdvertiserCallbacks* cbks)
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

binder_status_t BtAdvertiserCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtAdvertiserCallbacks_Class) {
        kIBtAdvertiserCallbacks_Class = AIBinder_Class_define(BT_ADVERTISER_CALLBACK_DESC, IBtAdvertiserCallbacks_Class_onCreate,
            IBtAdvertiserCallbacks_Class_onDestroy, IBtAdvertiserCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtAdvertiserCallbacks_Class);
}

IBtAdvertiserCallbacks* BtAdvertiserCallbacks_new(const advertiser_callback_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtAdvertiserCallbacks* cbks = malloc(sizeof(IBtAdvertiserCallbacks));

    clazz = AIBinder_Class_define(BT_ADVERTISER_CALLBACK_DESC, IBtAdvertiserCallbacks_Class_onCreate,
        IBtAdvertiserCallbacks_Class_onDestroy, IBtAdvertiserCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtAdvertiserCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtAdvertiserCallbacks_delete(IBtAdvertiserCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
