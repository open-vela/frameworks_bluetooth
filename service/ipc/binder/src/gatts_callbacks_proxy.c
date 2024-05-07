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
#include "gatts_service.h"

#include "gatts_callbacks_stub.h"
#include "gatts_proxy.h"
#include "gatts_stub.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBtGattServerCallbacks_onConnected(void* handle, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_CONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtGattServerCallbacks_onDisconnected(void* handle, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_DISCONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtGattServerCallbacks_onStarted(void* handle, gatt_status_t status)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_STARTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtGattServerCallbacks_onStopped(void* handle, gatt_status_t status)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_STOPPED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtGattServerCallbacks_onMtuChanged(void* handle, bt_address_t* addr, uint32_t mtu)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)mtu);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_MTU_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtGattServerCallbacks_onNotifyComplete(void* handle, gatt_status_t status, uint16_t attr_handle)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_NOTIFY_COMPLETE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static const gatts_callbacks_t static_gatts_cbks = {
    sizeof(static_gatts_cbks),
    BpBtGattServerCallbacks_onConnected,
    BpBtGattServerCallbacks_onDisconnected,
    BpBtGattServerCallbacks_onStarted,
    BpBtGattServerCallbacks_onStopped,
    BpBtGattServerCallbacks_onNotifyComplete,
    BpBtGattServerCallbacks_onMtuChanged,
};

const gatts_callbacks_t* BpBtGattServerCallbacks_getStatic(void)
{
    return &static_gatts_cbks;
}

uint16_t BpBtGattServerCallbacks_onRead(void* handle, uint16_t attr_handle, uint32_t req_handle)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)req_handle);
    if (stat != STATUS_OK)
        return 0;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_READ, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
    }

    return 0;
}

uint16_t BpBtGattServerCallbacks_onWrite(void* handle, uint16_t attr_handle, const uint8_t* value, uint16_t length, uint16_t offset)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = if_gatts_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)value, length);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)offset);
    if (stat != STATUS_OK)
        return 0;

    stat = AIBinder_transact(binder, ICBKS_GATT_SERVER_WRITE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
    }

    return length;
}
