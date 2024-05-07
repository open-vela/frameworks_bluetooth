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
#include "hid_device_service.h"
#include "service_manager.h"

#include "hid_device_callbacks_proxy.h"
#include "hid_device_callbacks_stub.h"
#include "hid_device_proxy.h"
#include "hid_device_stub.h"
#include "parcel.h"
#include "utils/log.h"

#define BT_HID_DEVICE_DESC "BluetoothHidDevice"

static void* IBtHidd_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtHidd_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtHidd_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;

    hid_device_interface_t* profile = (hid_device_interface_t*)service_manager_get_profile(PROFILE_HID_DEV);
    if (!profile)
        return stat;

    switch (code) {
    case IHIDD_REGISTER_CALLBACK: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtHiddCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        void* cookie = profile->register_callbacks(remote, BpBtHiddCallbacks_getStatic());
        stat = AParcel_writeUint32(reply, (uint32_t)cookie);
        break;
    }
    case IHIDD_UNREGISTER_CALLBACK: {
        AIBinder* remote = NULL;
        uint32_t cookie;

        stat = AParcel_readUint32(in, &cookie);
        if (stat != STATUS_OK)
            return stat;

        bool ret = profile->unregister_callbacks((void**)&remote, (void*)cookie);
        if (ret && remote)
            AIBinder_decStrong(remote);

        stat = AParcel_writeBool(reply, ret);
        break;
    }
    case IHIDD_REGISTER_APP: {
        uint32_t status;
        uint32_t u32Val;
        hid_device_sdp_settings_t sdp;
        bool le_hid;

        memset(&sdp, 0, sizeof(hid_device_sdp_settings_t));
        stat = AParcel_readString(in, &sdp.name, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            goto register_out;

        stat = AParcel_readString(in, &sdp.description, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            goto register_out;

        stat = AParcel_readString(in, &sdp.provider, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            goto register_out;

        stat = AParcel_readUint32(in, &sdp.hids_info.attr_mask);
        if (stat != STATUS_OK)
            goto register_out;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.sub_class = (uint8_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.country_code = (uint8_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.vendor_id = (uint16_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.product_id = (uint16_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.version = (uint16_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.supervision_timeout = (uint16_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.ssr_max_latency = (uint16_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.ssr_min_timeout = (uint16_t)u32Val;

        stat = AParcel_readUint32(in, &u32Val);
        if (stat != STATUS_OK)
            goto register_out;
        sdp.hids_info.dsc_list_length = (uint16_t)u32Val;

        stat = AParcel_readByteArray(in, (void*)&sdp.hids_info.dsc_list, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            goto register_out;

        stat = AParcel_readBool(in, &le_hid);
        if (stat != STATUS_OK)
            goto register_out;

        status = profile->register_app(&sdp, le_hid);
        stat = AParcel_writeUint32(reply, status);

    register_out:
        if (sdp.name)
            free((void*)sdp.name);
        if (sdp.description)
            free((void*)sdp.description);
        if (sdp.provider)
            free((void*)sdp.provider);
        if (sdp.hids_info.dsc_list)
            free((void*)sdp.hids_info.dsc_list);
        break;
    }
    case IHIDD_UNREGISTER_APP: {
        uint32_t status;

        status = profile->unregister_app();
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHIDD_CONNECT: {
        uint32_t status;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHIDD_DISCONNECT: {
        uint32_t status;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHIDD_SEND_REPORT: {
        uint32_t status, rpt_id;
        bt_address_t addr;
        int32_t rpt_size;
        uint8_t* rpt_data;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_id);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&rpt_data, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readInt32(in, &rpt_size);
        if (stat != STATUS_OK) {
            free(rpt_data);
            return stat;
        }

        status = profile->send_report(&addr, (uint8_t)rpt_id, rpt_data, rpt_size);
        free(rpt_data);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHIDD_RESPONSE_REPORT: {
        uint32_t status, rpt_type;
        bt_address_t addr;
        int32_t rpt_size;
        uint8_t* rpt_data;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rpt_type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&rpt_data, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readInt32(in, &rpt_size);
        if (stat != STATUS_OK) {
            free(rpt_data);
            return stat;
        }

        status = profile->response_report(&addr, (uint8_t)rpt_type, rpt_data, rpt_size);
        free(rpt_data);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHIDD_REPORT_ERROR: {
        uint32_t status;
        bt_address_t addr;
        int32_t error_code;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readInt32(in, &error_code);
        if (stat != STATUS_OK)
            return stat;

        status = profile->report_error(&addr, error_code);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHIDD_VIRTUAL_UNPLUG: {
        uint32_t status;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->virtual_unplug(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtHidd_getClass(void)
{

    AIBinder_Class* clazz = AIBinder_Class_define(BT_HID_DEVICE_DESC, IBtHidd_Class_onCreate,
        IBtHidd_Class_onDestroy, IBtHidd_Class_onTransact);

    return clazz;
}

static AIBinder* BtHidd_getBinder(IBtHidd* hidd)
{
    AIBinder* binder = NULL;

    if (hidd->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(hidd->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(hidd->clazz, (void*)hidd);
        if (hidd->WeakBinder != NULL) {
            AIBinder_Weak_delete(hidd->WeakBinder);
        }

        hidd->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtHidd_addService(IBtHidd* hidd, const char* instance)
{
    hidd->clazz = (AIBinder_Class*)BtHidd_getClass();
    AIBinder* binder = BtHidd_getBinder(hidd);
    hidd->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtHidd* BpBtHidd_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtHidd* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtHidd_getClass();
    binder = AServiceManager_getService(instance);
    if (!binder)
        return NULL;

    if (!AIBinder_associateClass(binder, clazz))
        goto bail;

    if (!AIBinder_isRemote(binder))
        goto bail;

    /* linktoDeath ? */

    bpBinder = malloc(sizeof(*bpBinder));
    if (!bpBinder)
        goto bail;

    bpBinder->binder = binder;
    bpBinder->clazz = clazz;

    return bpBinder;

bail:
    AIBinder_decStrong(binder);
    return NULL;
}

void BpBtHidd_delete(BpBtHidd* bpHidd)
{
    AIBinder_decStrong(bpHidd->binder);
    free(bpHidd);
}

AIBinder* BtHidd_getService(BpBtHidd** bpHidd, const char* instance)
{
    BpBtHidd* bpBinder = *bpHidd;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtHidd_new(instance);
    if (!bpBinder)
        return NULL;

    *bpHidd = bpBinder;

    return bpBinder->binder;
}
