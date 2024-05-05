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
#include "advertiser_callbacks_stub.h"
#include "bluetooth.h"
#include "bt_le_advertiser.h"
#include "bt_list.h"
#include "utils/log.h"

bt_advertiser_t* bt_le_start_advertising(bt_instance_t* ins,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    advertiser_callback_t* cbs)
{
    BpBtAdapter* bpAdapter = ins->adapter_proxy;

    IBtAdvertiserCallbacks* cbks = BtAdvertiserCallbacks_new(cbs);
    AIBinder* binder = BtAdvertiserCallbacks_getBinder(cbks);
    if (!binder) {
        BtAdvertiserCallbacks_delete(cbks);
        return NULL;
    }

    void* rmt_adver = BpBtAdapter_startAdvertising(bpAdapter, params,
        adv_data, adv_len,
        scan_rsp_data,
        scan_rsp_len, binder);
    AIBinder_decStrong(binder);
    if (!rmt_adver) {
        BtAdvertiserCallbacks_delete(cbks);
        return NULL;
    }

    cbks->cookie = rmt_adver;
    return (bt_advertiser_t*)cbks;
}

void bt_le_stop_advertising(bt_instance_t* ins, bt_advertiser_t* adver)
{
    BpBtAdapter* bpAdapter = ins->adapter_proxy;
    IBtAdvertiserCallbacks* cbks = adver;

    BpBtAdapter_stopAdvertising(bpAdapter, cbks->cookie);
}

void bt_le_stop_advertising_id(bt_instance_t* ins, uint8_t adv_id)
{
    BpBtAdapter* bpAdapter = ins->adapter_proxy;

    BpBtAdapter_stopAdvertisingId(bpAdapter, adv_id);
}

bool bt_le_advertising_is_supported(bt_instance_t* ins)
{
    return false;
}
