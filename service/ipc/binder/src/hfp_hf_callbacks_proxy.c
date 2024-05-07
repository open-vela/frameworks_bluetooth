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
#include "hfp_hf_callbacks_stub.h"
#include "hfp_hf_proxy.h"
#include "hfp_hf_stub.h"
#include "parcel.h"

#include "utils/log.h"

static void BpBtHfpHfCallbacks_connectionStateCallback(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_CONNECTION_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_audioStateCallback(void* cookie, bt_address_t* addr, hfp_audio_state_t state)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeUint32(parcelIn, state);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_AUDIO_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_vrCmdCallback(void* cookie, bt_address_t* addr, bool started)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeBool(parcelIn, started);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_VR_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_callStateChangeCallback(void* cookie, bt_address_t* addr, hfp_current_call_t* call)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    /* write call */
    stat = AParcel_writeCall(parcelIn, call);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_CALL_STATE_CHANGE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_cmdCompleteCallback(void* cookie, bt_address_t* addr, const char* resp)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeString(parcelIn, resp, strlen(resp));
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_CMD_COMPLETE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_ringIndicationCallback(void* cookie, bt_address_t* addr, bool inband_ring_tone)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeBool(parcelIn, inband_ring_tone);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_RING_INDICATION, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

#if 0 /* not support */
static void BpBtHfpHfCallbacks_roamingChangedCallback(void *cookie, bt_address_t *addr, int status)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeInt32(parcelIn, status);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_ROAMING_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_networkStateChangedCallback(void *cookie, bt_address_t *addr, int status)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeInt32(parcelIn, status);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_NETWOEK_STATE_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_signalStrengthChangedCallback(void *cookie, bt_address_t *addr, int signal)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeInt32(parcelIn, signal);
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_SIGNAL_STRENGTH_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}

static void BpBtHfpHfCallbacks_operatorChangedCallback(void *cookie, bt_address_t *addr, char *name)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder *binder = cookie;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return;

    stat = AParcel_writeString(parcelIn, name, strlen(name));
    if (stat != STATUS_OK)
        return;

    stat = AIBinder_transact(binder, ICBKS_HFP_HF_OPERATOR_CHANGED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK) {
        BT_LOGE("%s transact error:%d", __func__, stat);
        return;
    }
}
#endif

static const hfp_hf_callbacks_t static_hfp_hf_cbks = {
    sizeof(static_hfp_hf_cbks),
    BpBtHfpHfCallbacks_connectionStateCallback,
    BpBtHfpHfCallbacks_audioStateCallback,
    BpBtHfpHfCallbacks_vrCmdCallback,
    BpBtHfpHfCallbacks_callStateChangeCallback,
    BpBtHfpHfCallbacks_cmdCompleteCallback,
    BpBtHfpHfCallbacks_ringIndicationCallback,
};

const hfp_hf_callbacks_t* BpBtHfpHfCallbacks_getStatic(void)
{
    return &static_hfp_hf_cbks;
}
