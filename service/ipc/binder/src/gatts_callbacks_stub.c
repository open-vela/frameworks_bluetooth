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
#include "gatts_callbacks_stub.h"
#include "gatts_proxy.h"
#include "gatts_stub.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_GATT_SERVER_CALLBACK_DESC "BluetoothGattServerCallback"

static const AIBinder_Class* kIBtGattServerCallbacks_Class = NULL;

static void* IBtGattServerCallbacks_Class_onCreate(void* arg)
{
    BT_LOGD("%s", __func__);
    return arg;
}

static void IBtGattServerCallbacks_Class_onDestroy(void* userData)
{
    BT_LOGD("%s", __func__);
}

static binder_status_t IBtGattServerCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtGattServerCallbacks* cbks = AIBinder_getUserData(binder);
    bt_address_t addr;

    switch (code) {
    case ICBKS_GATT_SERVER_CONNECTED: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_connected)
            cbks->callbacks->on_connected(cbks, &addr);
        break;
    }
    case ICBKS_GATT_SERVER_DISCONNECTED: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_disconnected)
            cbks->callbacks->on_disconnected(cbks, &addr);
        break;
    }
    case ICBKS_GATT_SERVER_STARTED: {
        uint32_t status;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_started)
            cbks->callbacks->on_started(cbks, status);
        break;
    }
    case ICBKS_GATT_SERVER_STOPPED: {
        uint32_t status;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_stopped)
            cbks->callbacks->on_stopped(cbks, status);
        break;
    }
    case ICBKS_GATT_SERVER_MTU_CHANGED: {
        uint32_t mtu;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &mtu);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_mtu_changed)
            cbks->callbacks->on_mtu_changed(cbks, &addr, mtu);
        break;
    }
    case ICBKS_GATT_SERVER_READ: {
        uint32_t attr_handle;
        uint32_t req_handle;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &req_handle);
        if (stat != STATUS_OK)
            return stat;

        for (int i = 0; i < cbks->srv_db->attr_num; i++) {
            if (cbks->srv_db->attr_db[i].handle == attr_handle && cbks->srv_db->attr_db[i].read_cb) {
                cbks->srv_db->attr_db[i].read_cb(cbks, (uint16_t)attr_handle, req_handle);
                break;
            }
        }
        break;
    }
    case ICBKS_GATT_SERVER_WRITE: {
        uint32_t attr_handle;
        uint8_t* value;
        uint32_t length;
        uint32_t offset;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&value, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &length);
        if (stat != STATUS_OK) {
            free(value);
            return stat;
        }

        stat = AParcel_readUint32(in, &offset);
        if (stat != STATUS_OK) {
            free(value);
            return stat;
        }

        for (int i = 0; i < cbks->srv_db->attr_num; i++) {
            if (cbks->srv_db->attr_db[i].handle == attr_handle && cbks->srv_db->attr_db[i].write_cb) {
                cbks->srv_db->attr_db[i].write_cb(cbks, (uint16_t)attr_handle, value, length, offset);
                break;
            }
        }
        free(value);
        break;
    }
    case ICBKS_GATT_SERVER_NOTIFY_COMPLETE: {
        uint32_t status;
        uint32_t attr_handle;

        stat = AParcel_readUint32(in, &status);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        if (cbks->callbacks && cbks->callbacks->on_notify_complete)
            cbks->callbacks->on_notify_complete(cbks, status, (uint16_t)attr_handle);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtGattServerCallbacks_getBinder(IBtGattServerCallbacks* cbks)
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

binder_status_t BtGattServerCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtGattServerCallbacks_Class) {
        kIBtGattServerCallbacks_Class = AIBinder_Class_define(BT_GATT_SERVER_CALLBACK_DESC, IBtGattServerCallbacks_Class_onCreate,
            IBtGattServerCallbacks_Class_onDestroy, IBtGattServerCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtGattServerCallbacks_Class);
}

IBtGattServerCallbacks* BtGattServerCallbacks_new(const gatts_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtGattServerCallbacks* cbks = malloc(sizeof(IBtGattServerCallbacks));

    clazz = AIBinder_Class_define(BT_GATT_SERVER_CALLBACK_DESC, IBtGattServerCallbacks_Class_onCreate,
        IBtGattServerCallbacks_Class_onDestroy, IBtGattServerCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtGattServerCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtGattServerCallbacks_delete(IBtGattServerCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
