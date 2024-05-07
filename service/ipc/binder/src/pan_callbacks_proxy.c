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
#include "pan_callbacks_stub.h"
#include "pan_proxy.h"
#include "pan_stub.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBtPanCallbacks_connectionStateCallback(void* cookie, profile_connection_state_t state,
    bt_address_t* bdAddr, uint8_t localRole,
    uint8_t remotRole)
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

    stat = AParcel_writeAddress(parcelIn, bdAddr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)localRole);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)remotRole);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_PAN_CONNECTION_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtPanCallbacks_netIfStateCallback(void* cookie, pan_netif_state_t state,
    int localRole, const char* ifName)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)localRole);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeString(parcelIn, ifName, strlen(ifName));
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_NETIF_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static const pan_callbacks_t static_pan_cbks = {
    sizeof(static_pan_cbks),
    BpBtPanCallbacks_netIfStateCallback,
    BpBtPanCallbacks_connectionStateCallback,
};

const pan_callbacks_t* BpBtPanCallbacks_getStatic(void)
{
    return &static_pan_cbks;
}
