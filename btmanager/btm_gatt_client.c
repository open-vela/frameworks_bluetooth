/****************************************************************************
 * frameworks/bluetooth/src/btmanager/btm_gatt_client.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "btm_gattc"

#include "btm_gatt_client.h"

#include <stdlib.h>
#include <string.h>

#include "btm_manager.h"
#include "bts_gatt_client.h"
#include "bts_gatt_service.h"
#include "log.h"

typedef struct
{
    void** handle_ptr;
    bt_address remote_addr;
    const btm_gatt_client_callbacks* callbacks;
} btm_gattc_hdl_t;

static btm_interface_t* bt_mgr_interface = NULL;
static const bts_gattc_interface_t* client_interface = NULL;

static void on_bts_gattc_connection_state_changed_cb(void* hdl, profile_connection_state state)
{
    BT_LOGD("PERFORMANCE-GATT-CLIENT-PROFILE-BTM-STATE:%d", state);
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle);
    BT_CBACK(handle->callbacks, gattc_connection_state_changed_cb, handle, handle->remote_addr, state);
    if (state == PROFILE_DISCONNECTED) {
        BT_LOGD("free btm_gatt_client handle");
        free(handle);
    }
}

static void on_bts_gattc_service_discovered_cb(void* hdl, gatt_element_t* element,
    uint16_t size)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle || !element || size < 1);
    BT_CBACK(handle->callbacks, gattc_service_discovered_cb, handle, handle->remote_addr, element, size);
}

static void on_bts_gattc_read_result_cb(void* hdl, gatt_element_t* element, uint8_t* value,
    uint16_t size, gatt_status status)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle || !element || !value || size < 1);
    BT_CBACK(handle->callbacks, gattc_read_result_cb, handle, handle->remote_addr, element, value, size, status);
}

static void on_bts_gattc_write_result_cb(void* hdl, gatt_element_t* element, gatt_status status)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle || !element);
    BT_CBACK(handle->callbacks, gattc_write_result_cb, handle, handle->remote_addr, element, status);
}

static void on_bts_gattc_nofity_request_cb(void* hdl, gatt_element_t* element, uint8_t* value, uint16_t size)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle || !element || !value || size < 1);
    BT_CBACK(handle->callbacks, gattc_nofity_request_cb, handle, handle->remote_addr, element, value, size);
}

static void on_bts_gattc_rssi_read_cb(void* hdl, int32_t rssi, gatt_status status)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle);
    BT_CBACK(handle->callbacks, gattc_rssi_read_cb, handle, handle->remote_addr, rssi, status);
}

static void on_bts_gattc_phy_read_cb(void* hdl, ble_phy_type tx, ble_phy_type rx)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle);
    BT_CBACK(handle->callbacks, gattc_phy_read_cb, handle, handle->remote_addr, tx, rx);
}

static void on_bts_gattc_phy_update_cb(void* hdl, ble_phy_type tx, ble_phy_type rx)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle);
    BT_CBACK(handle->callbacks, gattc_phy_update_cb, handle, handle->remote_addr, tx, rx);
}

static void on_bts_gattc_mtu_changed_cb(void* hdl, uint32_t mtu)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT2(!handle);
    BT_CBACK(handle->callbacks, gattc_mtu_changed_cb, handle, handle->remote_addr, mtu);
}

static bts_gatt_client_callbacks client_callbacks = {
    .bts_gattc_connection_state_changed_cb = on_bts_gattc_connection_state_changed_cb,
    .bts_gattc_service_discovered_cb = on_bts_gattc_service_discovered_cb,
    .bts_gattc_read_result_cb = on_bts_gattc_read_result_cb,
    .bts_gattc_write_result_cb = on_bts_gattc_write_result_cb,
    .bts_gattc_nofity_request_cb = on_bts_gattc_nofity_request_cb,
    .bts_gattc_rssi_read_cb = on_bts_gattc_rssi_read_cb,
    .bts_gattc_phy_read_cb = on_bts_gattc_phy_read_cb,
    .bts_gattc_phy_update_cb = on_bts_gattc_phy_update_cb,
    .bts_gattc_mtu_changed_cb = on_bts_gattc_mtu_changed_cb,
};

static bt_result_code gatt_client_connect(void** hdl_ptr, bt_address remote_addr, btm_gatt_client_callbacks* callbacks)
{
    BT_LOGD("PERFORMANCE-GATT-CLIENT-PROFILE-BTM-CONNECT-START");
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);

    btm_gattc_hdl_t** handle_ptr = (btm_gattc_hdl_t**)(hdl_ptr);
    *handle_ptr = (btm_gattc_hdl_t*)malloc(sizeof(btm_gattc_hdl_t));
    if (!*handle_ptr) {
        BT_LOGE("error, malloc btm_gattc_hdl_t failed");
        return BT_RESULT_ALLOC_BUFFER_FAILED;
    }

    memset(*handle_ptr, 0, sizeof(btm_gattc_hdl_t));
    (*handle_ptr)->callbacks = callbacks;
    (*handle_ptr)->handle_ptr = (void**)handle_ptr;
    memcpy((*handle_ptr)->remote_addr, remote_addr, sizeof(bt_address));

    bts_gattc_hdl_t client = {
        .callbacks = &client_callbacks,
        .btm_handle = *handle_ptr,
    };
    memcpy(client.remote_addr, remote_addr, sizeof(bt_address));

    bt_result_code ret = client_interface->connect(client);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, connect err:%d", ret);
        free(*handle_ptr);
        *handle_ptr = NULL;
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_disconnect(void* hdl)
{
    BT_LOGD("PERFORMANCE-GATT-CLIENT-PROFILE-BTM-DISCONNECT-START");
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->disconnect(handle->remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, disconnect err:%d", ret);
        void** handle_ptr = handle->handle_ptr;
        free(handle);
        *handle_ptr = NULL;
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_discover_services(void* hdl, bt_uuid_t uuid)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->discover_services(handle->remote_addr, uuid);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, discover_services err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_read_request(void* hdl, gatt_element_t* element)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle || !element, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->read_request(handle->remote_addr, element);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, read_request err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_write_request(void* hdl, gatt_element_t* element, uint8_t* value,
    uint16_t length)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle || !element || !value || length < 1, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->write_request(handle->remote_addr, element, value, length);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, write_request err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_register_notification(void* hdl, gatt_element_t* element, bool enable)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle || !element, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->register_notification(handle->remote_addr, element, enable);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, register_notification err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_read_rssi(void* hdl)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->read_rssi(handle->remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, read_rssi err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_read_phy(void* hdl)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->read_phy(handle->remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, read_phy err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_update_phy(void* hdl, ble_phy_type tx_phy, ble_phy_type rx_phy)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->update_phy(handle->remote_addr, tx_phy, rx_phy);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, read_phy err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_update_mtu(void* hdl, uint32_t mtu)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->update_mtu(handle->remote_addr, mtu);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, read_phy err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_update_connection_parameter(void* hdl, uint32_t min_interval, uint32_t max_interval,
    uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    btm_gattc_hdl_t* handle = (btm_gattc_hdl_t*)(hdl);
    BT_ASSERT(!handle, BT_RESULT_FAILED);
    BT_ASSERT(!client_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = client_interface->update_connection_parameter(handle->remote_addr, min_interval, max_interval, latency, timeout, min_connection_event_length, max_connection_event_length);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, update_connection_parameter err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static btm_gatt_client_interface_t le_gattc_interface = {
    .size = sizeof(le_gattc_interface),

    .connect = gatt_client_connect,
    .disconnect = gatt_client_disconnect,
    .discover_services = gatt_client_discover_services,
    .read_request = gatt_client_read_request,
    .write_request = gatt_client_write_request,
    .register_notification = gatt_client_register_notification,
    .read_rssi = gatt_client_read_rssi,
    .read_phy = gatt_client_read_phy,
    .update_phy = gatt_client_update_phy,
    .update_mtu = gatt_client_update_mtu,
    .update_connection_parameter = gatt_client_update_connection_parameter,
};

btm_gatt_client_interface_t* get_btm_gattc_interface(void* bt_mgr)
{
    if (!bt_mgr) {
        BT_LOGE("fail, bt_mgr NULL");
        return NULL;
    }
    bt_mgr_interface = (btm_interface_t*)bt_mgr;
    const gatt_interface_t* interface = (gatt_interface_t*)bt_mgr_interface->get_profile_interface(BT_PROFILE_GATT);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface gatt");
        return NULL;
    }
    client_interface = interface->client;
    return &le_gattc_interface;
}