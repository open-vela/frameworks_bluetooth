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
#define LOG_TAG "gatts "
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <sys/types.h>

#include "bt_gatts.h"
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
        bt_list_node_t *_node;                                                                 \
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
        bt_list_node_t *_node;                                                                 \
        bt_list_t *_list = _cbsl;                                                              \
        for (_node = bt_list_head(_list); _node != NULL; _node = bt_list_next(_list, _node)) { \
            _type *_inst = (_type *)bt_list_node(_node);                                       \
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
    gatts_conn_state_t state;
    bt_address_t remote_addr;
    bt_list_t *services;
    pthread_mutex_t device_lock;

} gatts_manager_t;

typedef struct
{
    void *remote;
    uint16_t srv_id;
    gatts_srv_state_t state;
    pthread_mutex_t srv_lock;
    void **user_phandle;
    gatts_manager_t *manager;
    gatt_element_t *elements;
    int32_t element_size;
    gatts_callbacks_t *callbacks;
    bt_list_t *pend_ops;

} gatts_service_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static gatts_manager_t g_gatts_manager = {
    .started = false,
    .state = GATTS_CONN_STATE_DISCONNECTED,
    .services = NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static gatt_element_t *find_gatts_element_by_id(gatts_service_t *service, uint16_t element_id)
{
    gatt_element_t *element = service->elements;
    for (int i = 0; i < service->element_size; i++, element++) {
        if (element->handle == element_id)
            return element;
    }
    return NULL;
}

static bool service_id_cmp(void *service, void *id)
{
    return (((gatts_service_t *)service)->srv_id == (*((uint16_t *)id)));
}

static gatts_service_t *find_gatts_service_by_id(uint16_t srv_id)
{
    srv_id &= GATT_ELEMENT_GROUP_MASK;
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

static gatts_service_t *gatts_service_new(gatts_callbacks_t *callbacks)
{
    uint16_t new_id = generate_service_id();
    if (!new_id)
        return NULL;

    gatts_service_t *service = calloc(1, sizeof(gatts_service_t));
    if (!service)
        return NULL;

    service->srv_id = new_id;
    service->elements = NULL;
    service->element_size = 0;
    service->callbacks = callbacks;
    service->pend_ops = NULL;

    return service;
}

static void gatts_service_delete(gatts_service_t *service)
{
    if (!service)
        return;

    bt_list_free(service->pend_ops);
    service->pend_ops = NULL;
    pthread_mutex_destroy(&service->srv_lock);
    if (service->elements)
        free(service->elements);
    free(service);
}

static void gatts_pendops_delete(gatts_op_t *operation)
{
    if (!operation)
        return;

    free(operation);
}

static gatts_op_t *gatts_pendops_execute_out(gatts_service_t *service, gatts_request_t request, uint16_t attr_handle)
{
    bt_list_node_t *node;
    bt_list_t *list = service->pend_ops;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        gatts_op_t *operation = (gatts_op_t *)bt_list_node(node);
        if (operation->request != request) {
            continue;
        }
        if (request == GATTS_REQ_NOTIFY && ((operation->param.notify.attr_handle + service->srv_id) == attr_handle)) {
            return operation;
        }
    }

    return NULL;
}

static void gatts_process_message(void *data)
{
    gatts_service_t *service;
    gatts_msg_t *msg = (gatts_msg_t *)data;
    BT_LOGD("%s, event %d", __func__, msg->event);

    pthread_mutex_lock(&g_gatts_manager.device_lock);
    if (!g_gatts_manager.started)
        goto end;

    switch (msg->event) {
    case GATTS_EVENT_START: {
        service = find_gatts_service_by_id(msg->param.start.element_id);
        if (service) {
            service->state = GATTS_SRV_STATE_STARTED;
            GATT_CBACK(service->callbacks, on_started, service, msg->param.start.status);
        }
    } break;
    case GATTS_EVENT_STOP: {
        service = find_gatts_service_by_id(msg->param.stop.element_id);
        if (service) {
            service->state = GATTS_SRV_STATE_IDLE;
            GATT_CBACK(service->callbacks, on_stopped, service, msg->param.stop.status);
        }
    } break;
    case GATTS_EVENT_CONNECT_CHANGE: {
        profile_connection_state_t connect_state = msg->param.connect_change.state;
        if (connect_state == PROFILE_STATE_CONNECTED) {
            g_gatts_manager.state = GATTS_CONN_STATE_CONNECTED;
            memcpy(&g_gatts_manager.remote_addr, &msg->param.connect_change.addr, sizeof(g_gatts_manager.remote_addr));
            GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_connected, &msg->param.connect_change.addr);
        } else if (connect_state == PROFILE_STATE_DISCONNECTED) {
            g_gatts_manager.state = GATTS_CONN_STATE_DISCONNECTED;
            GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_disconnected, &msg->param.connect_change.addr);
        }
    } break;
    case GATTS_EVENT_READ_REQUEST: {
        service = find_gatts_service_by_id(msg->param.read.element_id);
        if (!service)
            break;

        gatt_element_t *element = find_gatts_element_by_id(service, msg->param.read.element_id);
        if (!element)
            break;

        if (element->rsp_type == ATTR_AUTO_RSP) {
            bt_sal_gatt_server_send_response(&msg->param.read.addr, msg->param.read.request_id, element->user_data, element->data_length);
        } else if (element->read_cb) {
            element->read_cb(service, msg->param.read.element_id ^ service->srv_id, msg->param.read.request_id);
        }
    } break;
    case GATTS_EVENT_WRITE_REQUEST: {
        service = find_gatts_service_by_id(msg->param.write.element_id);
        if (!service)
            break;

        gatt_element_t *element = find_gatts_element_by_id(service, msg->param.write.element_id);
        if (!element)
            break;

        if (element->rsp_type == ATTR_AUTO_RSP) {
            if (element->user_data) {
                msg->param.write.length = MIN(element->data_length, msg->param.write.length);
                memcpy(element->user_data, msg->param.write.value, msg->param.write.length);
            }
        } else if (element->write_cb) {
            element->write_cb(service, msg->param.write.element_id ^ service->srv_id, msg->param.write.value, msg->param.write.length, msg->param.write.offset);
        }

        if (msg->param.write.need_rsp)
            bt_sal_gatt_server_send_response(&msg->param.write.addr, msg->param.write.request_id, NULL, 0);
    } break;
    case GATTS_EVENT_MTU:
        GATTS_CALLBACK_FOREACH(g_gatts_manager.services, gatts_service_t, on_mtu_changed, &msg->param.mtu.addr, msg->param.mtu.mtu);
        break;
    case GATTS_EVENT_CHANGE_SEND: {
        service = find_gatts_service_by_id(msg->param.change_send.element_id);
        if (!service)
            break;

        gatts_op_t *operation = gatts_pendops_execute_out(service, GATTS_REQ_NOTIFY, msg->param.change_send.element_id);
        if (operation) {
            gatts_complete_cb_t cmpl_cb = operation->param.notify.cmpl_cb;
            cmpl_cb(service, msg->param.change_send.status, operation->param.notify.attr_handle);
            bt_list_remove(service->pend_ops, operation);
        }
    } break;
    default: {

    } break;
    }

end:
    pthread_mutex_unlock(&g_gatts_manager.device_lock);
    gatts_msg_destory(msg);
}

static bt_status_t gatts_send_message(gatts_msg_t *msg)
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
    gatts_manager_t *manager = &g_gatts_manager;

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

    status = bt_sal_gatt_server_enable();
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    manager->started = true;
    manager->state = GATTS_CONN_STATE_DISCONNECTED;
    pthread_mutex_unlock(&manager->device_lock);
    cb(PROFILE_GATTS, true);

    return BT_STATUS_SUCCESS;

fail:
    bt_list_free(manager->services);
    manager->services = NULL;
    pthread_mutex_unlock(&manager->device_lock);
    cb(PROFILE_GATTS, false);

    return status;
}

static bt_status_t if_gatts_shutdown(profile_on_shutdown_t cb)
{
    gatts_manager_t *manager = &g_gatts_manager;

    if (!manager->started) {
        cb(PROFILE_GATTS, true);
        return BT_STATUS_SUCCESS;
    }

    pthread_mutex_lock(&manager->device_lock);
    bt_list_free(manager->services);
    manager->services = NULL;
    manager->started = false;
    manager->state = GATTS_CONN_STATE_DISCONNECTED;
    cb(PROFILE_GATTS, true);
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
    BT_LOGD("%s", __func__);
    return 0;
}

static bt_status_t if_gatts_register_service(void **phandle, gatts_callbacks_t *callbacks)
{
    bt_status_t status;
    pthread_mutexattr_t attr;

    CHECK_ENABLED();
    if (!phandle)
        return BT_STATUS_PARM_INVALID;

    pthread_mutex_lock(&g_gatts_manager.device_lock);
    gatts_service_t *service = gatts_service_new(callbacks);
    if (!service) {
        pthread_mutex_unlock(&g_gatts_manager.device_lock);
        BT_LOGE("New gatts service alloc failed");
        return BT_STATUS_NOMEM;
    }

    service->pend_ops = bt_list_new((bt_list_free_cb_t)gatts_pendops_delete);
    if (!service->pend_ops) {
        pthread_mutex_unlock(&g_gatts_manager.device_lock);
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    bt_list_add_tail(g_gatts_manager.services, service);
    pthread_mutex_unlock(&g_gatts_manager.device_lock);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->srv_lock, &attr);

    service->remote = NULL;
    service->manager = &g_gatts_manager;
    service->state = GATTS_SRV_STATE_IDLE;
    service->user_phandle = phandle;
    *phandle = service;

    return BT_STATUS_SUCCESS;

fail:
    gatts_service_delete(service);
    return status;
}

static bt_status_t if_gatts_unregister_service(void *srv_handle)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    void **user_phandle = service->user_phandle;
    if (service->state != GATTS_SRV_STATE_IDLE)
        bt_sal_gatt_server_remove_elements(service->elements, service->element_size);
    pthread_mutex_lock(&g_gatts_manager.device_lock);
    bt_list_remove(g_gatts_manager.services, service);
    pthread_mutex_unlock(&g_gatts_manager.device_lock);
    *user_phandle = NULL;

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_connect(void *srv_handle, bt_address_t *addr, ble_addr_type_t addr_type)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    gatts_manager_t *manager = service->manager;
    bt_status_t status = bt_sal_gatt_server_connect(addr, addr_type);
    if (status == BT_STATUS_SUCCESS)
        manager->state = GATTS_CONN_STATE_CONNECTING;

    return status;
}

static bt_status_t if_gatts_disconnect(void *srv_handle)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    gatts_manager_t *manager = service->manager;
    bt_status_t status = bt_sal_gatt_server_cancel_connection(&manager->remote_addr);
    if (status == BT_STATUS_SUCCESS)
        manager->state = GATTS_CONN_STATE_DISCONNECTING;

    return status;
}

static bt_status_t if_gatts_create_service_table(void *srv_handle, gatt_srv_db_t *srv_db)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);
    if (!srv_db)
        return BT_STATUS_PARM_INVALID;

    if (service->state != GATTS_SRV_STATE_IDLE)
        return BT_STATUS_BUSY;

    gatt_element_t *elements = (gatt_element_t *)realloc(service->elements, sizeof(gatt_element_t) * srv_db->attr_num);
    if (elements == NULL)
        return BT_STATUS_NOMEM;

    service->elements = elements;
    service->element_size = srv_db->attr_num;

    gatt_attr_db_t *attr_inst = srv_db->attr_db;
    for (int i = 0; i < srv_db->attr_num; i++, elements++, attr_inst++) {

        if (elements->type == GATT_INCLUDED_SERVICE) {
            bt_list_node_t *node;
            bt_list_t *list = g_gatts_manager.services;
            for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
                gatts_service_t *include_service = (gatts_service_t *)bt_list_node(node);
                if (!bt_uuid_compare(&include_service->elements[0].uuid, attr_inst->uuid)) {
                    elements->handle = include_service->elements[0].handle;
                    break;
                }
            }
        } else {
            elements->handle = service->srv_id + attr_inst->handle;
        }

        elements->type = attr_inst->type;
        elements->properties = attr_inst->properties;
        elements->permissions = attr_inst->permissions;
        elements->rsp_type = attr_inst->rsp_type;
        elements->user_data = attr_inst->attr_value;
        elements->data_length = attr_inst->attr_length;

        elements->read_cb = attr_inst->read_cb;
        elements->write_cb = attr_inst->write_cb;

        elements->uuid.type = BT_UUID128_TYPE;
        bt_uuid_to_uuid128(attr_inst->uuid, &elements->uuid);
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t if_gatts_start(void *srv_handle)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    if (service->state != GATTS_SRV_STATE_IDLE)
        return BT_STATUS_BUSY;

    bt_status_t status = bt_sal_gatt_server_add_elements(service->elements, service->element_size);
    if (status == BT_STATUS_SUCCESS)
        service->state = GATTS_SRV_STATE_STARTING;

    return status;
}

static bt_status_t if_gatts_stop(void *srv_handle)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);

    bt_status_t status = bt_sal_gatt_server_remove_elements(service->elements, service->element_size);
    if (status == BT_STATUS_SUCCESS)
        service->state = GATTS_SRV_STATE_STOPPING;

    return status;
}

static bt_status_t if_gatts_response(void *srv_handle, uint32_t req_handle, uint8_t *value, uint16_t length)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);
    if (!value)
        return BT_STATUS_PARM_INVALID;

    gatts_manager_t *manager = service->manager;
    return bt_sal_gatt_server_send_response(&manager->remote_addr, req_handle, value, length);
}

static bt_status_t if_gatts_notify(void *srv_handle, uint16_t attr_handle, uint8_t *value, uint16_t length, gatts_complete_cb_t cmpl_cb)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);
    if (!value)
        return BT_STATUS_PARM_INVALID;

    gatts_manager_t *manager = service->manager;
    bt_status_t status = bt_sal_gatt_server_send_notification(&manager->remote_addr,
                                                attr_handle + service->srv_id, value, length);

    if (cmpl_cb && status == BT_STATUS_SUCCESS) {
        gatts_op_t *op = gatts_op_new(GATTS_REQ_NOTIFY);
        op->param.notify.srv_handle = srv_handle;
        op->param.notify.cmpl_cb = cmpl_cb;
        op->param.notify.type = GATT_CHANGE_TYPE_NOTIFY;
        op->param.notify.attr_handle = attr_handle;
        bt_list_add_tail(service->pend_ops, op);
    }

    return status;
}

static bt_status_t if_gatts_indicate(void *srv_handle, uint16_t attr_handle, uint8_t *value, uint16_t length, gatts_complete_cb_t cmpl_cb)
{
    gatts_service_t *service = srv_handle;

    CHECK_ENABLED();
    CHECK_SERVICE_VALID(g_gatts_manager.services, service);
    if (!value)
        return BT_STATUS_PARM_INVALID;

    gatts_manager_t *manager = service->manager;
    bt_status_t status = bt_sal_gatt_server_send_indication(&manager->remote_addr,
                                                attr_handle + service->srv_id, value, length);

    if (cmpl_cb && status == BT_STATUS_SUCCESS) {
        gatts_op_t *op = gatts_op_new(GATTS_REQ_NOTIFY);
        op->param.notify.srv_handle = srv_handle;
        op->param.notify.cmpl_cb = cmpl_cb;
        op->param.notify.type = GATT_CHANGE_TYPE_INDICATE;
        op->param.notify.attr_handle = attr_handle;
        bt_list_add_tail(service->pend_ops, op);
    }

    return status;
}

static const gatts_interface_t gatts_if = {
    .size = sizeof(gatts_if),
    .register_service = if_gatts_register_service,
    .unregister_service = if_gatts_unregister_service,
    .connect = if_gatts_connect,
    .disconnect = if_gatts_disconnect,
    .create_service_table = if_gatts_create_service_table,
    .start = if_gatts_start,
    .stop = if_gatts_stop,
    .response = if_gatts_response,
    .notify = if_gatts_notify,
    .indicate = if_gatts_indicate,
};

static const void *get_gatts_profile_interface(void)
{
    return (void *)&gatts_if;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void if_gatts_on_connection_state_changed(bt_address_t *addr, profile_connection_state_t state)
{
    gatts_msg_t *msg = gatts_msg_new(GATTS_EVENT_CONNECT_CHANGE, 0);
    memcpy(&msg->param.connect_change.addr, addr, sizeof(bt_address_t));
    msg->param.connect_change.state = state;
    msg->param.connect_change.reason = 0;
    gatts_send_message(msg);
}

void if_gatts_on_elements_added(gatt_status_t status, uint16_t element_id, uint16_t size)
{
    gatts_msg_t *msg = gatts_msg_new(GATTS_EVENT_START, 0);
    msg->param.start.element_id = element_id;
    msg->param.start.status = status;
    gatts_send_message(msg);
}

void if_gatts_on_elements_removed(gatt_status_t status, uint16_t element_id, uint16_t size)
{
    gatts_msg_t *msg = gatts_msg_new(GATTS_EVENT_STOP, 0);
    msg->param.start.element_id = element_id;
    msg->param.stop.status = status;
    gatts_send_message(msg);
}

void if_gatts_on_received_element_read_request(bt_address_t *addr, uint32_t request_id, uint16_t element_id)
{
    gatts_msg_t *msg = gatts_msg_new(GATTS_EVENT_READ_REQUEST, 0);
    msg->param.read.element_id = element_id;
    msg->param.read.request_id = request_id;
    memcpy(&msg->param.read.addr, addr, sizeof(bt_address_t));
    gatts_send_message(msg);
}

void if_gatts_on_received_element_write_request(bt_address_t *addr, uint32_t request_id, uint16_t element_id,
                                                uint8_t *value, uint16_t offset, uint16_t length, bool need_rsp)
{
    gatts_msg_t *msg = gatts_msg_new(GATTS_EVENT_WRITE_REQUEST, length);
    msg->param.write.element_id = element_id;
    msg->param.write.request_id = request_id;
    memcpy(&msg->param.write.addr, addr, sizeof(bt_address_t));
    msg->param.write.need_rsp = need_rsp;
    msg->param.write.offset = offset;
    msg->param.write.length = length;
    memcpy(msg->param.write.value, value, length);
    gatts_send_message(msg);
}

void if_gatts_on_mtu_changed(bt_address_t *addr, uint32_t mtu)
{
    gatts_msg_t *msg = gatts_msg_new(GATTS_EVENT_MTU, 0);
    memcpy(&msg->param.mtu.addr, addr, sizeof(bt_address_t));
    msg->param.mtu.mtu = mtu;
    gatts_send_message(msg);
}

void if_gatts_on_notification_sent(bt_address_t *addr, uint16_t element_id, gatt_status_t status)
{
    gatts_msg_t *msg = gatts_msg_new(GATTS_EVENT_CHANGE_SEND, 0);
    msg->param.change_send.element_id = element_id;
    msg->param.change_send.status = status;
    gatts_send_message(msg);
}

void if_gatts_set_remote(void *srv_handle, void *remote)
{
    if (!srv_handle)
        return;

    gatts_service_t *service = srv_handle;
    service->remote = remote;
}

void *if_gatts_get_remote(void *srv_handle)
{
    if (!srv_handle)
        return NULL;

    gatts_service_t *service = srv_handle;
    return service->remote;
}

static const profile_service_t gatts_service = {
    .auto_start = true,
    .name = PROFILE_GATTS_NAME,
    .id = PROFILE_GATTS,
    .transport = BT_TRANSPORT_BLE,
    .uuid = {BT_UUID128_TYPE, { 0 }},
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