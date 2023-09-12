/****************************************************************************
 * frameworks/bluetooth/btservice/gatt/bts_gatt_server.c
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
#define LOG_TAG "bts_gatts"

#include "bts_gatt_server.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bts_service.h"
#include "log.h"
#include "stack_adapter_gap.h"
#include "stack_adapter_gatt.h"
#include "utils.h"

typedef struct
{
    enum {
        ON_SERVER_OPENED = 0,
        ON_SERVER_CLOSED,
        ON_SERVER_CONNECTION_CHANGED,
        ON_SERVER_ELEMENTS_ADD,
        ON_SERVER_ELEMENTS_REMOVE,
        ON_SERVER_PHY_READ,
        ON_SERVER_PHY_UPDATE,
        ON_SERVER_READ_REQUEST,
        ON_SERVER_WRITE_REQUEST,
        ON_SRRVER_MTU_CHANGED,
        ON_SERVER_NOTIFICATION_SENT,
    } event;

    uint8_t server_if;
    bts_gatts_hdl_t* handle;
    size_t size;
    void* data;
} bts_gatts_msg_t;

typedef struct {
    bt_address addr;
    profile_connection_state state;
} bts_gatts_state_s;

typedef struct {
    gatt_status status;
    gatt_element_t* element;
    uint16_t size;
} bts_gatts_element_s;

typedef struct {
    bt_address addr;
    ble_phy_type tx;
    ble_phy_type rx;
    gatt_status status;
} bts_gatts_phy_s;

typedef struct
{
    bt_address addr;
    uint32_t request_id;
    gatt_element_t* element;
} bts_gatts_read_s;

typedef struct
{
    bt_address addr;
    uint32_t request_id;
    gatt_element_t* element;
    uint8_t* value;
    uint16_t offset;
    uint16_t size;
} bts_gatts_write_s;

typedef struct {
    bt_address addr;
    uint32_t mtu;
} bts_gatts_mtu_s;

typedef struct {
    bt_address addr;
    gatt_element_t element;
    gatt_status status;
} bts_gatts_notify_s;

static void send_msg(bts_gatts_msg_t* msg);
static void handle_msg_received(bt_profile_id id, void* data, size_t size);

static struct list_node gatts_list = LIST_INITIAL_VALUE(gatts_list);

static bts_gatts_hdl_t* find_gatts_handle(uint8_t server_if)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        if (handle->server_if == server_if) {
            return handle;
        }
    }
    return NULL;
}

static int8_t gen_gatts_id(void)
{
    uint8_t found = 0;
    bts_gatts_hdl_t* handle;
    for (uint8_t i = 1; i < 64; i++, found = 0) {
        list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
        {
            if (handle->server_if == i) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return i;
        }
    }
    BT_LOGE("handle id overflow");
    return -1;
}

static int8_t add_gatt_handle(bts_gatts_hdl_t server)
{
    bts_gatts_hdl_t* handle = (bts_gatts_hdl_t*)malloc(sizeof(bts_gatts_hdl_t));
    if (!handle) {
        BT_LOGE("malloc bts_gatts_hdl_t fail");
        return -1;
    }
    memset(handle, 0, sizeof(bts_gatts_hdl_t));

    handle->server_if = gen_gatts_id();
    handle->callbacks = server.callbacks;
    handle->btm_handle = server.btm_handle;
    list_add_tail(&gatts_list, &handle->node);
    return handle->server_if;
}

static bool remove_gatts_handle(bts_gatts_hdl_t* handle)
{
    list_delete(&handle->node);
    free(handle);
    return true;
}

static bt_result_code gatt_server_is_valid(uint8_t server_if)
{
    bts_gatts_hdl_t* server = find_gatts_handle(server_if);
    CHECK_PTR_RETURN(server, BT_RESULT_FAILED);
    return BT_RESULT_SUCCESS;
}

static bts_gatts_msg_t* create_adp_msg(uint8_t event, bts_gatts_hdl_t* handle, void* data, size_t size)
{
    bts_gatts_msg_t* msg = (bts_gatts_msg_t*)malloc(sizeof(bts_gatts_msg_t));
    CHECK_PTR_RETURN(msg, NULL);

    if (size < 0) {
        BT_LOGE("fail, invlaid size:%d", size);
        return NULL;
    }
    msg->event = event;
    msg->handle = handle;
    msg->size = size;
    msg->server_if = handle->server_if;
    if (size == 0) {
        return msg;
    }

    void* value = malloc(size);
    if (!value) {
        BT_LOGE("fail, malloc data");
        free(msg);
        return NULL;
    }
    memcpy(value, data, size);
    msg->data = value;

    return msg;
}

static void on_server_connection_state_changed(bt_address remote_addr, profile_connection_state state)
{
    BT_LOGD("PERFORMANCE-GATT-SERVER-PROFILE-BLUELET-CONNECTION-STATE:%d, addr:%s", state, addr_str(remote_addr));
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_state_s data;
        memset(&data, 0, sizeof(data));
        memcpy(data.addr, remote_addr, sizeof(bt_address));
        data.state = state;

        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_CONNECTION_CHANGED, handle, &data, sizeof(bts_gatts_state_s));
        CHECK_PTR(msg);
        send_msg(msg);
    }
}

static void on_server_elements_added(gatt_status status, gatt_element_t* elements,
    uint16_t size)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_element_s data;
        memset(&data, 0, sizeof(data));
        data.status = status;
        gatt_element_t* items = (gatt_element_t*)malloc(sizeof(gatt_element_t) * size);
        if (!items) {
            BT_LOGE("error, malloc items failed");
            return;
        }
        for (size_t index = 0; index < size; index++, items++, elements++) {
            items->id = elements->id;
            memcpy(items->uuid, elements->uuid, sizeof(bt_uuid_t));
            items->type = elements->type;
            items->properties = elements->properties;
            items->permissions = elements->permissions;
        }
        data.element = items - size;
        data.size = size;

        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_ELEMENTS_ADD, handle, &data, sizeof(bts_gatts_element_s));
        if (!msg) {
            free(items);
            BT_LOGE("error, create_adp_msg");
            return;
        }
        send_msg(msg);
    }
}

static void on_server_elements_removed(gatt_status status, gatt_element_t* elements,
    uint16_t size)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_element_s data;
        memset(&data, 0, sizeof(data));
        data.status = status;
        gatt_element_t* items = (gatt_element_t*)malloc(sizeof(gatt_element_t) * size);
        if (!items) {
            BT_LOGE("error, malloc items failed");
            return;
        }

        for (size_t index = 0; index < size; index++, items++, elements++) {
            items->id = elements->id;
            memcpy(items->uuid, elements->uuid, sizeof(bt_uuid_t));
            items->type = elements->type;
            items->properties = elements->properties;
            items->permissions = elements->permissions;
        }
        data.element = items - size;
        data.size = size;

        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_ELEMENTS_REMOVE, handle, &data, sizeof(bts_gatts_element_s));
        if (!msg) {
            free(items);
            BT_LOGE("error, create_adp_msg");
            return;
        }

        send_msg(msg);
    }
}

static void on_server_phy_read(bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_phy_s data;
        memset(&data, 0, sizeof(data));
        memcpy(data.addr, remote_addr, sizeof(bt_address));
        data.tx = tx;
        data.rx = rx;

        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_PHY_READ, handle, &data, sizeof(bts_gatts_phy_s));
        CHECK_PTR(msg);
        send_msg(msg);
    }
}

static void on_server_phy_update(bt_address remote_addr, ble_phy_type tx, ble_phy_type rx, gatt_status status)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_phy_s data;
        memset(&data, 0, sizeof(data));
        memcpy(data.addr, remote_addr, sizeof(bt_address));
        data.tx = tx;
        data.rx = rx;
        data.status = status;

        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_PHY_UPDATE, handle, &data, sizeof(bts_gatts_phy_s));
        CHECK_PTR(msg);
        send_msg(msg);
    }
}

static void on_server_read_request(bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_read_s data;
        memset(&data, 0, sizeof(data));
        memcpy(data.addr, remote_addr, sizeof(bt_address));
        data.request_id = request_id;
        gatt_element_t* elem = (gatt_element_t*)malloc(sizeof(gatt_element_t));
        if (!elem) {
            BT_LOGE("error, malloc elem failed");
            return;
        }

        memcpy(elem, element, sizeof(gatt_element_t));
        data.element = elem;

        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_READ_REQUEST, handle, &data, sizeof(bts_gatts_read_s));
        if (!msg) {
            free(elem);
            BT_LOGE("error, create_adp_msg");
            return;
        }

        send_msg(msg);
    }
}

static void on_server_write_request(bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_write_s data;
        memset(&data, 0, sizeof(data));
        memcpy(data.addr, remote_addr, sizeof(bt_address));
        data.request_id = request_id;
        gatt_element_t* elem = (gatt_element_t*)malloc(sizeof(gatt_element_t));
        if (!elem) {
            BT_LOGE("error, malloc elem failed");
            return;
        }

        memcpy(elem, element, sizeof(gatt_element_t));
        data.element = elem;
        uint8_t* v = (uint8_t*)malloc(size);
        memcpy(v, value, size);
        data.value = v;
        data.size = size;
        data.offset = offset;

        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_WRITE_REQUEST, handle, &data, sizeof(bts_gatts_write_s));
        if (!msg) {
            free(elem);
            BT_LOGE("error, create_adp_msg");
            return;
        }

        send_msg(msg);
    }
}

static void on_server_mtu_changed(bt_address remote_addr, uint32_t mtu)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_mtu_s data;
        memset(&data, 0, sizeof(data));
        memcpy(data.addr, remote_addr, sizeof(bt_address));
        data.mtu = mtu;
        bts_gatts_msg_t* msg = create_adp_msg(ON_SRRVER_MTU_CHANGED, handle, &data, sizeof(bts_gatts_mtu_s));
        CHECK_PTR(msg);
        send_msg(msg);
    }
}

static void on_server_notify_sent(bt_address remote_addr, gatt_element_t* element, gatt_status status)
{
    bts_gatts_hdl_t* handle;
    list_for_every_entry(&gatts_list, handle, bts_gatts_hdl_t, node)
    {
        bts_gatts_notify_s data;
        memset(&data, 0, sizeof(data));
        memcpy(data.addr, remote_addr, sizeof(bt_address));
        memcpy(&data.element, element, sizeof(gatt_element_t));
        data.status = status;
        bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_NOTIFICATION_SENT, handle, &data, sizeof(bts_gatts_notify_s));
        CHECK_PTR(msg);
        send_msg(msg);
    }
}

static GATT_SERVER_CALLBACKS_S gatt_server_cbs = {
    sizeof(GATT_SERVER_CALLBACKS_S),
    .gatt_server_connection_state_changed_cb = (gatt_server_connection_state_changed_callback)on_server_connection_state_changed,
    .gatt_server_elements_added_cb = (gatt_server_elements_added_callback)on_server_elements_added,
    .gatt_server_elements_removed_cb = (gatt_server_elements_removed_callback)on_server_elements_removed,
    .gatt_server_phy_read_cb = (gatt_server_phy_read_callback)on_server_phy_read,
    .gatt_server_phy_update_cb = (gatt_server_phy_update_callback)on_server_phy_update,
    .gatt_server_received_element_read_request_cb = (gatt_server_received_element_read_request_callback)on_server_read_request,
    .gatt_server_received_element_write_request_cb = (gatt_server_received_element_write_request_callback)on_server_write_request,
    .gatt_server_mtu_changed_cb = (gatt_server_mtu_changed_callback)on_server_mtu_changed,
    .gatt_server_notification_sent_cb = (gatt_server_notification_sent_callback)on_server_notify_sent,
};

static bt_result_code gatt_server_open(bts_gatts_hdl_t server)
{
    if (list_is_empty(&gatts_list)) {
        SERVICE_GATT_STATUS ret = service_adapter_gatt_server_open(&gatt_server_cbs);
        if (ret != GATT_SUCCESS) {
            BT_LOGE("fail, gatt server open, err:%d", ret);
            return BT_RESULT_FAILED;
        }
    }

    int8_t gatt_if = add_gatt_handle(server);
    if (gatt_if < 0) {
        BT_LOGE("fail, gatt add_gatt_handle, gatt_if:%d", gatt_if);
        return BT_RESULT_FAILED;
    }

    bts_gatts_hdl_t* handle = find_gatts_handle(gatt_if);
    if (!handle) {
        BT_LOGE("fail, null handle");
        return BT_RESULT_FAILED;
    }

    bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_OPENED, handle, NULL, 0);
    CHECK_PTR_RETURN(msg, BT_RESULT_FAILED);
    send_msg(msg);

    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_close(uint8_t server_if)
{
    if (list_length(&gatts_list) == 1) {
        SERVICE_GATT_STATUS ret = service_adapter_gatt_server_close();
        if (ret != GATT_SUCCESS) {
            BT_LOGE("fail, gatt server close, err:%d", ret);
        }
    }

    bts_gatts_hdl_t* handle = find_gatts_handle(server_if);
    if (!handle) {
        BT_LOGE("fail, null handle");
        return BT_RESULT_FAILED;
    }

    bts_gatts_msg_t* msg = create_adp_msg(ON_SERVER_CLOSED, handle, NULL, 0);
    CHECK_PTR_RETURN(msg, BT_RESULT_FAILED);
    send_msg(msg);

    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_connect(uint8_t server_if, bt_address remote_addr, bool auto_connect)
{
    BT_LOGD("PERFORMANCE-GATT-SERVER-PROFILE-BLUELET-CONNECTION-START, addr:%s", addr_str(remote_addr));
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_connect(remote_addr);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server connect, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_disconnect(uint8_t server_if, bt_address remote_addr)
{
    BT_LOGD("PERFORMANCE-GATT-SERVER-PROFILE-BLUELET-DISCONNECTION-START, addr:%s", addr_str(remote_addr));
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_cancel_connection(remote_addr);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server disconnect, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_add_element(uint8_t server_if, gatt_element_t* element, uint16_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_add_elements((SERVICE_GATT_ELEMENT_S*)(element), size);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server add elements, err:%d", ret2);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_remove_element(uint8_t server_if, uint32_t* ids, uint16_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_remove_elements(ids, size);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_read_phy(uint8_t server_if, bt_address remote_addr)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_read_phy(remote_addr);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_update_phy(uint8_t server_if, bt_address remote_addr, ble_phy_type tx_type, ble_phy_type rx_type)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_set_phy(remote_addr, tx_type, rx_type);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_notify(uint8_t server_if, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_send_notification(remote_addr, (SERVICE_GATT_ELEMENT_S*)(characteristic), value, size);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_indicate(uint8_t server_if, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_send_indication(remote_addr, (SERVICE_GATT_ELEMENT_S*)(characteristic), value, size);
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_response(uint8_t server_if, bt_address remote_addr, gatt_response_t* response)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    gatt_status ret2 = service_adapter_gatt_server_send_response(remote_addr, (SERVICE_GATT_RESPONSE_S*)(response));
    if (ret2 != GATT_STATUS_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static const bts_gatts_interface_t gatt_server_intance = {
    .size = sizeof(gatt_server_intance),
    .init = NULL,
    .clean_up = NULL,

    .open_server = gatt_server_open,
    .close_server = gatt_server_close,
    .connect = gatt_server_connect,
    .disconnect = gatt_server_disconnect,
    .add_element = gatt_server_add_element,
    .remove_element = gatt_server_remove_element,
    .read_phy = gatt_server_read_phy,
    .update_phy = gatt_server_update_phy,
    .send_notify = gatt_server_send_notify,
    .send_indicate = gatt_server_send_indicate,
    .send_response = gatt_server_send_response,
};

const bts_gatts_interface_t* get_bts_gatts_instance(void)
{
    return &gatt_server_intance;
}

static void handle_msg_received(bt_profile_id id, void* data, size_t size)
{
    if (id != BT_PROFILE_GATTS_ID) {
        BT_LOGE("error, invalid priofile id:%d", id);
        return;
    }
    bts_gatts_msg_t* msg = (bts_gatts_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }

    bts_gatts_hdl_t* handle = find_gatts_handle(msg->server_if);
    if (!handle) {
        BT_LOGE("event:%d, null handle", msg->event);
        return;
    }

    switch (msg->event) {
    case ON_SERVER_OPENED: {
        BT_CBACK(handle->callbacks, bts_gatts_server_opened_cb, handle->btm_handle, handle->server_if);
        break;
    }
    case ON_SERVER_CLOSED: {
        BT_CBACK(handle->callbacks, bts_gatts_server_closed_cb, handle->btm_handle);
        if (list_length(&gatts_list) == 1) {
            BT_LOGD("gatts unregistered");
        }
        remove_gatts_handle(handle);
        break;
    }
    case ON_SERVER_CONNECTION_CHANGED: {
        bts_gatts_state_s* value = (bts_gatts_state_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_connection_state_changed_cb, handle->btm_handle, value->addr, value->state);
        break;
    }
    case ON_SERVER_ELEMENTS_ADD: {
        bts_gatts_element_s* value = (bts_gatts_element_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_elements_added_cb, handle->btm_handle, value->status, value->element, value->size);
        free(value->element);
        break;
    }
    case ON_SERVER_ELEMENTS_REMOVE: {
        bts_gatts_element_s* value = (bts_gatts_element_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_elements_removed_cb, handle->btm_handle, value->status, value->element, value->size);
        free(value->element);
        break;
    }
    case ON_SERVER_PHY_READ: {
        bts_gatts_phy_s* value = (bts_gatts_phy_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_phy_read_cb, handle->btm_handle, value->addr, value->tx, value->rx);
        break;
    }
    case ON_SERVER_PHY_UPDATE: {
        bts_gatts_phy_s* value = (bts_gatts_phy_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_phy_update_cb, handle->btm_handle, value->addr, value->tx, value->rx, value->status);
        break;
    }
    case ON_SERVER_READ_REQUEST: {
        bts_gatts_read_s* value = (bts_gatts_read_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_read_request_cb, handle->btm_handle, value->addr, value->request_id, value->element);
        free(value->element);
        break;
    }
    case ON_SERVER_WRITE_REQUEST: {
        bts_gatts_write_s* value = (bts_gatts_write_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_write_request_cb, handle->btm_handle, value->addr, value->request_id, value->element, value->value, value->offset, value->size);
        free(value->value);
        free(value->element);
        break;
    }
    case ON_SRRVER_MTU_CHANGED: {
        bts_gatts_mtu_s* value = (bts_gatts_mtu_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_mtu_changed_cb, handle->btm_handle, value->addr, value->mtu);
        break;
    }
    case ON_SERVER_NOTIFICATION_SENT: {
        bts_gatts_notify_s* value = (bts_gatts_notify_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gatts_notify_sent_cb, handle->btm_handle, value->addr, &value->element, value->status);
        break;
    }
    default: {
        BT_LOGW("invalid event:%d", msg->event);
        break;
    }
    }
    if (msg->size > 0)
        free(msg->data);
    free(msg);
}

static void send_msg(bts_gatts_msg_t* msg)
{
    bts_send_uv_msg(BT_PROFILE_GATTS_ID, msg, sizeof(bts_gatts_msg_t));
}

void gatts_init(void)
{
    bts_register_profile_process(BT_PROFILE_GATTS_ID, &handle_msg_received);
}

void gatts_deinit(void)
{
    bts_unregister_profile_process(BT_PROFILE_GATTS_ID);
}
