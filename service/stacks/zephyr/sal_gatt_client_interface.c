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

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>

#include <debug.h>

#include "sal_adapter_le_interface.h"
#include "sal_gatt_client_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "sal_zephyr_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#undef CONFIG_GATT_CLIENT_LOG

#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
#define STACK_CALL(func) zblue_##func

typedef void (*sal_func_t)(void* args);

typedef struct {
    uint16_t value_handle;
    struct bt_gatt_subscribe_params indicate_params;
    struct bt_gatt_subscribe_params notify_params;
} gatt_subscribe_slot_t;

typedef struct {
    uint16_t decl_handle;
    uint16_t value_handle;
    uint8_t properties;
    bt_uuid_t uuid;
} gatt_element_char_t;

union uuid {
    struct bt_uuid uuid;
    struct bt_uuid_16 u16;
    struct bt_uuid_128 u128;
};

struct gatt_service {
    uint16_t start_handle;
    uint16_t end_handle;
    bt_uuid_t uuid;
};

struct gatt_instance {
    bool active;
    bt_address_t addr;
    uint8_t service_idx;
    uint8_t service_size;
    uint8_t element_idx;
    uint8_t element_size;
    uint8_t element_char_idx;
    uint8_t element_char_size;
    uint8_t current_element_base_idx;
    struct gatt_service service[CONFIG_GATT_CLIENT_SERVICE_MAX];
    gatt_element_t element[CONFIG_GATT_CLIENT_ELEMENT_MAX];
    gatt_element_char_t element_char[CONFIG_GATT_CLIENT_CHAR_PER_SERVICE_MAX];
    gatt_subscribe_slot_t subscribe_slot[CONFIG_GATT_CLIENT_CHAR_PER_SERVICE_MAX];
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

static bool zblue_uuid2_to_uuid1(struct bt_uuid* u1, const bt_uuid_t* u2);

static void zblue_gattc_mtu_updated_callback(struct bt_conn* conn, uint16_t tx, uint16_t rx);

static bt_status_t zblue_gatt_client_discover_include_service(struct bt_conn* conn, const struct bt_uuid* uuid,
    uint16_t start_handle, uint16_t end_handle);

static bt_status_t zblue_gatt_client_discover_chrc(struct bt_conn* conn, const struct bt_uuid* uuid,
    uint16_t start_handle, uint16_t end_handle);

static bt_status_t zblue_gatt_client_discover_descriptor(struct bt_conn* conn, const struct bt_uuid* uuid,
    uint16_t start_handle, uint16_t end_handle);

static struct gatt_instance g_gatt_client[CONFIG_BLUETOOTH_GATTC_MAX_CONNECTIONS];

static struct bt_gatt_cb zblue_gatt_callbacks = {
    .att_mtu_updated = zblue_gattc_mtu_updated_callback
};

static void gatt_discover_cleanup(struct gatt_instance* instance)
{
    instance->element_size = 0;
    instance->service_idx = 0;
    instance->service_size = 0;
    instance->current_element_base_idx = 0;
    instance->element_char_idx = 0;
    instance->element_char_size = 0;
}

static bool gatt_is_service_discovery_complete(struct gatt_instance* instance)
{
    return (instance->element_char_idx == 0 && instance->element_char_size == 0);
}

static gatt_subscribe_slot_t* gatt_find_subscribe_slot(struct gatt_instance* instance, uint16_t value_handle)
{
    for (int i = 0; i < CONFIG_GATT_CLIENT_CHAR_PER_SERVICE_MAX; i++) {
        if (instance->subscribe_slot[i].value_handle == value_handle) {
            return &instance->subscribe_slot[i];
        }
    }
    return NULL;
}

static gatt_subscribe_slot_t* gatt_get_or_create_subscribe_slot(struct gatt_instance* instance, uint16_t value_handle)
{
    gatt_subscribe_slot_t* slot = gatt_find_subscribe_slot(instance, value_handle);

    if (slot) {
        return slot;
    }

    for (int i = 0; i < CONFIG_GATT_CLIENT_CHAR_PER_SERVICE_MAX; i++) {
        if (instance->subscribe_slot[i].value_handle == 0) {
            instance->subscribe_slot[i].value_handle = value_handle;
            memset(&instance->subscribe_slot[i].notify_params, 0, sizeof(struct bt_gatt_subscribe_params));
            memset(&instance->subscribe_slot[i].indicate_params, 0, sizeof(struct bt_gatt_subscribe_params));
            return &instance->subscribe_slot[i];
        }
    }

    return NULL;
}

static void gatt_delete_subscribe_slot_by_param(struct gatt_instance* instance, struct bt_gatt_subscribe_params* param)
{
    for (int i = 0; i < CONFIG_GATT_CLIENT_CHAR_PER_SERVICE_MAX; i++) {
        gatt_subscribe_slot_t* slot = &instance->subscribe_slot[i];

        if (slot->value_handle == 0)
            continue;

        if (&slot->notify_params == param) {
            memset(&slot->notify_params, 0, sizeof(struct bt_gatt_subscribe_params));
            return;
        }

        if (&slot->indicate_params == param) {
            memset(&slot->indicate_params, 0, sizeof(struct bt_gatt_subscribe_params));
            return;
        }
    }
}

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
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void STACK_CALL(conn_connect)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t address = { 0 };
    struct bt_conn* conn = NULL;
    int err;

    if (bt_conn_set_role(BT_TRANSPORT_BLE, &req->addr, GATT_ROLE_CLIENT) != BT_STATUS_SUCCESS) {
        return;
    }

    address.type = req->addr_type;
    memcpy(&address.a, &req->addr, sizeof(address.a));

    err = bt_conn_le_create(&address, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &conn);
    if (err) {
        bt_conn_remove(&req->addr, BT_TRANSPORT_BLE);
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
        type = BT_ADDR_LE_PUBLIC;
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

static uint8_t zblue_gatt_client_disc_desc_callback(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    struct gatt_instance* instance;
    struct gatt_service* service;
    gatt_element_t* element;
    bt_address_t addr;
    uint16_t start_HDL, end_HDL;
    gatt_element_t* element_db;

    bt_sal_get_remote_address(conn, &addr);
    instance = gatt_find_instance_by_addr(&addr);
    if (!instance) {
        BT_LOGE("%s, instance null", __func__);
        return BT_GATT_ITER_STOP;
    }

    if (!attr) {
#ifdef CONFIG_GATT_CLIENT_LOG
        BT_LOGD("%s, descriptor discovery finished for service_idx:%d", __func__, instance->service_idx);
#endif

        if (gatt_is_service_discovery_complete(instance)) {

            if (instance->service_idx < instance->service_size) {
                uint8_t base = instance->current_element_base_idx;
                uint8_t size = instance->element_size - base;

                element_db = calloc(size, sizeof(gatt_element_t));
                memcpy(element_db, &instance->element[base], size * sizeof(gatt_element_t));

                if_gattc_on_service_discovered(&instance->addr, element_db, size);

                service = &instance->service[instance->service_idx];
                instance->current_element_base_idx = instance->element_size;

                /* Save new service declaration */
                element = gatt_alloc_element_by_addr(&addr);
                if (element) {
                    element->handle = service->start_handle;
                    memcpy(&element->uuid, &service->uuid, sizeof(bt_uuid_t));
                    element->type = BT_GATT_DISCOVER_PRIMARY;
                    element->properties = 0;
                    element->permissions = 0;
                }
                zblue_gatt_client_discover_include_service(conn, NULL, service->start_handle, service->end_handle);
            } else {
#ifdef CONFIG_GATT_CLIENT_LOG
                BT_LOGD("%s, all services discovered", __func__);
#endif
                uint8_t base = instance->current_element_base_idx;
                uint8_t size = instance->element_size - base;

                element_db = calloc(size, sizeof(gatt_element_t));
                memcpy(element_db, &instance->element[base], size * sizeof(gatt_element_t));

                if_gattc_on_service_discovered(&instance->addr, element_db, size);
                if_gattc_on_discover_completed(&addr, GATT_STATUS_SUCCESS);
                gatt_discover_cleanup(instance);
            }
            return BT_GATT_ITER_STOP;
        }

    iter:
        service = &instance->service[instance->service_idx];

        element = gatt_alloc_element_by_addr(&addr);
        if (!element) {
            BT_LOGE("%s, alloc element fail", __func__);
            return BT_GATT_ITER_STOP;
        }
        element->type = BT_GATT_DISCOVER_CHARACTERISTIC;
        element->handle = instance->element_char[instance->element_char_idx].value_handle;
        element->properties = instance->element_char[instance->element_char_idx].properties;
        memcpy(&element->uuid, &instance->element_char[instance->element_char_idx].uuid, sizeof(bt_uuid_t));

        start_HDL = instance->element_char[instance->element_char_idx++].value_handle + 1;

        if (instance->element_char_idx < instance->element_char_size) {
            end_HDL = instance->element_char[instance->element_char_idx].decl_handle - 1;
        } else {
            end_HDL = service->end_handle;
            instance->element_char_idx = 0;
            instance->element_char_size = 0;
            instance->service_idx++;
        }

        if (start_HDL <= end_HDL) {
            zblue_gatt_client_discover_descriptor(conn, NULL, start_HDL, end_HDL);
        } else if (gatt_is_service_discovery_complete(instance)) {

            if (instance->service_idx < instance->service_size) {
                uint8_t base = instance->current_element_base_idx;
                uint8_t size = instance->element_size - base;

                element_db = calloc(size, sizeof(gatt_element_t));
                memcpy(element_db, &instance->element[base], size * sizeof(gatt_element_t));

                if_gattc_on_service_discovered(&instance->addr, element_db, size);

                service = &instance->service[instance->service_idx];
                instance->current_element_base_idx = instance->element_size;

                /* Save new service declaration */
                element = gatt_alloc_element_by_addr(&addr);
                if (element) {
                    element->handle = service->start_handle;
                    memcpy(&element->uuid, &service->uuid, sizeof(bt_uuid_t));
                    element->type = BT_GATT_DISCOVER_PRIMARY;
                    element->properties = 0;
                    element->permissions = 0;
                }
                zblue_gatt_client_discover_chrc(conn, NULL, service->start_handle, service->end_handle);
            } else {
#ifdef CONFIG_GATT_CLIENT_LOG
                BT_LOGD("%s, all services discovered", __func__);
#endif
                uint8_t base = instance->current_element_base_idx;
                uint8_t size = instance->element_size - base;

                element_db = calloc(size, sizeof(gatt_element_t));
                memcpy(element_db, &instance->element[base], size * sizeof(gatt_element_t));

                if_gattc_on_service_discovered(&instance->addr, element_db, size);
                if_gattc_on_discover_completed(&addr, GATT_STATUS_SUCCESS);
                gatt_discover_cleanup(instance);
            }

            return BT_GATT_ITER_STOP;
        } else {
            goto iter;
        }
        return BT_GATT_ITER_STOP;
    }

#ifdef CONFIG_GATT_CLIENT_LOG
    BT_LOGD("%s, [DESCRIPTOR] handle 0x%04X", __func__, attr->handle);
#endif

    element = gatt_alloc_element_by_addr(&addr);
    if (!element) {
        BT_LOGE("%s, alloc element fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    element->type = params->type;
    element->handle = attr->handle;
    element->properties = 0;
    element->permissions = attr->perm;

    zblue_uuid1_to_uuid2(attr->uuid, &element->uuid);

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t zblue_gatt_client_disc_chrc_callback(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    struct bt_gatt_chrc* data;
    struct gatt_instance* instance;
    struct gatt_service* service;
    gatt_element_t* element;
    bt_address_t addr;
    uint16_t start_HDL, end_HDL;
    gatt_element_t* element_db;

    bt_sal_get_remote_address(conn, &addr);

    instance = gatt_find_instance_by_addr(&addr);
    if (!instance) {
        BT_LOGE("%s, instance null", __func__);
        bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &addr);
        return BT_GATT_ITER_STOP;
    }

    if (!attr) {
#ifdef CONFIG_GATT_CLIENT_LOG
        BT_LOGD("%s, finished discovering characteristics for service_idx:%d", __func__, instance->service_idx);
#endif
        service = &instance->service[instance->service_idx];

    iter:
        element = gatt_alloc_element_by_addr(&addr);
        if (!element) {
            BT_LOGE("%s, alloc element fail", __func__);
            bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &addr);
            return BT_GATT_ITER_STOP;
        }

        element->type = BT_GATT_DISCOVER_CHARACTERISTIC;
        element->handle = instance->element_char[instance->element_char_idx].value_handle;
        element->properties = instance->element_char[instance->element_char_idx].properties;
        memcpy(&element->uuid, &instance->element_char[instance->element_char_idx].uuid, sizeof(bt_uuid_t));

        start_HDL = instance->element_char[instance->element_char_idx++].value_handle + 1;

        if (instance->element_char_idx < instance->element_char_size) {
            end_HDL = instance->element_char[instance->element_char_idx].decl_handle - 1;
        } else {
            end_HDL = service->end_handle;
            instance->element_char_idx = 0;
            instance->element_char_size = 0;
            instance->service_idx++;
        }

        if (start_HDL <= end_HDL) {
            zblue_gatt_client_discover_descriptor(conn, NULL, start_HDL, end_HDL);
        } else if (gatt_is_service_discovery_complete(instance)) {
            if (instance->service_idx < instance->service_size) {
                uint8_t base = instance->current_element_base_idx;
                uint8_t size = instance->element_size - base;

                element_db = calloc(size, sizeof(gatt_element_t));
                memcpy(element_db, &instance->element[base], size * sizeof(gatt_element_t));

                if_gattc_on_service_discovered(&instance->addr, element_db, size);

                instance->current_element_base_idx = instance->element_size;
                service = &instance->service[instance->service_idx];
                /* Save new service declaration */
                element = gatt_alloc_element_by_addr(&addr);
                if (element) {
                    element->handle = service->start_handle;
                    memcpy(&element->uuid, &service->uuid, sizeof(bt_uuid_t));
                    element->type = BT_GATT_DISCOVER_PRIMARY;
                    element->properties = 0;
                    element->permissions = 0;
                    zblue_gatt_client_discover_include_service(conn, NULL, service->start_handle, service->end_handle);
                }
            } else {
#ifdef CONFIG_GATT_CLIENT_LOG
                BT_LOGD("%s, all services discovered", __func__);
#endif
                uint8_t base = instance->current_element_base_idx;
                uint8_t size = instance->element_size - base;

                element_db = calloc(size, sizeof(gatt_element_t));
                memcpy(element_db, &instance->element[base], size * sizeof(gatt_element_t));

                if_gattc_on_service_discovered(&instance->addr, element_db, size);
                if_gattc_on_discover_completed(&addr, GATT_STATUS_SUCCESS);
                gatt_discover_cleanup(instance);
            }
            return BT_GATT_ITER_STOP;
        } else {
            goto iter;
        }
        return BT_GATT_ITER_STOP;
    }

#ifdef CONFIG_GATT_CLIENT_LOG
    BT_LOGD("%s, [CHAR] handle 0x%04X", __func__, attr->handle);
#endif

    data = attr->user_data;

    if (instance->element_char_size >= CONFIG_GATT_CLIENT_CHAR_PER_SERVICE_MAX) {
        BT_LOGE("%s, too many chars", __func__);
        bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &addr);
        return BT_GATT_ITER_STOP;
    }

    gatt_element_char_t* ch = &instance->element_char[instance->element_char_size++];
    ch->decl_handle = attr->handle;
    ch->value_handle = data->value_handle;
    ch->properties = data->properties;
    zblue_uuid1_to_uuid2(data->uuid, &ch->uuid);

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t zblue_gatt_client_disc_service_callback(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    struct bt_gatt_service_val* data;
    struct gatt_instance* instance;
    struct gatt_service* service;
    bt_address_t addr;

    bt_sal_get_remote_address(conn, &addr);

    instance = gatt_find_alloc_instance_by_addr(&addr);
    if (!instance) {
        BT_LOGE("%s, instance find alloc fail", __func__);
        bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &addr);
        return BT_GATT_ITER_STOP;
    }

    if (!attr) {
#ifdef CONFIG_GATT_CLIENT_LOG
        BT_LOGD("%s, start discovery service finished, start discovery char service_idx:%d",
            __func__, instance->service_idx);
#endif

        service = &instance->service[instance->service_idx];

        /* Saving current first service in elements table */
        gatt_element_t* element = gatt_alloc_element_by_addr(&addr);
        if (element) {
            element->handle = service->start_handle;
            memcpy(&element->uuid, &service->uuid, sizeof(bt_uuid_t));
            element->type = BT_GATT_DISCOVER_PRIMARY;
            element->properties = 0;
            element->permissions = 0;
        }

        zblue_gatt_client_discover_include_service(conn, NULL, service->start_handle, service->end_handle);
        return BT_GATT_ITER_STOP;
    }

#ifdef CONFIG_GATT_CLIENT_LOG
    BT_LOGD("%s, [SERVICE] handle 0x%04X", __func__, attr->handle);
#endif

    service = gatt_alloc_service_by_addr(&addr);
    if (!service) {
        BT_LOGE("%s, alloc service fail", __func__);
        bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &addr);
        return BT_GATT_ITER_STOP;
    }

    data = attr->user_data;
    service->start_handle = attr->handle;
    service->end_handle = data->end_handle;
    zblue_uuid1_to_uuid2(data->uuid, &service->uuid);

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t zblue_gatt_client_disc_include_callback(struct bt_conn* conn,
    const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    struct bt_gatt_include* data;
    struct gatt_instance* instance;
    struct gatt_service* service;
    bt_address_t addr;

    bt_sal_get_remote_address(conn, &addr);

    instance = gatt_find_alloc_instance_by_addr(&addr);
    if (!instance) {
        BT_LOGE("%s, instance find fail", __func__);
        bt_sal_gatt_client_disconnect(PRIMARY_ADAPTER, &addr);
        return BT_GATT_ITER_STOP;
    }

    service = &instance->service[instance->service_idx];

    if (!attr) {
        zblue_gatt_client_discover_chrc(conn, NULL, service->start_handle, service->end_handle);
        return BT_GATT_ITER_STOP;
    }

    data = attr->user_data;
    if (!data) {
        BT_LOGW("%s, include user_data null", __func__);
        return BT_GATT_ITER_CONTINUE;
    }

    BT_LOGD("[INCLUDE] attr 0x%04x -> service 0x%04x - 0x%04x",
        attr->handle, data->start_handle, data->end_handle);

    gatt_element_t* element = gatt_alloc_element_by_addr(&addr);
    if (element) {
        element->handle = attr->handle;
        element->type = BT_GATT_DISCOVER_INCLUDE;
        element->properties = 0;
        element->permissions = 0;
        zblue_uuid1_to_uuid2(data->uuid, &element->uuid);
    }

    return BT_GATT_ITER_CONTINUE;
}

static bt_status_t zblue_gatt_client_discover_descriptor(struct bt_conn* conn, const struct bt_uuid* uuid,
    uint16_t start_handle, uint16_t end_handle)
{
    static struct bt_gatt_discover_params desc_params = { 0 };

    memset(&desc_params, 0, sizeof(desc_params));
    desc_params.uuid = uuid;
    desc_params.start_handle = start_handle;
    desc_params.end_handle = end_handle;
    desc_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
    desc_params.func = zblue_gatt_client_disc_desc_callback;

#ifdef CONFIG_GATT_CLIENT_LOG
    BT_LOGD("%s, element search: type=0x%02X, start handle=0x%04X, end handle=0x%04X", __func__,
        desc_params.type, desc_params.start_handle, desc_params.end_handle);
#endif

    if (bt_gatt_discover(conn, &desc_params) < 0) {
        BT_LOGE("%s, descriptor discovery failed", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
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

#ifdef CONFIG_GATT_CLIENT_LOG
    BT_LOGD("%s, element search: type=0x%02X, start handle=0x%04X, end handle=0x%04X", __func__,
        discover_params.type, discover_params.start_handle, discover_params.end_handle);
#endif

    if (bt_gatt_discover(conn, &discover_params) < 0) {
        BT_LOGE("%s, gatt discovery fail", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static uint8_t gatt_client_read_element_callback(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_read_params* params, const void* data, uint16_t length)
{
    bt_address_t addr;

    bt_sal_get_remote_address(conn, &addr);

    if (err) {
        BT_LOGE("%s, gatt read fail err:%d", __func__, err);
        if_gattc_on_element_read(&addr, params->single.handle, (uint8_t*)data, length, GATT_STATUS_FAILURE);
        return BT_GATT_ITER_STOP;
    }

    BT_LOGD("%s, [DATA] len:%d, handle:0x%0x", __func__, length, params->single.handle);
    lib_dumpbuffer("read element", data, length);

    if_gattc_on_element_read(&addr, params->single.handle, (uint8_t*)data, length, GATT_STATUS_SUCCESS);

    return BT_GATT_ITER_STOP;
}

static void gatt_client_write_cmd_callback(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_write_params* params)
{
    bt_address_t addr;

    bt_sal_get_remote_address(conn, &addr);

    if (err) {
        BT_LOGE("%s, gatt write fail err:%d", __func__, err);
        if_gattc_on_element_written(&addr, params->handle, GATT_STATUS_FAILURE);
        free(params);
        return;
    }

    if_gattc_on_element_written(&addr, params->handle, GATT_STATUS_SUCCESS);

    free(params);
}

static void gatt_client_write_callback(struct bt_conn* conn, void* user_data)
{
    uint16_t* handle = user_data;
    bt_address_t addr;

    bt_sal_get_remote_address(conn, &addr);

    if_gattc_on_element_written(&addr, *handle, GATT_STATUS_SUCCESS);

    free(handle);
}

static uint8_t bt_gatt_notify_handler(struct bt_conn* conn, struct bt_gatt_subscribe_params* params,
    const void* data, uint16_t length)
{
    uint16_t handle;
    bt_address_t addr;

    bt_sal_get_remote_address(conn, &addr);

    handle = params->value_handle;
    if (data == NULL) {
        BT_LOGE("[UNSUBSCRIBED] 0x%04X", params->value_handle);
        return BT_GATT_ITER_STOP;
    }

    if_gattc_on_element_changed(&addr, handle, (uint8_t*)data, length);
    return BT_GATT_ITER_CONTINUE;
}

static void bt_gatt_subscribe_response(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_subscribe_params* params)
{
    bt_address_t addr;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    BT_LOGD("%s, err:%d", __func__, err);

    if_gattc_on_element_subscribed(&addr, params->value_handle, err ? GATT_STATUS_FAILURE : GATT_STATUS_SUCCESS, true);
}

static void bt_gatt_unsubscribe_response(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_subscribe_params* params)
{
    bt_address_t addr;
    struct gatt_instance* instance;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    BT_LOGD("%s, err:%d", __func__, err);

    if_gattc_on_element_subscribed(&addr, params->value_handle, err ? GATT_STATUS_FAILURE : GATT_STATUS_SUCCESS, false);

    if (err == 0) {
        instance = gatt_find_instance_by_addr(&addr);
        if (instance) {
            gatt_delete_subscribe_slot_by_param(instance, params);
        }
    }
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

static bt_status_t zblue_gatt_client_discover_include_service(struct bt_conn* conn, const struct bt_uuid* uuid,
    uint16_t start_handle, uint16_t end_handle)
{
    static struct bt_gatt_discover_params disc_params = { 0 };
    int err;

    disc_params.uuid = NULL;
    disc_params.start_handle = start_handle;
    disc_params.end_handle = end_handle;
    disc_params.type = BT_GATT_DISCOVER_INCLUDE;
    disc_params.func = zblue_gatt_client_disc_include_callback;

    err = bt_gatt_discover(conn, &disc_params);
    if (err < 0) {
        BT_LOGE("%s, bt_gatt_discover(include) fail", __func__);
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
    int err = 0;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    switch (write_type) {
    case GATT_WRITE_TYPE_RSP: {
        struct bt_gatt_write_params* write_params = (struct bt_gatt_write_params*)zalloc(sizeof(struct bt_gatt_write_params));
        if (!write_params) {
            return BT_STATUS_NOMEM;
        }

        write_params->func = gatt_client_write_cmd_callback;
        write_params->handle = element_id;
        write_params->data = value;
        write_params->length = length;
        write_params->offset = 0;

        err = bt_gatt_write(conn, write_params);
        if (err) {
            BT_LOGE("%s, gatt write fail err:%d", __func__, err);
            free(write_params);
            return BT_STATUS_FAIL;
        }
        break;
    }

    case GATT_WRITE_TYPE_NO_RSP: {
        uint16_t* handle = (uint16_t*)malloc(sizeof(uint16_t));
        if (!handle) {
            return BT_STATUS_NOMEM;
        }
        *handle = element_id;

        err = bt_gatt_write_without_response_cb(conn, element_id, value, length,
            false, gatt_client_write_callback, handle);
        if (err) {
            BT_LOGE("%s, gatt write without rsp fail err:%d", __func__, err);
            free(handle);
            return BT_STATUS_FAIL;
        }
        break;
    }

#ifdef CONFIG_BT_SIGNING
    case GATT_WRITE_TYPE_SIGNED: {
        uint16_t* handle = (uint16_t*)malloc(sizeof(uint16_t));
        if (!handle) {
            return BT_STATUS_NOMEM;
        }
        *handle = element_id;

        err = bt_gatt_write_without_response_cb(conn, element_id, value, length,
            true, gatt_client_write_callback, handle);
        if (err) {
            BT_LOGE("%s, gatt write (signed) fail err:%d", __func__, err);
            free(handle);
            return BT_STATUS_FAIL;
        }
        break;
    }
#endif

    default:
        BT_LOGE("%s, unsupported write_type:%d", __func__, write_type);
        return BT_STATUS_NOT_SUPPORTED;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_register_notifications(bt_controller_id_t id, bt_address_t* addr, uint16_t element_id, uint16_t properties, bool enable)
{
    struct gatt_instance* instance;
    struct bt_conn* conn;
    int err;
    uint16_t ccc_handle;
    gatt_subscribe_slot_t* slot;

    if (!(properties & (GATT_PROP_NOTIFY | GATT_PROP_INDICATE))) {
        BT_LOGE("%s, invalid properties:0x%04x", __func__, properties);
        return BT_STATUS_PARM_INVALID;
    }

    BT_LOGD("%s, addr:%s, element_id:0x%0x, properties:0x%0x, enable:%d", __func__, bt_addr_str(addr), element_id, properties, enable);
    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    instance = gatt_find_instance_by_addr(addr);
    if (!instance) {
        BT_LOGE("%s, instance not found", __func__);
        return BT_STATUS_FAIL;
    }

    ccc_handle = if_gattc_find_ccc_handle_by_value_handle(&instance->addr, element_id);
    if (!ccc_handle) {
        BT_LOGE("%s, no CCC handle found for element:0x%04x", __func__, element_id);
        return BT_STATUS_FAIL;
    }

    slot = gatt_get_or_create_subscribe_slot(instance, element_id);
    if (!slot) {
        BT_LOGE("%s, no slot available", __func__);
        return BT_STATUS_FAIL;
    }

    if (properties & GATT_PROP_NOTIFY) {
        struct bt_gatt_subscribe_params* notify = &slot->notify_params;

        notify->value_handle = element_id;
        notify->ccc_handle = ccc_handle;
        notify->notify = bt_gatt_notify_handler;
        /* Set VOLATILE flag to clear subscription on disconnect */
        atomic_set_bit(notify->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
        notify->value = BT_GATT_CCC_NOTIFY;

        if (enable) {
            notify->subscribe = bt_gatt_subscribe_response;
            err = bt_gatt_subscribe(conn, notify);
        } else {
            notify->subscribe = bt_gatt_unsubscribe_response;
            err = bt_gatt_unsubscribe(conn, notify);
        }

        if (err && err != -EALREADY) {
            BT_LOGE("%s, %s NOTIFY failed, err:%d", __func__,
                enable ? "subscribe" : "unsubscribe", err);
            if_gattc_on_element_subscribed(addr, element_id, GATT_STATUS_FAILURE, enable);
            return BT_STATUS_FAIL;
        }

        if (err == -EALREADY) {
            if_gattc_on_element_subscribed(addr, element_id, GATT_STATUS_SUCCESS, enable);
        }
    }

    if (properties & GATT_PROP_INDICATE) {
        struct bt_gatt_subscribe_params* indicate = &slot->indicate_params;

        indicate->value_handle = element_id;
        indicate->ccc_handle = ccc_handle;
        indicate->notify = bt_gatt_notify_handler;
        /* Set VOLATILE flag to clear subscription on disconnect */
        atomic_set_bit(indicate->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
        indicate->value = BT_GATT_CCC_INDICATE;

        if (enable) {
            indicate->subscribe = bt_gatt_subscribe_response;
            err = bt_gatt_subscribe(conn, indicate);
        } else {
            indicate->subscribe = bt_gatt_unsubscribe_response;
            err = bt_gatt_unsubscribe(conn, indicate);
        }

        if (err && err != -EALREADY) {
            BT_LOGE("%s, %s INDICATE failed, err:%d", __func__,
                enable ? "subscribe" : "unsubscribe", err);
            if_gattc_on_element_subscribed(addr, element_id, GATT_STATUS_FAILURE, enable);
            return BT_STATUS_FAIL;
        }

        if (err == -EALREADY) {
            if_gattc_on_element_subscribed(addr, element_id, GATT_STATUS_SUCCESS, enable);
        }
    }

    return BT_STATUS_SUCCESS;
}

static void zblue_gattc_mtu_updated_callback(struct bt_conn* conn, uint16_t tx, uint16_t rx)
{
    bt_address_t addr;
    uint16_t att_mtu = MIN(tx, rx);
    uint16_t att_payload = (att_mtu >= 23) ? (att_mtu - 3) : 20;

    bt_sal_get_remote_address(conn, &addr);
    if_gattc_on_mtu_changed(&addr, att_payload, GATT_STATUS_SUCCESS);
}

static void gatt_exchange_mtu_func(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_exchange_params* params)
{
    bt_address_t addr;
    uint16_t mtu;

    bt_sal_get_remote_address(conn, &addr);

    if (err) {
        BT_LOGE("%s, exchange MTU failed err: %u", __func__, err);
        if_gattc_on_mtu_changed(&addr, 0, GATT_STATUS_FAILURE);
        return;
    }

    mtu = bt_gatt_get_mtu(conn);
    if_gattc_on_mtu_changed(&addr, mtu, GATT_STATUS_SUCCESS);
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

bt_status_t bt_sal_gatt_client_enable(void)
{
    bt_gatt_cb_register(&zblue_gatt_callbacks);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_disable(void)
{
    bt_gatt_cb_unregister(&zblue_gatt_callbacks);

    return BT_STATUS_SUCCESS;
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

    err = bt_conn_read_rssi(conn, &rssi);
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

void bt_sal_gatt_client_connection_state_changed_callback(bt_controller_id_t id, bt_address_t* addr, profile_connection_state_t state)
{
    if (state == PROFILE_STATE_DISCONNECTED) {
        gatt_free_instance(addr);
    }
    if_gattc_on_connection_state_changed(addr, state);
}

#endif /* CONFIG_BLUETOOTH_GATT_CLIENT */