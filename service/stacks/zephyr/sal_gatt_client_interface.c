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

#define LOG_TAG "sal_gattc"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

#include "sal_gatt_client_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "sal_zephyr_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#ifndef BT_LOGV
#define BT_LOGV(...)
#endif

/** Attribute Opcode (1) + Attribute Handle (2)
 *  This is valid for most ATT PDUs and maintains the same behavior as in other stacks.
 *  FIXME: Some PDUs have different header sizes, e.g., ATT_PREPARE_WRITE_REQ.
 */
#define ATT_HEADER_SIZE (3)
#define LE_ATT_MTU_MIN (23)

#define SAL_LE_CONNECTION_INTERVAL_MIN (0x0006)
#define SAL_LE_CONNECTION_INTERVAL_MAX (0x0C80)
#define SAL_LE_LATENCY_MAX (0x01F3)
#define SAL_LE_SUPERVISION_TIMEOUT_MAX (0x0C80)

typedef void (*sal_func_t)(void* args);
typedef struct sal_gattc_info {
    bool terminating;
    bt_list_t* list; /**< sal_gattc_conn_t */
} sal_gattc_info_t;

typedef struct sal_gattc_subscribe {
    uint16_t value_handle;
    uint16_t ccc_handle;
    struct bt_gatt_subscribe_params* notify_params;
    struct bt_gatt_subscribe_params* indicate_params;
} sal_gattc_subscribe_t;

typedef struct sal_gattc_conn {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type;
    profile_connection_state_t state;
    struct bt_conn* conn;
    struct bt_gatt_read_params read_params;
    bt_list_t* services; /**< sal_gattc_service_t */
    bt_list_t* subscribes; /**< sal_gattc_subscribe_t */
} sal_gattc_conn_t;

typedef struct sal_gattc_service sal_gattc_service_t;
typedef struct sal_gattc_service {
    sal_gattc_conn_t* gattc;
    sal_gattc_service_t* service; /**< the primary service, only valid for include services */
    gatt_element_t* element;
    uint8_t element_size;
    uint16_t end_handle;
    bt_list_t* include_services; /**< sal_gattc_service_t */
    bt_list_t* chrcs; /**< sal_gattc_characteristic_t */
} sal_gattc_service_t;

typedef struct sal_gattc_characteristic {
    sal_gattc_service_t* service; /**< the primary service */
    gatt_element_t* element;
    uint8_t element_size;
    uint16_t end_handle;
    bt_list_t* descriptors; /**< sal_gattc_descriptor_t */
} sal_gattc_characteristic_t;

typedef struct sal_gattc_descriptor {
    sal_gattc_characteristic_t* chrc;
    gatt_element_t* element;
    uint8_t element_size;
} sal_gattc_descriptor_t;

typedef struct {
    struct bt_conn_le_create_param create_param;
    struct bt_le_conn_param conn_param;
} sal_gattc_conn_args_t;

typedef struct {
    struct bt_gatt_discover_params params;
    union {
        struct bt_uuid uuid;
        struct bt_uuid_16 uuid_16;
        struct bt_uuid_32 uuid_32;
        struct bt_uuid_128 uuid_128;
    };
} sal_gattc_discover_args_t;

typedef struct {
    struct bt_le_conn_param param;
} sal_gattc_conn_param_args_t;

typedef struct {
    uint16_t element_id;
} sal_gattc_read_args_t;

typedef struct {
    uint16_t element_id;
    uint16_t length;
    gatt_write_type_t write_type;
    uint8_t value[];
} sal_gattc_write_args_t;

typedef struct {
    uint16_t element_id;
    uint16_t properties;
    bool enable;
} sal_gattc_subscribe_args_t;

typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type;
    sal_func_t func;
    uint8_t data[];
} sal_gattc_req_t;

static sal_gattc_info_t* g_sal_gattc_info = NULL;
static bt_status_t uuid_zephyr_to_sal(bt_uuid_t* out, const struct bt_uuid* in);
static bt_status_t uuid_sal_to_zephyr(struct bt_uuid* out, const bt_uuid_t* in);
static bt_status_t addr_sal_to_zephyr(bt_addr_le_t* out, const bt_address_t* addr,
    ble_addr_type_t addr_type);
static void zblue_discover(void* data);
static void zblue_read_element(void* data);
static void zblue_write_element(void* data);
static void zblue_register_notifications(void* data);
static void zblue_send_mtu_req(void* data);
static sal_gattc_conn_t* find_gattc_conn_by_conn(struct bt_conn* conn);
static sal_gattc_conn_t* find_gattc_conn(bt_controller_id_t id, const bt_address_t* addr,
    ble_addr_type_t addr_type);
static sal_gattc_service_t* find_service_by_handle(sal_gattc_conn_t* gattc, uint16_t handle);
static sal_gattc_characteristic_t* find_characteristic_by_handle(sal_gattc_service_t* service,
    uint16_t handle);
static sal_gattc_subscribe_t* find_subscribe_by_value_handle(sal_gattc_conn_t* gattc,
    uint16_t value_handle);
static bt_status_t discover_include_services(const sal_gattc_conn_t* gattc,
    sal_gattc_service_t* service);
static bt_status_t discover_characteristics(const sal_gattc_conn_t* gattc,
    sal_gattc_service_t* service);
static bt_status_t discover_descriptors(const sal_gattc_conn_t* gattc,
    sal_gattc_characteristic_t* chrc);

static sal_gattc_req_t* sal_gattc_req(bt_controller_id_t id, const bt_address_t* addr,
    ble_addr_type_t addr_type, sal_func_t func, size_t size)
{
    sal_gattc_req_t* req = zalloc(sizeof(sal_gattc_req_t) + size);

    if (!req)
        return NULL;

    req->id = id;
    req->func = func;
    req->addr_type = addr_type;
    if (addr)
        memcpy(&req->addr, addr, sizeof(bt_address_t));

    return req;
}

static void sal_invoke_async(service_work_t* work, void* userdata)
{
    sal_gattc_req_t* req = userdata;

    SAL_ASSERT(req);
    req->func(req);
    free(userdata);
}

static bt_status_t sal_send_req(sal_gattc_req_t* req)
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

static void subscribe_delete(void* data)
{
    sal_gattc_subscribe_t* subscribe = data;

    free(subscribe->indicate_params);
    free(subscribe->notify_params);
    free(subscribe);
}

static void update_subscribes(sal_gattc_conn_t* gattc, sal_gattc_descriptor_t* descriptor)
{
    sal_gattc_characteristic_t* chrc = descriptor->chrc;
    sal_gattc_subscribe_t* subscribe;
    uint16_t value_handle = chrc->element->handle;
    uint16_t ccc_handle = descriptor->element->handle;

    BT_LOGV("%s", __func__);

    /** Step 1. find if there is already a handle-pair with the current value handle */
    subscribe = find_subscribe_by_value_handle(gattc, value_handle);
    if (subscribe) {
        /** Step 1.1. check if the previous handle-pair still valid */
        if (subscribe->ccc_handle == ccc_handle) {
            /** repeated discovery, nothing to do */
            BT_LOGV("handle-pair [V:0x%04" PRIx16 "] -> [D:0x%04" PRIx16 "] already recorded",
                value_handle, ccc_handle);
            return;
        }

        /** Step 1.2. remove if the previous handle-pair is outdated */
        BT_LOGV("value handle [V:%04" PRIx16 "] now has a new ccc handle [D:%04" PRIx16 "] -> "
                "[D:%04" PRIx16 "]",
            value_handle, subscribe->ccc_handle, ccc_handle);
        bt_list_remove(gattc->subscribes, subscribe);
    }

    /** Step 2. value handle is now unique for this connection, add to list */
    subscribe = zalloc(sizeof(sal_gattc_subscribe_t));
    if (!subscribe)
        return;

    subscribe->value_handle = value_handle;
    subscribe->ccc_handle = ccc_handle;
    bt_list_add_tail(gattc->subscribes, subscribe);
}

/** Merge descriptor elements into the parent characteristic's element array.
 *  This uses realloc to grow chrc->element, then appends descriptor->element entries.
 *  On realloc failure the descriptor data is silently dropped — the original chrc->element
 *  remains valid but this descriptor's attributes won't appear in the reported element list. */
static void descriptor_delete(void* data)
{
    sal_gattc_descriptor_t* descriptor = data;
    sal_gattc_characteristic_t* chrc = descriptor->chrc;
    gatt_element_t* element;
    bt_uuid_t uuid_ccc;

    element = realloc(chrc->element,
        sizeof(gatt_element_t) * (chrc->element_size + descriptor->element_size));
    if (!element) {
        BT_LOGE("realloc failed, descriptor data dropped");
        free(descriptor->element);
        free(descriptor);
        return;
    }

    chrc->element = element;
    memcpy(&element[chrc->element_size], descriptor->element,
        sizeof(gatt_element_t) * descriptor->element_size);
    chrc->element_size += descriptor->element_size;

    /** Vela service will operate CCCDs via the value handle of the characteristic. Zephyr stack,
     *  however, will recognize the descriptor handle. Therefore, we need to back up handle-pairs
     *  if this is a CCCD. */
    bt_uuid16_create(&uuid_ccc, BT_UUID_GATT_CCC_VAL);
    if (bt_uuid_compare(&uuid_ccc, &descriptor->element->uuid) == 0) {
        BT_LOGV("cccd found, value handle = 0x%04" PRIx16 ", ccc handle = 0x%04" PRIx16,
            chrc->element->handle, descriptor->element->handle);
        update_subscribes(chrc->service->gattc, descriptor);
    }

    free(descriptor->element);
    free(descriptor);
}

/** Merge characteristic elements (including its already-merged descriptors) into the parent
 *  service's element array. On realloc failure the characteristic data is silently dropped. */
static void characteristic_delete(void* data)
{
    sal_gattc_characteristic_t* chrc = data;
    sal_gattc_service_t* service = chrc->service;
    gatt_element_t* element;

    bt_list_free(chrc->descriptors);

    element = realloc(service->element,
        sizeof(gatt_element_t) * (service->element_size + chrc->element_size));
    if (element) {
        service->element = element;
        memcpy(&element[service->element_size], chrc->element,
            sizeof(gatt_element_t) * chrc->element_size);
        service->element_size += chrc->element_size;
    } else {
        BT_LOGE("realloc failed, characteristic data dropped");
    }

    free(chrc->element);
    free(chrc);
}

/** For primary services: report the merged element array to the upper layer via callback, then
 *  free the service node (element ownership transfers to the callback).
 *  For included services: merge elements into the parent primary service's element array.
 *  On realloc failure the included service data is silently dropped. */
static void service_delete(void* data)
{
    sal_gattc_service_t* service = data;
    sal_gattc_service_t* primary_service;
    gatt_element_t* element;

    bt_list_free(service->include_services);
    bt_list_free(service->chrcs);

    if (service->element->type != GATT_INCLUDED_SERVICE) {
        if_gattc_on_service_discovered(&service->gattc->addr, service->element,
            service->element_size);
        /** @p service->element to be freed at @ref gattc_service_delete */
        free(service);
        return;
    }

    primary_service = service->service;
    element = realloc(primary_service->element,
        sizeof(gatt_element_t) * (primary_service->element_size + service->element_size));
    if (element) {
        primary_service->element = element;
        memcpy(&element[primary_service->element_size], service->element,
            sizeof(gatt_element_t) * service->element_size);
        primary_service->element_size += service->element_size;
    } else {
        BT_LOGE("realloc failed, included service data dropped");
    }

    free(service->element);
    free(service);
}

static void gattc_conn_delete(void* data)
{
    sal_gattc_conn_t* gattc = data;

    bt_list_free(gattc->subscribes);
    bt_list_free(gattc->services);
    if_gattc_on_connection_state_changed(&gattc->addr, PROFILE_STATE_DISCONNECTED);

    free(gattc);
}

static sal_gattc_conn_t* gattc_conn_new(bt_controller_id_t id, const bt_address_t* addr,
    ble_addr_type_t addr_type)
{
    sal_gattc_conn_t* gattc;

    if (!g_sal_gattc_info || !g_sal_gattc_info->list || !addr)
        return NULL;

    gattc = zalloc(sizeof(sal_gattc_conn_t));
    if (!gattc)
        return NULL;

    gattc->id = id;
    gattc->addr_type = addr_type;
    gattc->state = PROFILE_STATE_DISCONNECTED;
    memcpy(&gattc->addr, addr, sizeof(bt_address_t));
    gattc->subscribes = bt_list_new(subscribe_delete);
    if (!gattc->subscribes) {
        free(gattc);
        return NULL;
    }

    bt_list_add_tail(g_sal_gattc_info->list, gattc);

    return gattc;
}

static void att_mtu_updated(struct bt_conn* conn, uint16_t tx, uint16_t rx)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);
    uint16_t mtu = MAX(MIN(tx, rx), LE_ATT_MTU_MIN);
    uint16_t att_payload = mtu - ATT_HEADER_SIZE;

    BT_LOGV("%s, mtu = %d", __func__, mtu);

    if (!gattc) {
        BT_LOGW("gattc does not exist");
        return;
    }

    if_gattc_on_mtu_changed(&gattc->addr, att_payload, GATT_STATUS_SUCCESS);
}

static bool characteristic_not_discovered(void* data, void* context)
{
    sal_gattc_characteristic_t* chrc = data;

    return (chrc->descriptors == NULL);
}

static void finish_services(void* data, void* context)
{
    sal_gattc_conn_t* gattc = context;
    sal_gattc_service_t* service = data;

    BT_LOGV("%s", __func__);

    if (!service->include_services || !service->chrcs)
        return;

    if (bt_list_find(service->chrcs, characteristic_not_discovered, NULL))
        return;

    /** This service is discovered*/
    bt_list_remove(gattc->services, service);
}

static bool service_not_discovered(void* data, void* context)
{
    sal_gattc_characteristic_t* chrc;
    sal_gattc_service_t* service = data;

    if (!service->include_services || !service->chrcs)
        return true; /**< include services or characteristics to be discovered */

    chrc = bt_list_find(service->chrcs, characteristic_not_discovered, NULL);

    if (chrc != NULL) { /**< descriptors to be discovered */
        *(sal_gattc_characteristic_t**)context = chrc;
        return true;
    }

    return false;
}

static void discover_end(sal_gattc_conn_t* gattc, gatt_status_t status)
{
    BT_LOGV("%s, status = %d", __func__, status);

    bt_list_free(gattc->services);
    gattc->services = NULL;
    if_gattc_on_discover_completed(&gattc->addr, status);
}

static void discover_next(sal_gattc_conn_t* gattc)
{
    bt_status_t status;
    sal_gattc_service_t* service;
    sal_gattc_characteristic_t* chrc;

    BT_LOGV("%s", __func__);

    for (;;) {
        chrc = NULL;
        /** Step 1. clean the attributes that is discovered */
        bt_list_foreach(gattc->services, finish_services, gattc);

        /** Step 2. find a service that does not finish */
        service = bt_list_find(gattc->services, service_not_discovered, &chrc);
        if (!service)
            break;

        /** Step 3. check if include services to be discovered */
        if (!service->include_services) {
            status = discover_include_services(gattc, service);
            if (status == BT_STATUS_SUCCESS)
                return;
        }

        /** Step 4. check if characteristics to be discovered */
        if (!service->chrcs) {
            status = discover_characteristics(gattc, service);
            if (status == BT_STATUS_SUCCESS)
                return;
        }

        /** Step 5. check if descriptors to be discovered */
        if (chrc) {
            status = discover_descriptors(gattc, chrc);
            if (status == BT_STATUS_DONE)
                continue; /**< no descriptor, try next */

            if (status == BT_STATUS_SUCCESS)
                return;
        }

        BT_LOGE("failed to start a discovery");
        discover_end(gattc, GATT_STATUS_FAILURE);
        return;
    }

    BT_LOGV("no more attribute to discover");
    discover_end(gattc, GATT_STATUS_SUCCESS);
}

static void extract_descriptor(sal_gattc_characteristic_t* chrc, const struct bt_gatt_attr* attr)
{
    bt_uuid_t uuid;
    char uuid_str[40];
    bt_status_t status;
    sal_gattc_descriptor_t* descriptor;

    status = uuid_zephyr_to_sal(&uuid, attr->uuid);
    if (status != BT_STATUS_SUCCESS)
        return;

    bt_uuid_to_string(&uuid, uuid_str, 40);
    BT_LOGV("%s, [V:%04" PRIx16 "] -> [D:%04" PRIx16 "][%s]", __func__, chrc->element->handle,
        attr->handle, uuid_str);

    descriptor = zalloc(sizeof(sal_gattc_descriptor_t));
    if (!descriptor)
        return;

    descriptor->chrc = chrc;
    descriptor->element_size = 1;
    descriptor->element = zalloc(sizeof(gatt_element_t));
    if (!descriptor->element) {
        free(descriptor);
        return;
    }

    descriptor->element->type = GATT_DESCRIPTOR;
    descriptor->element->handle = attr->handle;
    descriptor->element->permissions = attr->perm;
    memcpy(&descriptor->element->uuid, &uuid, sizeof(bt_uuid_t));

    bt_list_add_tail(chrc->descriptors, descriptor);
}

static uint8_t on_descriptor_discovered(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    sal_gattc_service_t* service;
    sal_gattc_characteristic_t* chrc;
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s", __func__);

    if (!gattc) {
        BT_LOGW("gattc does not exist");
        free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
        return BT_GATT_ITER_STOP;
    }

    if (attr) {
        BT_LOGV("descriptor found");
        service = find_service_by_handle(gattc, params->start_handle);
        if (!service) {
            BT_LOGE("service does not exist");
            return BT_GATT_ITER_CONTINUE;
        }

        chrc = find_characteristic_by_handle(service, params->start_handle);
        if (!chrc) {
            BT_LOGE("characteristic does not exist");
            return BT_GATT_ITER_CONTINUE;
        }

        extract_descriptor(chrc, attr);
        return BT_GATT_ITER_CONTINUE;
    }

    BT_LOGV("descriptors discovered, go to the next stage");
    free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
    discover_next(gattc);
    return BT_GATT_ITER_STOP;
}

static bt_status_t discover_descriptors(const sal_gattc_conn_t* gattc,
    sal_gattc_characteristic_t* chrc)
{
    bt_status_t status;
    sal_gattc_req_t* req;
    sal_gattc_discover_args_t* arg;

    BT_LOGV("%s", __func__);

    req = sal_gattc_req(gattc->id, &gattc->addr, gattc->addr_type, zblue_discover,
        sizeof(sal_gattc_discover_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    chrc->descriptors = bt_list_new(descriptor_delete);
    if (!chrc->descriptors) {
        BT_LOGE("failed to start discovery");
        free(req);
        return BT_STATUS_NOMEM;
    }

    if (chrc->element->handle >= chrc->end_handle) {
        /** nothing to discover, keep chrc->descriptors so we won't enter again */
        free(req);
        return BT_STATUS_DONE;
    }

    arg = (sal_gattc_discover_args_t*)req->data;
    arg->params.uuid = NULL;
    arg->params.start_handle = chrc->element->handle + 1;
    arg->params.end_handle = chrc->end_handle;
    arg->params.type = BT_GATT_DISCOVER_DESCRIPTOR;
    arg->params.func = on_descriptor_discovered;

    status = sal_send_req(req);
    if (status != BT_STATUS_SUCCESS) {
        bt_list_free(chrc->descriptors);
        chrc->descriptors = NULL;
    }

    return status;
}

static void extract_characteristic(sal_gattc_service_t* service, const struct bt_gatt_attr* attr)
{
    bt_uuid_t uuid;
    char uuid_str[40];
    bt_status_t status;
    bt_list_node_t* node;
    sal_gattc_characteristic_t* chrc;
    sal_gattc_characteristic_t* chrc_prev;

    struct bt_gatt_chrc* val = attr->user_data;

    status = uuid_zephyr_to_sal(&uuid, val->uuid);
    if (status != BT_STATUS_SUCCESS)
        return;

    bt_uuid_to_string(&uuid, uuid_str, 40);
    BT_LOGV("%s, [S:%04" PRIx16 "] -> [D:%04" PRIx16 "][V:%04" PRIx16 "][%s]", __func__,
        service->element->handle, attr->handle, val->value_handle, uuid_str);

    chrc = zalloc(sizeof(sal_gattc_characteristic_t));
    if (!chrc)
        return;

    chrc->service = service;
    chrc->element_size = 1;
    chrc->element = zalloc(sizeof(gatt_element_t));
    if (!chrc->element) {
        free(chrc);
        return;
    }

    node = bt_list_tail(service->chrcs); /**< the previous characteristic */
    chrc->element->type = GATT_CHARACTERISTIC;
    chrc->element->handle = val->value_handle;
    chrc->element->properties = val->properties;
    memcpy(&chrc->element->uuid, &uuid, sizeof(bt_uuid_t));

    /** the end handle is set to the last possible handle, until the next characteristic is found */
    chrc->end_handle = service->end_handle;

    bt_list_add_tail(service->chrcs, chrc);

    if (node && attr->handle && (chrc_prev = bt_list_node(node))) {
        /** now we know the end handle of the previous characteristic */
        chrc_prev->end_handle = attr->handle > chrc_prev->element->handle /**< check overflow */
            ? attr->handle - 1
            : chrc_prev->element->handle;
    }
}

static uint8_t on_characteristic_discovered(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    sal_gattc_service_t* service;
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s", __func__);

    if (!gattc) {
        BT_LOGW("gattc does not exist");
        free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
        return BT_GATT_ITER_STOP;
    }

    if (attr) {
        BT_LOGV("characteristic found");
        service = find_service_by_handle(gattc, params->start_handle);
        if (!service) {
            BT_LOGE("service does not exist");
            return BT_GATT_ITER_CONTINUE;
        }

        extract_characteristic(service, attr);
        return BT_GATT_ITER_CONTINUE;
    }

    BT_LOGV("characteristics discovered, go to the next stage");
    free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
    discover_next(gattc);
    return BT_GATT_ITER_STOP;
}

static bt_status_t discover_characteristics(const sal_gattc_conn_t* gattc,
    sal_gattc_service_t* service)
{
    bt_status_t status;
    sal_gattc_req_t* req;
    sal_gattc_discover_args_t* arg;

    BT_LOGV("%s", __func__);

    req = sal_gattc_req(gattc->id, &gattc->addr, gattc->addr_type, zblue_discover,
        sizeof(sal_gattc_discover_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    service->chrcs = bt_list_new(characteristic_delete);
    if (!service->chrcs) {
        BT_LOGE("failed to start discovery");
        free(req);
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_discover_args_t*)req->data;
    arg->params.uuid = NULL;
    arg->params.start_handle = service->element->handle;
    arg->params.end_handle = service->end_handle;
    arg->params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
    arg->params.func = on_characteristic_discovered;

    status = sal_send_req(req);
    if (status != BT_STATUS_SUCCESS) {
        bt_list_free(service->chrcs);
        service->chrcs = NULL;
    }

    return status;
}

static void extract_include_service(sal_gattc_service_t* service, const struct bt_gatt_attr* attr)
{
    bt_uuid_t uuid;
    char uuid_str[40];
    bt_status_t status;
    sal_gattc_service_t* include_service;
    struct bt_gatt_include* val = attr->user_data;

    status = uuid_zephyr_to_sal(&uuid, val->uuid);
    if (status != BT_STATUS_SUCCESS)
        return;

    bt_uuid_to_string(&uuid, uuid_str, 40);
    BT_LOGV("%s, [S:%04" PRIx16 "] -> [I:%04" PRIx16 "][%s]", __func__, service->element->handle,
        attr->handle, uuid_str);

    include_service = zalloc(sizeof(sal_gattc_service_t));
    if (!include_service)
        return;

    include_service->gattc = service->gattc;
    include_service->service = service;
    include_service->element_size = 1;
    include_service->element = zalloc(sizeof(gatt_element_t));
    if (!include_service->element) {
        free(include_service);
        return;
    }

    include_service->element->type = GATT_INCLUDED_SERVICE;
    include_service->element->handle = attr->handle;
    include_service->end_handle = val->end_handle;
    memcpy(&include_service->element->uuid, &uuid, sizeof(bt_uuid_t));
    bt_list_add_tail(service->include_services, include_service);
}

static uint8_t on_include_service_discovered(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    struct bt_gatt_discover_params* params)
{
    sal_gattc_service_t* service;
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s", __func__);

    if (!gattc) {
        BT_LOGW("gattc does not exist");
        free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
        return BT_GATT_ITER_STOP;
    }

    if (attr) {
        BT_LOGV("include service found");

        service = find_service_by_handle(gattc, params->start_handle);
        if (!service) {
            BT_LOGE("service does not exist");
            return BT_GATT_ITER_CONTINUE;
        }

        extract_include_service(service, attr);
        return BT_GATT_ITER_CONTINUE;
    }

    BT_LOGV("include services discovered, go to the next stage");
    free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
    discover_next(gattc);
    return BT_GATT_ITER_STOP;
}

static bt_status_t discover_include_services(const sal_gattc_conn_t* gattc,
    sal_gattc_service_t* service)
{
    bt_status_t status;
    sal_gattc_req_t* req;
    sal_gattc_discover_args_t* arg;

    BT_LOGV("%s", __func__);

    req = sal_gattc_req(gattc->id, &gattc->addr, gattc->addr_type, zblue_discover,
        sizeof(sal_gattc_discover_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    service->include_services = bt_list_new(service_delete);
    if (!service->include_services) {
        BT_LOGE("failed to start discovery");
        free(req);
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_discover_args_t*)req->data;
    arg->params.uuid = NULL;
    arg->params.start_handle = service->element->handle;
    arg->params.end_handle = service->end_handle;
    arg->params.type = BT_GATT_DISCOVER_INCLUDE;
    arg->params.func = on_include_service_discovered;

    status = sal_send_req(req);
    if (status != BT_STATUS_SUCCESS) {
        bt_list_free(service->include_services);
        service->include_services = NULL;
    }

    return status;
}

static void extract_service(sal_gattc_conn_t* gattc, const struct bt_gatt_attr* attr)
{
    bt_uuid_t uuid;
    char uuid_str[40];
    bt_status_t status;
    sal_gattc_service_t* service;
    struct bt_gatt_service_val* val = attr->user_data;

    status = uuid_zephyr_to_sal(&uuid, val->uuid);
    if (status != BT_STATUS_SUCCESS)
        return;

    bt_uuid_to_string(&uuid, uuid_str, 40);
    BT_LOGV("%s, [%04" PRIx16 "][%s]", __func__, attr->handle, uuid_str);

    service = zalloc(sizeof(sal_gattc_service_t));
    if (!service)
        return;

    service->gattc = gattc;
    service->element_size = 1;
    service->element = zalloc(sizeof(gatt_element_t));
    if (!service->element) {
        free(service);
        return;
    }

    service->element->type = GATT_PRIMARY_SERVICE;
    service->element->handle = attr->handle;
    service->end_handle = val->end_handle;
    memcpy(&service->element->uuid, &uuid, sizeof(bt_uuid_t));
    bt_list_add_tail(gattc->services, service);
}

static uint8_t on_service_discovered(struct bt_conn* conn,
    const struct bt_gatt_attr* attr, struct bt_gatt_discover_params* params)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s", __func__);

    if (!gattc) {
        BT_LOGW("gattc does not exist");
        free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
        return BT_GATT_ITER_STOP;
    }

    if (attr) {
        BT_LOGV("service found");
        extract_service(gattc, attr);
        return BT_GATT_ITER_CONTINUE;
    }

    BT_LOGV("all primary services discovered, go to the next stage");
    free(CONTAINER_OF(params, sal_gattc_discover_args_t, params));
    discover_next(gattc);
    return BT_GATT_ITER_STOP;
}

static void zblue_discover(void* data)
{
    int err;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;
    sal_gattc_discover_args_t* arg = (sal_gattc_discover_args_t*)req->data;
    sal_gattc_discover_args_t* context; /** valid until @ref arg->params.func */

    BT_LOGV("%s, from 0x%04" PRIx16 " to 0x%04" PRIx16, __func__, arg->params.start_handle,
        arg->params.end_handle);

    gattc = find_gattc_conn(req->id, &req->addr, req->addr_type);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        if_gattc_on_discover_completed(&req->addr, GATT_STATUS_FAILURE);
        return;
    }

    if (!gattc->conn) {
        BT_LOGW("acl does not exist");
        discover_end(gattc, GATT_STATUS_FAILURE);
        return;
    }

    context = zalloc(sizeof(sal_gattc_discover_args_t));
    if (!context) {
        BT_LOGE("context malloc failed");
        discover_end(gattc, GATT_STATUS_FAILURE);
        return;
    }

    memcpy(context, arg, sizeof(sal_gattc_discover_args_t));
    if (context->params.uuid != NULL) {
        /** redirect to @ref context->uuid that is valid until @ref context->params.func */
        context->params.uuid = &context->uuid;
    }

    err = bt_gatt_discover(gattc->conn, &context->params);
    if (err != 0) {
        BT_LOGE("gatt discovery fail, err = %d", err);
        free(context);
        discover_end(gattc, GATT_STATUS_FAILURE);
        return;
    }

    return;
}

bt_status_t bt_sal_gatt_client_discover_all_services(bt_controller_id_t id, bt_address_t* addr)
{
    bt_status_t status;
    sal_gattc_req_t* req;
    sal_gattc_conn_t* gattc;
    sal_gattc_discover_args_t* arg;

    BT_LOGD("%s", __func__);

    gattc = find_gattc_conn(id, addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    if (gattc->services) {
        BT_LOGW("already discovering");
        return BT_STATUS_BUSY;
    }

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_discover,
        sizeof(sal_gattc_discover_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    gattc->services = bt_list_new(service_delete);
    if (!gattc->services) {
        BT_LOGE("failed to start discovery");
        free(req);
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_discover_args_t*)req->data;
    arg->params.uuid = NULL;
    arg->params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    arg->params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    arg->params.type = BT_GATT_DISCOVER_PRIMARY;
    arg->params.func = on_service_discovered;

    status = sal_send_req(req);
    if (status != BT_STATUS_SUCCESS) {
        bt_list_free(gattc->services);
        gattc->services = NULL;
    }

    return status;
}

bt_status_t bt_sal_gatt_client_discover_service_by_uuid(bt_controller_id_t id, bt_address_t* addr,
    bt_uuid_t* uuid)
{
    bt_status_t status;
    sal_gattc_req_t* req;
    sal_gattc_conn_t* gattc;
    sal_gattc_discover_args_t* arg;

    BT_LOGD("%s", __func__);

    gattc = find_gattc_conn(id, addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    if (gattc->services) {
        BT_LOGW("already discovering");
        return BT_STATUS_BUSY;
    }

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_discover,
        sizeof(sal_gattc_discover_args_t));
    if (!req) {
        BT_LOGE("req malloc failed");
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_discover_args_t*)req->data;

    if (uuid_sal_to_zephyr(&arg->uuid, uuid) != BT_STATUS_SUCCESS) {
        BT_LOGE("failed to convert uuid");
        free(req);
        return BT_STATUS_PARM_INVALID;
    }

    gattc->services = bt_list_new(service_delete);
    if (!gattc->services) {
        BT_LOGE("failed to start discovery");
        free(req);
        return BT_STATUS_NOMEM;
    }

    arg->params.uuid = &arg->uuid;
    arg->params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    arg->params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    arg->params.type = BT_GATT_DISCOVER_PRIMARY;
    arg->params.func = on_service_discovered;

    status = sal_send_req(req);
    if (status != BT_STATUS_SUCCESS) {
        bt_list_free(gattc->services);
        gattc->services = NULL;
    }

    return status;
}

static uint8_t on_read_complete(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_read_params* params, const void* data, uint16_t length)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s, len: %d", __func__, length);

    if (!gattc) {
        BT_LOGE("gattc does not exist");
        return BT_GATT_ITER_STOP;
    }

    if (err)
        BT_LOGE("gatt read failed, err = %d", err);

    if_gattc_on_element_read(&gattc->addr, params->single.handle,
        data ? (uint8_t*)data : (uint8_t*)"", data ? length : 0,
        err ? GATT_STATUS_FAILURE : GATT_STATUS_SUCCESS);
    memset(&gattc->read_params, 0x00, sizeof(struct bt_gatt_read_params));

    return BT_GATT_ITER_STOP;
}

static void zblue_read_element(void* data)
{
    int err;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;
    sal_gattc_read_args_t* arg = (sal_gattc_read_args_t*)req->data;

    BT_LOGV("%s", __func__);

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        goto fail;
    }

    if (gattc->read_params.func) {
        BT_LOGE("previous read not finished");
        goto fail;
    }

    gattc->read_params.handle_count = 1;
    gattc->read_params.single.handle = arg->element_id;
    gattc->read_params.single.offset = 0;
    gattc->read_params.func = on_read_complete;

    err = bt_gatt_read(gattc->conn, &gattc->read_params);
    if (err) {
        BT_LOGE("gatt read fail, err = %d", err);
        memset(&gattc->read_params, 0x00, sizeof(struct bt_gatt_read_params));
        goto fail;
    }

    return;

fail:
    if_gattc_on_element_read(gattc ? &gattc->addr : &req->addr,
        arg->element_id, (uint8_t*)"", 0, GATT_STATUS_FAILURE);
}

bt_status_t bt_sal_gatt_client_read_element(bt_controller_id_t id, bt_address_t* addr,
    uint16_t element_id)
{
    sal_gattc_req_t* req;
    sal_gattc_read_args_t* arg;

    BT_LOGV("%s", __func__);

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_read_element,
        sizeof(sal_gattc_read_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_read_args_t*)req->data;
    arg->element_id = element_id;

    return sal_send_req(req);
}

static void on_write_complete(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_write_params* params)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);
    uint16_t handle;

    BT_LOGV("%s", __func__);

    handle = params->handle;
    free(params);

    if (!gattc) {
        BT_LOGE("gattc does not exist");
        return;
    }

    if (err)
        BT_LOGE("gatt write failed, err = %d", err);

    if_gattc_on_element_written(&gattc->addr, handle,
        err ? GATT_STATUS_FAILURE : GATT_STATUS_SUCCESS);
}

static bt_status_t write_rsp(sal_gattc_conn_t* gattc, uint16_t handle, const uint8_t* value,
    uint16_t length)
{
    int err;
    /* Allocate params + value together so that params->data remains valid across
     * the entire write sequence (including Prepare Write / Execute Write for
     * long writes where Zephyr keeps referencing params->data in callbacks). */
    struct bt_gatt_write_params* params = zalloc(sizeof(struct bt_gatt_write_params) + length);

    if (!params)
        return BT_STATUS_NOMEM;

    uint8_t* buf = (uint8_t*)params + sizeof(struct bt_gatt_write_params);
    memcpy(buf, value, length);

    params->handle = handle;
    params->data = buf;
    params->length = length;
    params->offset = 0;
    params->func = on_write_complete;

    err = bt_gatt_write(gattc->conn, params);
    if (err) {
        BT_LOGE("%s, gatt write failed, err = %d", __func__, err);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void on_write_no_rsp_complete(struct bt_conn* conn, void* user_data)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);
    uint16_t* p_handle = user_data;
    uint16_t handle;

    BT_LOGV("%s", __func__);

    handle = *p_handle;
    free(p_handle);

    if (!gattc) {
        BT_LOGE("gattc does not exist");
        return;
    }

    if_gattc_on_element_written(&gattc->addr, handle, GATT_STATUS_SUCCESS);
}

static bt_status_t write_no_rsp(sal_gattc_conn_t* gattc, uint16_t handle, const uint8_t* value,
    uint16_t length, bool with_sign)
{
    int err;
    uint16_t* p_handle = zalloc(sizeof(uint16_t));
    if (!p_handle)
        return BT_STATUS_NOMEM;

    *p_handle = handle;
    err = bt_gatt_write_without_response_cb(gattc->conn, handle, value, length,
        with_sign, on_write_no_rsp_complete, p_handle);
    if (err) {
        BT_LOGE("%s, gatt write without response failed, err = %d", __func__, err);
        free(p_handle);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void zblue_write_element(void* data)
{
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;
    sal_gattc_write_args_t* arg = (sal_gattc_write_args_t*)req->data;
    uint8_t* value = arg->value;
    bt_status_t status;

    BT_LOGV("%s", __func__);

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        goto fail;
    }

    switch (arg->write_type) {
    case GATT_WRITE_TYPE_RSP:
        status = write_rsp(gattc, arg->element_id, value, arg->length);
        break;
    case GATT_WRITE_TYPE_NO_RSP:
        status = write_no_rsp(gattc, arg->element_id, value, arg->length, false);
        break;
#ifdef CONFIG_BT_SIGNING
    case GATT_WRITE_TYPE_SIGNED:
        status = write_no_rsp(gattc, arg->element_id, value, arg->length, true);
        break;
#endif
    default:
        BT_LOGE("unsupported write type:%d", arg->write_type);
        status = BT_STATUS_NOT_SUPPORTED;
        break;
    }

    if (status == BT_STATUS_SUCCESS)
        return;

fail:
    if_gattc_on_element_written(gattc ? &gattc->addr : &req->addr,
        arg->element_id, GATT_STATUS_FAILURE);
}

bt_status_t bt_sal_gatt_client_write_element(bt_controller_id_t id, bt_address_t* addr,
    uint16_t element_id, uint8_t* value, uint16_t length, gatt_write_type_t write_type)
{
    sal_gattc_req_t* req;
    sal_gattc_write_args_t* arg;

    BT_LOGV("%s", __func__);

    if (!value) {
        BT_LOGE("value is NULL");
        return BT_STATUS_PARM_INVALID;
    }

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_write_element,
        sizeof(sal_gattc_write_args_t) + length);
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_write_args_t*)req->data;
    arg->element_id = element_id;
    arg->length = length;
    arg->write_type = write_type;
    memcpy(arg->value, value, length);

    return sal_send_req(req);
}

static uint8_t on_notify(struct bt_conn* conn, struct bt_gatt_subscribe_params* params,
    const void* data, uint16_t length)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s", __func__);

    if (!gattc) {
        BT_LOGE("gattc does not exist");
        return BT_GATT_ITER_STOP;
    }

    if (data == NULL) {
        /* Do NOT null the cached pointer here.
         *
         * Zephyr delivers notify(NULL) from multiple paths:
         *  (a) remove_subscriptions → gatt_sub_remove: disconnect tears down
         *      VOLATILE subscriptions.  No subscribe callback follows.
         *  (b) bt_gatt_unsubscribe (has_subscription=true): another subscription
         *      on the same value_handle still exists, CCC write skipped.
         *      No subscribe callback follows.
         *  (c) gatt_write_ccc_rsp (!params->value): CCC write-back succeeded
         *      for unsubscribe.  subscribe callback follows immediately.
         *  (d) gatt_write_ccc_rsp (err): CCC write failed, subscription removed
         *      via gatt_sub_remove.  subscribe callback follows immediately.
         *
         * For (c)/(d) the subscribe callback (on_unsubscribe/subscribe_complete)
         * will null the pointer and free params.  Nulling here would be harmless
         * but unnecessary.
         *
         * For (a)/(b) there is NO subscribe callback, so nobody else will free
         * params.  Keeping the pointer allows subscribe_delete() to free it
         * during gattc teardown.  Nulling here would leak. */
        BT_LOGD("unsubscribed 0x%04" PRIx16, params->value_handle);
        return BT_GATT_ITER_STOP;
    }

    if_gattc_on_element_changed(&gattc->addr, params->value_handle, (uint8_t*)data, length);
    return BT_GATT_ITER_CONTINUE;
}

static void on_characteristic_unsubscribed(sal_gattc_conn_t* gattc,
    struct bt_gatt_subscribe_params* params)
{
    sal_gattc_subscribe_t* subscribe = find_subscribe_by_value_handle(gattc, params->value_handle);

    BT_LOGV("%s", __func__);

    if (!subscribe)
        return;

    if (subscribe->notify_params == params)
        subscribe->notify_params = NULL;
    else if (subscribe->indicate_params == params)
        subscribe->indicate_params = NULL;

    /* Do not free params here. In the gatt_write_ccc_rsp path,
     * notify callback is invoked before subscribe callback.
     * Freeing here causes use-after-free and double-free when
     * subscribe callback (on_subscribe/unsubscribe_complete)
     * accesses params afterwards. Let callers free instead.
     */
}

static void on_subscribe_complete(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_subscribe_params* params)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s", __func__);

    if (!gattc) {
        BT_LOGE("gattc does not exist");
        free(params);
        return;
    }

    if_gattc_on_element_subscribed(&gattc->addr, params->value_handle,
        err ? GATT_STATUS_FAILURE : GATT_STATUS_SUCCESS, true);

    if (err) {
        on_characteristic_unsubscribed(gattc, params);
        free(params);
    }
}

static void on_unsubscribe_complete(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_subscribe_params* params)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);

    BT_LOGV("%s", __func__);

    if (!gattc) {
        BT_LOGE("gattc does not exist");
        free(params);
        return;
    }

    if_gattc_on_element_subscribed(&gattc->addr, params->value_handle,
        err ? GATT_STATUS_FAILURE : GATT_STATUS_SUCCESS, false);

    on_characteristic_unsubscribed(gattc, params);
    free(params);
}

/** Subscribe or unsubscribe a characteristic's CCCD. */
static bt_status_t subscribe_characteristic(sal_gattc_conn_t* gattc,
    sal_gattc_subscribe_t* subscribe, uint16_t type, bool enable)
{
    int err;
    struct bt_gatt_subscribe_params* param;
    struct bt_gatt_subscribe_params** p_param;

    BT_LOGV("%s", __func__);

    if (type == BT_GATT_CCC_NOTIFY) {
        p_param = &subscribe->notify_params;
    } else {
        p_param = &subscribe->indicate_params;
    }

    param = *p_param;
    if (enable) { /**< subscribe */
        if (param) {
            BT_LOGW("already subscribed");
            if_gattc_on_element_subscribed(&gattc->addr, subscribe->value_handle,
                GATT_STATUS_SUCCESS, true);
            return BT_STATUS_DONE;
        }

        param = zalloc(sizeof(struct bt_gatt_subscribe_params));
        if (!param)
            return BT_STATUS_NOMEM;

        *p_param = param;
    } else { /**< unsubscribe */
        if (!param) {
            BT_LOGW("not subscribed");
            if_gattc_on_element_subscribed(&gattc->addr, subscribe->value_handle,
                GATT_STATUS_SUCCESS, false);
            return BT_STATUS_DONE;
        }
    }

    param->value_handle = subscribe->value_handle;
    param->ccc_handle = subscribe->ccc_handle;
    param->value = type;
    param->notify = on_notify;
    /* Mark as volatile so Zephyr removes the subscription on disconnect instead of
     * attempting automatic resubscription — our params are dynamically allocated and
     * will be freed when the gattc connection is torn down. */
    atomic_set_bit(param->flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);
    if (enable) {
        param->subscribe = on_subscribe_complete;
        err = bt_gatt_subscribe(gattc->conn, param);
    } else {
        param->subscribe = on_unsubscribe_complete;
        err = bt_gatt_unsubscribe(gattc->conn, param);
    }

    if (err) {
        BT_LOGE("%s failed, err = %d", enable ? "subscribe" : "unsubscribe", err);
        if (enable) {
            free(param);
            *p_param = NULL;
        }
        return err == -EAGAIN ? BT_STATUS_BUSY : BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void zblue_register_notifications(void* data)
{
    bt_status_t status;
    sal_gattc_conn_t* gattc;
    sal_gattc_subscribe_t* subscribe;
    sal_gattc_req_t* req = data;
    sal_gattc_subscribe_args_t* arg = (sal_gattc_subscribe_args_t*)req->data;

    BT_LOGV("%s", __func__);

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        if_gattc_on_element_subscribed(&req->addr, arg->element_id,
            GATT_STATUS_FAILURE, arg->enable);
        return;
    }

    subscribe = find_subscribe_by_value_handle(gattc, arg->element_id);
    if (!subscribe) {
        BT_LOGW("handle pair not found");
        if_gattc_on_element_subscribed(&gattc->addr, arg->element_id,
            GATT_STATUS_FAILURE, arg->enable);
        return;
    }

    if (arg->properties & GATT_PROP_NOTIFY) {
        status = subscribe_characteristic(gattc, subscribe, BT_GATT_CCC_NOTIFY, arg->enable);
        if (status != BT_STATUS_SUCCESS && status != BT_STATUS_DONE) {
            if_gattc_on_element_subscribed(&gattc->addr, arg->element_id,
                GATT_STATUS_FAILURE, arg->enable);
            return;
        }
    }

    if (arg->properties & GATT_PROP_INDICATE) {
        status = subscribe_characteristic(gattc, subscribe, BT_GATT_CCC_INDICATE, arg->enable);
        if (status != BT_STATUS_SUCCESS && status != BT_STATUS_DONE) {
            if_gattc_on_element_subscribed(&gattc->addr, arg->element_id,
                GATT_STATUS_FAILURE, arg->enable);
        }
    }
}

bt_status_t bt_sal_gatt_client_register_notifications(bt_controller_id_t id, bt_address_t* addr,
    uint16_t element_id, uint16_t properties, bool enable)
{
    sal_gattc_req_t* req;
    sal_gattc_subscribe_args_t* arg;

    BT_LOGD("%s", __func__);

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_register_notifications,
        sizeof(sal_gattc_subscribe_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_subscribe_args_t*)req->data;
    arg->element_id = element_id;
    arg->properties = properties;
    arg->enable = enable;

    return sal_send_req(req);
}

static void on_mtu_exchanged(struct bt_conn* conn, uint8_t err,
    struct bt_gatt_exchange_params* params)
{
    sal_gattc_conn_t* gattc = find_gattc_conn_by_conn(conn);
    uint16_t mtu;
    uint16_t att_payload;

    BT_LOGD("%s", __func__);

    if (!gattc) {
        BT_LOGE("gattc does not exist");
        return;
    }

    mtu = bt_gatt_get_mtu(conn);
    att_payload = mtu - ATT_HEADER_SIZE;

    if_gattc_on_mtu_changed(&gattc->addr, att_payload,
        err ? GATT_STATUS_FAILURE : GATT_STATUS_SUCCESS);
}

static struct bt_gatt_exchange_params mtu_exchange_params = {
    .func = on_mtu_exchanged,
};

static struct bt_gatt_cb gattc_cbs = {
    .att_mtu_updated = att_mtu_updated,
};

static void zblue_send_mtu_req(void* data)
{
    int err;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;

    BT_LOGV("%s", __func__);

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc || !gattc->conn) {
        BT_LOGW("gattc or acl does not exist");
        if_gattc_on_mtu_changed(&req->addr, 0, GATT_STATUS_FAILURE);
        return;
    }

    err = bt_gatt_exchange_mtu(gattc->conn, &mtu_exchange_params);
    if (err) {
        BT_LOGE("failed to exchange mtu, err = %d", err);
        if (err == -EALREADY) {
            uint16_t mtu = bt_gatt_get_mtu(gattc->conn);
            uint16_t att_payload = mtu - ATT_HEADER_SIZE;

            if_gattc_on_mtu_changed(&gattc->addr, att_payload, GATT_STATUS_SUCCESS);
        } else {
            if_gattc_on_mtu_changed(&gattc->addr, 0, GATT_STATUS_FAILURE);
        }
    }
}

bt_status_t bt_sal_gatt_client_send_mtu_req(bt_controller_id_t id, bt_address_t* addr, uint32_t mtu)
{
    sal_gattc_req_t* req;

    BT_LOGD("%s", __func__);

    /* @p mtu is ignored.  Zephyr's bt_gatt_exchange_mtu does not accept a
     * requested MTU value — it always uses the compile-time constant
     * BT_LOCAL_ATT_MTU_UATT = MIN(CONFIG_BT_BUF_ACL_RX_SIZE - L2CAP_HDR,
     *                             CONFIG_BT_L2CAP_TX_MTU).
     * The parameter exists in the SAL interface for other stack backends. */
    (void)mtu;

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_send_mtu_req, 0);
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

bt_status_t bt_sal_gatt_client_enable(void)
{
    BT_LOGD("%s", __func__);

    if (g_sal_gattc_info) {
        BT_LOGW("already enabled");
        return BT_STATUS_SUCCESS;
    }

    g_sal_gattc_info = zalloc(sizeof(sal_gattc_info_t));
    if (!g_sal_gattc_info) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    g_sal_gattc_info->list = bt_list_new(gattc_conn_delete);
    if (!g_sal_gattc_info->list) {
        BT_LOGE("malloc failed");
        free(g_sal_gattc_info);
        g_sal_gattc_info = NULL;
        return BT_STATUS_NOMEM;
    }

    bt_gatt_cb_register(&gattc_cbs);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_disable(void)
{
    BT_LOGD("%s", __func__);

    if (!g_sal_gattc_info) {
        BT_LOGW("already disabled");
        return BT_STATUS_SUCCESS;
    }

    g_sal_gattc_info->terminating = true;
    if (bt_list_length(g_sal_gattc_info->list)) {
        BT_LOGW("waiting for acl disconnected");
        /** TODO: return an error code, and hold the profile shutdown callback until all the gatt
         *  connections are released.
         *
         *  TODO: in case of non-safe shutdown, the acl disconnection callback might missing,
         *  therefore we need an extra timeout to ensure the sal can finally be disabled.
         */
        return BT_STATUS_SUCCESS;
    }

    BT_LOGD("now safe to disable");
    bt_gatt_cb_unregister(&gattc_cbs);
    bt_list_free(g_sal_gattc_info->list);
    free(g_sal_gattc_info);
    g_sal_gattc_info = NULL;

    return BT_STATUS_SUCCESS;
}

static void zblue_connect(void* data)
{
    int err;
    bt_addr_le_t zblue_addr;
    bt_status_t status;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;
    sal_gattc_conn_args_t* arg = (sal_gattc_conn_args_t*)req->data;

    BT_LOGV("%s", __func__);

    /** Step 1. sanity checks */
    if (!g_sal_gattc_info || !g_sal_gattc_info->list) {
        BT_LOGE("not initialized");
        if_gattc_on_connection_state_changed(&req->addr, PROFILE_STATE_DISCONNECTED);
        return;
    }

    /** Step 2. check if gattc already exist */
    gattc = find_gattc_conn(req->id, &req->addr, req->addr_type);
    if (gattc) {
        BT_LOGW("gattc already exist");
        if_gattc_on_connection_state_changed(&gattc->addr, gattc->state);
        return;
    }

    /** Step 3. create new gattc conn instance */
    gattc = gattc_conn_new(req->id, &req->addr, req->addr_type);
    if (!gattc) {
        BT_LOGE("failed to create gattc");
        if_gattc_on_connection_state_changed(&req->addr, PROFILE_STATE_DISCONNECTED);
        return;
    }

    if (addr_sal_to_zephyr(&zblue_addr, &gattc->addr, gattc->addr_type) != BT_STATUS_SUCCESS) {
        BT_LOGE("invalid address");
        bt_list_remove(g_sal_gattc_info->list, gattc);
        return;
    }

    /** Step 4. check if acl already connected */
    status = bt_conn_set_role(BT_TRANSPORT_BLE, &req->addr, GATT_ROLE_CLIENT);
    if (status != BT_STATUS_SUCCESS && status != BT_STATUS_DONE) {
        BT_LOGE("bt_conn_set_role failed, status = %d", status);
        bt_list_remove(g_sal_gattc_info->list, gattc);
        return;
    }

    /** Step 5. regenerate gattc conn if acl already exists */
    if (status == BT_STATUS_DONE) {
        BT_LOGW("acl already exists");
        gattc->conn = get_le_conn_from_addr(&req->addr);
        if (!gattc->conn) {
            BT_LOGE("failed to get acl conn");
            bt_list_remove(g_sal_gattc_info->list, gattc);
            bt_conn_remove(&req->addr, BT_TRANSPORT_BLE);
            return;
        }

        /** PROFILE_STATE_CONNECTED won't be set via at the second bt_conn_set_role, so we manually
         *  add the state */
        if (gattc->state != PROFILE_STATE_CONNECTED) {
            BT_LOGW("gattc already connected");
            bt_sal_gatt_client_connection_state_changed_callback(req->id, &req->addr,
                PROFILE_STATE_CONNECTED);
        }

        return;
    }

    /** Step 6. send connection request to zephyr */
    err = bt_conn_le_create(&zblue_addr, &arg->create_param, &arg->conn_param, &gattc->conn);
    if (err || !gattc->conn) {
        BT_LOGE("%s, failed to create connection, err = %d, conn = %p", __func__, err, gattc->conn);
        bt_list_remove(g_sal_gattc_info->list, gattc);
        bt_conn_remove(&req->addr, BT_TRANSPORT_BLE);
        return;
    }

    return;
}

bt_status_t bt_sal_gatt_client_connect(bt_controller_id_t id, bt_address_t* addr,
    ble_addr_type_t addr_type)
{
    sal_gattc_req_t* req;
    sal_gattc_conn_args_t* arg;

    BT_LOGD("%s", __func__);

    req = sal_gattc_req(id, addr, addr_type, zblue_connect, sizeof(sal_gattc_conn_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    struct bt_conn_le_create_param* create_param = BT_CONN_LE_CREATE_CONN_AUTO;
    struct bt_le_conn_param* conn_param = BT_LE_CONN_PARAM_DEFAULT;

    arg = (sal_gattc_conn_args_t*)req->data;
    memcpy(&arg->create_param, create_param, sizeof(struct bt_conn_le_create_param));
    memcpy(&arg->conn_param, conn_param, sizeof(struct bt_le_conn_param));

    return sal_send_req(req);
}

static void zblue_disconnect(void* data)
{
    int err;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;

    BT_LOGV("%s", __func__);

    if (!g_sal_gattc_info || !g_sal_gattc_info->list) {
        BT_LOGE("not initialized");
        if_gattc_on_connection_state_changed(&req->addr, PROFILE_STATE_DISCONNECTED);
        return;
    }

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        if_gattc_on_connection_state_changed(&req->addr, PROFILE_STATE_DISCONNECTED);
        return;
    }

    if (!gattc->conn) {
        BT_LOGE("acl does not exist");
        bt_list_remove(g_sal_gattc_info->list, gattc);
        bt_conn_remove(&req->addr, BT_TRANSPORT_BLE);
        return;
    }

    err = bt_conn_disconnect(gattc->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err != 0) {
        if (err == -ENOTCONN) {
            BT_LOGW("acl already disconnected");
        } else {
            BT_LOGE("failed to disconnect acl");
        }
        bt_list_remove(g_sal_gattc_info->list, gattc);
        bt_conn_remove(&req->addr, BT_TRANSPORT_BLE);
        return;
    }
}

bt_status_t bt_sal_gatt_client_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
    sal_gattc_req_t* req;

    BT_LOGD("%s", __func__);

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_disconnect, 0);
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

static void zblue_update_connection_parameter(void* data)
{
    int err;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;
    sal_gattc_conn_param_args_t* arg = (sal_gattc_conn_param_args_t*)req->data;

    BT_LOGV("%s", __func__);

    if (!g_sal_gattc_info || !g_sal_gattc_info->list) {
        BT_LOGE("not initialized");
        return;
    }

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc || !gattc->conn) {
        BT_LOGW("gattc or acl does not exist");
        return;
    }

    err = bt_conn_le_param_update(gattc->conn, &arg->param);
    if (err != 0) {
        BT_LOGE("param update failed, err = %d", err);
        return;
    }
}

bt_status_t bt_sal_gatt_client_update_connection_parameter(bt_controller_id_t id,
    bt_address_t* addr, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    sal_gattc_req_t* req;
    sal_gattc_conn_param_args_t* arg;

    BT_LOGV("%s", __func__);

    /** Sanity checks to prevent overflow from uint32_t to uint16_t*/
    if ((SAL_LE_CONNECTION_INTERVAL_MIN > min_interval) || (min_interval > max_interval)
        || (max_interval > SAL_LE_CONNECTION_INTERVAL_MAX) || (latency > SAL_LE_LATENCY_MAX)
        || (timeout > SAL_LE_SUPERVISION_TIMEOUT_MAX))
        return BT_STATUS_PARM_INVALID;

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_update_connection_parameter,
        sizeof(sal_gattc_conn_param_args_t));
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    arg = (sal_gattc_conn_param_args_t*)req->data;
    arg->param.interval_min = min_interval;
    arg->param.interval_max = max_interval;
    arg->param.latency = latency;
    arg->param.timeout = timeout;

    return sal_send_req(req);
}

static void zblue_read_rssi(void* data)
{
    int err;
    int8_t rssi;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;

    BT_LOGV("%s", __func__);

    if (!g_sal_gattc_info || !g_sal_gattc_info->list) {
        BT_LOGE("not initialized");
        if_gattc_on_rssi_read(&req->addr, 0, GATT_STATUS_FAILURE);
        return;
    }

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc || !gattc->conn) {
        BT_LOGW("gattc or acl does not exist");
        if_gattc_on_rssi_read(&req->addr, 0, GATT_STATUS_FAILURE);
        return;
    }

    err = bt_conn_read_rssi(gattc->conn, &rssi);
    if (err) {
        BT_LOGE("read rssi failed, err = %d", err);
        if_gattc_on_rssi_read(&gattc->addr, 0, GATT_STATUS_FAILURE);
        return;
    }

    if_gattc_on_rssi_read(&gattc->addr, rssi, GATT_STATUS_SUCCESS);
}

bt_status_t bt_sal_gatt_client_read_remote_rssi(bt_controller_id_t id, bt_address_t* addr)
{
    sal_gattc_req_t* req;

    BT_LOGD("%s", __func__);

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_read_rssi, 0);
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

#ifdef CONFIG_BT_USER_PHY_UPDATE
static void zblue_read_phy(void* data)
{
    int err;
    sal_gattc_conn_t* gattc;
    sal_gattc_req_t* req = data;
    struct bt_conn_info info;
    ble_phy_type_t tx_phy;
    ble_phy_type_t rx_phy;

    BT_LOGV("%s", __func__);

    if (!g_sal_gattc_info || !g_sal_gattc_info->list) {
        BT_LOGE("not initialized");
        /** FIXME: return something via @ref if_gattc_on_phy_read */
        return;
    }

    gattc = find_gattc_conn(req->id, &req->addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc || !gattc->conn) {
        BT_LOGW("gattc or acl does not exist");
        /** FIXME: return something via @ref if_gattc_on_phy_read */
        return;
    }

    err = bt_conn_get_info(gattc->conn, &info);
    if (err) {
        BT_LOGE("get conn info failed, err = %d", err);
        /** FIXME: return something via @ref if_gattc_on_phy_read */
        return;
    }

    tx_phy = le_phy_convert_from_stack(info.le.phy->tx_phy);
    rx_phy = le_phy_convert_from_stack(info.le.phy->rx_phy);

    BT_LOGV("%s, tx_phy = %d, rx_phy = %d", __func__, tx_phy, rx_phy);
    if_gattc_on_phy_read(&gattc->addr, tx_phy, rx_phy);
}
#endif

bt_status_t bt_sal_gatt_client_read_phy(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BT_USER_PHY_UPDATE
    sal_gattc_req_t* req;

    BT_LOGD("%s", __func__);

    req = sal_gattc_req(id, addr, BT_LE_ADDR_TYPE_UNKNOWN, zblue_read_phy, 0);
    if (!req) {
        BT_LOGE("malloc failed");
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
#else
    SAL_NOT_SUPPORT;
#endif
}

bt_status_t bt_sal_gatt_client_set_phy(bt_controller_id_t id, bt_address_t* addr,
    ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    BT_LOGV("%s, tx_phy = %d, rx_phy = %d", __func__, tx_phy, rx_phy);
    return bt_sal_le_set_phy(id, addr, tx_phy, rx_phy);
}

static bool req_cmp(void* data, void* context)
{
    sal_gattc_conn_t* gattc = data;
    sal_gattc_req_t* req = context;

    if (memcmp(&gattc->addr, &req->addr, sizeof(bt_address_t)))
        return false;

    if (req->addr_type != BT_LE_ADDR_TYPE_UNKNOWN && gattc->addr_type != req->addr_type)
        return false;

    if (gattc->id != req->id)
        return false;

    return true;
}

static sal_gattc_conn_t* find_gattc_conn_by_req(sal_gattc_req_t* req)
{
    if (!g_sal_gattc_info || !g_sal_gattc_info->list)
        return NULL;

    return bt_list_find(g_sal_gattc_info->list, req_cmp, req);
}

static bool conn_cmp(void* data, void* context)
{
    sal_gattc_conn_t* gattc = data;
    struct bt_conn* conn = context;

    return gattc->conn == conn;
}

static sal_gattc_conn_t* find_gattc_conn_by_conn(struct bt_conn* conn)
{
    if (!g_sal_gattc_info || !g_sal_gattc_info->list)
        return NULL;

    return bt_list_find(g_sal_gattc_info->list, conn_cmp, conn);
}

static sal_gattc_conn_t* find_gattc_conn(bt_controller_id_t id, const bt_address_t* addr,
    ble_addr_type_t addr_type)
{
    sal_gattc_req_t req = { 0 };

    req.id = id;
    req.addr_type = addr_type;
    if (addr)
        memcpy(&req.addr, addr, sizeof(bt_address_t));

    return find_gattc_conn_by_req(&req);
}

static bool service_handle_cmp(void* data, void* context)
{
    sal_gattc_service_t* service = data;
    uint16_t handle = *(uint16_t*)context;

    if (handle < service->element->handle)
        return false;

    if (handle > service->end_handle)
        return false;

    return true;
}

static sal_gattc_service_t* find_service_by_handle(sal_gattc_conn_t* gattc, uint16_t handle)
{
    BT_LOGV("%s, handle = 0x%04" PRIx16, __func__, handle);

    if (!gattc->services)
        return NULL;

    return bt_list_find(gattc->services, service_handle_cmp, &handle);
}

static bool characteristic_handle_cmp(void* data, void* context)
{
    sal_gattc_characteristic_t* chrc = data;
    uint16_t handle = *(uint16_t*)context;

    if (handle < chrc->element->handle)
        return false;

    if (handle > chrc->end_handle)
        return false;

    return true;
}

static sal_gattc_characteristic_t* find_characteristic_by_handle(sal_gattc_service_t* service,
    uint16_t handle)
{
    BT_LOGV("%s, handle = 0x%04" PRIx16, __func__, handle);

    if (!service->chrcs)
        return NULL;

    return bt_list_find(service->chrcs, characteristic_handle_cmp, &handle);
}

static bool value_handle_cmp(void* data, void* context)
{
    sal_gattc_subscribe_t* subscribe = data;
    uint16_t value_handle = *(uint16_t*)context;

    return subscribe->value_handle == value_handle;
}

static sal_gattc_subscribe_t* find_subscribe_by_value_handle(sal_gattc_conn_t* gattc,
    uint16_t value_handle)
{
    return bt_list_find(gattc->subscribes, value_handle_cmp, &value_handle);
}

static bt_status_t addr_type_sal_to_zephyr(uint8_t* out, ble_addr_type_t in)
{
    uint8_t type;

    switch (in) {
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
        BT_LOGW("%s, input type unknown, set to public", __func__);
        type = BT_ADDR_LE_PUBLIC;
        break;
    default:
        BT_LOGE("%s, invalid type: %d", __func__, in);
        return BT_STATUS_PARM_INVALID;
    }

    *out = type;

    return BT_STATUS_SUCCESS;
}

static bt_status_t addr_sal_to_zephyr(bt_addr_le_t* out, const bt_address_t* addr,
    ble_addr_type_t addr_type)
{
    bt_status_t ret;

    if (!addr)
        return BT_STATUS_PARM_INVALID;

    ret = addr_type_sal_to_zephyr(&out->type, addr_type);
    if (ret != BT_STATUS_SUCCESS)
        return ret;

    memcpy(out->a.val, addr, BT_ADDR_SIZE);

    return BT_STATUS_SUCCESS;
}

static bt_status_t uuid_zephyr_to_sal(bt_uuid_t* out, const struct bt_uuid* in)
{
    switch (in->type) {
    case BT_UUID_TYPE_16:
        out->type = BT_UUID16_TYPE;
        memcpy(&out->val.u16, &BT_UUID_16(in)->val, sizeof(out->val.u16));
        break;
    case BT_UUID_TYPE_32:
        out->type = BT_UUID32_TYPE;
        memcpy(&out->val.u32, &BT_UUID_32(in)->val, sizeof(out->val.u32));
        break;
    case BT_UUID_TYPE_128:
        out->type = BT_UUID128_TYPE;
        memcpy(&out->val.u128, &BT_UUID_128(in)->val, sizeof(out->val.u128));
        break;
    default:
        BT_LOGE("%s, invalid type: %d", __func__, in->type);
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t uuid_sal_to_zephyr(struct bt_uuid* out, const bt_uuid_t* in)
{
    uint8_t data_len;

    switch (in->type) {
    case BT_UUID16_TYPE:
        data_len = BT_UUID_SIZE_16;
        break;
    case BT_UUID32_TYPE:
        data_len = BT_UUID_SIZE_32;
        break;
    case BT_UUID128_TYPE:
        data_len = BT_UUID_SIZE_128;
        break;
    default:
        BT_LOGE("%s, invalid type: %d", __func__, in->type);
        return BT_STATUS_PARM_INVALID;
    }

    if (bt_uuid_create(out, (uint8_t*)&in->val, data_len))
        return BT_STATUS_SUCCESS;

    return BT_STATUS_FAIL;
}

void bt_sal_gatt_client_connection_updated_callback(bt_controller_id_t id, bt_address_t* addr,
    uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout, bt_status_t status)
{
    /* Nothing to do, connection parameter updates are handled via zblue_on_param_updated */
}

void bt_sal_gatt_client_connection_state_changed_callback(bt_controller_id_t id, bt_address_t* addr,
    profile_connection_state_t state)
{
    sal_gattc_conn_t* gattc;

    BT_LOGD("%s", __func__);

    gattc = find_gattc_conn(id, addr, BT_LE_ADDR_TYPE_UNKNOWN);
    if (!gattc) {
        BT_LOGW("gattc does not exist");
        return;
    }

    gattc->state = state;
    if (state != PROFILE_STATE_DISCONNECTED) {
        if_gattc_on_connection_state_changed(addr, state);
    } else {
        bt_list_remove(g_sal_gattc_info->list, gattc); /**< callback invoked in gattc_conn_delete */

        if (g_sal_gattc_info->terminating && bt_list_is_empty(g_sal_gattc_info->list))
            bt_sal_gatt_client_disable();
    }
}
