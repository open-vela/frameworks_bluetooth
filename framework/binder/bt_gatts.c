/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#define LOG_TAG "gatts"

#include "bt_gatts.h"
#include "bt_profile.h"

#include "gatts_callbacks_stub.h"
#include "gatts_proxy.h"
#include "gatts_stub.h"

#include "utils/log.h"
#include <stdint.h>

bt_status_t bt_gatts_register_service(bt_instance_t* ins, gatts_handle_t* phandle, gatts_callbacks_t* callbacks)
{
    BpBtGattServer* gatts = (BpBtGattServer*)bluetooth_get_proxy(ins, PROFILE_GATTS);

    IBtGattServerCallbacks* cbks = BtGattServerCallbacks_new(callbacks);
    AIBinder* binder = BtGattServerCallbacks_getBinder(cbks);
    if (!binder) {
        BtGattServerCallbacks_delete(cbks);
        return BT_STATUS_FAIL;
    }

    void* handle = BpBtGattServer_registerService(gatts, binder);
    if (!handle) {
        BtGattServerCallbacks_delete(cbks);
        return BT_STATUS_FAIL;
    }
    cbks->proxy = gatts;
    cbks->cookie = handle;
    *phandle = cbks;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_gatts_unregister_service(gatts_handle_t srv_handle)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    bt_status_t status = BpBtGattServer_unregisterService(cbks->proxy, cbks->cookie);
    if (status == BT_STATUS_SUCCESS)
        BtGattServerCallbacks_delete(cbks);
    return status;
}

bt_status_t bt_gatts_connect(gatts_handle_t srv_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    return BpBtGattServer_connect(cbks->proxy, cbks->cookie, addr, addr_type);
}

bt_status_t bt_gatts_disconnect(gatts_handle_t srv_handle)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    return BpBtGattServer_disconnect(cbks->proxy, cbks->cookie);
}

bt_status_t bt_gatts_create_service_table(gatts_handle_t srv_handle, gatt_srv_db_t* srv_db)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    bt_status_t status = BpBtGattServer_createServiceTable(cbks->proxy, cbks->cookie, srv_db);
    if (status == BT_STATUS_SUCCESS)
        cbks->srv_db = srv_db;
    return status;
}

bt_status_t bt_gatts_start(gatts_handle_t srv_handle)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    return BpBtGattServer_start(cbks->proxy, cbks->cookie);
}

bt_status_t bt_gatts_stop(gatts_handle_t srv_handle)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    return BpBtGattServer_stop(cbks->proxy, cbks->cookie);
}

bt_status_t bt_gatts_response(gatts_handle_t srv_handle, uint32_t req_handle, uint8_t* value, uint16_t length)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    return BpBtGattServer_response(cbks->proxy, cbks->cookie, req_handle, value, length);
}

bt_status_t bt_gatts_notify(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    return BpBtGattServer_notify(cbks->proxy, cbks->cookie, attr_handle, value, length);
}

bt_status_t bt_gatts_indicate(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    IBtGattServerCallbacks* cbks = srv_handle;
    return BpBtGattServer_indicate(cbks->proxy, cbks->cookie, attr_handle, value, length);
}
