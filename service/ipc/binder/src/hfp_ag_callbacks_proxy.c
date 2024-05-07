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
#include "hfp_ag_callbacks_stub.h"
#include "hfp_ag_proxy.h"
#include "hfp_ag_stub.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBtHfpAgCallbacks_connectionStateCallback(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_AG_CONNECTION_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpAgCallbacks_audioStateCallback(void* cookie, bt_address_t* addr, hfp_audio_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_AG_AUDIO_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpAgCallbacks_vrCmdCallback(void* cookie, bt_address_t* addr, bool started)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeBool(parcelIn, started);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_AG_VR_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpAgCallbacks_batteryUpdateCallback(void* cookie, bt_address_t* addr, uint8_t value)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)value);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_AG_BATTERY_UPDATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static const hfp_ag_callbacks_t static_hfp_ag_cbks = {
    sizeof(static_hfp_ag_cbks),
    BpBtHfpAgCallbacks_connectionStateCallback,
    BpBtHfpAgCallbacks_audioStateCallback,
    BpBtHfpAgCallbacks_vrCmdCallback,
    BpBtHfpAgCallbacks_batteryUpdateCallback
};

const hfp_ag_callbacks_t* BpBtHfpAgCallbacks_getStatic(void)
{
    return &static_hfp_ag_cbks;
}
