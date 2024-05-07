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
#include <stdio.h>
#include <stdlib.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "binder_utils.h"
#include "gattc_callbacks_stub.h"
#include "gattc_proxy.h"
#include "gattc_stub.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_GATT_CLIENT_CALLBACK_DESC "BluetoothGattClientCallback"

static const AIBinder_Class* kIBtGattClientCallbacks_Class = NULL;

static void* IBtGattClientCallbacks_Class_onCreate(void* arg)
{
    BT_LOGD("%s", __func__);
    return arg;
}

static void IBtGattClientCallbacks_Class_onDestroy(void* userData)
{
    BT_LOGD("%s", __func__);
}

static binder_status_t IBtGattClientCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtGattClientCallbacks* cbks = AIBinder_getUserData(binder);
    bt_address_t addr;

    switch (code) {
    case ICBKS_GATT_CLIENT_CONNECTED: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_connected)
            cbks->callbacks->on_connected(cbks, &addr);
        break;
    }
    case ICBKS_GATT_CLIENT_DISCONNECTED: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_disconnected)
            cbks->callbacks->on_disconnected(cbks, &addr);
        break;
    }
    case ICBKS_GATT_CLIENT_DISCOVERED: {
        uint32_t status;
        bt_uuid_t uuid;
        uint32_t start_handle;
        uint32_t end_handle;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUuid(in, &uuid);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &start_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &end_handle);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_discovered)
            cbks->callbacks->on_discovered(cbks, status, &uuid, (uint16_t)start_handle, (uint16_t)end_handle);
        break;
    }
    case ICBKS_GATT_CLIENT_MTU_EXCHANGE: {
        uint32_t status;
        uint32_t mtu;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &mtu);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_mtu_exchange)
            cbks->callbacks->on_mtu_exchange(cbks, status, mtu);
        break;
    }
    case ICBKS_GATT_CLIENT_READ: {
        uint32_t status;
        uint32_t attr_handle;
        uint8_t* value;
        uint32_t length;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &length);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&value, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_read)
            cbks->callbacks->on_read(cbks, status, (uint16_t)attr_handle, value, (uint16_t)length);
        free(value);
        break;
    }
    case ICBKS_GATT_CLIENT_WRITTEN: {
        uint32_t status;
        uint32_t attr_handle;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_written)
            cbks->callbacks->on_written(cbks, status, (uint16_t)attr_handle);
        break;
    }
    case ICBKS_GATT_CLIENT_NOTIFIED: {
        uint32_t attr_handle;
        uint8_t* value;
        uint32_t length;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &length);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&value, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_notified)
            cbks->callbacks->on_notified(cbks, (uint16_t)attr_handle, value, (uint16_t)length);
        free(value);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtGattClientCallbacks_getBinder(IBtGattClientCallbacks* cbks)
{
    AIBinder* binder = NULL;

    if (cbks->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(cbks->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(cbks->clazz, (void*)cbks);
        if (cbks->WeakBinder != NULL) {
            AIBinder_Weak_delete(cbks->WeakBinder);
        }

        cbks->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtGattClientCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtGattClientCallbacks_Class) {
        kIBtGattClientCallbacks_Class = AIBinder_Class_define(BT_GATT_CLIENT_CALLBACK_DESC, IBtGattClientCallbacks_Class_onCreate,
            IBtGattClientCallbacks_Class_onDestroy, IBtGattClientCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtGattClientCallbacks_Class);
}

IBtGattClientCallbacks* BtGattClientCallbacks_new(const gattc_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtGattClientCallbacks* cbks = malloc(sizeof(IBtGattClientCallbacks));

    clazz = AIBinder_Class_define(BT_GATT_CLIENT_CALLBACK_DESC, IBtGattClientCallbacks_Class_onCreate,
        IBtGattClientCallbacks_Class_onDestroy, IBtGattClientCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtGattClientCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtGattClientCallbacks_delete(IBtGattClientCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
