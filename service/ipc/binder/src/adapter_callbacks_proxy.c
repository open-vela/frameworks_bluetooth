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

#include "adapter_callbacks_stub.h"
#include "adapter_internel.h"
#include "adapter_proxy.h"
#include "adapter_stub.h"
#include "bluetooth.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBtAdapterCallbacks_onAdapterStateChanged(void* context, bt_adapter_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_ADAPTER_STATE_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onDiscoveryStateChanged(void* context, bt_discovery_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_DISCOVERY_STATE_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onDiscoveryResult(void* context, bt_discovery_result_t* remote)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, &remote->addr);
    if (stat != STATUS_OK)
        return;

    int len = strlen(remote->name);
    stat = AParcel_writeString(parcelIn, len ? remote->name : NULL, len ? len : -1);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, remote->cod);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeByte(parcelIn, remote->rssi);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_DISCOVERY_RESULT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onScanModeChanged(void* context, bt_scan_mode_t mode)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, mode);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_SCAN_MODE_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onDeviceNameChanged(void* context, const char* device_name)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeString(parcelIn, device_name, strlen(device_name));
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_DEVICE_NAME_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onPairRequest(void* context, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_PAIR_REQUEST, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onPairDisplay(void* context, bt_address_t* addr, bt_transport_t transport, bt_pair_type_t type, uint32_t pass_key)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, transport);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, type);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, pass_key);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_PAIR_DISPLAY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onConnectionStateChanged(void* context, bt_address_t* addr, bt_transport_t transport, connection_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, transport);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_CONNECTION_STATE_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onBondStateChanged(void* context, bt_address_t* addr, bt_transport_t transport, bond_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, transport);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_BOND_STATE_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onRemoteNameChanged(void* context, bt_address_t* addr, const char* name)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeString(parcelIn, name, strlen(name));
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_REMOTE_NAME_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onRemoteAliasChanged(void* context, bt_address_t* addr, const char* alias)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeString(parcelIn, alias, strlen(alias));
    if (stat != STATUS_OK)
        return;
    stat = AIBinder_transact(binder, ICBKS_REMOTE_ALIAS_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onRemoteCodChanged(void* context, bt_address_t* addr, uint32_t cod)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, cod);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_REMOTE_COD_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtAdapterCallbacks_onRemoteUuidsChanged(void* context, bt_address_t* addr, bt_uuid_t* uuids, uint16_t size)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = context;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUuidArray(parcelIn, uuids, (int32_t)size);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_REMOTE_UUIDS_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static const adapter_callbacks_t static_adapter_cbks = {
    BpBtAdapterCallbacks_onAdapterStateChanged,
    BpBtAdapterCallbacks_onDiscoveryStateChanged,
    BpBtAdapterCallbacks_onDiscoveryResult,
    BpBtAdapterCallbacks_onScanModeChanged,
    BpBtAdapterCallbacks_onDeviceNameChanged,
    BpBtAdapterCallbacks_onPairRequest,
    BpBtAdapterCallbacks_onPairDisplay,
    BpBtAdapterCallbacks_onConnectionStateChanged,
    BpBtAdapterCallbacks_onBondStateChanged,
    BpBtAdapterCallbacks_onRemoteNameChanged,
    BpBtAdapterCallbacks_onRemoteAliasChanged,
    BpBtAdapterCallbacks_onRemoteCodChanged,
    BpBtAdapterCallbacks_onRemoteUuidsChanged,
};

const adapter_callbacks_t* BpBtAdapterCallbacks_getStatic(void)
{
    return &static_adapter_cbks;
}
