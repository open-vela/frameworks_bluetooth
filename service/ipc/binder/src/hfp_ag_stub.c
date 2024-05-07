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
#include "hfp_ag_service.h"
#include "service_manager.h"

#include "hfp_ag_callbacks_proxy.h"
#include "hfp_ag_callbacks_stub.h"
#include "hfp_ag_proxy.h"
#include "hfp_ag_stub.h"
#include "parcel.h"
#include "utils/log.h"

#define BT_HFP_AG_DESC "BluetoothHfpAg"

static void* IBtHfpAg_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtHfpAg_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtHfpAg_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    bt_address_t addr;
    uint32_t status;

    hfp_ag_interface_t* profile = (hfp_ag_interface_t*)service_manager_get_profile(PROFILE_HFP_AG);
    if (!profile)
        return stat;

    switch (code) {
    case IHFP_AG_REGISTER_CALLBACK: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtHfpAgCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        void* cookie = profile->register_callbacks(remote, BpBtHfpAgCallbacks_getStatic());
        stat = AParcel_writeUint32(reply, (uint32_t)cookie);
        break;
    }
    case IHFP_AG_UNREGISTER_CALLBACK: {
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
    case IHFP_AG_IS_CONNECTED: {
        bool ret;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        ret = profile->is_connected(&addr);
        stat = AParcel_writeBool(reply, ret);
        break;
    }
    case IHFP_AG_IS_AUDIO_CONNECTED: {
        bool ret;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        ret = profile->is_audio_connected(&addr);
        stat = AParcel_writeBool(reply, ret);
        break;
    }
    case IHFP_AG_GET_CONNECTION_STATE: {
        profile_connection_state_t state;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        state = profile->get_connection_state(&addr);
        stat = AParcel_writeUint32(reply, state);
        break;
    }
    case IHFP_AG_CONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_AG_DISCONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_AG_AUDIO_CONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect_audio(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_AG_AUDIO_DISCONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect_audio(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_AG_START_VOICE_RECOGNITION: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->start_voice_recognition(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_AG_STOP_VOICE_RECOGNITION: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->stop_voice_recognition(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtHfpAg_getClass(void)
{

    AIBinder_Class* clazz = AIBinder_Class_define(BT_HFP_AG_DESC, IBtHfpAg_Class_onCreate,
        IBtHfpAg_Class_onDestroy, IBtHfpAg_Class_onTransact);

    return clazz;
}

static AIBinder* BtHfpAg_getBinder(IBtHfpAg* hfpAg)
{
    AIBinder* binder = NULL;

    if (hfpAg->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(hfpAg->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(hfpAg->clazz, (void*)hfpAg);
        if (hfpAg->WeakBinder != NULL) {
            AIBinder_Weak_delete(hfpAg->WeakBinder);
        }

        hfpAg->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtHfpAg_addService(IBtHfpAg* hfpAg, const char* instance)
{
    hfpAg->clazz = (AIBinder_Class*)BtHfpAg_getClass();
    AIBinder* binder = BtHfpAg_getBinder(hfpAg);
    hfpAg->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtHfpAg* BpBtHfpAg_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtHfpAg* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtHfpAg_getClass();
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

void BpBtHfpAg_delete(BpBtHfpAg* bpHfpAg)
{
    AIBinder_decStrong(bpHfpAg->binder);
    free(bpHfpAg);
}

AIBinder* BtHfpAg_getService(BpBtHfpAg** bpHfpAg, const char* instance)
{
    BpBtHfpAg* bpBinder = *bpHfpAg;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtHfpAg_new(instance);
    if (!bpBinder)
        return NULL;

    *bpHfpAg = bpBinder;

    return bpBinder->binder;
}
