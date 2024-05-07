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

#include "hfp_hf_proxy.h"
#include "hfp_hf_stub.h"
#include "parcel.h"
#include "utils/log.h"

void* BpBtHfpHf_registerCallback(BpBtHfpHf* bpBinder, AIBinder* cbksBinder)
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

    stat = AIBinder_transact(binder, IHFP_HF_REGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &cookie);
    if (stat != STATUS_OK)
        return NULL;

    return (void*)cookie;
}

bool BpBtHfpHf_unRegisterCallback(BpBtHfpHf* bpBinder, void* cookie)
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

    stat = AIBinder_transact(binder, IHFP_HF_UNREGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bool BpBtHfpHf_isConnected(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_IS_CONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bool BpBtHfpHf_isAudioConnected(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_IS_AUDIO_CONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

profile_connection_state_t BpBtHfpHf_getConnectionState(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_GET_CONNECTION_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return PROFILE_STATE_DISCONNECTED;

    stat = AParcel_readUint32(parcelOut, &state);
    if (stat != STATUS_OK)
        return PROFILE_STATE_DISCONNECTED;

    return state;
}

bt_status_t BpBtHfpHf_connect(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_disconnect(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_connectAudio(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_AUDIO_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_disconnectAudio(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_AUDIO_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_startVoiceRecognition(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_START_VOICE_RECOGNITION, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_stopVoiceRecognition(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_STOP_VOICE_RECOGNITION, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_dial(BpBtHfpHf* bpBinder, bt_address_t* addr, const char* number)
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

    stat = AParcel_writeString(parcelIn, number, strlen(number));
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_HF_DIAL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_dialMemory(BpBtHfpHf* bpBinder, bt_address_t* addr, uint32_t memory)
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

    stat = AParcel_writeUint32(parcelIn, memory);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_HF_DIAL_MEMORY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_redial(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_REDIAL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_acceptCall(BpBtHfpHf* bpBinder, bt_address_t* addr, hfp_call_accept_t flag)
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

    stat = AParcel_writeUint32(parcelIn, flag);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_HF_ACCEPT_CALL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_rejectCall(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_REJECT_CALL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_holdCall(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_HOLD_CALL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_terminateCall(BpBtHfpHf* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHFP_HF_TERMINATE_CALL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_controlCall(BpBtHfpHf* bpBinder, bt_address_t* addr, hfp_call_control_t chld, uint8_t index)
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

    stat = AParcel_writeUint32(parcelIn, chld);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)index);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_HF_CONTROL_CALL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_queryCurrentCalls(BpBtHfpHf* bpBinder, bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator)
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

    stat = AIBinder_transact(binder, IHFP_HF_QUERY_CURRENT_CALL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readCallArray(parcelOut, calls, num, allocator);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHfpHf_sendAtCmd(BpBtHfpHf* bpBinder, bt_address_t* addr, const char* cmd)
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

    stat = AParcel_writeString(parcelIn, cmd, strlen(cmd));
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHFP_HF_SEND_AT_CMD, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}
