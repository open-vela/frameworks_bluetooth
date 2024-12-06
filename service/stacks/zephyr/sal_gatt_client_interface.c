/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/uuid.h>

#include <debug.h>

#include "sal_adapter_le_interface.h"
#include "sal_gatt_client_interface.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#define CONFIG_GATT_CLIENT_SERVICE_MAX 10
#define CONFIG_GATT_CLIENT_ELEMENT_MAX 20

#ifdef CONFIG_BLUETOOTH_GATT
#define STACK_CALL(func) zblue_##func

typedef void (*sal_func_t)(void* args);

union uuid {
    struct bt_uuid uuid;
    struct bt_uuid_16 u16;
    struct bt_uuid_128 u128;
};

struct gatt_service {
    uint16_t start_handle;
    uint16_t end_handle;
    const struct bt_uuid* uuid;
};

struct gatt_instance {
    bool active;
    bt_address_t addr;
    uint8_t service_idx;
    uint8_t service_size;
    uint8_t element_idx;
    uint8_t element_size;
    struct gatt_service service[CONFIG_GATT_CLIENT_SERVICE_MAX];
    gatt_element_t element[CONFIG_GATT_CLIENT_ELEMENT_MAX];
};

typedef union {
    struct bt_le_conn_param conn_param;
} sal_adapter_args_t;

typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type;
    sal_func_t func;
    sal_adapter_args_t adpt;
} sal_adapter_req_t;

static bt_status_t zblue_gatt_client_discover_chrc(struct bt_conn* conn, const struct bt_uuid* uuid,
    uint16_t start_handle, uint16_t end_handle);

static struct gatt_instance g_gatt_client[CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS];

static struct gatt_instance* gatt_find_instance_by_addr(bt_address_t* addr)
{
    for (int i = 0; i < CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS; i++) {
        if (!g_gatt_client[i].active) {
            continue;
        }

        if (memcmp(&g_gatt_client[i].addr, addr, sizeof(bt_address_t)) == 0) {
            return &g_gatt_client[i];
        }
    }

    return NULL;
}

static struct gatt_instance* gatt_find_alloc_instance_by_addr(bt_address_t* addr)
{
    struct gatt_instance* instance;

    instance = gatt_find_instance_by_addr(addr);
    if (instance) {
        return instance;
    }

    for (int i = 0; i < CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS; i++) {
        instance = &g_gatt_client[i];

        if (!instance->active) {
            instance->active = true;
            memcpy(&instance->addr, addr, sizeof(bt_address_t));
            return instance;
        }
    }

    return NULL;
}

static void gatt_free_instance(bt_address_t* addr)
{
    struct gatt_instance* instance;

    instance = gatt_find_instance_by_addr(addr);
    if (!instance) {
        BT_LOGE("%s, instance null", __func__);
        return;
    }

    memset(instance, 0, sizeof(struct gatt_instance));
}

static struct gatt_service* gatt_alloc_service_by_addr(bt_address_t* addr)
{
    struct gatt_instance* instance;

    instance = gatt_find_instance_by_addr(addr);
    if (!instance) {
        BT_LOGE("%s, instance null", __func__);
        return NULL;
    }

    if (instance->service_size >= CONFIG_GATT_CLIENT_SERVICE_MAX) {
        BT_LOGE("%s, service_size:%d overflow", __func__, instance->service_size);
        return NULL;
    }

    return &instance->service[instance->service_size++];
}

static gatt_element_t* gatt_alloc_element_by_addr(bt_address_t* addr)
{
    struct gatt_instance* instance;

    instance = gatt_find_instance_by_addr(addr);
    if (!instance) {
        BT_LOGE("%s, instance null", __func__);
        return NULL;
    }

    if (instance->element_size >= CONFIG_GATT_CLIENT_ELEMENT_MAX) {
        BT_LOGE("%s, element_size:%d overflow", __func__, instance->element_size);
        return NULL;
    }

    return &instance->element[instance->element_size++];
}

static sal_adapter_req_t* sal_adapter_req(bt_controller_id_t id, bt_address_t* addr, sal_func_t func)
{
    sal_adapter_req_t* req = calloc(sizeof(sal_adapter_req_t), 1);

    if (req) {
        req->id = id;
        req->func = func;
        if (addr)
            memcpy(&req->addr, addr, sizeof(bt_address_t));
    }

    return req;
}

static void sal_invoke_async(service_work_t* work, void* userdata)
{
    sal_adapter_req_t* req = userdata;

    SAL_ASSERT(req);
    req->func(req);
    free(userdata);
}

static bt_status_t sal_send_req(sal_adapter_req_t* req)
{
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(conn_connect)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t address = { 0 };
    struct bt_conn* conn;
    int err;

    address.type = req->addr_type;
    memcpy(&address.a, &req->addr, sizeof(address.a));

    err = bt_conn_le_create(&address, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
    if (err) {
        BT_LOGE("%s, failed to create connection (%d)", __func__, err);
        return;
    }
}

bt_status_t bt_sal_gatt_client_connect(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type)
{
    sal_adapter_req_t* req;
    uint8_t type;

    req = sal_adapter_req(id, addr, STACK_CALL(conn_connect));
    if (!req) {
        BT_LOGE("%s, req null", __func__)
        return BT_STATUS_NOMEM;
    }

    switch (addr_type) {
    case BT_LE_ADDR_TYPE_PUBLIC:
        type = BT_ADDR_LE_PUBLIC;
        break;
    case BT_LE_ADDR_TYPE_RANDOM:
        type = BT_ADDR_LE_RANDOM;
        break;
    case BT_LE_ADDR_TYPE_PUBLIC_ID:
        type = BT_ADDR_LE_PUBLIC_ID;
        break;
    case BT_LE_ADDR_TYPE_RANDOM_ID:
        type = BT_ADDR_LE_RANDOM_ID;
        break;
    case BT_LE_ADDR_TYPE_ANONYMOUS:
        type = BT_ADDR_LE_ANONYMOUS;
        break;
    case BT_LE_ADDR_TYPE_UNKNOWN:
        type = BT_ADDR_LE_RANDOM;
        break;
    default:
        BT_LOGE("%s, invalid type:%d", __func__, addr_type);
        assert(0);
    }

    BT_LOGD("%s, addr_type:%d, type:%d", __func__, addr_type, type);
    req->addr_type = type;

    return sal_send_req(req);
}

static void STACK_CALL(conn_disconnect)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;
    int err;

    gatt_free_instance(&req->addr);

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        BT_LOGE("%s, disconnect fail err:%d", __func__, err);
        return;
    }
}

bt_status_t bt_sal_gatt_client_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(conn_disconnect));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

static bool zblue_uuid2_to_uuid1(struct bt_uuid* u1, const bt_uuid_t* u2)
{
    if (!bt_uuid_create(u1, (uint8_t*)&u2->val, u2->type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return false;
    }

    return true;
}

static bool zblue_uuid1_to_uuid2(const struct bt_uuid* u1, bt_uuid_t* u2)
{
    if (u1->type == BT_UUID_TYPE_16) {
        u2->type = BT_UUID16_TYPE;
        memcpy(&u2->val, &BT_UUID_16(u1)->val, 2);
    } else if (u1->type == BT_UUID_TYPE_32) {
        u2->type = BT_UUID32_TYPE;
        memcpy(&u2->val, &BT_UUID_32(u1)->val, 4);
    } else if (u1->type == BT_UUID_TYPE_128) {
        u2->type = BT_UUID128_TYPE;
        memcpy(&u2->val, BT_UUID_128(u1)->val, 16);
    } else {
        BT_LOGE("%s, invalid type:%d", __func__, u1->type);
        return false;
    }

    return true;
}

static uint8_t zblue_gatt_client_disc_chrc_callback(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    struct bt_gatt_chrc* data;
    struct gatt_instance* instance;
    gatt_element_t* element;
    bt_address_t addr;

    get_le_addr_from_conn(conn, &addr);

    instance = gatt_find_instance_by_addr(&addr);
    if (!instance) {
        BT_LOGE("%s, instance null", __func__);
        return BT_GATT_ITER_STOP;
    }

    if (!attr) {
        BT_LOGD("%s, uuid discovery chrc finish", __func__);
        if (instance->service_idx < instance->service_size) {
            struct gatt_service* service;

            BT_LOGD("%s, service_idx:%d", __func__, instance->service_idx);
            service = &instance->service[instance->service_idx++];
            zblue_gatt_client_discover_chrc(conn, service->uuid, service->start_handle, service->end_handle);
        } else {
            BT_LOGD("%s, discover service finished", __func__);
            if_gattc_on_service_discovered(&instance->addr, instance->element, instance->element_size);
            if_gattc_on_discover_completed(&addr, GATT_STATUS_SUCCESS);
        }
        return BT_GATT_ITER_STOP;
    }

    BT_LOGD("%s, [ATTRIBUTE] handle 0x%04X", __func__, attr->handle);

    data = attr->user_data;
    element = gatt_alloc_element_by_addr(&addr);
    if (!element) {
        BT_LOGE("%s, alloc element fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    element->handle = data->value_handle;
    element->properties = data->properties;

    zblue_uuid1_to_uuid2(data->uuid, &element->uuid);
    return BT_GATT_ITER_CONTINUE;
}

static bt_status_t zblue_gatt_client_discover_chrc(struct bt_conn* conn, const struct bt_uuid* uuid,
    uint16_t start_handle, uint16_t end_handle)
{
    static struct bt_gatt_discover_params discover_params = { 0 };

    discover_params.uuid = uuid;
    discover_params.start_handle = start_handle;
    discover_params.end_handle = end_handle;
    discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
    discover_params.func = zblue_gatt_client_disc_chrc_callback;

    if (bt_gatt_discover(conn, &discover_params) < 0) {
        BT_LOGE("%s, gatt discovery fail", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static uint8_t zblue_gatt_client_disc_service_callback(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    struct bt_gatt_service_val* data;
    struct gatt_instance* instance;
    struct gatt_service* service;
    bt_address_t addr;

    get_le_addr_from_conn(conn, &addr);

    instance = gatt_find_alloc_instance_by_addr(&addr);
    if (!instance) {
        BT_LOGE("%s, instance find alloc fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    if (!attr) {
        BT_LOGD("%s, start discovery service finished, start discovery char service_idx:%d", __func__, instance->service_idx);
        service = &instance->service[instance->service_idx++];
        zblue_gatt_client_discover_chrc(conn, service->uuid, service->start_handle, service->end_handle);
        return BT_GATT_ITER_STOP;
    }

    BT_LOGD("%s, [ATTRIBUTE] handle 0x%04X", __func__, attr->handle);

    service = gatt_alloc_service_by_addr(&addr);
    if (!service) {
        BT_LOGE("%s, alloc service fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    data = attr->user_data;
    service->start_handle = attr->handle;
    service->end_handle = data->end_handle;
    service->uuid = data->uuid;

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t gatt_client_read_element_callback(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_read_params* params, const void* data, uint16_t length)
{
    struct bt_conn_info info;
    bt_address_t addr;

    if (err) {
        BT_LOGE("%s, gatt read fail err:%d", __func__, err);
        if_gattc_on_element_read(&addr, params->single.handle, (uint8_t*)data, length, BT_STATUS_FAIL);
        return BT_GATT_ITER_STOP;
    }

    BT_LOGD("%s, [DATA] len:%d, handle:0x%0x", __func__, length, params->single.handle);
    lib_dumpbuffer("read element", data, length);

    bt_conn_get_info(conn, &info);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    if_gattc_on_element_read(&addr, params->single.handle, (uint8_t*)data, length, BT_STATUS_SUCCESS);

    return BT_GATT_ITER_STOP;
}

static void gatt_client_write_cmd_callback(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_write_params* params)
{
    struct bt_conn_info info;
    bt_address_t addr;

    if (err) {
        BT_LOGE("%s, gatt write fail err:%d", __func__, err);
        if_gattc_on_element_written(&addr, params->handle, BT_STATUS_FAIL);
        return;
    }

    bt_conn_get_info(conn, &info);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    if_gattc_on_element_written(&addr, params->handle, BT_STATUS_SUCCESS);
}

static void gatt_client_write_callback(struct bt_conn* conn, void* user_data)
{
    uint16_t* handle = user_data;
    struct bt_conn_info info;
    bt_address_t addr;

    bt_conn_get_info(conn, &info);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    if_gattc_on_element_written(&addr, *handle, BT_STATUS_SUCCESS);

    free(handle);
}

static uint8_t bt_gatt_notify_handler(struct bt_conn* conn, struct bt_gatt_subscribe_params* params,
    const void* data, uint16_t length)
{
    uint16_t handle;
    struct bt_conn_info info;
    bt_address_t addr;

    bt_conn_get_info(conn, &info);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    handle = params->ccc_handle;
    if (data == NULL) {
        BT_LOGE("[UNSUBSCRIBED] 0x%04X", params->ccc_handle);
        return BT_GATT_ITER_STOP;
    }

    if_gattc_on_element_changed(&addr, handle, (uint8_t*)data, length);
    return BT_GATT_ITER_CONTINUE;
}

static void bt_gatt_subscribe_response(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_subscribe_params* params)
{
    struct bt_conn_info info;
    bt_address_t addr;

    BT_LOGD("%s, err:%d", __func__, err);
    bt_conn_get_info(conn, &info);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    if_gattc_on_element_subscribed(&addr, params->value_handle, err ? BT_STATUS_FAIL : BT_STATUS_SUCCESS, true);
}

bt_status_t bt_sal_gatt_client_discover_all_services(bt_controller_id_t id, bt_address_t* addr)
{
    static struct bt_gatt_discover_params disc_params = { 0 };
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    disc_params.uuid = NULL;
    disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    disc_params.type = BT_GATT_DISCOVER_PRIMARY;
    disc_params.func = zblue_gatt_client_disc_service_callback;

    err = bt_gatt_discover(conn, &disc_params);
    if (err < 0) {
        BT_LOGE("%s, gatt discovery fail", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_discover_service_by_uuid(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t* uuid)
{
    static struct bt_gatt_discover_params disc_params = { 0 };
    struct bt_conn* conn;
    int err;
    static union uuid u;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    if (!zblue_uuid2_to_uuid1(&u.uuid, uuid)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return BT_STATUS_FAIL;
    }

    disc_params.uuid = &u.uuid;
    disc_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    disc_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    disc_params.type = BT_GATT_DISCOVER_PRIMARY;
    disc_params.func = zblue_gatt_client_disc_service_callback;
    err = bt_gatt_discover(conn, &disc_params);
    if (err < 0) {
        BT_LOGE("%s, gatt discovery fail", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_read_element(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id)
{
    static struct bt_gatt_read_params read_params = { 0 };
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    read_params.func = gatt_client_read_element_callback;
    read_params.handle_count = 1;
    read_params.single.handle = element_id;
    read_params.single.offset = 0;

    err = bt_gatt_read(conn, &read_params);
    if (err) {
        BT_LOGE("%s, gatt read fail err:%d", __func__, err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_write_element(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length, gatt_write_type_t write_type)
{
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    if (write_type == GATT_WRITE_TYPE_RSP) {
        struct bt_gatt_write_params write_params = { 0 };

        write_params.func = gatt_client_write_cmd_callback;
        write_params.handle = element_id;
        write_params.data = value;
        write_params.length = length;
        write_params.offset = 0;

        err = bt_gatt_write(conn, &write_params);
        if (err) {
            BT_LOGE("%s, gatt write fail err:%d", __func__, err);
            return BT_STATUS_FAIL;
        }
    } else if (write_type == GATT_WRITE_TYPE_NO_RSP) {
        uint16_t* handle;

        handle = (uint16_t*)malloc(sizeof(uint16_t));
        *handle = element_id;

        err = bt_gatt_write_without_response_cb(conn, element_id, value, length, false, gatt_client_write_callback, handle);
        if (err) {
            BT_LOGE("%s, gatt write without rsp fail err:%d", __func__, err);
            return BT_STATUS_FAIL;
        }
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_register_notifications(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id, uint16_t properties, bool enable)
{
    static struct bt_gatt_subscribe_params subscribe_params = { 0 };
    uint16_t value;
    struct bt_conn* conn;
    int err;

    BT_LOGD("%s, addr:%s, element_id:0x%0x, properties:0x%0x, enable:%d", __func__, bt_addr_str(addr), element_id, properties, enable);
    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    if (properties == GATT_PROP_NOTIFY) {
        value = BT_GATT_CCC_NOTIFY;
    } else if (properties == GATT_PROP_INDICATE) {
        value = BT_GATT_CCC_INDICATE;
    } else {
        BT_LOGE("%s, properties:%d invalid", __func__, properties);
        return BT_STATUS_PARM_INVALID;
    }

    subscribe_params.value_handle = element_id - 1;
    subscribe_params.ccc_handle = element_id;
    subscribe_params.notify = bt_gatt_notify_handler;
    subscribe_params.subscribe = bt_gatt_subscribe_response;
    subscribe_params.value = value;

    if (enable) {
        err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err) {
            BT_LOGE("%s, gatt subscribe fail err:%d", __func__, err);
            return BT_STATUS_FAIL;
        }
    } else {
        err = bt_gatt_unsubscribe(conn, &subscribe_params);
        if (err) {
            BT_LOGE("%s, gatt subscribe fail err:%d", __func__, err);
            return BT_STATUS_FAIL;
        }
    }

    return BT_STATUS_SUCCESS;
}

static void gatt_exchange_mtu_func(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_exchange_params* params)
{
    struct bt_conn_info info;
    bt_address_t addr;
    uint16_t mtu;

    bt_conn_get_info(conn, &info);
    memcpy(&addr, info.le.dst->a.val, sizeof(addr));

    if (err) {
        BT_LOGE("%s, exchange MTU failed err: %u", __func__, err);
        if_gattc_on_mtu_changed(&addr, 0, BT_STATUS_FAIL);
        return;
    }

    mtu = bt_gatt_get_mtu(conn);
    if_gattc_on_mtu_changed(&addr, mtu, BT_STATUS_SUCCESS);
}

static struct bt_gatt_exchange_params gatt_exchange_params = {
    .func = gatt_exchange_mtu_func,
};

bt_status_t bt_sal_gatt_client_send_mtu_req(bt_controller_id_t id, bt_address_t* addr, uint32_t mtu)
{
    int err;

    err = bt_gatt_exchange_mtu(get_le_conn_from_addr(addr), &gatt_exchange_params);
    if (err) {
        BT_LOGE("%s, exchange MTU failed err: %u", __func__, err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(update_connection_parameter)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return;
    }

    err = bt_conn_le_param_update(conn, &req->adpt.conn_param);
    if (err < 0) {
        BT_LOGE("%s, update param failed err:%d", __func__, err);
        return;
    }
}

bt_status_t bt_sal_gatt_client_update_connection_parameter(bt_controller_id_t id, bt_address_t* addr, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    sal_adapter_req_t* req;

    if (min_interval > max_interval) {
        BT_LOGE("%s, min_interval > max_interval", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    req = sal_adapter_req(id, addr, STACK_CALL(update_connection_parameter));
    if (!req) {
        return BT_STATUS_NOMEM;
    }

    req->adpt.conn_param.interval_min = min_interval;
    req->adpt.conn_param.interval_max = max_interval;
    req->adpt.conn_param.latency = latency;
    req->adpt.conn_param.timeout = timeout;

    return sal_send_req(req);
}

bt_status_t bt_sal_gatt_client_read_remote_rssi(bt_controller_id_t id, bt_address_t* addr)
{
    struct bt_conn* conn;
    int err;
    int8_t rssi;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    err = bt_conn_le_read_rssi(conn, &rssi);
    if (err) {
        BT_LOGE("%s, read rssi failed err:%d", __func__, err)
        return BT_STATUS_FAIL;
    }

    if_gattc_on_rssi_read(addr, rssi, BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_read_phy(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BT_USER_PHY_UPDATE
    struct bt_conn* conn;
    struct bt_conn_info info;
    int err;
    ble_phy_type_t tx_mode;
    ble_phy_type_t rx_mode;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    err = bt_conn_get_info(conn, &info);
    if (err) {
        BT_LOGE("%s, conn get info err:%d", __func__, err);
        return BT_STATUS_FAIL;
    }

    tx_mode = le_phy_convert_from_stack(info.le.phy->tx_phy);
    rx_mode = le_phy_convert_from_stack(info.le.phy->rx_phy);

    BT_LOGD("%s, tx phy:%d, rx phy:%d", __func__, tx_mode, rx_mode);
    if_gattc_on_phy_read(addr, tx_mode, rx_mode);

    return BT_STATUS_SUCCESS;
#else
    SAL_NOT_SUPPORT;
#endif
}

bt_status_t bt_sal_gatt_client_set_phy(bt_controller_id_t id, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    return bt_sal_le_set_phy(id, addr, tx_phy, rx_phy);
}

void bt_sal_gatt_client_connection_updated_callback(bt_controller_id_t id, bt_address_t* addr, uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout, bt_status_t status)
{
    /* Notthing to do, implement within zblue_on_param_updated*/
}
#endif /* CONFIG_BLUETOOTH_GATT */