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

#include "gattc_proxy.h"
#include "gattc_stub.h"
#include "parcel.h"
#include "utils/log.h"

void *BpBleGattClient_createConnect(BpBleGattClient *bpBinder, AIBinder *cbksBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t handle;

    if (!bpBinder || !bpBinder->binder)
        return NULL;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeStrongBinder(parcelIn, cbksBinder);
    if (stat != STATUS_OK)
        return NULL;

    stat = AIBinder_transact(binder, IGATT_CLIENT_CREATE_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &handle);
    if (stat != STATUS_OK)
        return NULL;

    return (void *)handle;
}

bt_status_t BpBleGattClient_deleteConnect(BpBleGattClient *bpBinder, void *handle)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_DELETE_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_connect(BpBleGattClient *bpBinder, void *handle, bt_address_t *addr, ble_addr_type_t addr_type)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

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

    stat = AIBinder_transact(binder, IGATT_CLIENT_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_disconnect(BpBleGattClient *bpBinder, void *handle)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_discoverService(BpBleGattClient *bpBinder, void *handle, bt_uuid_t *filter_uuid)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t state;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUuid(parcelIn, filter_uuid);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_DISCOVER_SERVICE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &state);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return state;
}

bt_status_t BpBleGattClient_getAttributeByHandle(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle, gatt_attr_desc_t *attr_desc)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_GET_ATTRIBUTE_BY_HANDLE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    // stat = AParcel_readAttrDesc(parcelOut, attr_desc);
    // if (stat != STATUS_OK)
    //     return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_getAttributeByUUID(BpBleGattClient *bpBinder, void *handle, bt_uuid_t *attr_uuid, gatt_attr_desc_t *attr_desc)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUuid(parcelIn, attr_uuid);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_GET_ATTRIBUTE_BY_UUID, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    // stat = AParcel_readAttrDesc(parcelOut, attr_desc);
    // if (stat != STATUS_OK)
    //     return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_read(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle, gattc_read_cb_t read_cb)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_READ, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_write(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle, uint8_t *value, uint16_t length, uint16_t offset, gattc_write_cb_t write_cb)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t *)value, length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_WRITE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_writeWithoutResponse(BpBleGattClient *bpBinder, void *handle, uint16_t attr_handle, uint8_t *value, uint16_t length, gattc_write_cb_t write_cb)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)attr_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t *)value, length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)length);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_WRITE_WITHOUT_RESPONSE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_subscribe(BpBleGattClient *bpBinder, void *handle, uint16_t value_handle, uint16_t cccd_handle, gattc_write_cb_t write_cb, gattc_notify_cb_t notify_cb)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)value_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)cccd_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_SUBSCRIBE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_unsubscribe(BpBleGattClient *bpBinder, void *handle, uint16_t value_handle, uint16_t cccd_handle, gattc_write_cb_t write_cb)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)value_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)cccd_handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_UNSUBSCRIBE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBleGattClient_exchangeMtu(BpBleGattClient *bpBinder, void *handle, uint32_t mtu)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    uint32_t status;

    if (!bpBinder || !bpBinder->binder)
        return BT_STATUS_PARM_INVALID;

    AIBinder *binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)handle);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)mtu);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IGATT_CLIENT_EXCHANGE_MTU, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}
