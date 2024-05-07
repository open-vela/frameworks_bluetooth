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
#include <string.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "bluetooth.h"
#include "parcel.h"
#include "scanner_callbacks_stub.h"

#include "utils/log.h"

static void BpBtScannerCallbacks_onScanResult(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = scanner;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeBleScanResult(parcelIn, result);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_ON_SCAN_RESULT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtScannerCallbacks_onScanStatus(bt_scanner_t* scanner, uint8_t status)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = scanner;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_ON_SCAN_START_STATUS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtScannerCallbacks_onScanStopped(bt_scanner_t* scanner)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = scanner;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_ON_SCAN_STOPPED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
    AIBinder_decStrong(binder);
}

static const scanner_callbacks_t static_scanner_cbks = {
    sizeof(static_scanner_cbks),
    BpBtScannerCallbacks_onScanResult,
    BpBtScannerCallbacks_onScanStatus,
    BpBtScannerCallbacks_onScanStopped
};

const scanner_callbacks_t* BpBtScannerCallbacks_getStatic(void)
{
    return &static_scanner_cbks;
}
