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
#include "spp_callbacks_stub.h"
#include "spp_proxy.h"
#include "spp_stub.h"

#include "utils/log.h"

static void BpBtSppCallbacks_connectionStateCallback(void* handle, bt_address_t* addr,
    uint16_t scn, uint16_t port,
    profile_connection_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = handle;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)scn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)port);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_SPP_CONNECTION_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtSppCallbacks_ptyOpenCallback(void* handle, bt_address_t* addr, uint16_t scn, uint16_t port, char* name)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = handle;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)scn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)port);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeString(parcelIn, name, name ? strlen(name) : -1);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_PTY_OPEN, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static const spp_callbacks_t static_spp_cbks = {
    sizeof(static_spp_cbks),
    BpBtSppCallbacks_ptyOpenCallback,
    BpBtSppCallbacks_connectionStateCallback,
};

const spp_callbacks_t* BpBtSppCallbacks_getStatic(void)
{
    return &static_spp_cbks;
}
