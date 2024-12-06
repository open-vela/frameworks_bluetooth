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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <sys/types.h>

#include "bt_config.h"
#include "bt_list.h"
#include "bt_profile.h"
#include "gattc_event.h"
#include "gattc_service.h"
#include "index_allocator.h"
#include "sal_gatt_client_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_gattc_manager.started)     \
            return BT_STATUS_NOT_ENABLED; \
    }

#define CHECK_CONNECTION_VALID(_list, _conn)                                                   \
    do {                                                                                       \
        bt_list_node_t* _node;                                                                 \
        if (!_conn)                                                                            \
            return BT_STATUS_PARM_INVALID;                                                     \
        for (_node = bt_list_head(_list); _node != NULL; _node = bt_list_next(_list, _node)) { \
            if (bt_list_node(_node) == _conn)                                                  \
                break;                                                                         \
        }                                                                                      \
        if (!_node)                                                                            \
            return BT_STATUS_PARM_INVALID;                                                     \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
    bool started;
    index_allocator_t* allocator;
    bt_list_t* connections;
    pthread_mutex_t device_lock;

} gattc_manager_t;

typedef struct
{
    bt_uuid_t* uuid;
    uint16_t start_handle;
    uint16_t end_handle;
    int element_size;
    gatt_element_t* elements;

} gattc_service_t;

typedef struct
{
    void* remote;
    int conn_id;
    profile_connection_state_t state;
    pthread_mutex_t conn_lock;
    void** user_phandle;
    bt_address_t remote_addr;
    gattc_manager_t* manager;
    gattc_callbacks_t* callbacks;
    bt_list_t* services;
    bt_list_t* pend_ops;

} gattc_connection_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static gattc_manager_t g_gattc_manager = {
    .started = false,
    .connections = NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool connection_addr_cmp(void* connection, void* addr)
{
    return bt_addr_compare(&((gattc_connection_t*)connection)->remote_addr, addr) == 0;
}

static gattc_connection_t* find_gattc_connection_by_addr(bt_address_t* addr)
{
    return bt_list_find(g_gattc_manager.connections, connection_addr_cmp, addr);
}

static gattc_connection_t* gattc_connection_new(gattc_callbacks_t* callbacks)
{
    int new_id = index_alloc(g_gattc_manager.allocator);
    if (new_id < 0)
        return NULL;

    gattc_connection_t* connection = calloc(1, sizeof(gattc_connection_t));
    if (!connection) {
        index_free(g_gattc_manager.allocator, new_id);
        return NULL;
    }

    connection->conn_id = new_id;
    connection->callbacks = callbacks;
    connection->services = NULL;
    connection->pend_ops = NULL;

    return connection;
}

static void gattc_connection_delete(gattc_connection_t* connection)
{
    if (!connection)
        return;

    if (connection->state == PROFILE_STATE_CONNECTING || connection->state == PROFILE_STATE_CONNECTED)
        bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &connection->remote_addr);
    index_free(g_gattc_manager.allocator, connection->conn_id);
    bt_list_free(connection->services);
    connection->services = NULL;
    bt_list_free(connection->pend_ops);
    connection->pend_ops = NULL;
    pthread_mutex_destroy(&connection->conn_lock);
    free(connection);
}

static bool attribute_handle_cmp(void* service, void* handle)
{
    uint16_t f_handle = *(uint16_t*)handle;
    gattc_service_t* f_service = (gattc_service_t*)service;
    return (f_handle >= f_service->start_handle && f_handle <= f_service->end_handle);
}

static gatt_element_t* find_gattc_element_by_handle(gattc_connection_t* connection, uint16_t handle)
{
    gattc_service_t* service = bt_list_find(connection->services, attribute_handle_cmp, &handle);
    if (!service)
        return NULL;

    gatt_element_t* element = service->elements;
    for (int i = 0; i < service->element_size; i++, element++) {
        if (element->handle == handle)
            return element;
    }

    return NULL;
}

static gatt_element_t* find_gattc_element_by_uuid(gattc_connection_t* connection, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid)
{
    bt_list_node_t* node;
    bt_list_t* list = connection->services;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        gattc_service_t* service = (gattc_service_t*)bt_list_node(node);
        if (service->end_handle < start_handle || service->start_handle > end_handle)
            continue;

        gatt_element_t* element = service->elements;
        for (int i = 0; i < service->element_size; i++, element++) {
            if (element->handle >= start_handle && element->handle <= end_handle && !bt_uuid_compare(&element->uuid, attr_uuid)) {
                return element;
            }
        }
    }

    return NULL;
}

static gattc_service_t* gattc_service_new(bt_uuid_t* uuid)
{
    gattc_service_t* service = malloc(sizeof(gattc_service_t));
    if (!service)
        return NULL;

    service->uuid = uuid;
    service->start_handle = 0;
    service->end_handle = 0;
    service->elements = NULL;
    service->element_size = 0;

    return service;
}

static void gattc_service_delete(gattc_service_t* service)
{
    if (!service)
        return;

    if (service->elements)
        free(service->elements);
    free(service);
}

static void gattc_pendops_delete(gattc_op_t* operation)
{
    if (!operation)
        return;

    free(operation);
}

static void gattc_process_message(void* data)
{
    gattc_msg_t* msg = (gattc_msg_t*)data;
    gattc_connection_t* connection;

    pthread_mutex_lock(&g_gattc_manager.device_lock);
    if (!g_gattc_manager.started) {
        goto end;
    }

    connection = find_gattc_connection_by_addr(&msg->addr);
    if (!connection) {
        goto end;
    }

    switch (msg->event) {
    case GATTC_EVENT_CONNECT_CHANGE: {
        profile_connection_state_t connect_state = msg->param.connect_change.state;
        BT_ADDR_LOG("GATTC-CONNECTION-STATE-EVENT from:%s, state:%d", &connection->remote_addr, connect_state);
        if (connect_state == PROFILE_STATE_CONNECTED) {
            connection->state = connect_state;
            GATT_CBACK(connection->callbacks, on_connected, connection, &connection->remote_addr);
        } else if (connect_state == PROFILE_STATE_DISCONNECTED) {
            connection->state = connect_state;
            GATT_CBACK(connection->callbacks, on_disconnected, connection, &connection->remote_addr);
            bt_addr_set_empty(&connection->remote_addr);
            bt_list_clear(connection->services);
        }
    } break;
    case GATTC_EVENT_DISCOVER_RESULT: {
        gattc_service_t* service = bt_list_find(connection->services, attribute_handle_cmp, &msg->param.discover_res.elements[0].handle);
        if (service) {
            bt_list_remove(connection->services, service);
        }

        service = gattc_service_new(&msg->param.discover_res.elements->uuid);
        service->start_handle = msg->param.discover_res.elements[0].handle;
        service->end_handle = msg->param.discover_res.elements[msg->param.discover_res.size - 1].handle;
        service->elements = msg->param.discover_res.elements;
        service->element_size = msg->param.discover_res.size;
        bt_list_add_tail(connection->services, service);

        GATT_CBACK(connection->callbacks, on_discovered, connection, GATT_STATUS_SUCCESS, service->uuid, service->start_handle, service->end_handle);
    } break;
    case GATTC_EVENT_DISOCVER_CMPL: {
        GATT_CBACK(connection->callbacks, on_discovered, connection, msg->param.discover_cmpl.status, NULL, 0, 0);
    } break;
    case GATTC_EVENT_READ: {
        GATT_CBACK(connection->callbacks, on_read, connection, msg->param.read.status, msg->param.read.element_id, msg->param.read.value, msg->param.read.length);
    } break;
    case GATTC_EVENT_WRITE: {
        GATT_CBACK(connection->callbacks, on_written, connection, msg->param.write.status, msg->param.write.element_id);
    } break;
    case GATTC_EVENT_SUBSCRIBE: {
        gatt_element_t* element = find_gattc_element_by_handle(connection, msg->param.subscribe.element_id);
        if (element) {
            if (msg->param.subscribe.status == GATT_STATUS_SUCCESS) {
                element->notify_enable = msg->param.subscribe.enable;
            }
            GATT_CBACK(connection->callbacks, on_subscribed, connection, msg->param.subscribe.status, msg->param.subscribe.element_id, msg->param.subscribe.enable);
        } else {
            BT_LOGE("GATTC receives a subscribe event with unknown element (id:0x%04x)", msg->param.subscribe.element_id);
        }
    } break;
    case GATTC_EVENT_NOTIFY: {
        gatt_element_t* element = find_gattc_element_by_handle(connection, msg->param.notify.element_id);
        if (element && element->notify_enable) {
            GATT_CBACK(connection->callbacks, on_notified, connection, msg->param.notify.element_id, msg->param.notify.value, msg->param.notify.length);
        }
    } break;
    case GATTC_EVENT_MTU_UPDATE: {
        GATT_CBACK(connection->callbacks, on_mtu_updated, connection, msg->param.mtu.status, msg->param.mtu.mtu);
    } break;
    case GATTC_EVENT_PHY_READ: {
        GATT_CBACK(connection->callbacks, on_phy_read, connection, msg->param.phy.tx_phy, msg->param.phy.rx_phy);
    } break;
    case GATTC_EVENT_PHY_UPDATE: {
        GATT_CBACK(connection->callbacks, on_phy_updated, connection, msg->param.phy.status, msg->param.phy.tx_phy, msg->param.phy.rx_phy);
    } break;
    case GATTC_EVENT_RSSI_READ: {
        GATT_CBACK(connection->callbacks, on_rssi_read, connection, msg->param.rssi_read.status, msg->param.rssi_read.rssi);
    } break;
    case GATTC_EVENT_CONN_PARAM_UPDATE: {
        GATT_CBACK(connection->callbacks, on_conn_param_updated, connection, msg->param.conn_param.status, msg->param.conn_param.interval,
            msg->param.conn_param.latency, msg->param.conn_param.timeout);
    } break;
    default: {

    } break;
    }

end:
    pthread_mutex_unlock(&g_gattc_manager.device_lock);
    gattc_msg_destory(msg);
}

static bt_status_t gattc_send_message(gattc_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(gattc_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gattc_init(void)
{
    pthread_mutexattr_t attr;

    memset(&g_gattc_manager, 0, sizeof(g_gattc_manager));

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_gattc_manager.device_lock, &attr) < 0)
        return BT_STATUS_FAIL;

    g_gattc_manager.started = false;

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gattc_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    gattc_manager_t* manager = &g_gattc_manager;

    pthread_mutex_lock(&manager->device_lock);
    if (manager->started) {
        pthread_mutex_unlock(&manager->device_lock);
        cb(PROFILE_GATTC, true);
        return BT_STATUS_SUCCESS;
    }

    manager->allocator = index_allocator_create(CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS - 1);
    if (!manager->allocator) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    manager->connections = bt_list_new((bt_list_free_cb_t)gattc_connection_delete);
    if (!manager->connections) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    manager->started = true;
    pthread_mutex_unlock(&manager->device_lock);
    cb(PROFILE_GATTC, true);

    return BT_STATUS_SUCCESS;

fail:
    index_allocator_delete(&manager->allocator);
    bt_list_free(manager->connections);
    manager->connections = NULL;
    pthread_mutex_unlock(&manager->device_lock);
    cb(PROFILE_GATTC, false);

    return status;
}

static bt_status_t if_gattc_shutdown(profile_on_shutdown_t cb)
{
    gattc_manager_t* manager = &g_gattc_manager;

    pthread_mutex_lock(&manager->device_lock);

    if (!manager->started) {
        pthread_mutex_unlock(&manager->device_lock);
        cb(PROFILE_GATTC, true);
        return BT_STATUS_SUCCESS;
    }

    bt_list_free(manager->connections);
    manager->connections = NULL;
    index_allocator_delete(&manager->allocator);
    manager->started = false;
    cb(PROFILE_GATTC, true);
    pthread_mutex_unlock(&manager->device_lock);
    cb(PROFILE_GATTC, true);

    return BT_STATUS_SUCCESS;
}

static void if_gattc_cleanup(void)
{
    g_gattc_manager.started = false;
    pthread_mutex_destroy(&g_gattc_manager.device_lock);
}

static int if_gattc_get_state(void)
{
    return 1;
}

static int if_gattc_dump(void)
{
    bt_list_node_t* cnode;
    bt_list_t* clist = g_gattc_manager.connections;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    char uuid_str[40] = { 0 };

    pthread_mutex_lock(&g_gattc_manager.device_lock);

    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        gattc_connection_t* connection = (gattc_connection_t*)bt_list_node(cnode);
        bt_list_node_t* snode;
        bt_list_t* slist = connection->services;
        int s_id = 0;

        bt_addr_ba2str(&connection->remote_addr, addr_str);
        BT_LOGI("GATT Client[%d]: State:%d, Peer:%s", connection->conn_id, connection->state, addr_str);
        for (snode = bt_list_head(slist); snode != NULL; snode = bt_list_next(slist, snode)) {
            gattc_service_t* service = (gattc_service_t*)bt_list_node(snode);
            gatt_element_t* element = service->elements;

            BT_LOGI("\tAttribute Table[%d]: Handle:0x%04x~0x%04x, Num:%d", s_id++, service->start_handle, service->end_handle, service->element_size);
            for (int i = 0; i < service->element_size; i++, element++) {
                bt_uuid_to_string(&element->uuid, uuid_str, 40);
                BT_LOGI("\t\t>[0x%04x][Type:%d][Prop:%04x][UUID:%s]", element->handle, element->type, element->properties,
                    uuid_str);
            }
        }

        if (bt_list_is_empty(slist))
            BT_LOGI("\tNo Services found");
    }

    pthread_mutex_unlock(&g_gattc_manager.device_lock);

    return 0;
}

static bt_status_t if_gattc_create_connect(void* remote, void** phandle, gattc_callbacks_t* callbacks)
{
    bt_status_t status;
    pthread_mutexattr_t attr;

    CHECK_ENABLED();
    if (!phandle)
        return BT_STATUS_PARM_INVALID;

    pthread_mutex_lock(&g_gattc_manager.device_lock);
    gattc_connection_t* connection = gattc_connection_new(callbacks);
    if (!connection) {
        pthread_mutex_unlock(&g_gattc_manager.device_lock);
        BT_LOGE("New gattc connection alloc failed");
        return BT_STATUS_NOMEM;
    }

    connection->services = bt_list_new((bt_list_free_cb_t)gattc_service_delete);
    if (!connection->services) {
        pthread_mutex_unlock(&g_gattc_manager.device_lock);
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    connection->pend_ops = bt_list_new((bt_list_free_cb_t)gattc_pendops_delete);
    if (!connection->pend_ops) {
        pthread_mutex_unlock(&g_gattc_manager.device_lock);
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    bt_list_add_tail(g_gattc_manager.connections, connection);
    pthread_mutex_unlock(&g_gattc_manager.device_lock);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&connection->conn_lock, &attr);

    connection->remote = remote;
    connection->manager = &g_gattc_manager;
    connection->state = PROFILE_STATE_DISCONNECTED;
    connection->user_phandle = phandle;
    *phandle = connection;

    return BT_STATUS_SUCCESS;

fail:
    gattc_connection_delete(connection);
    return status;
}

static bt_status_t if_gattc_delete_connect(void* conn_handle)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    void** user_phandle = connection->user_phandle;
    bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &connection->remote_addr);
    bt_list_free(connection->services);
    connection->services = NULL;
    pthread_mutex_lock(&g_gattc_manager.device_lock);
    bt_list_remove(g_gattc_manager.connections, connection);
    pthread_mutex_unlock(&g_gattc_manager.device_lock);
    *user_phandle = NULL;

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gattc_connect(void* conn_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    BT_ADDR_LOG("GATTC-CONNECT-REQUEST addr:%s", addr);
    bt_status_t status = bt_sal_gatt_client_connect(PRIMARY_ADAPTER, addr, addr_type);
    if (status == BT_STATUS_SUCCESS) {
        connection->state = PROFILE_STATE_CONNECTING;
        memcpy(&connection->remote_addr, addr, sizeof(connection->remote_addr));
    }

    return status;
}

static bt_status_t if_gattc_disconnect(void* conn_handle)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    BT_ADDR_LOG("GATTC-DISCONNECT-REQUEST addr:%s", &connection->remote_addr);
    bt_status_t status = bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &connection->remote_addr);
    if (status == BT_STATUS_SUCCESS)
        connection->state = PROFILE_STATE_DISCONNECTING;

    return status;
}

static bt_status_t if_gattc_discover_service(void* conn_handle, bt_uuid_t* filter_uuid)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    bt_status_t status;
    if (!filter_uuid || !filter_uuid->type) {
        status = bt_sal_gatt_client_discover_all_services(PRIMARY_ADAPTER, &connection->remote_addr);
    } else {
        status = bt_sal_gatt_client_discover_service_by_uuid(PRIMARY_ADAPTER, &connection->remote_addr, filter_uuid);
    }

    return status;
}

static bt_status_t if_gattc_get_attribute_by_handle(void* conn_handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);
    if (!attr_desc)
        return BT_STATUS_PARM_INVALID;

    gatt_element_t* element = find_gattc_element_by_handle(connection, attr_handle);
    if (!element)
        return BT_STATUS_NO_RESOURCES;

    attr_desc->handle = element->handle;
    attr_desc->type = element->type;
    attr_desc->properties = element->properties;
    memcpy(&attr_desc->uuid, &element->uuid, sizeof(attr_desc->uuid));

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gattc_get_attribute_by_uuid(void* conn_handle, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);
    if (!attr_desc)
        return BT_STATUS_PARM_INVALID;

    gatt_element_t* element = find_gattc_element_by_uuid(connection, start_handle, end_handle, attr_uuid);
    if (!element)
        return BT_STATUS_NO_RESOURCES;

    attr_desc->handle = element->handle;
    attr_desc->type = element->type;
    attr_desc->properties = element->properties;
    memcpy(&attr_desc->uuid, &element->uuid, sizeof(attr_desc->uuid));

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gattc_read(void* conn_handle, uint16_t attr_handle)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    return bt_sal_gatt_client_read_element(PRIMARY_ADAPTER, &connection->remote_addr, attr_handle);
}

static bt_status_t if_gattc_write(void* conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    return bt_sal_gatt_client_write_element(PRIMARY_ADAPTER, &connection->remote_addr, attr_handle,
        value, length, GATT_WRITE_TYPE_RSP);
}

static bt_status_t if_gattc_write_without_response(void* conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    return bt_sal_gatt_client_write_element(PRIMARY_ADAPTER, &connection->remote_addr, attr_handle,
        value, length, GATT_WRITE_TYPE_NO_RSP);
}

static bt_status_t if_gattc_subscribe(void* conn_handle, uint16_t attr_handle, uint16_t ccc_value)
{
    gattc_connection_t* connection = conn_handle;
    gatt_element_t* element;
    uint16_t properties;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    element = find_gattc_element_by_handle(connection, attr_handle);
    if (!element) {
        return BT_STATUS_NOT_FOUND;
    }

    if (ccc_value & GATT_CCC_NOTIFY) {
        if (!(element->properties & GATT_PROP_NOTIFY)) {
            return BT_STATUS_NOT_SUPPORTED;
        }
        properties = GATT_PROP_NOTIFY;
    } else if (ccc_value & GATT_CCC_INDICATE) {
        if (!(element->properties & GATT_PROP_INDICATE)) {
            return BT_STATUS_NOT_SUPPORTED;
        }
        properties = GATT_PROP_INDICATE;
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    return bt_sal_gatt_client_register_notifications(PRIMARY_ADAPTER, &connection->remote_addr, attr_handle, properties, true);
}

static bt_status_t if_gattc_unsubscribe(void* conn_handle, uint16_t attr_handle)
{
    gattc_connection_t* connection = conn_handle;
    gatt_element_t* element;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    element = find_gattc_element_by_handle(connection, attr_handle);
    if (!element) {
        return BT_STATUS_NOT_FOUND;
    }

    if (!(element->properties & (GATT_PROP_NOTIFY | GATT_PROP_INDICATE))) {
        return BT_STATUS_NOT_SUPPORTED;
    }

    return bt_sal_gatt_client_register_notifications(PRIMARY_ADAPTER, &connection->remote_addr, attr_handle, element->properties, false);
}

static bt_status_t if_gattc_exchange_mtu(void* conn_handle, uint32_t mtu)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    if (mtu > GATT_MAX_MTU_SIZE) {
        mtu = GATT_MAX_MTU_SIZE;
    }

    return bt_sal_gatt_client_send_mtu_req(PRIMARY_ADAPTER, &connection->remote_addr, mtu);
}

static bt_status_t if_gattc_update_connection_parameter(void* conn_handle, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    return bt_sal_gatt_client_update_connection_parameter(PRIMARY_ADAPTER, &connection->remote_addr, min_interval, max_interval, latency,
        timeout, min_connection_event_length, max_connection_event_length);
}

static bt_status_t if_gattc_read_phy(void* conn_handle)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    return bt_sal_gatt_client_read_phy(PRIMARY_ADAPTER, &connection->remote_addr);
}

static bt_status_t if_gattc_update_phy(void* conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    return bt_sal_gatt_client_set_phy(PRIMARY_ADAPTER, &connection->remote_addr, tx_phy, rx_phy);
}

static bt_status_t if_gattc_read_rssi(void* conn_handle)
{
    gattc_connection_t* connection = conn_handle;

    CHECK_ENABLED();
    CHECK_CONNECTION_VALID(g_gattc_manager.connections, connection);

    return bt_sal_gatt_client_read_remote_rssi(PRIMARY_ADAPTER, &connection->remote_addr);
}

static const gattc_interface_t gattc_if = {
    .size = sizeof(gattc_if),
    .create_connect = if_gattc_create_connect,
    .delete_connect = if_gattc_delete_connect,
    .connect = if_gattc_connect,
    .disconnect = if_gattc_disconnect,
    .discover_service = if_gattc_discover_service,
    .get_attribute_by_handle = if_gattc_get_attribute_by_handle,
    .get_attribute_by_uuid = if_gattc_get_attribute_by_uuid,
    .read = if_gattc_read,
    .write = if_gattc_write,
    .write_without_response = if_gattc_write_without_response,
    .subscribe = if_gattc_subscribe,
    .unsubscribe = if_gattc_unsubscribe,
    .exchange_mtu = if_gattc_exchange_mtu,
    .update_connection_parameter = if_gattc_update_connection_parameter,
    .read_phy = if_gattc_read_phy,
    .update_phy = if_gattc_update_phy,
    .read_rssi = if_gattc_read_rssi,
};

static const void* get_gattc_profile_interface(void)
{
    return (void*)&gattc_if;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void if_gattc_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_CONNECT_CHANGE, addr, 0);
    msg->param.connect_change.state = state;
    msg->param.connect_change.reason = 0;
    gattc_send_message(msg);
}

void if_gattc_on_service_discovered(bt_address_t* addr, gatt_element_t* elements, uint16_t size)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_DISCOVER_RESULT, addr, 0);
    msg->param.discover_res.elements = elements;
    msg->param.discover_res.size = size;
    gattc_send_message(msg);
}

void if_gattc_on_discover_completed(bt_address_t* addr, gatt_status_t status)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_DISOCVER_CMPL, addr, 0);
    msg->param.discover_cmpl.status = status;
    gattc_send_message(msg);
}

void if_gattc_on_element_read(bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length, gatt_status_t status)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_READ, addr, length);
    msg->param.read.status = status;
    msg->param.read.element_id = element_id;
    msg->param.read.length = length;
    memcpy(msg->param.read.value, value, length);
    gattc_send_message(msg);
}

void if_gattc_on_element_written(bt_address_t* addr, uint16_t element_id, gatt_status_t status)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_WRITE, addr, 0);
    msg->param.write.status = status;
    msg->param.write.element_id = element_id;
    gattc_send_message(msg);
}

void if_gattc_on_element_subscribed(bt_address_t* addr, uint16_t element_id, gatt_status_t status, bool enable)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_SUBSCRIBE, addr, 0);
    msg->param.subscribe.status = status;
    msg->param.subscribe.element_id = element_id;
    msg->param.subscribe.enable = enable;
    gattc_send_message(msg);
}

void if_gattc_on_element_changed(bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_NOTIFY, addr, length);
    msg->param.notify.is_notify = true;
    msg->param.notify.element_id = element_id;
    msg->param.notify.length = length;
    memcpy(msg->param.notify.value, value, length);
    gattc_send_message(msg);
}

void if_gattc_on_mtu_changed(bt_address_t* addr, uint32_t mtu, gatt_status_t status)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_MTU_UPDATE, addr, 0);
    msg->param.mtu.status = status;
    msg->param.mtu.mtu = mtu;
    gattc_send_message(msg);
}

void if_gattc_on_phy_read(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_PHY_READ, addr, 0);
    msg->param.phy.tx_phy = tx_phy;
    msg->param.phy.rx_phy = rx_phy;
    gattc_send_message(msg);
}

void if_gattc_on_phy_updated(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, gatt_status_t status)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_PHY_UPDATE, addr, 0);
    msg->param.phy.status = status;
    msg->param.phy.tx_phy = tx_phy;
    msg->param.phy.rx_phy = rx_phy;
    gattc_send_message(msg);
}

void if_gattc_on_rssi_read(bt_address_t* addr, int32_t rssi, gatt_status_t status)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_RSSI_READ, addr, 0);
    msg->param.rssi_read.status = status;
    msg->param.rssi_read.rssi = rssi;
    gattc_send_message(msg);
}

void if_gattc_on_connection_parameter_updated(bt_address_t* addr, uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout, bt_status_t status)
{
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_CONN_PARAM_UPDATE, addr, 0);
    msg->param.conn_param.status = status;
    msg->param.conn_param.interval = connection_interval;
    msg->param.conn_param.latency = peripheral_latency;
    msg->param.conn_param.timeout = supervision_timeout;
    gattc_send_message(msg);
}

void* if_gattc_get_remote(void* conn_handle)
{
    if (!conn_handle)
        return NULL;

    gattc_connection_t* connection = conn_handle;
    return connection->remote;
}

static const profile_service_t gattc_service = {
    .auto_start = true,
    .name = PROFILE_GATTC_NAME,
    .id = PROFILE_GATTC,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = if_gattc_init,
    .startup = if_gattc_startup,
    .shutdown = if_gattc_shutdown,
    .process_msg = NULL,
    .get_state = if_gattc_get_state,
    .get_profile_interface = get_gattc_profile_interface,
    .cleanup = if_gattc_cleanup,
    .dump = if_gattc_dump,
};

void register_gattc_service(void)
{
    register_service(&gattc_service);
}
