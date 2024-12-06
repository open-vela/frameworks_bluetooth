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
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <sys/types.h>

#include "bt_list.h"
#include "bt_profile.h"
#include "gatts_event.h"
#include "gatts_service.h"
#include "sal_gatt_server_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_gatts_manager.started)     \
            return BT_STATUS_NOT_ENABLED; \
    }

#define CHECK_SERVICE_VALID(_list, _srv)                                                       \
    do {                                                                                       \
        bt_list_node_t* _node;                                                                 \
        if (!_srv)                                                                             \
            return BT_STATUS_PARM_INVALID;                                                     \
        for (_node = bt_list_head(_list); _node != NULL; _node = bt_list_next(_list, _node)) { \
            if (bt_list_node(_node) == _srv)                                                   \
                break;                                                                         \
        }                                                                                      \
        if (!_node)                                                                            \
            return BT_STATUS_PARM_INVALID;                                                     \
    } while (0)

#define GATTS_CALLBACK_FOREACH(_cbsl, _type, _cback, args...)                                  \
    do {                                                                                       \
        bt_list_node_t* _node;                                                                 \
        bt_list_t* _list = _cbsl;                                                              \
        for (_node = bt_list_head(_list); _node != NULL; _node = bt_list_next(_list, _node)) { \
            _type* _inst = (_type*)bt_list_node(_node);                                        \
            if (_inst->callbacks && _inst->callbacks->_cback)                                  \
                _inst->callbacks->_cback(_inst, args);                                         \
        }                                                                                      \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
    bool started;
    pthread_mutex_t device_lock;
    bt_list_t* services;
    bt_list_t* pend_ops;

} gatts_manager_t;

typedef struct
{
    uint16_t start_handle;
    uint16_t end_handle;
    int element_size;
    gatt_element_t elements[0];

} service_table_t;

typedef struct
{
    void* remote;
    uint16_t srv_id;
    pthread_mutex_t srv_lock;
    void** user_phandle;
    gatts_manager_t* manager;
    gatts_callbacks_t* callbacks;
    bt_list_t* tables;

} gatts_service_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static gatts_manager_t g_gatts_manager = {
    .started = false,
    .services = NULL,
    .pend_ops = NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool element_id_cmp(void* table, void* id)
{
    uint16_t f_handle = *(uint16_t*)id;
    service_table_t* f_svc_table = (service_table_t*)table;
    return (f_handle >= f_svc_table->start_handle && f_handle <= f_svc_table->end_handle);
}

static service_table_t* find_service_table_by_id(gatts_service_t* service, uint16_t element_id)
{
    return bt_list_find(service->tables, element_id_cmp, &element_id);
}

static gatt_element_t* find_service_element_by_id(gatts_service_t* service, uint16_t element_id)
{
    service_table_t* svc_table = find_service_table_by_id(service, element_id);
    if (!svc_table)
        return NULL;

    gatt_element_t* element = svc_table->elements;
    for (int i = 0; i < svc_table->element_size; i++, element++) {
        if (element->handle == element_id)
            return element;
    }
    return NULL;
}

static bool service_id_cmp(void* service, void* id)
{
    return (((gatts_service_t*)service)->srv_id == (*((uint16_t*)id)));
}

static gatts_service_t* find_gatts_service_by_id(uint16_t srv_id)
{
    srv_id = GATT_ELEMENT_GROUP_ID(srv_id);
    return bt_list_find(g_gatts_manager.services, service_id_cmp, &srv_id);
}

static uint16_t generate_service_id(void)
{
    for (uint16_t i = 0x100; i < GATT_ELEMENT_GROUP_MAX; i += 0x100) {
        if (find_gatts_service_by_id(i) == NULL) {
            return i;
        }
    }
    BT_LOGE("service id overflow");
    return 0;
}

static service_table_t* service_table_new(int element_size)
{
    service_table_t* table = malloc(sizeof(service_table_t) + sizeof(gatt_element_t) * element_size);
    if (!table)
        return NULL;

    table->element_size = element_size;

    return table;
}

static void service_table_delete(service_table_t* table)
{
    if (!table)
        return;

    gatt_element_t* elements = table->elements;
    for (int i = 0; i < table->element_size; i++, elements++) {
        if (elements->attr_data)
            free(elements->attr_data);
    }

    free(table);
}

static gatts_service_t* gatts_service_new(gatts_callbacks_t* callbacks)
{
    uint16_t new_id = generate_service_id();
    if (!new_id)
        return NULL;

    gatts_service_t* service = calloc(1, sizeof(gatts_service_t));
    if (!service)
        return NULL;

    service->tables = bt_list_new((bt_list_free_cb_t)service_table_delete);
    if (!service->tables) {
        free(service);
        return NULL;
    }

    service->srv_id = new_id;
    service->callbacks = callbacks;

    return service;
}

static void gatts_service_delete(gatts_service_t* service)
{
    if (!service)
        return;

    pthread_mutex_destroy(&service->srv_lock);
    bt_list_free(service->tables);
    free(service);
}

static void gatts_pendops_delete(gatts_op_t* operation)
{
    if (!operation)
        return;

    free(operation);
}

static gatts_op_t* gatts_pendops_execute_out(gatts_manager_t* manager, gatts_request_t request)
{
    bt_list_node_t* node;
    bt_list_t* list = manager->pend_ops;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        gatts_op_t* operation = (gatts_op_t*)bt_list_node(node);
        if (operation->request == request) {
            return operation;
        }
    }
    return NULL;
}

static void gatts_process_message(void* data)
{
    gatts_service_t* service;
    gatts_msg_t* msg = (gatts_msg_t*)data;

    pthread_mutex_lock(&g_gatts_manager.device_lock);
    if (!g_gatts_manager.started)
        goto end;

    switch (msg->event) {
    case GATTS_EVENT_ATTR_TABLE_ADDED: {
        service = find_gatts_service_by_id(msg->param.added.element_id);
        if (service) {
            GATT_CBACK(service->callbacks, on_attr_table_added, service, msg->param.added.status, msg->param.added.element_id ^ service->srv_id);
        }
    } break;
    case GATTS_EVENT_ATTR_TABLE_REMOVED: {
        service = find_gatts_service_by_id(msg->param.removed.element_id);
        if (service) {
            service_table_t* svc_table = find_service_table_by_id(service, msg->param.removed.element_id);
            if (svc_table) {
                bt_list_remove(service->tables, svc_table);
            }
            GATT_CBACK(service->callbacks, on_attr_table_removed, service, msg->param.removed.status, msg->param.removed.element_id ^ service->srv_id);
        }
    } break;
    case GATTS_EVENT_CONNECT_CHANGE: {
        profile_connection_state_t connect_state = msg->param.connect_change.state;
        BT_ADDR_LOG("GATTS-CONNECTION-STATE-EVENT from:%s, state:%d", &msg->param.connect_change.addr, connect_state);
        if (connect_state == PROFILE_STATE_CONNECTED) {
            GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_connected, &msg->param.connect_change.addr);
        } else if (connect_state == PROFILE_STATE_DISCONNECTED) {
            GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_disconnected, &msg->param.connect_change.addr);
        }
    } break;
    case GATTS_EVENT_READ_REQUEST: {
        service = find_gatts_service_by_id(msg->param.read.element_id);
        if (!service)
            break;

        gatt_element_t* element = find_service_element_by_id(service, msg->param.read.element_id);
        if (!element)
            break;

        if (element->rsp_type == ATTR_AUTO_RSP) {
            bt_sal_gatt_server_send_response(PRIMARY_ADAPTER, &msg->param.read.addr, msg->param.read.request_id, element->attr_data, element->attr_length);
        } else if (element->read_cb) {
            element->read_cb(service, &msg->param.read.addr, msg->param.read.element_id ^ service->srv_id, msg->param.read.request_id);
        }
    } break;
    case GATTS_EVENT_WRITE_REQUEST: {
        service = find_gatts_service_by_id(msg->param.write.element_id);
        if (!service)
            break;

        gatt_element_t* element = find_service_element_by_id(service, msg->param.write.element_id);
        if (!element)
            break;

        bt_sal_gatt_server_send_response(PRIMARY_ADAPTER, &msg->param.write.addr, msg->param.write.request_id, NULL, 0);
        if (element->rsp_type == ATTR_AUTO_RSP) {
            if (element->attr_data) {
                msg->param.write.length = MIN(element->attr_length, msg->param.write.length);
                memcpy(element->attr_data, msg->param.write.value, msg->param.write.length);
            }
        } else if (element->write_cb) {
            element->write_cb(service, &msg->param.write.addr, msg->param.write.element_id ^ service->srv_id, msg->param.write.value,
                msg->param.write.length, msg->param.write.offset);
        }
    } break;
    case GATTS_EVENT_MTU_CHANGE:
        GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_mtu_changed, &msg->param.mtu_change.addr, msg->param.mtu_change.mtu);
        break;
    case GATTS_EVENT_CHANGE_SEND: {
        service = find_gatts_service_by_id(msg->param.change_send.element_id);
        if (service) {
            GATT_CBACK(service->callbacks, on_notify_complete, service, &msg->param.change_send.addr, msg->param.change_send.status,
                msg->param.change_send.element_id ^ service->srv_id);
        }
    } break;
    case GATTS_EVENT_PHY_READ: {
        gatts_op_t* operation = gatts_pendops_execute_out(&g_gatts_manager, GATTS_REQ_READ_PHY);
        if (operation) {
            service = (gatts_service_t*)operation->param.phy.srv_handle;
            GATT_CBACK(service->callbacks, on_phy_read, service, &msg->param.phy.addr, msg->param.phy.tx_phy, msg->param.phy.rx_phy);
            bt_list_remove(g_gatts_manager.pend_ops, operation);
        }
    } break;
    case GATTS_EVENT_PHY_UPDATE: {
        if (msg->param.phy.status == GATT_STATUS_SUCCESS) {
            GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_phy_updated, &msg->param.phy.addr, msg->param.phy.status,
                msg->param.phy.tx_phy, msg->param.phy.rx_phy);
        } else {
            gatts_op_t* operation = gatts_pendops_execute_out(&g_gatts_manager, GATTS_REQ_UPDATE_PHY);
            if (operation) {
                service = (gatts_service_t*)operation->param.phy.srv_handle;
                GATT_CBACK(service->callbacks, on_phy_updated, service, &msg->param.phy.addr, msg->param.phy.status, msg->param.phy.tx_phy,
                    msg->param.phy.rx_phy);
                bt_list_remove(g_gatts_manager.pend_ops, operation);
            }
        }
    } break;
    case GATTS_EVENT_CONN_PARAM_CHANGE:
        GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_conn_param_changed, &msg->param.conn_param.addr,
            msg->param.conn_param.interval, msg->param.conn_param.latency, msg->param.conn_param.timeout);
        break;
    default: {

    } break;
    }

end:
    pthread_mutex_unlock(&g_gatts_manager.device_lock);
    gatts_msg_destory(msg);
}

static bt_status_t gatts_send_message(gatts_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(gatts_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_init(void)
{
    pthread_mutexattr_t attr;

    memset(&g_gatts_manager, 0, sizeof(g_gatts_manager));
    g_gatts_manager.started = false;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_gatts_manager.device_lock, &attr) < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    gatts_manager_t* manager = &g_gatts_manager;

    pthread_mutex_lock(&manager->device_lock);
    if (manager->started) {
        pthread_mutex_unlock(&manager->device_lock);
        cb(PROFILE_GATTS, true);
        return BT_STATUS_SUCCESS;
    }

    manager->services = bt_list_new((bt_list_free_cb_t)gatts_service_delete);
    if (!manager->services) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    manager->pend_ops = bt_list_new((bt_list_free_cb_t)gatts_pendops_delete);
    if (!manager->pend_ops) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    status = bt_sal_gatt_server_enable();
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    manager->started = true;
    pthread_mutex_unlock(&manager->device_lock);
    cb(PROFILE_GATTS, true);

    return BT_STATUS_SUCCESS;

fail:
    bt_list_free(manager->services);
    manager->services = NULL;
    bt_list_free(manager->pend_ops);
    manager->pend_ops = NULL;
    pthread_mutex_unlock(&manager->device_lock);
    cb(PROFILE_GATTS, false);

    return status;
}

static bt_status_t if_gatts_shutdown(profile_on_shutdown_t cb)
{
    gatts_manager_t* manager = &g_gatts_manager;

    pthread_mutex_lock(&manager->device_lock);

    if (!manager->started) {
        pthread_mutex_unlock(&manager->device_lock);
        cb(PROFILE_GATTS, true);
        return BT_STATUS_SUCCESS;
    }

    bt_list_free(manager->services);
    manager->services = NULL;
    bt_list_free(manager->pend_ops);
    manager->pend_ops = NULL;
    manager->started = false;
    pthread_mutex_unlock(&manager->device_lock);
    bt_sal_gatt_server_disable();
    cb(PROFILE_GATTS, true);

    return BT_STATUS_SUCCESS;
}

static void if_gatts_cleanup(void)
{
    g_gatts_manager.started = false;
    pthread_mutex_destroy(&g_gatts_manager.device_lock);
}

static int if_gatts_get_state(void)
{
    return 1;
}

static int if_gatts_dump(void)
{
    bt_list_node_t* snode;
    bt_list_t* slist = g_gatts_manager.services;
    int s_id = 0;
    char uuid_str[40] = { 0 };

    pthread_mutex_lock(&g_gatts_manager.device_lock);

    for (snode = bt_list_head(slist); snode != NULL; snode = bt_list_next(slist, snode)) {
        gatts_service_t* service = (gatts_service_t*)bt_list_node(snode);
        bt_list_node_t* tnode;
        bt_list_t* tlist = service->tables;
        int t_id = 0;

        BT_LOGI("GATT Service[%d]: ID:0x%04x", s_id++, service->srv_id);
        for (tnode = bt_list_head(tlist); tnode != NULL; tnode = bt_list_next(tlist, tnode)) {
            service_table_t* table = (service_table_t*)bt_list_node(tnode);
            gatt_element_t* element = table->elements;

            BT_LOGI("\tAttribute Table[%d]: Handle:0x%04x~0x%04x, Num:%d", t_id++, table->start_handle, table->end_handle, table->element_size);
            for (int i = 0; i < table->element_size; i++, element++) {
                bt_uuid_to_string(&element->uuid, uuid_str, 40);
                BT_LOGI("\t\t>[0x%04x][Type:%d][Prop:%04x][UUID:%s]", element->handle, element->type, element->properties,
                    uuid_str);
            }
        }

        if (bt_list_is_empty(tlist))
            BT_LOGI("\tNo Attributes were added");
    }

    pthread_mutex_unlock(&g_gatts_manager.device_lock);

    return 0;
}

static bt_status_t if_gatts_register_service(void* remote, void** phandle, gatts_callbacks_t* callbacks)
{
    pthread_mutexattr_t attr;

    CHECK_ENABLED();
    if (!phandle)
        return BT_STATUS_PARM_INVALID;

    pthread_mutex_lock(&g_gatts_manager.device_lock);
    gatts_service_t* service = gatts_service_new(callbacks);
    if (!service) {
        pthread_mutex_unlock(&g_gatts_manager.device_lock);
        BT_LOGE("New gatts service alloc failed");
        return BT_STATUS_NOMEM;
    }

    bt_list_add_tail(g_gatts_manager.services, service);
    pthread_mutex_unlock(&g_gatts_manager.device_lock);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->srv_lock, &attr);

    service->remote = remote;
    service->manager = &g_gatts_manager;
    service->user_phandle = phandle;
    *phandle = service;

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_unregister_service(void* srv_handle)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    bt_list_node_t* node;
    bt_list_t* list = service->tables;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        service_table_t* svc_table = (service_table_t*)bt_list_node(node);
        bt_sal_gatt_server_remove_elements(svc_table->elements, svc_table->element_size);
    }

    void** user_phandle = service->user_phandle;
    pthread_mutex_lock(&g_gatts_manager.device_lock);
    bt_list_remove(g_gatts_manager.services, service);
    pthread_mutex_unlock(&g_gatts_manager.device_lock);
    *user_phandle = NULL;

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_connect(void* srv_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    BT_ADDR_LOG("GATTS-CONNECT-REQUEST addr:%s", addr);
    return bt_sal_gatt_server_connect(PRIMARY_ADAPTER, addr, addr_type);
}

static bt_status_t if_gatts_disconnect(void* srv_handle, bt_address_t* addr)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    BT_ADDR_LOG("GATTS-DISCONNECT-REQUEST addr:%s", addr);
    return bt_sal_gatt_server_cancel_connection(PRIMARY_ADAPTER, addr);
}

static bt_status_t if_gatts_add_attr_table(void* srv_handle, gatt_srv_db_t* srv_db)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    service_table_t* svc_table = find_service_table_by_id(service, srv_db->attr_db[0].handle + service->srv_id);
    if (svc_table)
        return BT_STATUS_PARM_INVALID;

    svc_table = service_table_new(srv_db->attr_num);
    if (svc_table == NULL)
        return BT_STATUS_NOMEM;

    gatt_element_t* elements = svc_table->elements;
    gatt_attr_db_t* attr_inst = srv_db->attr_db;
    for (int i = 0; i < srv_db->attr_num; i++, elements++, attr_inst++) {

        elements->handle = service->srv_id + attr_inst->handle;
        elements->type = attr_inst->type;
        elements->properties = attr_inst->properties;
        elements->permissions = attr_inst->permissions;
        elements->rsp_type = attr_inst->rsp_type;
        elements->read_cb = attr_inst->read_cb;
        elements->write_cb = attr_inst->write_cb;
        elements->attr_length = attr_inst->attr_length;
        elements->attr_data = NULL;
        if (elements->rsp_type == ATTR_AUTO_RSP && elements->attr_length) {
            elements->attr_data = malloc(elements->attr_length);
            memcpy(elements->attr_data, attr_inst->attr_value, elements->attr_length);
        }

        elements->uuid.type = BT_UUID128_TYPE;
        bt_uuid_to_uuid128(&attr_inst->uuid, &elements->uuid);
    }
    svc_table->start_handle = svc_table->elements[0].handle;
    svc_table->end_handle = svc_table->elements[svc_table->element_size - 1].handle;

    bt_status_t status = bt_sal_gatt_server_add_elements(svc_table->elements, svc_table->element_size);
    if (status != BT_STATUS_SUCCESS) {
        service_table_delete(svc_table);
        return status;
    }

    bt_list_add_tail(service->tables, svc_table);
    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_remove_attr_table(void* srv_handle, uint16_t attr_handle)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    service_table_t* svc_table = find_service_table_by_id(service, attr_handle + service->srv_id);
    if (!svc_table)
        return BT_STATUS_PARM_INVALID;

    return bt_sal_gatt_server_remove_elements(svc_table->elements, svc_table->element_size);
}

static bt_status_t if_gatts_set_attr_value(void* srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    if (!value)
        return BT_STATUS_PARM_INVALID;

    gatt_element_t* element = find_service_element_by_id(service, attr_handle + service->srv_id);
    if (!element)
        return BT_STATUS_PARM_INVALID;

    if (!element->attr_data || !element->attr_length)
        return BT_STATUS_NOT_FOUND;

    length = MIN(element->attr_length, length);
    memcpy(element->attr_data, value, length);
    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_get_attr_value(void* srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t* length)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    if (!value || !length)
        return BT_STATUS_PARM_INVALID;

    gatt_element_t* element = find_service_element_by_id(service, attr_handle + service->srv_id);
    if (!element)
        return BT_STATUS_PARM_INVALID;

    if (!element->attr_data || !element->attr_length)
        return BT_STATUS_NOT_FOUND;

    *length = MIN(element->attr_length, *length);
    memcpy(value, element->attr_data, *length);
    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_response(void* srv_handle, bt_address_t* addr, uint32_t req_handle, uint8_t* value, uint16_t length)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);
    if (!value)
        return BT_STATUS_PARM_INVALID;

    return bt_sal_gatt_server_send_response(PRIMARY_ADAPTER, addr, req_handle, value, length);
}

static bt_status_t if_gatts_notify(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatts_service_t* service = srv_handle;
#if defined(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
    gatt_element_t* element;
#endif

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);
    if (!value)
        return BT_STATUS_PARM_INVALID;

#if defined(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
    element = find_service_element_by_id(service, attr_handle + service->srv_id);
    if (!element) {
        BT_LOGE("%s element null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    return bt_sal_gatt_server_send_notification(PRIMARY_ADAPTER, addr, element->uuid, value, length);
#else
    return bt_sal_gatt_server_send_notification(PRIMARY_ADAPTER, addr, attr_handle + service->srv_id, value, length);
#endif
}

static bt_status_t if_gatts_indicate(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatts_service_t* service = srv_handle;
#if defined(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
    gatt_element_t* element;
#endif

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);
    if (!value)
        return BT_STATUS_PARM_INVALID;

#if defined(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
    element = find_service_element_by_id(service, attr_handle + service->srv_id);
    if (!element) {
        BT_LOGE("%s element null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    return bt_sal_gatt_server_send_indication(PRIMARY_ADAPTER, addr, element->uuid, value, length);
#else
    return bt_sal_gatt_server_send_indication(PRIMARY_ADAPTER, addr, attr_handle + service->srv_id, value, length);
#endif
}

static bt_status_t if_gatts_read_phy(void* srv_handle, bt_address_t* addr)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    bt_status_t status = bt_sal_gatt_server_read_phy(PRIMARY_ADAPTER, addr);

    if (status == BT_STATUS_SUCCESS && service->callbacks->on_phy_read) {
        gatts_op_t* op = gatts_op_new(GATTS_REQ_READ_PHY);
        op->param.phy.srv_handle = srv_handle;
        pthread_mutex_lock(&g_gatts_manager.device_lock);
        bt_list_add_tail(service->manager->pend_ops, op);
        pthread_mutex_unlock(&g_gatts_manager.device_lock);
    }
    return status;
}

static bt_status_t if_gatts_update_phy(void* srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gatts_service_t* service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    bt_status_t status = bt_sal_gatt_server_set_phy(PRIMARY_ADAPTER, addr, tx_phy, rx_phy);

    if (status == BT_STATUS_SUCCESS && service->callbacks->on_phy_updated) {
        gatts_op_t* op = gatts_op_new(GATTS_REQ_UPDATE_PHY);
        op->param.phy.srv_handle = srv_handle;
        op->param.phy.tx_phy = tx_phy;
        op->param.phy.rx_phy = rx_phy;
        pthread_mutex_lock(&g_gatts_manager.device_lock);
        bt_list_add_tail(service->manager->pend_ops, op);
        pthread_mutex_unlock(&g_gatts_manager.device_lock);
    }
    return status;
}

static const gatts_interface_t gatts_if = {
    .size = sizeof(gatts_if),
    .register_service = if_gatts_register_service,
    .unregister_service = if_gatts_unregister_service,
    .connect = if_gatts_connect,
    .disconnect = if_gatts_disconnect,
    .add_attr_table = if_gatts_add_attr_table,
    .remove_attr_table = if_gatts_remove_attr_table,
    .set_attr_value = if_gatts_set_attr_value,
    .get_attr_value = if_gatts_get_attr_value,
    .response = if_gatts_response,
    .notify = if_gatts_notify,
    .indicate = if_gatts_indicate,
    .read_phy = if_gatts_read_phy,
    .update_phy = if_gatts_update_phy,
};

static const void* get_gatts_profile_interface(void)
{
    return (void*)&gatts_if;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void if_gatts_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_CONNECT_CHANGE, 0);
    memcpy(&msg->param.connect_change.addr, addr, sizeof(bt_address_t));
    msg->param.connect_change.state = state;
    msg->param.connect_change.reason = 0;
    gatts_send_message(msg);
}

void if_gatts_on_elements_added(gatt_status_t status, uint16_t element_id, uint16_t size)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_ATTR_TABLE_ADDED, 0);
    msg->param.added.element_id = element_id;
    msg->param.added.status = status;
    gatts_send_message(msg);
}

void if_gatts_on_elements_removed(gatt_status_t status, uint16_t element_id, uint16_t size)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_ATTR_TABLE_REMOVED, 0);
    msg->param.removed.element_id = element_id;
    msg->param.removed.status = status;
    gatts_send_message(msg);
}

void if_gatts_on_received_element_read_request(bt_address_t* addr, uint32_t request_id, uint16_t element_id)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_READ_REQUEST, 0);
    msg->param.read.element_id = element_id;
    msg->param.read.request_id = request_id;
    memcpy(&msg->param.read.addr, addr, sizeof(bt_address_t));
    gatts_send_message(msg);
}

void if_gatts_on_received_element_write_request(bt_address_t* addr, uint32_t request_id, uint16_t element_id,
    uint8_t* value, uint16_t offset, uint16_t length)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_WRITE_REQUEST, length);
    memcpy(&msg->param.write.addr, addr, sizeof(bt_address_t));
    msg->param.write.element_id = element_id;
    msg->param.write.request_id = request_id;
    msg->param.write.offset = offset;
    msg->param.write.length = length;
    memcpy(msg->param.write.value, value, length);
    gatts_send_message(msg);
}

void if_gatts_on_mtu_changed(bt_address_t* addr, uint32_t mtu)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_MTU_CHANGE, 0);
    memcpy(&msg->param.mtu_change.addr, addr, sizeof(bt_address_t));
    msg->param.mtu_change.mtu = mtu;
    gatts_send_message(msg);
}

void if_gatts_on_notification_sent(bt_address_t* addr, uint16_t element_id, gatt_status_t status)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_CHANGE_SEND, 0);
    msg->param.change_send.element_id = element_id;
    msg->param.change_send.status = status;
    memcpy(&msg->param.change_send.addr, addr, sizeof(bt_address_t));
    gatts_send_message(msg);
}

void if_gatts_on_phy_read(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_PHY_READ, 0);
    msg->param.phy.tx_phy = tx_phy;
    msg->param.phy.rx_phy = rx_phy;
    memcpy(&msg->param.phy.addr, addr, sizeof(bt_address_t));
    gatts_send_message(msg);
}

void if_gatts_on_phy_updated(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, gatt_status_t status)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_PHY_UPDATE, 0);
    msg->param.phy.status = status;
    msg->param.phy.tx_phy = tx_phy;
    msg->param.phy.rx_phy = rx_phy;
    memcpy(&msg->param.phy.addr, addr, sizeof(bt_address_t));
    gatts_send_message(msg);
}

void if_gatts_on_connection_parameter_changed(bt_address_t* addr, uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_CONN_PARAM_CHANGE, 0);
    msg->param.conn_param.interval = connection_interval;
    msg->param.conn_param.latency = peripheral_latency;
    msg->param.conn_param.timeout = supervision_timeout;
    memcpy(&msg->param.conn_param.addr, addr, sizeof(bt_address_t));
    gatts_send_message(msg);
}

void* if_gatts_get_remote(void* srv_handle)
{
    if (!srv_handle)
        return NULL;

    gatts_service_t* service = srv_handle;
    return service->remote;
}

static const profile_service_t gatts_service = {
    .auto_start = true,
    .name = PROFILE_GATTS_NAME,
    .id = PROFILE_GATTS,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = if_gatts_init,
    .startup = if_gatts_startup,
    .shutdown = if_gatts_shutdown,
    .process_msg = NULL,
    .get_state = if_gatts_get_state,
    .get_profile_interface = get_gatts_profile_interface,
    .cleanup = if_gatts_cleanup,
    .dump = if_gatts_dump,
};

void register_gatts_service(void)
{
    register_service(&gatts_service);
}