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
#include "hfp_hf_service.h"
#include "service_manager.h"

#include "hfp_hf_callbacks_proxy.h"
#include "hfp_hf_callbacks_stub.h"
#include "hfp_hf_proxy.h"
#include "hfp_hf_stub.h"
#include "parcel.h"
#include "utils/log.h"

#define BT_HFP_HF_DESC "BluetoothHfpHf"

static void* IBtHfpHf_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtHfpHf_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtHfpHf_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    bt_address_t addr;
    uint32_t status;

    hfp_hf_interface_t* profile = (hfp_hf_interface_t*)service_manager_get_profile(PROFILE_HFP_HF);
    if (!profile)
        return stat;

    switch (code) {
    case IHFP_HF_REGISTER_CALLBACK: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtHfpHfCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        void* cookie = profile->register_callbacks(remote, BpBtHfpHfCallbacks_getStatic());
        stat = AParcel_writeUint32(reply, (uint32_t)cookie);
        break;
    }
    case IHFP_HF_UNREGISTER_CALLBACK: {
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
    case IHFP_HF_IS_CONNECTED: {
        bool ret;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        ret = profile->is_connected(&addr);
        stat = AParcel_writeBool(reply, ret);
        break;
    }
    case IHFP_HF_IS_AUDIO_CONNECTED: {
        bool ret;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        ret = profile->is_audio_connected(&addr);
        stat = AParcel_writeBool(reply, ret);
        break;
    }
    case IHFP_HF_GET_CONNECTION_STATE: {
        profile_connection_state_t state;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        state = profile->get_connection_state(&addr);
        stat = AParcel_writeUint32(reply, state);
        break;
    }
    case IHFP_HF_CONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_DISCONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_AUDIO_CONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect_audio(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_AUDIO_DISCONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect_audio(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_START_VOICE_RECOGNITION: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->start_voice_recognition(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_STOP_VOICE_RECOGNITION: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->stop_voice_recognition(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_DIAL: {
        char* number = NULL;
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &number, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        status = profile->dial(&addr, number);
        free(number);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_DIAL_MEMORY: {
        uint32_t memory;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &memory);
        if (stat != STATUS_OK)
            return stat;

        status = profile->dial_memory(&addr, memory);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_REDIAL: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->redial(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_ACCEPT_CALL: {
        uint32_t flag;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &flag);
        if (stat != STATUS_OK)
            return stat;

        status = profile->accept_call(&addr, flag);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_REJECT_CALL: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->reject_call(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_HOLD_CALL: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->hold_call(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_TERMINATE_CALL: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->terminate_call(&addr);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_CONTROL_CALL: {
        uint32_t chld, index;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &chld);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &index);
        if (stat != STATUS_OK)
            return stat;

        status = profile->control_call(&addr, chld, (uint8_t)index);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_QUERY_CURRENT_CALL: {
        hfp_current_call_t* calls = NULL;
        int num = 0;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = profile->query_current_calls(&addr, &calls, &num, AParcelUtils_btCommonAllocator);
        stat = AParcel_writeCallArray(reply, calls, num);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IHFP_HF_SEND_AT_CMD: {
        char* cmd = NULL;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &cmd, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        status = profile->send_at_cmd(&addr, cmd);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtHfpHf_getClass(void)
{

    AIBinder_Class* clazz = AIBinder_Class_define(BT_HFP_HF_DESC, IBtHfpHf_Class_onCreate,
        IBtHfpHf_Class_onDestroy, IBtHfpHf_Class_onTransact);

    return clazz;
}

static AIBinder* BtHfpHf_getBinder(IBtHfpHf* hfpHf)
{
    AIBinder* binder = NULL;

    if (hfpHf->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(hfpHf->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(hfpHf->clazz, (void*)hfpHf);
        if (hfpHf->WeakBinder != NULL) {
            AIBinder_Weak_delete(hfpHf->WeakBinder);
        }

        hfpHf->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtHfpHf_addService(IBtHfpHf* hfpHf, const char* instance)
{
    hfpHf->clazz = (AIBinder_Class*)BtHfpHf_getClass();
    AIBinder* binder = BtHfpHf_getBinder(hfpHf);
    hfpHf->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtHfpHf* BpBtHfpHf_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtHfpHf* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtHfpHf_getClass();
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

void BpBtHfpHf_delete(BpBtHfpHf* bpHfpHf)
{
    AIBinder_decStrong(bpHfpHf->binder);
    free(bpHfpHf);
}

AIBinder* BtHfpHf_getService(BpBtHfpHf** bpHfpHf, const char* instance)
{
    BpBtHfpHf* bpBinder = *bpHfpHf;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtHfpHf_new(instance);
    if (!bpBinder)
        return NULL;

    *bpHfpHf = bpBinder;

    return bpBinder->binder;
}
