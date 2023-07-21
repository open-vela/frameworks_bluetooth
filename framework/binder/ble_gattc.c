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
#define LOG_TAG "gattc"

#include "ble_gattc.h"
#include "bt_profile.h"

#include "gattc_callbacks_stub.h"
#include "gattc_proxy.h"
#include "gattc_stub.h"

#include "utils/log.h"
#include <stdint.h>

bt_status_t ble_gattc_create_connect(bt_instance_t *ins, gattc_handle_t *phandle, gattc_callbacks_t *callbacks)
{
    BpBleGattClient *gattc = (BpBleGattClient *)bluetooth_get_proxy(ins, PROFILE_GATTC);

    IBleGattClientCallbacks *cbks = BleGattClientCallbacks_new(callbacks);
    AIBinder *binder = BleGattClientCallbacks_getBinder(cbks);
    if (!binder) {
        BleGattClientCallbacks_delete(cbks);
        return BT_STATUS_FAIL;
    }

    void *handle = BpBleGattClient_createConnect(gattc, binder);
    if (!handle) {
        BleGattClientCallbacks_delete(cbks);
        return BT_STATUS_FAIL;
    }
    cbks->proxy = gattc;
    cbks->cookie = handle;
    *phandle = cbks;

    return BT_STATUS_SUCCESS;
}

bt_status_t ble_gattc_delete_connect(gattc_handle_t conn_handle)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    bt_status_t status = BpBleGattClient_deleteConnect(cbks->proxy, cbks->cookie);
    if (status == BT_STATUS_SUCCESS)
        BleGattClientCallbacks_delete(cbks);
    return status;
}

bt_status_t ble_gattc_connect(gattc_handle_t conn_handle, bt_address_t *addr, ble_addr_type_t addr_type)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_connect(cbks->proxy, cbks->cookie, addr, addr_type);
}

bt_status_t ble_gattc_disconnect(gattc_handle_t conn_handle)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_disconnect(cbks->proxy, cbks->cookie);
}

bt_status_t ble_gattc_discover_service(gattc_handle_t conn_handle, bt_uuid_t *filter_uuid)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_discoverService(cbks->proxy, cbks->cookie, filter_uuid);
}

bt_status_t ble_gattc_get_attribute_by_handle(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t *attr_desc)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_getAttributeByHandle(cbks->proxy, cbks->cookie, attr_handle, attr_desc);
}

bt_status_t ble_gattc_get_attribute_by_uuid(gattc_handle_t conn_handle, bt_uuid_t *attr_uuid, gatt_attr_desc_t *attr_desc)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_getAttributeByUUID(cbks->proxy, cbks->cookie, attr_uuid, attr_desc);
}

bt_status_t ble_gattc_read(gattc_handle_t conn_handle, uint16_t attr_handle)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_read(cbks->proxy, cbks->cookie, attr_handle);
}

bt_status_t ble_gattc_write(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t *value, uint16_t length, uint16_t offset)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_write(cbks->proxy, cbks->cookie, attr_handle, value, length, offset);
}

bt_status_t ble_gattc_write_without_response(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_writeWithoutResponse(cbks->proxy, cbks->cookie, attr_handle, value, length);
}

bt_status_t ble_gattc_subscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle, gattc_notify_cb_t notify_cb)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    bt_status_t status = BpBleGattClient_subscribe(cbks->proxy, cbks->cookie, value_handle, cccd_handle);
    if (status == BT_STATUS_SUCCESS && notify_cb)
        BleGattClientCallbacks_registerNotify(cbks, value_handle, notify_cb);
    return status;
}

bt_status_t ble_gattc_unsubscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    bt_status_t status = BpBleGattClient_unsubscribe(cbks->proxy, cbks->cookie, value_handle, cccd_handle);
    if (status == BT_STATUS_SUCCESS)
        BleGattClientCallbacks_unregisterNotify(cbks, value_handle);
    return status;
}

bt_status_t ble_gattc_exchange_mtu(gattc_handle_t conn_handle, uint32_t mtu)
{
    IBleGattClientCallbacks *cbks = conn_handle;
    return BpBleGattClient_exchangeMtu(cbks->proxy, cbks->cookie, mtu);
}
