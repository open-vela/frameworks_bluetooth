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
#include "pan_service.h"
#include "service_manager.h"

#include "pan_callbacks_proxy.h"
#include "pan_callbacks_stub.h"
#include "pan_proxy.h"
#include "pan_stub.h"
#include "parcel.h"
#include "utils/log.h"

#define BT_PAN_DESC "BluetoothPan"

static void* IBtPan_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtPan_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtPan_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;

    pan_interface_t* profile = (pan_interface_t*)service_manager_get_profile(PROFILE_PANU);
    if (!profile)
        return stat;

    switch (code) {
    case IPAN_REGISTER_CALLBACK: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtPanCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        void* cookie = profile->register_callbacks(remote, BpBtPanCallbacks_getStatic());
        stat = AParcel_writeUint32(reply, (uint32_t)cookie);
        break;
    }
    case IPAN_UNREGISTER_CALLBACK: {
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
    case IPAN_CONNECT: {
        uint32_t status;
        bt_address_t addr;
        uint32_t dstRole, srcRole;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &dstRole);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &srcRole);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect(&addr, dstRole, srcRole);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IPAN_DISCONNECT: {
        uint32_t status;
        bt_address_t addr;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtPan_getClass(void)
{

    AIBinder_Class* clazz = AIBinder_Class_define(BT_PAN_DESC, IBtPan_Class_onCreate,
        IBtPan_Class_onDestroy, IBtPan_Class_onTransact);

    return clazz;
}

static AIBinder* BtPan_getBinder(IBtPan* pan)
{
    AIBinder* binder = NULL;

    if (pan->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(pan->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(pan->clazz, (void*)pan);
        if (pan->WeakBinder != NULL) {
            AIBinder_Weak_delete(pan->WeakBinder);
        }

        pan->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtPan_addService(IBtPan* pan, const char* instance)
{
    pan->clazz = (AIBinder_Class*)BtPan_getClass();
    AIBinder* binder = BtPan_getBinder(pan);
    pan->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtPan* BpBtPan_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtPan* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtPan_getClass();
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

void BpBtPan_delete(BpBtPan* bpPan)
{
    AIBinder_decStrong(bpPan->binder);
    free(bpPan);
}

AIBinder* BtPan_getService(BpBtPan** bpPan, const char* instance)
{
    BpBtPan* bpBinder = *bpPan;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtPan_new(instance);
    if (!bpBinder)
        return NULL;

    *bpPan = bpBinder;

    return bpBinder->binder;
}
