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
#include "service_manager.h"
#include "spp_service.h"

#include "parcel.h"
#include "spp_callbacks_proxy.h"
#include "spp_callbacks_stub.h"
#include "spp_proxy.h"
#include "spp_stub.h"
#include "utils/log.h"

#define BT_SPP_DESC "BluetoothSpp"

static void* IBtSpp_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtSpp_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtSpp_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;

    spp_interface_t* profile = (spp_interface_t*)service_manager_get_profile(PROFILE_SPP);
    if (!profile)
        return stat;

    switch (code) {
    case ISPP_REGISTER_APP: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtSppCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        void* handle = profile->register_app(remote, BpBtSppCallbacks_getStatic());
        stat = AParcel_writeUint32(reply, (uint32_t)handle);
        break;
    }
    case ISPP_UNREGISTER_APP: {
        AIBinder* remote = NULL;
        uint32_t handle;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        bt_status_t status = profile->unregister_app((void**)&remote, (void*)handle);
        if ((status == BT_STATUS_SUCCESS) && remote)
            AIBinder_decStrong(remote);

        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case ISPP_SERVER_START: {
        uint32_t handle;
        uint32_t status;
        bt_uuid_t uuid;
        uint32_t scn, maxConns;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &scn);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUuid(in, &uuid);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &maxConns);
        if (stat != STATUS_OK)
            return stat;

        status = profile->server_start((void*)handle, (uint16_t)scn, &uuid, (uint8_t)maxConns);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case ISPP_SERVER_STOP: {
        uint32_t handle;
        uint32_t status;
        uint32_t scn;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &scn);
        if (stat != STATUS_OK)
            return stat;

        status = profile->server_stop((void*)handle, (uint16_t)scn);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case ISPP_CONNECT: {
        uint32_t handle;
        uint32_t status;
        bt_uuid_t uuid;
        bt_address_t addr;
        int32_t scn;
        uint16_t port = 0;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readInt32(in, &scn);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUuid(in, &uuid);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect((void*)handle, &addr, (int16_t)scn, &uuid, &port);
        stat = AParcel_writeUint32(reply, (uint32_t)port);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case ISPP_DISCONNECT: {
        uint32_t handle;
        uint32_t status;
        bt_address_t addr;
        uint32_t port = 0;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &port);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect((void*)handle, &addr, (uint16_t)port);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtSpp_getClass(void)
{

    AIBinder_Class* clazz = AIBinder_Class_define(BT_SPP_DESC, IBtSpp_Class_onCreate,
        IBtSpp_Class_onDestroy, IBtSpp_Class_onTransact);

    return clazz;
}

static AIBinder* BtSpp_getBinder(IBtSpp* spp)
{
    AIBinder* binder = NULL;

    if (spp->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(spp->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(spp->clazz, (void*)spp);
        if (spp->WeakBinder != NULL) {
            AIBinder_Weak_delete(spp->WeakBinder);
        }

        spp->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtSpp_addService(IBtSpp* spp, const char* instance)
{
    spp->clazz = (AIBinder_Class*)BtSpp_getClass();
    AIBinder* binder = BtSpp_getBinder(spp);
    spp->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtSpp* BpBtSpp_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtSpp* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtSpp_getClass();
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

void BpBtSpp_delete(BpBtSpp* bpSpp)
{
    AIBinder_decStrong(bpSpp->binder);
    free(bpSpp);
}

AIBinder* BtSpp_getService(BpBtSpp** bpSpp, const char* instance)
{
    BpBtSpp* bpBinder = *bpSpp;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtSpp_new(instance);
    if (!bpBinder)
        return NULL;

    *bpSpp = bpBinder;

    return bpBinder->binder;
}
