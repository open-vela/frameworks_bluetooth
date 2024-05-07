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
#include "gattc_service.h"
#include "service_manager.h"

#include "gattc_callbacks_proxy.h"
#include "gattc_callbacks_stub.h"
#include "gattc_proxy.h"
#include "gattc_stub.h"
#include "parcel.h"
#include "utils/log.h"

#define BT_GATT_CLIENT_DESC "BluetoothGattClient"

static void* IBtGattClient_Class_onCreate(void* arg)
{
    BT_LOGD("%s", __func__);
    return arg;
}

static void IBtGattClient_Class_onDestroy(void* userData)
{
    BT_LOGD("%s", __func__);
}

static binder_status_t IBtGattClient_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* reply)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    uint32_t handle;
    uint32_t status;

    gattc_interface_t* profile = (gattc_interface_t*)service_manager_get_profile(PROFILE_GATTC);
    if (!profile)
        return stat;

    switch (code) {
    case IGATT_CLIENT_CREATE_CONNECT: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtGattClientCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        if (profile->create_connect((void*)remote, (void**)&handle, (gattc_callbacks_t*)BpBtGattClientCallbacks_getStatic()) != BT_STATUS_SUCCESS) {
            AIBinder_decStrong(remote);
            stat = AParcel_writeUint32(reply, (uint32_t)NULL);
        } else {
            stat = AParcel_writeUint32(reply, (uint32_t)handle);
        }
        break;
    }
    case IGATT_CLIENT_DELETE_CONNECT: {
        AIBinder* remote = NULL;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        remote = if_gattc_get_remote((void*)handle);
        status = profile->delete_connect((void*)handle);
        if (status == BT_STATUS_SUCCESS)
            AIBinder_decStrong(remote);

        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_CONNECT: {
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
    case IGATT_CLIENT_DISCONNECT: {
        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->disconnect((void*)handle);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_DISCOVER_SERVICE: {
        bt_uuid_t uuid;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUuid(in, &uuid);
        if (stat != STATUS_OK)
            return stat;

        status = profile->discover_service((void*)handle, &uuid);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_GET_ATTRIBUTE_BY_HANDLE: {
        uint32_t attr_handle;
        gatt_attr_desc_t attr_desc;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->get_attribute_by_handle((void*)handle, attr_handle, &attr_desc);

        stat = AParcel_writeUint32(reply, (uint32_t)attr_desc.handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUuid(reply, &attr_desc.uuid);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, attr_desc.type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, attr_desc.properties);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_GET_ATTRIBUTE_BY_UUID: {
        bt_uuid_t uuid;
        gatt_attr_desc_t attr_desc;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUuid(in, &uuid);
        if (stat != STATUS_OK)
            return stat;

        status = profile->get_attribute_by_uuid((void*)handle, &uuid, &attr_desc);

        stat = AParcel_writeUint32(reply, (uint32_t)attr_desc.handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUuid(reply, &attr_desc.uuid);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, attr_desc.type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, attr_desc.properties);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_READ: {
        uint32_t attr_handle;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &attr_handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->read((void*)handle, attr_handle);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_WRITE: {
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

        status = profile->write((void*)handle, (uint16_t)attr_handle, value, (uint16_t)length);
        free(value);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_WRITE_WITHOUT_RESPONSE: {
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

        status = profile->write_without_response((void*)handle, (uint16_t)attr_handle, value, (uint16_t)length);
        free(value);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_SUBSCRIBE: {
        uint32_t value_handle;
        uint32_t cccd_handle;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &value_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &cccd_handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->subscribe((void*)handle, (uint16_t)value_handle, (uint16_t)cccd_handle);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_UNSUBSCRIBE: {
        uint32_t value_handle;
        uint32_t cccd_handle;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &value_handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &cccd_handle);
        if (stat != STATUS_OK)
            return stat;

        status = profile->unsubscribe((void*)handle, (uint16_t)value_handle, (uint16_t)cccd_handle);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_EXCHANGE_MTU: {
        uint32_t mtu;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &mtu);
        if (stat != STATUS_OK)
            return stat;

        status = profile->exchange_mtu((void*)handle, mtu);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    case IGATT_CLIENT_UPDATE_CONNECTION_PARAM: {
        uint32_t min_interval;
        uint32_t max_interval;
        uint32_t latency;
        uint32_t timeout;
        uint32_t min_connection_event_length;
        uint32_t max_connection_event_length;

        stat = AParcel_readUint32(in, &handle);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &min_interval);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &max_interval);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &latency);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &timeout);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &min_connection_event_length);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &max_connection_event_length);
        if (stat != STATUS_OK)
            return stat;

        status = profile->update_connection_parameter((void*)handle, min_interval, max_interval, latency,
            timeout, min_connection_event_length, max_connection_event_length);
        stat = AParcel_writeUint32(reply, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static const AIBinder_Class* BtGattClient_getClass(void)
{

    AIBinder_Class* clazz = AIBinder_Class_define(BT_GATT_CLIENT_DESC, IBtGattClient_Class_onCreate,
        IBtGattClient_Class_onDestroy, IBtGattClient_Class_onTransact);

    return clazz;
}

static AIBinder* BtGattClient_getBinder(IBtGattClient* iGattc)
{
    AIBinder* binder = NULL;

    if (iGattc->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(iGattc->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(iGattc->clazz, (void*)iGattc);
        if (iGattc->WeakBinder != NULL) {
            AIBinder_Weak_delete(iGattc->WeakBinder);
        }

        iGattc->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

binder_status_t BtGattClient_addService(IBtGattClient* iGattc, const char* instance)
{
    iGattc->clazz = (AIBinder_Class*)BtGattClient_getClass();
    AIBinder* binder = BtGattClient_getBinder(iGattc);
    iGattc->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtGattClient* BpBtGattClient_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtGattClient* bpBinder = NULL;

    clazz = (AIBinder_Class*)BtGattClient_getClass();
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

void BpBtGattClient_delete(BpBtGattClient* bpBinder)
{
    AIBinder_decStrong(bpBinder->binder);
    free(bpBinder);
}

AIBinder* BtGattClient_getService(BpBtGattClient** bpGattc, const char* instance)
{
    BpBtGattClient* bpBinder = *bpGattc;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtGattClient_new(instance);
    if (!bpBinder)
        return NULL;

    *bpGattc = bpBinder;

    return bpBinder->binder;
}
