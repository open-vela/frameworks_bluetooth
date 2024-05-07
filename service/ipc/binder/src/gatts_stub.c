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
#include "gatts_service.h"
#include "service_manager.h"

#include "gatts_callbacks_proxy.h"
#include "gatts_callbacks_stub.h"
#include "gatts_proxy.h"
#include "gatts_stub.h"
#include "parcel.h"
#include "utils/log.h"

#define BT_GATT_SERVER_DESC "BluetoothGattServer"

static void* IBtGattServer_Class_onCreate(void* arg)
{
    BT_LOGD("%s", __func__);
    return arg;
}

static void IBtGattServer_Class_onDestroy(void* userData)
{
    BT_LOGD("%s", __func__);
}

static binder_status_t IBtGattServer_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    uint32_t handle;
    uint32_t status;

    gatts_interface_t* profile = (gatts_interface_t*)service_manager_get_profile(PROFILE_GATTS);
    if (!profile)
        return stat;

    switch (code) {
    case IGATT_SERVER_REGISTER_SERVICE: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtGattServerCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        if (profile->register_service((void*)remote, (void**)&handle, (gatts_callbacks_t*)BpBtGattServerCallbacks_getStatic()) != BT_STATUS_SUCCESS) {
            AIBinder_decStrong(remote);
            stat = AParcel_writeUint32(reply, (uint32_t)NULL);
        } else {
            stat = AParcel_writeUint32(reply, (uint32_t)handle);
        }
        break;
    }
    case IGATT_SERVER_UNREGISTER_SERVICE: {
        AIBinder* remote = NULL;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        remote = if_gatts_get_remote((void*)handle);
        status = profile->unregister_service((void*)handle);
        if (status == BT_STATUS_SUCCESS)
            AIBinder_decStrong(remote);

        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_SERVER_CONNECT: {
        bt_address_t addr;
        uint32_t addr_type;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &addr_type);
        if (stat != STATUS_OK)
            return stat;

        status = profile->connect((void*)handle, &addr, addr_type);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_SERVER_DISCONNECT: {
        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect((void*)handle);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_SERVER_CREATE_SERVICE_TABLE: {
        gatt_srv_db_t srv_db;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readServiceTable(in, &srv_db.attr_db, &srv_db.attr_num);
        if (stat != STATUS_OK) // cleanup srv_db.attr_db ?
            return stat;

        gatt_attr_db_t* attr_inst = srv_db.attr_db;
        for (int i = 0; i < srv_db.attr_num; i++, attr_inst++) {
            if (attr_inst->read_cb)
                attr_inst->read_cb = BpBtGattServerCallbacks_onRead;
            if (attr_inst->write_cb)
                attr_inst->write_cb = BpBtGattServerCallbacks_onWrite;
        }

        status = profile->create_service_table((void*)handle, &srv_db);
        stat = AParcel_writeUint32(reply, status);
        attr_inst = srv_db.attr_db;
        for (int i = 0; i < srv_db.attr_num; i++, attr_inst++) {
            free(attr_inst->uuid);
            // free(attr_inst->attr_value);
        }
        free(srv_db.attr_db);
        break;
    }
    case IGATT_SERVER_START: {
        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->start((void*)handle);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_SERVER_STOP: {
        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->stop((void*)handle);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_SERVER_RESPONSE: {
        uint32_t req_handle;
        uint8_t* value;
        uint32_t length;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &req_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &length);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&value, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        status = profile->response((void*)handle, req_handle, value, (uint16_t)length);
        free(value);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_SERVER_NOTIFY: {
        uint32_t attr_handle;
        uint8_t* value;
        uint32_t length;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &length);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&value, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        status = profile->notify((void*)handle, (uint16_t)attr_handle, value, (uint16_t)length);
        free(value);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_SERVER_INDICATE: {
        uint32_t attr_handle;
        uint8_t* value;
        uint32_t length;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &length);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&value, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        status = profile->indicate((void*)handle, (uint16_t)attr_handle, value, (uint16_t)length);
        free(value);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtGattServer_getClass(void)
{

    AIBinder_Class* clazz = AIBinder_Class_define(BT_GATT_SERVER_DESC, IBtGattServer_Class_onCreate,
        IBtGattServer_Class_onDestroy, IBtGattServer_Class_onTransact);

    return clazz;
}

static AIBinder* BtGattServer_getBinder(IBtGattServer* iGatts)
{
    AIBinder* binder = NULL;

    if (iGatts->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(iGatts->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(iGatts->clazz, (void*)iGatts);
        if (iGatts->WeakBinder != NULL) {
            AIBinder_Weak_delete(iGatts->WeakBinder);
        }

        iGatts->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtGattServer_addService(IBtGattServer* iGatts, const char* instance)
{
    iGatts->clazz = (AIBinder_Class*)BtGattServer_getClass();
    AIBinder* binder = BtGattServer_getBinder(iGatts);
    iGatts->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtGattServer* BpBtGattServer_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtGattServer* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtGattServer_getClass();
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

void BpBtGattServer_delete(BpBtGattServer* bpBinder)
{
    AIBinder_decStrong(bpBinder->binder);
    free(bpBinder);
}

AIBinder* BtGattServer_getService(BpBtGattServer** bpGatts, const char* instance)
{
    BpBtGattServer* bpBinder = *bpGatts;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtGattServer_new(instance);
    if (!bpBinder)
        return NULL;

    *bpGatts = bpBinder;

    return bpBinder->binder;
}
