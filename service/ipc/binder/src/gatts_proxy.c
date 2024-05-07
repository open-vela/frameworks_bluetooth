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

#include "gatts_proxy.h"
#include "gatts_stub.h"
#include "parcel.h"
#include "utils/log.h"

void* BpBtGattServer_registerService(BpBtGattServer* bpBinder, AIBinder* cbksBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t handle;

    if (!bpBinder || !bpBinder->binder)
        return NULL;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeStrongBinder(parcelIn, cbksBinder);
    if (stat != STATUS_OK)
        return NULL;

    stat = AIBinder_transact(binder, IGATT_SERVER_REGISTER_SERVICE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &handle);
    if (stat != STATUS_OK)
        return NULL;

    return (void*)handle;
}

bt_status_t BpBtGattServer_unregisterService(BpBtGattServer* bpBinder, void* handle)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_UNREGISTER_SERVICE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtGattServer_connect(BpBtGattServer* bpBinder, void* handle, bt_address_t* addr, ble_addr_type_t addr_type)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)addr_type);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtGattServer_disconnect(BpBtGattServer* bpBinder, void* handle)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtGattServer_createServiceTable(BpBtGattServer* bpBinder, void* handle, gatt_srv_db_t* srv_db)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t state;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    if (!srv_db)
        return BT_STATUS_PARM_INVALID;

    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeServiceTable(parcelIn, srv_db->attr_db, srv_db->attr_num);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_CREATE_SERVICE_TABLE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &state);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return state;
}

bt_status_t BpBtGattServer_start(BpBtGattServer* bpBinder, void* handle)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_START, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtGattServer_stop(BpBtGattServer* bpBinder, void* handle)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_STOP, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtGattServer_response(BpBtGattServer* bpBinder, void* handle, uint32_t req_handle, uint8_t* value, uint16_t length)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)req_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)value, length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_RESPONSE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtGattServer_notify(BpBtGattServer* bpBinder, void* handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)value, length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_NOTIFY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtGattServer_indicate(BpBtGattServer* bpBinder, void* handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
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

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)value, length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_SERVER_INDICATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}
