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
#include "gattc_service.h"

#include "gattc_callbacks_stub.h"
#include "gattc_proxy.h"
#include "gattc_stub.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBleGattClientCallbacks_onConnected(void *handle, bt_address_t *addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = if_gattc_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_CLIENT_CONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBleGattClientCallbacks_onDisconnected(void *handle, bt_address_t *addr, uint8_t reason)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = if_gattc_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)reason);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_CLIENT_DISCONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBleGattClientCallbacks_onDiscover(void *handle, gatt_status_t status, bt_uuid_t *uuid, uint16_t start_handle, uint16_t end_handle)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = if_gattc_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUuid(parcelIn, uuid);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)start_handle);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)end_handle);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_CLIENT_DISCOVER, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBleGattClientCallbacks_onMtuExchange(void *handle, gatt_status_t status, uint32_t mtu)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = if_gattc_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)mtu);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_CLIENT_MTU_EXCHANGE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static const gattc_callbacks_t static_gattc_cbks = {
    sizeof(static_gattc_cbks),
    BpBleGattClientCallbacks_onConnected,
    BpBleGattClientCallbacks_onDisconnected,
    BpBleGattClientCallbacks_onDiscover,
    BpBleGattClientCallbacks_onMtuExchange,
};

const gattc_callbacks_t *BpBleGattClientCallbacks_getStatic(void)
{
    return &static_gattc_cbks;
}

void BpBleGattClientCallbacks_onRead(void *handle, gatt_status_t status, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = if_gattc_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t *)value, length);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_CLIENT_READ, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

void BpBleGattClientCallbacks_onWrite(void *handle, gatt_status_t status, uint16_t attr_handle, uint16_t offset)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = if_gattc_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)status);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)offset);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_CLIENT_WRITE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

void BpBleGattClientCallbacks_onNotify(void *handle, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = if_gattc_get_remote(handle);

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t *)value, length);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_GATT_CLIENT_NOTIFY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}
