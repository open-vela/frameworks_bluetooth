/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#include "sal_connection_manager.h"
#include "bt_list.h"
#include "sal_interface.h"
#include "service_loop.h"

#include <zephyr/bluetooth/conn.h>

#include "utils/log.h"

bt_status_t bt_sal_disconnect_internal(bt_controller_id_t id, bt_address_t* addr, uint8_t reason);
bt_status_t bt_sal_remove_bond_internal(bt_controller_id_t id, bt_address_t* addr);

typedef struct {
    bt_address_t device_addr;
    bool is_unpair;
    bt_list_t* profile_conn_handler_list;
} bt_profile_connection_manager_t;

typedef struct {
    bt_address_t device_addr;
    uint8_t profile_id;
    uint16_t conn_id;
    bt_profile_conn_handler_t handler;
    bt_controller_id_t id;
    bt_list_t* manager_list;
    void* user_data;
} sal_async_profile_req_t;

static bt_list_t* bt_sal_connecting_list = NULL;
static bt_list_t* bt_sal_disconnecting_list = NULL;

static bt_status_t bt_sal_trigger_profile_conn_act(bt_profile_connection_manager_t* manager, bt_list_t* manager_list);
static void remove_from_connection_manager_list(bt_list_t* list, bt_address_t* addr, uint8_t profile_id,
    uint16_t conn_id, bool try_acl_disconnect);

static bool match_profile_id_and_conn_id(void* data, void* context)
{
    bt_profile_conn_handler_node_t* handler_node = (bt_profile_conn_handler_node_t*)data;
    uint32_t key = (uint32_t)(uintptr_t)context;
    uint8_t profile_id = (key >> 16) & 0xFF;
    uint16_t conn_id = key & 0xFFFF;

    if (!handler_node) {
        return false;
    }

    return handler_node->profile_id == profile_id && handler_node->conn_id == conn_id;
}

static bt_profile_conn_handler_node_t* find_handler_node(
    bt_profile_connection_manager_t* manager,
    uint8_t profile_id,
    uint16_t conn_id)
{
    uint32_t key;

    if (!manager || !manager->profile_conn_handler_list) {
        return NULL;
    }

    key = (((uint32_t)profile_id) << 16) | (uint32_t)conn_id;

    return bt_list_find(manager->profile_conn_handler_list,
        match_profile_id_and_conn_id,
        (void*)(uintptr_t)key);
}

static void bt_connection_manager_destory(void* data)
{
    bt_profile_connection_manager_t* manager = (bt_profile_connection_manager_t*)data;
    if (manager) {
        if (manager->profile_conn_handler_list) {
            bt_list_free(manager->profile_conn_handler_list);
            manager->profile_conn_handler_list = NULL;
        }

        free(manager);
    }
}

static bool bt_connection_manager_find(void* data, void* context)
{
    bt_profile_connection_manager_t* manager = (bt_profile_connection_manager_t*)data;
    if (!manager)
        return false;

    return memcmp(&manager->device_addr, context, sizeof(bt_address_t)) == 0;
}

static void profile_entry_destroy(void* data)
{
    if (data) {
        bt_profile_conn_handler_node_t* handler_node = (bt_profile_conn_handler_node_t*)data;
        free(handler_node);
    }
}

static bt_profile_connection_manager_t* create_connection_manager(bt_address_t* addr)
{
    bt_profile_connection_manager_t* manager;

    manager = zalloc(sizeof(*manager));
    if (!manager) {
        return NULL;
    }

    memcpy(&manager->device_addr, addr, sizeof(bt_address_t));

    manager->profile_conn_handler_list = bt_list_new(profile_entry_destroy);
    if (!manager->profile_conn_handler_list) {
        free(manager);
        return NULL;
    }

    return manager;
}

static bt_profile_connection_manager_t* find_or_create_connection_manager(bt_list_t* list, bt_address_t* addr)
{
    bt_profile_connection_manager_t* manager;

    if (!addr || !list) {
        return NULL;
    }

    manager = bt_list_find(list, bt_connection_manager_find, addr);
    if (manager) {
        return manager;
    }

    manager = create_connection_manager(addr);
    if (!manager) {
        return NULL;
    }

    bt_list_add_tail(list, manager);
    return manager;
}

static sal_async_profile_req_t* sal_async_profile_req(bt_address_t* addr, bt_profile_conn_handler_t handler,
    uint8_t profile_id, uint16_t conn_id, bt_controller_id_t id, bt_list_t* manager_list, void* user_data)
{
    sal_async_profile_req_t* req = calloc(sizeof(sal_async_profile_req_t), 1);

    if (req) {
        req->profile_id = profile_id;
        req->conn_id = conn_id;
        req->id = id;
        req->handler = handler;
        req->manager_list = manager_list;
        req->user_data = user_data;
        if (addr)
            memcpy(&req->device_addr, addr, sizeof(bt_address_t));
    }

    return req;
}

static void sal_invoke_async(service_work_t* work, void* userdata)
{
    sal_async_profile_req_t* req = userdata;
    bt_status_t status;
    bt_profile_connection_manager_t* manager;
    bt_profile_conn_handler_node_t* handler_node;
    bt_list_t* manager_list;

    SAL_ASSERT(req);

    if (!bt_is_ready()) {
        free(req);
        return;
    }

    if (!req->manager_list) {
        /* !req->user_data means a direct profile "disconnection" */
        req->handler(req->id, &req->device_addr, req->user_data);
        free(req);
        return;
    }

    manager_list = (bt_list_t*)req->manager_list;

    manager = (bt_profile_connection_manager_t*)bt_list_find(manager_list,
        bt_connection_manager_find, &req->device_addr);

    if (!manager) {
        BT_LOGW("%s, manager not found.", __func__);
        free(req);
        return;
    }

    handler_node = find_handler_node(manager, req->profile_id, req->conn_id);

    if (!handler_node) {
        BT_LOGW("%s, handler_node not found.", __func__);
        free(req);
        return;
    }

    status = req->handler(req->id, &req->device_addr, req->user_data);

    if (status != BT_STATUS_SUCCESS) {
        remove_from_connection_manager_list(manager_list, &req->device_addr, req->profile_id, req->conn_id, false);
    }

    free(req);
}

static bt_status_t sal_send_async_req(sal_async_profile_req_t* req)
{
    if (!req)
        return BT_STATUS_PARM_INVALID;

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

cm_data_t* cm_data_new(bt_address_t* addr, uint8_t profile_id, uint16_t conn_id)
{
    cm_data_t* data = (cm_data_t*)zalloc(sizeof(cm_data_t));
    if (!data)
        return NULL;

    if (addr != NULL)
        memcpy(&data->addr, addr, sizeof(bt_address_t));

    data->profile_id = profile_id;
    data->conn_id = conn_id;
    return data;
}

void bt_sal_cm_conn_init(void)
{
    bt_sal_disconnecting_list = bt_list_new(bt_connection_manager_destory);
    bt_sal_connecting_list = bt_list_new(bt_connection_manager_destory);
}

void cm_data_destory(cm_data_t* data)
{
    free(data);
}

static bt_status_t bt_sal_trigger_profile_conn_act(bt_profile_connection_manager_t* manager, bt_list_t* manager_list)
{
    bt_list_node_t* node;
    bt_profile_conn_handler_node_t* handler_node;
    bt_address_t* addr;
    bt_list_t* list;
    sal_async_profile_req_t* req;

    if (!manager) {
        return BT_STATUS_PARM_INVALID;
    }

    list = manager->profile_conn_handler_list;

    if (!manager_list || !list) {
        return BT_STATUS_NOMEM;
    }

    addr = &manager->device_addr;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        handler_node = (bt_profile_conn_handler_node_t*)bt_list_node(node);

        if (!handler_node || !handler_node->handler)
            continue;

        if (handler_node->is_busy) {
            continue;
        }

        /* async invoke to service_worker thread */
        req = sal_async_profile_req(addr, handler_node->handler, handler_node->profile_id, handler_node->conn_id,
            handler_node->id, manager_list, handler_node->user_data);

        if (sal_send_async_req(req) != BT_STATUS_SUCCESS) {
            BT_LOGE("%s, profile_id: %u", __func__, handler_node->profile_id);
            free(req);
            continue;
        }

        handler_node->is_busy = true;
    }

    return BT_STATUS_SUCCESS;
}

static void remove_from_connection_manager_list(bt_list_t* list, bt_address_t* addr, uint8_t profile_id,
    uint16_t conn_id, bool try_acl_disconnect)
{
    if (list == NULL) {
        return;
    }

    bt_profile_connection_manager_t* manager = bt_list_find(list, bt_connection_manager_find, addr);
    bt_profile_conn_handler_node_t* handler_node;

    if (manager == NULL) {
        BT_LOGW("%s, manager not found.", __func__);
        return;
    }

    handler_node = find_handler_node(manager, profile_id, conn_id);

    if (handler_node) {
        bt_list_remove(manager->profile_conn_handler_list, handler_node);
    }

    if (bt_list_is_empty(manager->profile_conn_handler_list)) {

        if (try_acl_disconnect) {
            bt_sal_disconnect_internal(PRIMARY_ADAPTER, addr, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        } else {
            bt_list_remove(list, manager);
        }
    }
}

static void bt_sal_cm_acl_connected(void* data)
{
    if (data == NULL)
        return;

    cm_data_t* cm_data;
    bt_profile_connection_manager_t* manager;

    cm_data = (cm_data_t*)data;

    if (!bt_sal_connecting_list) {
        BT_LOGW("%s, bt_sal_connecting_list is NULL", __func__);
        cm_data_destory(cm_data);
        return;
    }

    manager = bt_list_find(bt_sal_connecting_list, bt_connection_manager_find, &cm_data->addr);

    if (manager != NULL) {
        bt_sal_trigger_profile_conn_act(manager, bt_sal_connecting_list);
    }

    cm_data_destory(cm_data);
}

static void bt_sal_cm_acl_disconnected(void* data)
{
    if (data == NULL)
        return;

    cm_data_t* cm_data = data;
    bt_profile_connection_manager_t* manager;

    if (bt_sal_connecting_list != NULL) {
        manager = bt_list_find(bt_sal_connecting_list, bt_connection_manager_find, &cm_data->addr);
        if (manager != NULL) {
            /*
             * Trigger pending profile handlers to let them detect ACL failure
             * and clean up properly. The handlers will fail (e.g., bt_conn_lookup
             * returns NULL) and report disconnected state to upper layer.
             */
            if (bt_list_is_empty(manager->profile_conn_handler_list)) {
                /* No pending handlers, remove manager directly */
                bt_list_remove(bt_sal_connecting_list, manager);
            } else {
                bt_sal_trigger_profile_conn_act(manager, bt_sal_connecting_list);
            }
        }
    }

    if (bt_sal_disconnecting_list != NULL) {
        manager = bt_list_find(bt_sal_disconnecting_list, bt_connection_manager_find, &cm_data->addr);
        if (manager != NULL) {
            if (manager->is_unpair) {
                /* must call stack api in service worker */
                bt_sal_remove_bond_internal(PRIMARY_ADAPTER, &manager->device_addr);
            }
            bt_list_remove(bt_sal_disconnecting_list, manager);
        }
    }

    cm_data_destory(cm_data);
}

void bt_sal_cm_profile_connected_callback(bt_address_t* addr, uint8_t profile_id,
    uint16_t conn_id)
{
    /*
     * To avoid generating duplicate profile connect requests for an
     * already-connected profile, we do not change the manager state.
     */
    return;
}

void bt_sal_cm_profile_disconnected_callback(bt_address_t* addr, uint8_t profile_id,
    uint16_t conn_id)
{
    if (!addr) {
        return;
    }

    remove_from_connection_manager_list(
        bt_sal_connecting_list,
        addr,
        profile_id,
        conn_id,
        false);

    remove_from_connection_manager_list(
        bt_sal_disconnecting_list,
        addr,
        profile_id,
        conn_id,
        true);
}

void bt_sal_cm_acl_connected_callback(cm_data_t* data)
{
    if (data == NULL)
        return;

    do_in_service_loop(bt_sal_cm_acl_connected, data);
}

void bt_sal_cm_acl_disconnected_callback(cm_data_t* data)
{
    if (data == NULL)
        return;

    do_in_service_loop(bt_sal_cm_acl_disconnected, data);
}

bt_status_t bt_sal_cm_try_disconnect_profiles(bt_address_t* addr, bool is_unpair)
{
    bt_profile_connection_manager_t* manager;
    struct bt_conn* conn;
    struct bt_conn_info info;

    if (!addr)
        return BT_STATUS_PARM_INVALID;

    if (bt_sal_disconnecting_list == NULL)
        return BT_STATUS_FAIL;

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        return BT_STATUS_FAIL;
    }

    if (bt_conn_get_info(conn, &info) < 0) {
        bt_conn_unref(conn);
        return BT_STATUS_FAIL;
    }

    bt_conn_unref(conn);

    if (info.state != BT_CONN_STATE_CONNECTED && info.state != BT_CONN_STATE_DISCONNECTING) {
        return BT_STATUS_NOT_READY;
    }

    manager = bt_list_find(bt_sal_disconnecting_list, bt_connection_manager_find, addr);

    if (manager) {
        if (manager->is_unpair) {
            BT_LOGD("removeboned procedure still running");
            return BT_STATUS_SUCCESS;
        }

        manager->is_unpair = manager->is_unpair || is_unpair;

        if (info.state == BT_CONN_STATE_DISCONNECTING) {
            BT_LOGD("ACL disconnecting procedure still running");
            return BT_STATUS_SUCCESS;
        }

        if (bt_list_is_empty(manager->profile_conn_handler_list)) {
            return bt_sal_disconnect_internal(PRIMARY_ADAPTER, addr, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        }

        return bt_sal_trigger_profile_conn_act(manager, bt_sal_disconnecting_list);
    }

    manager = create_connection_manager(addr);
    if (!manager) {
        BT_LOGE("%s, malloc failed", __func__);
        return BT_STATUS_NOMEM;
    }

    manager->is_unpair = is_unpair;
    bt_list_add_tail(bt_sal_disconnecting_list, manager);

    return bt_sal_disconnect_internal(PRIMARY_ADAPTER, addr, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static bt_status_t bt_sal_try_profile_connect(bt_address_t* addr)
{
    bt_profile_connection_manager_t* manager;
    struct bt_conn* conn;
    struct bt_conn_info info;

    if (!addr)
        return BT_STATUS_PARM_INVALID;

    manager = find_or_create_connection_manager(bt_sal_connecting_list, addr);
    if (!manager)
        return BT_STATUS_NOMEM;

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        return bt_sal_connect(PRIMARY_ADAPTER, addr);
    }

    if (bt_conn_get_info(conn, &info) < 0) {
        bt_conn_unref(conn);
        return BT_STATUS_FAIL;
    }

    bt_conn_unref(conn);

    switch (info.state) {
    case BT_CONN_STATE_CONNECTING:
        return BT_STATUS_SUCCESS;
    case BT_CONN_STATE_DISCONNECTED:
        return bt_sal_connect(PRIMARY_ADAPTER, addr);
    case BT_CONN_STATE_DISCONNECTING:
        return BT_STATUS_BUSY;
    case BT_CONN_STATE_CONNECTED:
    default:
        break;
    }

    return bt_sal_trigger_profile_conn_act(manager, bt_sal_connecting_list);
}

bt_status_t bt_sal_profile_connect_request(bt_address_t* addr, uint8_t profile_id, uint16_t conn_id, bt_controller_id_t id,
    bt_profile_conn_handler_t handler, void* user_data)
{
    bt_profile_connection_manager_t* manager;
    bt_profile_conn_handler_node_t* handler_node;

    if (!addr || !handler)
        return BT_STATUS_PARM_INVALID;

    manager = find_or_create_connection_manager(bt_sal_connecting_list, addr);
    if (!manager) {
        return BT_STATUS_NOMEM;
    }

    if (find_handler_node(manager, profile_id, conn_id)) {
        return BT_STATUS_SUCCESS;
    }

    handler_node = (bt_profile_conn_handler_node_t*)zalloc(sizeof(bt_profile_conn_handler_node_t));
    if (!handler_node) {
        return BT_STATUS_NOMEM;
    }

    handler_node->handler = handler;
    handler_node->profile_id = profile_id;
    handler_node->conn_id = conn_id;
    handler_node->id = id;
    handler_node->user_data = user_data;
    bt_list_add_tail(manager->profile_conn_handler_list, handler_node);

    return bt_sal_try_profile_connect(addr);
}

bt_status_t bt_sal_profile_disconnect_register(bt_address_t* addr, uint8_t profile_id, uint16_t conn_id, bt_controller_id_t id,
    bt_profile_conn_handler_t handler, void* user_data)
{
    bt_profile_connection_manager_t* manager;
    bt_profile_conn_handler_node_t* handler_node;

    if (!addr || !handler)
        return BT_STATUS_PARM_INVALID;

    manager = find_or_create_connection_manager(bt_sal_disconnecting_list, addr);
    if (!manager) {
        return BT_STATUS_NOMEM;
    }

    if (find_handler_node(manager, profile_id, conn_id)) {
        return BT_STATUS_SUCCESS;
    }

    handler_node = (bt_profile_conn_handler_node_t*)zalloc(sizeof(bt_profile_conn_handler_node_t));
    if (!handler_node) {
        return BT_STATUS_NOMEM;
    }

    handler_node->handler = handler;
    handler_node->profile_id = profile_id;
    handler_node->conn_id = conn_id;
    handler_node->id = id;
    handler_node->user_data = user_data;
    bt_list_add_tail(manager->profile_conn_handler_list, handler_node);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_profile_disconnect_request(bt_address_t* addr, uint8_t profile_id, uint16_t conn_id, bt_controller_id_t id,
    bt_profile_conn_handler_t handler, void* user_data)
{
    sal_async_profile_req_t* req;

    /* async invoke to service_worker thread */
    req = sal_async_profile_req(addr, handler, profile_id, conn_id, id, NULL, user_data);

    if (sal_send_async_req(req) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, profile_id: %u", __func__, profile_id);
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

void bt_sal_cm_conn_cleanup(void)
{
    bt_list_free(bt_sal_disconnecting_list);
    bt_sal_disconnecting_list = NULL;

    bt_list_free(bt_sal_connecting_list);
    bt_sal_connecting_list = NULL;
}