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
#include "scanner_callbacks_stub.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_SCANNER_CALLBACK_DESC "BluetoothScannerCallback"

static const AIBinder_Class* kIBtScannerCallbacks_Class = NULL;

static void* IBtScannerCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtScannerCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtScannerCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtScannerCallbacks* cbks = AIBinder_getUserData(binder);

    switch (code) {
    case ICBKS_ON_SCAN_RESULT: {
        ble_scan_result_t* result = NULL;

        stat = AParcel_readBleScanResult(in, &result);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_scan_result(cbks, result);
        free(result);
        break;
    }
    case ICBKS_ON_SCAN_START_STATUS: {
        uint32_t status;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->on_scan_start_status(cbks, (uint8_t)status);
        break;
    }
    case ICBKS_ON_SCAN_STOPPED: {
        cbks->callbacks->on_scan_stopped(cbks);
        BtScannerCallbacks_delete(cbks);
        stat = STATUS_OK;
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtScannerCallbacks_getBinder(IBtScannerCallbacks* cbks)
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

binder_status_t BtScannerCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtScannerCallbacks_Class) {
        kIBtScannerCallbacks_Class = AIBinder_Class_define(BT_SCANNER_CALLBACK_DESC, IBtScannerCallbacks_Class_onCreate,
            IBtScannerCallbacks_Class_onDestroy, IBtScannerCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtScannerCallbacks_Class);
}

IBtScannerCallbacks* BtScannerCallbacks_new(const scanner_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtScannerCallbacks* cbks = malloc(sizeof(IBtScannerCallbacks));

    clazz = AIBinder_Class_define(BT_SCANNER_CALLBACK_DESC, IBtScannerCallbacks_Class_onCreate,
        IBtScannerCallbacks_Class_onDestroy, IBtScannerCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtScannerCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtScannerCallbacks_delete(IBtScannerCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
