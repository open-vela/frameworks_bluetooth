/****************************************************************************
 * frameworks/bluetooth/src/btmanager/btm_gatt_server.c
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
#define LOG_TAG "btm_gatts"

#include "btm_gatt_server.h"

#include <stdlib.h>
#include <string.h>

#include "btm_manager.h"
#include "bts_gatt_server.h"
#include "bts_gatt_service.h"
#include "log.h"

typedef struct
{
    void** handle_ptr;
    uint8_t server_if;
    const btm_gatt_server_callbacks* callbacks;
} btm_gatts_hdl_t;

static btm_interface_t* bt_mgr_interface = NULL;

static const bts_gatts_interface_t* server_interface = NULL;

static void on_bts_gatts_connection_state_changed(void* hdl, bt_address remote_addr, profile_connection_state state)
{
    BT_LOGD("PERFORMANCE-GATT-SERVER-PROFILE-BTM-CONNECTION-STATE:%d, addr:%s", state, addr_str(remote_addr));
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_connection_state_changed_cb, handle, remote_addr, state);
}

static void on_bts_gatts_opened_cb(void* hdl, uint8_t server_if)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    handle->server_if = server_if;
    BT_CBACK(handle->callbacks, gatts_server_opened_cb, handle);
}

static void on_bts_gatts_closed_cb(void* hdl)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_server_closed_cb, handle);
    void** handle_ptr = handle->handle_ptr;
    free(handle);
    *handle_ptr = NULL;
}

static void on_bts_gatts_element_added(void* hdl, gatt_status status, gatt_element_t* element,
    size_t size)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_service_added_cb, handle, status, element, size);
}

static void on_bts_gatts_element_removed(void* hdl, gatt_status status, gatt_element_t* element,
    size_t size)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_service_removed_cb, handle, status, element, size);
}

static void on_bts_gatts_phy_read(void* hdl, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_phy_read_cb, handle, remote_addr, tx, rx);
}

static void on_bts_gatts_phy_update(void* hdl, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx, gatt_status status)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_phy_update_cb, handle, remote_addr, tx, rx, status);
}

static void on_bts_gatts_read_request(void* hdl, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_read_request_cb, handle, remote_addr, request_id, element);
}

static void on_bts_gatts_write_request(void* hdl, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_write_request_cb, handle, remote_addr, request_id, element, value, offset, size);
}

static void on_bts_gatts_mtu_changed(void* hdl, bt_address remote_addr, uint32_t mtu)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_mtu_changed_cb, handle, remote_addr, mtu);
}

static void on_bts_gatts_notify_sent(void* hdl, bt_address remote_addr, gatt_status status)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, gatts_notify_sent_cb, handle, remote_addr, status);
}

static bts_gatt_server_callbacks server_callbacks = {
    .bts_gatts_connection_state_changed_cb = on_bts_gatts_connection_state_changed,
    .bts_gatts_server_opened_cb = on_bts_gatts_opened_cb,
    .bts_gatts_server_closed_cb = on_bts_gatts_closed_cb,
    .bts_gatts_elements_added_cb = on_bts_gatts_element_added,
    .bts_gatts_elements_removed_cb = on_bts_gatts_element_removed,
    .bts_gatts_phy_read_cb = on_bts_gatts_phy_read,
    .bts_gatts_phy_update_cb = on_bts_gatts_phy_update,
    .bts_gatts_read_request_cb = on_bts_gatts_read_request,
    .bts_gatts_write_request_cb = on_bts_gatts_write_request,
    .bts_gatts_mtu_changed_cb = on_bts_gatts_mtu_changed,
    .bts_gatts_notify_sent_cb = on_bts_gatts_notify_sent,
};

static bt_result_code gatt_server_open(void** hdl_ptr, btm_gatt_server_callbacks* callbacks)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);

    btm_gatts_hdl_t** handle_ptr = (btm_gatts_hdl_t**)(hdl_ptr);
    *handle_ptr = (btm_gatts_hdl_t*)malloc(sizeof(btm_gatts_hdl_t));
    memset(*handle_ptr, 0, sizeof(btm_gatts_hdl_t));
    (*handle_ptr)->callbacks = callbacks;
    (*handle_ptr)->handle_ptr = (void**)handle_ptr;

    bts_gatts_hdl_t server = {
        .callbacks = &server_callbacks,
        .btm_handle = *handle_ptr,
    };
    bt_result_code ret = server_interface->open_server(server);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, open_server err:%d", ret);
        free(*handle_ptr);
        *handle_ptr = NULL;
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_close(void* hdl)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->close_server(handle->server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, close_server err:%d", ret);
        void** handle_ptr = handle->handle_ptr;
        free(handle);
        *handle_ptr = NULL;
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_connect(void* hdl, bt_address remote_addr, bool auto_connect)
{
    BT_LOGD("PERFORMANCE-GATT-SERVER-PROFILE-BTM-CONNECTION-START, addr:%s", addr_str(remote_addr));
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->connect(handle->server_if, remote_addr, auto_connect);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, connect err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_disconnect(void* hdl, bt_address remote_addr)
{
    BT_LOGD("PERFORMANCE-GATT-SERVER-PROFILE-BTM-DISCONNECTION-START, addr:%s", addr_str(remote_addr));
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->disconnect(handle->server_if, remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, disconnect err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_add_service(void* hdl, gatt_element_t* element, uint16_t size)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->add_element(handle->server_if, element, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, add_service err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_remove_service(void* hdl, uint32_t* ids, uint16_t size)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->remove_element(handle->server_if, ids, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, remove_service err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_read_phy(void* hdl, bt_address remote_addr)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->read_phy(handle->server_if, remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, read_phy err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_update_phy(void* hdl, bt_address remote_addr, ble_phy_type tx_type, ble_phy_type rx_type)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->update_phy(handle->server_if, remote_addr, tx_type, rx_type);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, update_phy err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_notify(void* hdl, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->send_notify(handle->server_if, remote_addr, characteristic, value, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, send_notify err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_indicate(void* hdl, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->send_indicate(handle->server_if, remote_addr, characteristic, value, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, send_indicate err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_response(void* hdl, bt_address remote_addr, gatt_response_t* response)
{
    btm_gatts_hdl_t* handle = hdl;
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->send_response(handle->server_if, remote_addr, response);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, send_response err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static btm_gatt_server_interface_t le_gatts_interface = {
    .size = sizeof(le_gatts_interface),

    .open = gatt_server_open,
    .close = gatt_server_close,
    .connect = gatt_server_connect,
    .disconnect = gatt_server_disconnect,
    .add_service = gatt_server_add_service,
    .remove_service = gatt_server_remove_service,
    .read_phy = gatt_server_read_phy,
    .update_phy = gatt_server_update_phy,
    .send_notify = gatt_server_send_notify,
    .send_indicate = gatt_server_send_indicate,
    .send_response = gatt_server_send_response,
};

btm_gatt_server_interface_t* get_btm_gatts_interface(void* bt_mgr)
{
    if (!bt_mgr) {
        BT_LOGE("fail, bt_mgr NULL");
        return NULL;
    }
    bt_mgr_interface = (btm_interface_t*)bt_mgr;
    const gatt_interface_t* interface = bt_mgr_interface->get_profile_interface(BT_PROFILE_GATT);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface gatt");
        return NULL;
    }

    server_interface = interface->server;
    return &le_gatts_interface;
}