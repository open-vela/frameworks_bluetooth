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
#include "hid_device_callbacks_stub.h"
#include "hid_device_proxy.h"
#include "hid_device_stub.h"
#include "parcel.h"

#include "bluetooth.h"
#include "utils/log.h"

#define BT_HID_DEVICE_CALLBACK_DESC "BluetoothHidDeviceCallback"

static const AIBinder_Class* kIBtHiddCallbacks_Class = NULL;

static void* IBtHiddCallbacks_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtHiddCallbacks_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtHiddCallbacks_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    IBtHiddCallbacks* cbks = AIBinder_getUserData(binder);

    switch (code) {
    case ICBKS_HIDD_APP_STATE: {
        uint32_t state;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->app_state_cb(cbks, state);
        break;
    }
    case ICBKS_HIDD_CONNECTION_STATE: {
        uint32_t state;
        bt_address_t addr;
        bool leLink;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &leLink);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &state);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->connection_state_cb(cbks, &addr, leLink, state);
        break;
    }
    case ICBKS_GET_REPORT: {
        bt_address_t addr;
        uint32_t rpt_type, rpt_id, buffer_size;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_id);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &buffer_size);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->get_report_cb(cbks, &addr, (uint8_t)rpt_type, (uint8_t)rpt_id, (uint16_t)buffer_size);
        break;
    }
    case ICBKS_SET_REPORT: {
        bt_address_t addr;
        uint32_t rpt_type, rpt_size;
        uint8_t* rpt_data;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_size);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&rpt_data, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->set_report_cb(cbks, &addr, (uint8_t)rpt_type, (uint16_t)rpt_size, rpt_data);
        free(rpt_data);
        break;
    }
    case ICBKS_RECEIVE_REPORT: {
        bt_address_t addr;
        uint32_t rpt_type, rpt_size;
        uint8_t* rpt_data;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_size);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&rpt_data, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->receive_report_cb(cbks, &addr, (uint8_t)rpt_type, (uint16_t)rpt_size, rpt_data);
        free(rpt_data);
        break;
    }
    case ICBKS_VIRTUAL_UNPLUG: {
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        cbks->callbacks->virtual_unplug_cb(cbks, &addr);
        break;
    }
    default:
        break;
    }

    return stat;
}

AIBinder* BtHiddCallbacks_getBinder(IBtHiddCallbacks* cbks)
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

binder_status_t BtHiddCallbacks_associateClass(AIBinder* binder)
{
    if (!kIBtHiddCallbacks_Class) {
        kIBtHiddCallbacks_Class = AIBinder_Class_define(BT_HID_DEVICE_CALLBACK_DESC, IBtHiddCallbacks_Class_onCreate,
            IBtHiddCallbacks_Class_onDestroy, IBtHiddCallbacks_Class_onTransact);
    }

    return AIBinder_associateClass(binder, kIBtHiddCallbacks_Class);
}

IBtHiddCallbacks* BtHiddCallbacks_new(const hid_device_callbacks_t* callbacks)
{
    AIBinder_Class* clazz;
    AIBinder* binder;
    IBtHiddCallbacks* cbks = malloc(sizeof(IBtHiddCallbacks));

    clazz = AIBinder_Class_define(BT_HID_DEVICE_CALLBACK_DESC, IBtHiddCallbacks_Class_onCreate,
        IBtHiddCallbacks_Class_onDestroy, IBtHiddCallbacks_Class_onTransact);

    cbks->clazz = clazz;
    cbks->WeakBinder = NULL;
    cbks->callbacks = callbacks;

    binder = BtHiddCallbacks_getBinder(cbks);
    AIBinder_decStrong(binder);

    return cbks;
}

void BtHiddCallbacks_delete(IBtHiddCallbacks* cbks)
{
    assert(cbks);

    if (cbks->WeakBinder)
        AIBinder_Weak_delete(cbks->WeakBinder);

    free(cbks);
}
