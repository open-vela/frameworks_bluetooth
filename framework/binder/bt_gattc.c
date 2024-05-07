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

#include "bt_gattc.h"
#include "bt_profile.h"

#include "gattc_callbacks_stub.h"
#include "gattc_proxy.h"
#include "gattc_stub.h"

#include "utils/log.h"
#include <stdint.h>

bt_status_t bt_gattc_create_connect(bt_instance_t* ins, gattc_handle_t* phandle, gattc_callbacks_t* callbacks)
{
    BpBtGattClient* gattc = (BpBtGattClient*)bluetooth_get_proxy(ins, PROFILE_GATTC);

    IBtGattClientCallbacks* cbks = BtGattClientCallbacks_new(callbacks);
    AIBinder* binder = BtGattClientCallbacks_getBinder(cbks);
    if (!binder) {
        BtGattClientCallbacks_delete(cbks);
        return BT_STATUS_FAIL;
    }

    void* handle = BpBtGattClient_createConnect(gattc, binder);
    if (!handle) {
        BtGattClientCallbacks_delete(cbks);
        return BT_STATUS_FAIL;
    }
    cbks->proxy = gattc;
    cbks->cookie = handle;
    *phandle = cbks;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_delete_connect(gattc_handle_t conn_handle)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    bt_status_t status = BpBtGattClient_deleteConnect(cbks->proxy, cbks->cookie);
    if (status == BT_STATUS_SUCCESS)
        BtGattClientCallbacks_delete(cbks);
    return status;
}

bt_status_t bt_gattc_connect(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_connect(cbks->proxy, cbks->cookie, addr, addr_type);
}

bt_status_t bt_gattc_disconnect(gattc_handle_t conn_handle)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_disconnect(cbks->proxy, cbks->cookie);
}

bt_status_t bt_gattc_discover_service(gattc_handle_t conn_handle, bt_uuid_t* filter_uuid)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_discoverService(cbks->proxy, cbks->cookie, filter_uuid);
}

bt_status_t bt_gattc_get_attribute_by_handle(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_getAttributeByHandle(cbks->proxy, cbks->cookie, attr_handle, attr_desc);
}

bt_status_t bt_gattc_get_attribute_by_uuid(gattc_handle_t conn_handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_getAttributeByUUID(cbks->proxy, cbks->cookie, attr_uuid, attr_desc);
}

bt_status_t bt_gattc_read(gattc_handle_t conn_handle, uint16_t attr_handle)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_read(cbks->proxy, cbks->cookie, attr_handle);
}

bt_status_t bt_gattc_write(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_write(cbks->proxy, cbks->cookie, attr_handle, value, length);
}

bt_status_t bt_gattc_write_without_response(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_writeWithoutResponse(cbks->proxy, cbks->cookie, attr_handle, value, length);
}

bt_status_t bt_gattc_subscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_subscribe(cbks->proxy, cbks->cookie, value_handle, cccd_handle);
}

bt_status_t bt_gattc_unsubscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_unsubscribe(cbks->proxy, cbks->cookie, value_handle, cccd_handle);
}

bt_status_t bt_gattc_exchange_mtu(gattc_handle_t conn_handle, uint32_t mtu)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_exchangeMtu(cbks->proxy, cbks->cookie, mtu);
}

bt_status_t bt_gattc_update_connection_parameter(gattc_handle_t conn_handle, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    IBtGattClientCallbacks* cbks = conn_handle;
    return BpBtGattClient_updateConnectionParameter(cbks->proxy, cbks->cookie, min_interval, max_interval, latency,
        timeout, min_connection_event_length, max_connection_event_length);
}
