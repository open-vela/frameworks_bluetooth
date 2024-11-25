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
#include <stdio.h>
#include <uchar.h>

#include <android/binder_manager.h>

#include "bluetooth.h"
#include "bt_adapter.h"

#include "adapter_proxy.h"
#include "adapter_stub.h"
#include "parcel.h"
#include "utils/log.h"

void* BpBtAdapter_registerCallback(BpBtAdapter* bpBinder, AIBinder* cbksBinder)
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

    stat = AIBinder_transact(binder, IBTADAPTER_REGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &cookie);
    if (stat != STATUS_OK)
        return NULL;

    return (void*)cookie;
}

bool BpBtAdapter_unRegisterCallback(BpBtAdapter* bpBinder, void* cookie)
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

    stat = AIBinder_transact(binder, IBTADAPTER_UNREGISTER_CALLBACK, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bt_status_t BpBtAdapter_enable(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_ENABLE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_disable(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_DISABLE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_enableLe(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_ENABLE_LE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_disableLe(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_DISABLE_LE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_adapter_state_t BpBtAdapter_getState(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bt_adapter_state_t state;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_ADAPTER_STATE_OFF;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_ADAPTER_STATE_OFF;

    stat = AParcel_readUint32(parcelOut, &state);
    if (stat != STATUS_OK)
        return BT_ADAPTER_STATE_OFF;

    return state;
}

bool BpBtAdapter_isLeEnabled(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IBTADAPTER_IS_LE_ENABLED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bt_device_type_t BpBtAdapter_getType(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bt_device_type_t type;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_DEVICE_TYPE_UNKNOW;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_TYPE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_DEVICE_TYPE_UNKNOW;

    stat = AParcel_readUint32(parcelOut, &type);
    if (stat != STATUS_OK)
        return BT_DEVICE_TYPE_UNKNOW;

    return type;
}

bt_status_t BpBtAdapter_setDiscoveryFilter(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_DISCOVERY_FILTER, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_startDiscovery(BpBtAdapter* bpBinder, uint32_t timeout)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, timeout);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_START_DISCOVERY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_cancelDiscovery(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_CANCEL_DISCOVERY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bool BpBtAdapter_isDiscovering(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IBTADAPTER_IS_DISCOVERING, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bt_status_t BpBtAdapter_getAddress(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_ADDR, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readAddress(parcelOut, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return BT_STATUS_SUCCESS;
}

bt_status_t BpBtAdapter_setName(BpBtAdapter* bpBinder, const char* name)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeString(parcelIn, name, strlen(name));
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_NAME, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_getName(BpBtAdapter* bpBinder, char* name, int length)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    char* btName = NULL;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_NAME, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readString(parcelOut, &btName, AParcelUtils_stringAllocator);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    snprintf(name, length, "%s", btName);
    free(btName);

    return BT_STATUS_SUCCESS;
}

bt_status_t BpBtAdapter_getUuids(BpBtAdapter* bpBinder, bt_uuid_t* uuids, uint16_t* size)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t BpBtAdapter_setScanMode(BpBtAdapter* bpBinder, bt_scan_mode_t mode, bool bondable)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, mode);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, bondable);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_SCAN_MODE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_scan_mode_t BpBtAdapter_getScanMode(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t mode;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_BR_SCAN_MODE_NONE;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_SCAN_MODE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_BR_SCAN_MODE_NONE;

    stat = AParcel_readUint32(parcelOut, &mode);
    if (stat != STATUS_OK)
        return BT_BR_SCAN_MODE_NONE;

    return mode;
}

bt_status_t BpBtAdapter_setDeviceClass(BpBtAdapter* bpBinder, uint32_t cod)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, cod);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_DEVICE_CLASS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

uint32_t BpBtAdapter_getDeviceClass(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t cod;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_DEVICE_CLASS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_readUint32(parcelOut, &cod);
    if (stat != STATUS_OK)
        return 0;

    return cod;
}

bt_status_t BpBtAdapter_setIOCapability(BpBtAdapter* bpBinder, bt_io_capability_t cap)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, cap);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_IO_CAP, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_io_capability_t BpBtAdapter_getIOCapability(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t io;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_IO_CAPABILITY_UNKNOW;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_IO_CAP, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_IO_CAPABILITY_UNKNOW;

    stat = AParcel_readUint32(parcelOut, &io);
    if (stat != STATUS_OK)
        return BT_IO_CAPABILITY_UNKNOW;

    return io;
}

bt_status_t BpBtAdapter_SetLeIOCapability(BpBtAdapter* bpBinder, uint32_t le_io_cap)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, le_io_cap);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_LE_IO_CAP, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

uint32_t BpBtAdapter_getLeIOCapability(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t le_io;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_IO_CAPABILITY_UNKNOW;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_LE_IO_CAP, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_IO_CAPABILITY_UNKNOW;

    stat = AParcel_readUint32(parcelOut, &le_io);
    if (stat != STATUS_OK)
        return BT_IO_CAPABILITY_UNKNOW;

    return le_io;
}

bt_status_t BpBtAdapter_getLeAddress(BpBtAdapter* bpBinder, bt_address_t* addr, ble_addr_type_t* type)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_LE_ADDR, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readAddress(parcelOut, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, type);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return BT_STATUS_SUCCESS;
}

bt_status_t BpBtAdapter_setLeAddress(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_LE_ADDR, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_setLeIdentityAddress(BpBtAdapter* bpBinder, bt_address_t* addr, bool public)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, public);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_LE_ID, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_setLeAppearance(BpBtAdapter* bpBinder, uint16_t appearance)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)appearance);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_SET_LE_APPEARANCE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

uint16_t BpBtAdapter_getLeAppearance(BpBtAdapter* bpBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t appearance;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_LE_APPEARANCE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_readUint32(parcelOut, &appearance);
    if (stat != STATUS_OK)
        return 0;

    return appearance;
}

bt_status_t BpBtAdapter_getBondedDevices(BpBtAdapter* bpBinder, bt_address_t** addr, int* num, bt_allocator_t allocator)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_BONDED_DEVICES, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readAddressArray(parcelOut, addr, num, allocator);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_getConnectedDevices(BpBtAdapter* bpBinder, bt_address_t** addr, int* num, bt_allocator_t allocator)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_GET_CONNECTED_DEVICES, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readAddressArray(parcelOut, addr, num, allocator);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

void BpBtAdapter_disconnectAllDevices(BpBtAdapter* bpBinder)
{
}

bool BpBtAdapter_isSupportBredr(BpBtAdapter* bpBinder)
{
    return false;
}

bool BpBtAdapter_isSupportLe(BpBtAdapter* bpBinder)
{
    return false;
}

bool BpBtAdapter_isSupportLeaudio(BpBtAdapter* bpBinder)
{
    return false;
}

bt_status_t BpBtAdapter_leEnableKeyDerivation(BpBtAdapter* bpBinder,
    bool brkey_to_lekey,
    bool lekey_to_brkey)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, brkey_to_lekey);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, lekey_to_brkey);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_ENABLE_KEY_DERIVATION, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_advertiser_t* BpBtAdapter_startAdvertising(BpBtAdapter* bpBinder,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    AIBinder* cbksBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t adver;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeStrongBinder(parcelIn, cbksBinder);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeBleAdvParam(parcelIn, params);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)adv_data, adv_len);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeUint32(parcelIn, adv_len);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeByteArray(parcelIn, (const int8_t*)scan_rsp_data, scan_rsp_len);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeUint32(parcelIn, scan_rsp_len);
    if (stat != STATUS_OK)
        return NULL;

    stat = AIBinder_transact(binder, IBTADAPTER_START_ADVERTISING, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &adver);
    if (stat != STATUS_OK)
        return NULL;

    return (bt_advertiser_t*)adver;
}

bt_status_t BpBtAdapter_stopAdvertising(BpBtAdapter* bpBinder, bt_advertiser_t* adver)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)adver);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_STOP_ADVERTISING, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return BT_STATUS_SUCCESS;
}

bt_status_t BpBtAdapter_stopAdvertisingId(BpBtAdapter* bpBinder, uint8_t adver_id)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)adver_id);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_STOP_ADVERTISING_ID, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return BT_STATUS_SUCCESS;
}

bt_scanner_t* BpBtAdapter_startScan(BpBtAdapter* bpBinder, AIBinder* cbksBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t scanner;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeStrongBinder(parcelIn, cbksBinder);
    if (stat != STATUS_OK)
        return NULL;

    stat = AIBinder_transact(binder, IBTADAPTER_START_SCAN, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &scanner);
    if (stat != STATUS_OK)
        return NULL;

    return (bt_scanner_t*)scanner;
}

bt_scanner_t* BpBtAdapter_startScanSettings(BpBtAdapter* bpBinder,
    ble_scan_settings_t* settings,
    AIBinder* cbksBinder)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t scanner;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeStrongBinder(parcelIn, cbksBinder);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeInt32(parcelIn, settings->scan_mode);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeBool(parcelIn, settings->legacy);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_writeUint32(parcelIn, settings->scan_phy);
    if (stat != STATUS_OK)
        return NULL;

    stat = AIBinder_transact(binder, IBTADAPTER_START_SCAN_SETTINGS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return NULL;

    stat = AParcel_readUint32(parcelOut, &scanner);
    if (stat != STATUS_OK)
        return NULL;

    return (bt_scanner_t*)scanner;
}

bt_status_t BpBtAdapter_stopScan(BpBtAdapter* bpBinder, bt_scanner_t* scanner)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, (uint32_t)scanner);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IBTADAPTER_STOP_SCAN, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return BT_STATUS_SUCCESS;
}

bt_device_type_t BpBtAdapter_getRemoteDeviceType(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bt_device_type_t device_type;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return 0;

    stat = AIBinder_transact(binder, IREMOTE_GET_DEVICE_TYPE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_readUint32(parcelOut, &device_type);
    if (stat != STATUS_OK)
        return 0;

    return device_type;
}

bool BpBtAdapter_getRemoteName(BpBtAdapter* bpBinder, bt_address_t* addr, char* name, uint32_t length)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    char* remoteName = NULL;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IREMOTE_GET_NAME, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readString(parcelOut, &remoteName, AParcelUtils_stringAllocator);
    if (stat != STATUS_OK)
        return false;

    snprintf(name, length, "%s", remoteName);
    free(remoteName);

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

uint32_t BpBtAdapter_getRemoteDeviceClass(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t cod;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return 0;

    stat = AIBinder_transact(binder, IREMOTE_GET_DEVICE_CLASS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_readUint32(parcelOut, &cod);
    if (stat != STATUS_OK)
        return 0;

    return cod;
}

bt_status_t BpBtAdapter_getRemoteUuids(BpBtAdapter* bpBinder, bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;
    bt_uuid_t* uuidArray = NULL;
    int uuidSize = 0;
    int length;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_GET_UUIDS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUuidArray(parcelOut, (bt_uuid_t*)&uuidArray, &uuidSize);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    *size = (uint16_t)uuidSize;
    if (uuidSize) {
        length = sizeof(bt_uuid_t) * uuidSize;
        if (!allocator((void**)uuids, length)) {
            free(uuidArray);
            return BT_STATUS_NOMEM;
        }

        memcpy(*uuids, uuidArray, length);
        free(uuidArray);
    }

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

uint16_t BpBtAdapter_getRemoteAppearance(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t appearance;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return 0;

    stat = AIBinder_transact(binder, IREMOTE_GET_APPEARANCE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return 0;

    stat = AParcel_readUint32(parcelOut, &appearance);
    if (stat != STATUS_OK)
        return 0;

    return (uint16_t)appearance;
}

int8_t BpBtAdapter_getRemoteRssi(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    int8_t rssi;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return 0;
    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return 0;
    stat = AIBinder_transact(binder, IREMOTE_GET_RSSI, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return 0;
    stat = AParcel_readByte(parcelOut, &rssi);
    if (stat != STATUS_OK)
        return 0;

    return rssi;
}

bool BpBtAdapter_getRemoteAlias(BpBtAdapter* bpBinder, bt_address_t* addr, char* alias, uint32_t length)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    char* Alias = NULL;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IREMOTE_GET_ALIAS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readString(parcelOut, &Alias, AParcelUtils_stringAllocator);
    if (stat != STATUS_OK)
        return false;

    snprintf(alias, length, "%s", Alias);
    free(Alias);

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bt_status_t BpBtAdapter_setRemoteAlias(BpBtAdapter* bpBinder, bt_address_t* addr, const char* alias)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeString(parcelIn, alias, strlen(alias));
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_SET_ALIAS, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bool BpBtAdapter_isRemoteConnected(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IREMOTE_IS_CONNECTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bool BpBtAdapter_isRemoteEncrypted(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IREMOTE_IS_ENCRYPTED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bool BpBtAdapter_isBondInitiateLocal(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IREMOTE_IS_BOND_INIT_LOCAL, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bond_state_t BpBtAdapter_getRemoteBondState(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bond_state_t state;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BOND_STATE_NONE;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BOND_STATE_NONE;

    stat = AIBinder_transact(binder, IREMOTE_GET_BOND_STATE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BOND_STATE_NONE;

    stat = AParcel_readUint32(parcelOut, &state);
    if (stat != STATUS_OK)
        return BOND_STATE_NONE;

    return state;
}

bool BpBtAdapter_isRemoteBonded(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    bool ret;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return false;

    stat = AIBinder_transact(binder, IREMOTE_IS_BONDED, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return false;

    stat = AParcel_readBool(parcelOut, &ret);
    if (stat != STATUS_OK)
        return false;

    return ret;
}

bt_status_t BpBtAdapter_connect(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_CONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_disconnect(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_DISCONNECT, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_leConnect(BpBtAdapter* bpBinder, bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param)
{

    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, type);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBleConnectParam(parcelIn, param);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_CONNECT_LE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_leDisconnect(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_DISCONNECT_LE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_leSetPhy(BpBtAdapter* bpBinder, bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, tx_phy);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, rx_phy);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_SET_LE_PHY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_createBond(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, transport);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_CREATE_BOND, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_removeBond(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, transport);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_REMOVE_BOND, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_cancelBond(BpBtAdapter* bpBinder, bt_address_t* addr)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_CANCEL_BOND, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_pairRequestReply(BpBtAdapter* bpBinder, bt_address_t* addr, bool accept)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, accept);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_PAIR_REQUEST_REPLY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_setPairingConfirmation(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport, bool accept)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, transport);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, accept);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_SET_PAIRING_CONFIRM, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_setPinCode(BpBtAdapter* bpBinder, bt_address_t* addr, bool accept,
    char* pincode, int len)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, accept);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeString(parcelIn, pincode, len);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, len);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_SET_PIN_CODE, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}

bt_status_t BpBtAdapter_setPassKey(BpBtAdapter* bpBinder, bt_address_t* addr, bt_transport_t transport, bool accept, uint32_t passkey)
{
    binder_status_t stat = STATUS_OK;
    AParcel *parcelIn, *parcelOut;
    AIBinder* binder = bpBinder->binder;
    uint32_t status;

    stat = AIBinder_prepareTransaction(binder, &parcelIn);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeAddress(parcelIn, addr);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, transport);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeBool(parcelIn, accept);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_writeUint32(parcelIn, passkey);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AIBinder_transact(binder, IREMOTE_SET_PASSKEY, &parcelIn, &parcelOut, 0 /*flags*/);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    stat = AParcel_readUint32(parcelOut, &status);
    if (stat != STATUS_OK)
        return BT_STATUS_IPC_ERROR;

    return status;
}
