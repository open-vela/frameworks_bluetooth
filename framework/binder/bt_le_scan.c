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
#define LOG_TAG "adv"

#include <stdlib.h>

#include "adapter_proxy.h"
#include "adapter_stub.h"
#include "bluetooth.h"
#include "bt_le_scan.h"
#include "bt_list.h"
#include "scanner_callbacks_stub.h"
#include "utils/log.h"

bt_scanner_t* bt_le_start_scan(bt_instance_t* ins, const scanner_callbacks_t* cbs)
{
    return bt_le_start_scan_settings(ins, NULL, cbs);
}

bt_scanner_t* bt_le_start_scan_settings(bt_instance_t* ins,
    ble_scan_settings_t* settings,
    const scanner_callbacks_t* cbs)
{
    BpBtAdapter* bpAdapter = ins->adapter_proxy;
    void* rmt_scanner = NULL;

    IBtScannerCallbacks* cbks = BtScannerCallbacks_new(cbs);
    AIBinder* binder = BtScannerCallbacks_getBinder(cbks);
    if (!binder) {
        BtScannerCallbacks_delete(cbks);
        return NULL;
    }

    if (settings)
        rmt_scanner = BpBtAdapter_startScanSettings(bpAdapter, settings, binder);
    else
        rmt_scanner = BpBtAdapter_startScan(bpAdapter, binder);

    AIBinder_decStrong(binder);
    if (!rmt_scanner) {
        BtScannerCallbacks_delete(cbks);
        return NULL;
    }

    cbks->cookie = rmt_scanner;
    return (bt_scanner_t*)cbks;
}

void bt_le_stop_scan(bt_instance_t* ins, bt_scanner_t* scanner)
{
    BpBtAdapter* bpAdapter = ins->adapter_proxy;
    IBtScannerCallbacks* cbks = scanner;

    BpBtAdapter_stopScan(bpAdapter, cbks->cookie);
}

bool bt_le_scan_is_supported(bt_instance_t* ins)
{
    return false;
}
