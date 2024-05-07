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

#include "advertiser_callbacks_stub.h"
#include "bluetooth.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBtAdvertiserCallbacks_onAdvertisingStart(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = adv;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)adv_id);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_ON_ADVERTISING_START, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdvertiserCallbacks_onAdvertisingStopped(bt_advertiser_t* adv, uint8_t adv_id)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = adv;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)adv_id);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_ON_ADVERTISING_STOPPED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
    AIBinder_decStrong(binder);
}

static const advertiser_callback_t static_advertiser_cbks = {
    sizeof(static_advertiser_cbks),
    BpBtAdvertiserCallbacks_onAdvertisingStart,
    BpBtAdvertiserCallbacks_onAdvertisingStopped,
};

const advertiser_callback_t* BpBtAdvertiserCallbacks_getStatic(void)
{
    return &static_advertiser_cbks;
}
