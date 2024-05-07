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
#include "hid_device_callbacks_stub.h"
#include "hid_device_proxy.h"
#include "hid_device_stub.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBtHiddCallbacks_appStateCallback(void* cookie, hid_app_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HIDD_APP_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHiddCallbacks_connectionStateCallback(void* cookie, bt_address_t* bdAddr, bool le_hid,
    profile_connection_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, bdAddr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeBool(parcelIn, le_hid);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HIDD_CONNECTION_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHiddCallbacks_getReportCallback(void* cookie, bt_address_t* bdAddr, uint8_t rpt_type,
    uint8_t rpt_id, uint16_t buffer_size)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, bdAddr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_type);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_id);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)buffer_size);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GET_REPORT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHiddCallbacks_setReportCallback(void* cookie, bt_address_t* bdAddr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, bdAddr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_type);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_size);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)rpt_data, rpt_size);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_SET_REPORT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHiddCallbacks_receiveReportCallback(void* cookie, bt_address_t* bdAddr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, bdAddr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_type);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_size);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)rpt_data, rpt_size);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_RECEIVE_REPORT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHiddCallbacks_virtualUnplugCallback(void* cookie, bt_address_t* bdAddr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, bdAddr);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_VIRTUAL_UNPLUG, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static const hid_device_callbacks_t static_hidd_cbks = {
    sizeof(static_hidd_cbks),
    BpBtHiddCallbacks_appStateCallback,
    BpBtHiddCallbacks_connectionStateCallback,
    BpBtHiddCallbacks_getReportCallback,
    BpBtHiddCallbacks_setReportCallback,
    BpBtHiddCallbacks_receiveReportCallback,
    BpBtHiddCallbacks_virtualUnplugCallback,
};

const hid_device_callbacks_t* BpBtHiddCallbacks_getStatic(void)
{
    return &static_hidd_cbks;
}
