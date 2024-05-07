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

// #include <android/binder_auto_utils.h>
#include <android/binder_manager.h>

#include "bluetooth.h"
#include "bt_adapter.h"

#include "hid_device_proxy.h"
#include "hid_device_stub.h"
#include "parcel.h"
#include "utils/log.h"

void* BpBtHidd_registerCallback(BpBtHidd* bpBinder, AIBinder* cbksBinder)
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

    stat = AIBinder_transact(binder, IHIDD_REGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &cookie);
    if (stat != STATUS_OK)
        return NULL;

    return (void*)cookie;
}

bool BpBtHidd_unRegisterCallback(BpBtHidd* bpBinder, void* cookie)
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

    stat = AIBinder_transact(binder, IHIDD_UNREGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bt_status_t BpBtHidd_registerApp(BpBtHidd* bpBinder, hid_device_sdp_settings_t* sdp, bool le_hid)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    if (!sdp)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeString(parcelIn, sdp->name, strlen(sdp->name));
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeString(parcelIn, sdp->description, strlen(sdp->description));
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeString(parcelIn, sdp->provider, strlen(sdp->provider));
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.attr_mask);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.sub_class);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.country_code);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.vendor_id);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.product_id);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.version);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.supervision_timeout);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.ssr_max_latency);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.ssr_min_timeout);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)sdp->hids_info.dsc_list_length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)sdp->hids_info.dsc_list, sdp->hids_info.dsc_list_length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, le_hid);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHIDD_REGISTER_APP, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHidd_unregisterApp(BpBtHidd* bpBinder)
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

    stat = AIBinder_transact(binder, IHIDD_UNREGISTER_APP, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHidd_connect(BpBtHidd* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHIDD_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHidd_disconnect(BpBtHidd* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHIDD_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHidd_sendReport(BpBtHidd* bpBinder, bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_id);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)rpt_data, rpt_size);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeInt32(parcelIn, (int32_t)rpt_size);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHIDD_SEND_REPORT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHidd_responseReport(BpBtHidd* bpBinder, bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)rpt_type);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)rpt_data, rpt_size);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeInt32(parcelIn, (int32_t)rpt_size);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHIDD_RESPONSE_REPORT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHidd_reportError(BpBtHidd* bpBinder, bt_address_t* addr, hid_status_error_t error)
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

    stat = AParcel_writeInt32(parcelIn, (int32_t)error);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IHIDD_REPORT_ERROR, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtHidd_virtualUnplug(BpBtHidd* bpBinder, bt_address_t* addr)
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

    stat = AIBinder_transact(binder, IHIDD_VIRTUAL_UNPLUG, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}
