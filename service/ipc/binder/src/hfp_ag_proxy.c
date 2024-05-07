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
#include "bt_adapter.h"

#include "hfp_ag_proxy.h"
#include "hfp_ag_stub.h"
#include "parcel.h"
#include "utils/log.h"

void* BpBtHfpAg_registerCallback(BpBtHfpAg* bpBinder, AIBinder* cbksBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t cookie;

    if (!bpBinder || !bpBinder->binder)
        return NULL;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeStrongBinder(parcelIn, cbksBinder);
    if (stat != STATUS_OK)
        return NULL;

    stat = AIBinder_transact(binder, IHFP_AG_REGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &cookie);
    if (stat != STATUS_OK)
        return NULL;

    return (void*)cookie;
}

bool BpBtHfpAg_unRegisterCallback(BpBtHfpAg* bpBinder, void* cookie)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    bool ret;

    if (!bpBinder || !bpBinder->binder)
        return false;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)cookie);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IHFP_AG_UNREGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bool BpBtHfpAg_isConnected(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    bool ret;

    if (!bpBinder || !bpBinder->binder)
        return false;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IHFP_AG_IS_CONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bool BpBtHfpAg_isAudioConnected(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    bool ret;

    if (!bpBinder || !bpBinder->binder)
        return false;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IHFP_AG_IS_AUDIO_CONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

profile_connection_state_t BpBtHfpAg_getConnectionState(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t state;

    if (!bpBinder || !bpBinder->binder)
        return PROFILE_STATE_DISCONNECTED;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return PROFILE_STATE_DISCONNECTED;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return PROFILE_STATE_DISCONNECTED;

    stat = AIBinder_transact(binder, IHFP_AG_GET_CONNECTION_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return PROFILE_STATE_DISCONNECTED;

    stat = AParcel_readUint32(parcelOut, &state);
    if (stat != STATUS_OK)
        return PROFILE_STATE_DISCONNECTED;

    return state;
}

bt_status_t BpBtHfpAg_connect(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_AG_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpAg_disconnect(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_AG_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpAg_connectAudio(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_AG_AUDIO_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpAg_disconnectAudio(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_AG_AUDIO_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpAg_startVoiceRecognition(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_AG_START_VOICE_RECOGNITION, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpAg_stopVoiceRecognition(BpBtHfpAg* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_AG_STOP_VOICE_RECOGNITION, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}
