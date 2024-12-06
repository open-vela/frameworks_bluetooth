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
#include <stdint.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/uuid.h>

#include "sal_gatt_server_interface.h"

#include "bluetooth.h"
#include "bt_list.h"
#include "bt_status.h"
#include "gatt_define.h"
#include "gatts_service.h"
#include "sal_adapter_le_interface.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_GATT

#ifndef CONFIG_GATT_SERVER_MAX_SERVICES
#define CONFIG_GATT_SERVER_MAX_SERVICES 10
#endif

#ifndef CONFIG_GATT_SERVER_MAX_ATTRIBUTES
#define CONFIG_GATT_SERVER_MAX_ATTRIBUTES 30
#endif

#define NEXT_DB_ATTR(attr) (attr + 1)
#define LAST_DB_ATTR (server_db + (attr_count - 1))

#define GATT_PERM_MASK (BT_GATT_PERM_READ | BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE | BT_GATT_PERM_WRITE_AUTHEN | BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_PREPARE_WRITE)
#define GATT_PERM_ENC_READ_MASK (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_ENC_WRITE_MASK (BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_WRITE_AUTHEN)
#define GATT_PERM_READ_AUTHORIZATION 0x40
#define GATT_PERM_WRITE_AUTHORIZATION 0x80

#define STACK_CALL(func) zblue_##func

typedef void (*sal_func_t)(void* args);

union uuid {
    struct bt_uuid uuid;
    struct bt_uuid_16 u16;
    struct bt_uuid_128 u128;
};

struct add_descriptor {
    uint16_t desc_id;
    uint8_t permissions;
    uint8_t properties;
    const struct bt_uuid* uuid;
};

struct add_characteristic {
    uint16_t char_id;
    uint8_t properties;
    uint8_t permissions;
    const struct bt_uuid* uuid;
    uint32_t attr_length;
    uint8_t* attr_data;
};

struct gatt_value {
    uint16_t len;
    uint8_t* data;
    uint8_t flags[1];
};

struct set_value {
    const uint8_t* value;
    uint16_t len;
};

struct gatt_server_context {
    bt_address_t* addr;
    bt_uuid_t* uuid;
    uint8_t* value;
    uint16_t length;
    struct bt_conn* conn;
};

typedef union {
    bool reason;
} sal_adapter_args_t;

typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type;
    sal_func_t func;
    sal_adapter_args_t adpt;
} sal_adapter_req_t;

static uint8_t attr_count;
static uint8_t svc_attr_count;
static uint8_t svc_count;

static struct bt_gatt_service server_svcs[CONFIG_GATT_SERVER_MAX_SERVICES];
static struct bt_gatt_attr server_db[CONFIG_GATT_SERVER_MAX_ATTRIBUTES];

static void zblue_conn_get_addr(struct bt_conn* conn, bt_address_t* addr)
{
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);
    bt_addr_set(addr, info.le.dst->a.val);
}

static ssize_t read_value(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    void* buf, uint16_t len, uint16_t offset)
{
    struct gatt_value* user_data = attr->user_data;

    BT_LOGD("%s, handle:0x%0x, user_data 0x%p, user_data_len:%d", __func__, attr->handle, user_data, user_data->len);

    return bt_gatt_attr_read(conn, attr, buf, len, offset, user_data->data, user_data->len);
}

static ssize_t write_value(struct bt_conn* conn, const struct bt_gatt_attr* attr, const void* buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    struct gatt_value* user_data = attr->user_data;

    BT_LOGD("%s", __func__);

    memcpy(user_data->data + offset, buf, len);
    return len;
}

static struct bt_gatt_attr* gatt_db_add(const struct bt_gatt_attr* pattern, size_t user_data_len)
{
    static struct bt_gatt_attr* attr = server_db;
    const union uuid* u = CONTAINER_OF(pattern->uuid, union uuid, uuid);
    size_t uuid_size = u->uuid.type == BT_UUID_TYPE_16 ? sizeof(u->u16) : sizeof(u->u128);

    memcpy(attr, pattern, sizeof(*attr));

    attr->uuid = malloc(uuid_size);
    memcpy((void*)attr->uuid, &u->uuid, uuid_size);

    attr->user_data = malloc(user_data_len);
    memcpy(attr->user_data, pattern->user_data, user_data_len);

    BT_LOGD("user_data 0x%p, user_data_len:%d", attr->user_data, user_data_len);

    attr_count++;
    svc_attr_count++;

    return attr++;
}

static bt_status_t register_service(void)
{
    int err;

    server_svcs[svc_count].attrs = server_db + (attr_count - svc_attr_count);
    server_svcs[svc_count].attr_count = svc_attr_count;

    err = bt_gatt_service_register(&server_svcs[svc_count]);
    if (err) {
        BT_LOGD("%s, gatt service register", __func__);
        return BT_STATUS_FAIL;
    }

    svc_attr_count = 0U;
    return BT_STATUS_SUCCESS;
}

static void add_service(gatt_element_t* element)
{
    struct bt_gatt_attr* attr_svc;
    union uuid u;
    size_t size;

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&element->uuid.val, element->uuid.type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return;
    }

    size = u.uuid.type == BT_UUID_TYPE_16 ? sizeof(u.u16) : sizeof(u.u128);
    if (svc_attr_count) {
        if (register_service()) {
            BT_LOGE("%s, register service fail", __func__);
            return;
        }
    }

    svc_count++;

    switch (element->type) {
    case GATT_PRIMARY_SERVICE:
        attr_svc = gatt_db_add(&(struct bt_gatt_attr)BT_GATT_PRIMARY_SERVICE(&u.uuid), size);
        break;
    case GATT_SECONDARY_SERVICE:
        attr_svc = gatt_db_add(&(struct bt_gatt_attr)BT_GATT_SECONDARY_SERVICE(&u.uuid), size);
        break;
    default:
        attr_svc = NULL;
        break;
    }

    if (!attr_svc) {
        svc_count--;
        BT_LOGE("%s, attr_svc is null", __func__);
        return;
    }
}

static int alloc_characteristic(struct add_characteristic* ch)
{
    struct bt_gatt_attr *attr_chrc, *attr_value;
    struct bt_gatt_chrc* chrc_data;
    struct gatt_value* user_data;

    /* Add Characteristic Declaration */
    attr_chrc = gatt_db_add(&(struct bt_gatt_attr)BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC, BT_GATT_PERM_READ, bt_gatt_attr_read_chrc, NULL, (&(struct bt_gatt_chrc) {})), sizeof(*chrc_data));
    if (!attr_chrc) {
        return -EINVAL;
    }

    user_data = zalloc(sizeof(*user_data));
    user_data->data = ch->attr_data;
    user_data->len = ch->attr_length;

    attr_value = gatt_db_add(&(struct bt_gatt_attr)BT_GATT_ATTRIBUTE(ch->uuid, ch->permissions & GATT_PERM_MASK, read_value, write_value, user_data), sizeof(*user_data));
    if (!attr_value) {
        return -EINVAL;
    }

    chrc_data = attr_chrc->user_data;
    chrc_data->properties = ch->properties;
    chrc_data->uuid = attr_value->uuid;

    ch->char_id = attr_chrc->handle;
    return 0;
}

static void add_characteristic(gatt_element_t* element)
{
    struct add_characteristic chr = { 0 };
    union uuid u = { 0 };

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&element->uuid.val, element->uuid.type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return;
    }

    chr.permissions = element->permissions;
    chr.properties = element->properties;
    chr.uuid = &u.uuid;
    chr.attr_length = element->attr_length;
    chr.attr_data = element->attr_data;

    if (alloc_characteristic(&chr)) {
        BT_LOGE("%s, alloc characteristic fail", __func__);
        return;
    }
}

static void ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    BT_LOGD("%s, value:%d", __func__, value);
}

static int alloc_descriptor(const struct bt_gatt_attr* attr, struct add_descriptor* desc)
{
    struct bt_gatt_attr* attr_desc;
    struct bt_gatt_chrc* chrc = attr->user_data;

    if (bt_uuid_cmp(desc->uuid, BT_UUID_GATT_CCC)) {
        BT_LOGE("%s uuid not match", __func__);
        return -EINVAL;
    }

    if (!(chrc->properties & (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE))) {
        BT_LOGE("%s, invald properties:0x%0x", __func__, chrc->properties);
        return -EINVAL;
    }

    attr_desc = gatt_db_add(&(struct bt_gatt_attr)BT_GATT_CCC(ccc_cfg_changed, desc->permissions & GATT_PERM_MASK), sizeof(struct _bt_gatt_ccc));
    if (!attr_desc) {
        BT_LOGE("%s attr_desc null", __func__);
        return -EINVAL;
    }

    desc->desc_id = attr_desc->handle;
    return 0;
}

static struct bt_gatt_attr* get_base_chrc(struct bt_gatt_attr* attr)
{
    struct bt_gatt_attr* tmp;

    for (tmp = attr; tmp > server_db; tmp--) {
        if (!bt_uuid_cmp(tmp->uuid, BT_UUID_GATT_PRIMARY) || !bt_uuid_cmp(tmp->uuid, BT_UUID_GATT_SECONDARY)) {
            break;
        }

        if (!bt_uuid_cmp(tmp->uuid, BT_UUID_GATT_CHRC)) {
            return tmp;
        }
    }

    return NULL;
}

static void add_descriptor(gatt_element_t* element)
{
    struct add_descriptor desc = { 0 };
    struct bt_gatt_attr* chrc;
    union uuid u;

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&element->uuid.val, element->uuid.type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return;
    }

    desc.permissions = element->permissions;
    desc.properties = element->properties;
    desc.uuid = &u.uuid;

    chrc = get_base_chrc(LAST_DB_ATTR);
    if (!chrc) {
        BT_LOGE("%s, get base chrc fail", __func__);
        return;
    }

    if (alloc_descriptor(chrc, &desc)) {
        BT_LOGE("%s, alloc descriptor fail", __func__);
        return;
    }
}

static void set_value(uint16_t attr_id, uint8_t* val, uint16_t len)
{
    struct bt_gatt_attr* attr = &server_db[attr_id - server_db[0].handle];
    struct gatt_value* value;

    if (!bt_uuid_cmp(attr->uuid, BT_UUID_GATT_CCC)) {
        BT_LOGE("%s cccd not set", __func__);
        return;
    }

    value = attr->user_data;

    memcpy(value->data, val, len);
    value->len = len;
}

static void zblue_gatt_mtu_updated_callback(struct bt_conn* conn, uint16_t tx, uint16_t rx)
{
    bt_address_t addr;

    BT_LOGD("Updated MTU: TX: %d RX: %d bytes\n", tx, rx);
    zblue_conn_get_addr(conn, &addr);
    if_gatts_on_mtu_changed(&addr, tx);
}

static struct bt_gatt_cb zblue_gatt_callbacks = {
    .att_mtu_updated = zblue_gatt_mtu_updated_callback
};

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

bt_status_t bt_sal_gatt_server_enable(void)
{
    bt_gatt_cb_register(&zblue_gatt_callbacks);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_disable(void)
{
    bt_gatt_cb_register(NULL);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_add_elements(gatt_element_t* elements, uint16_t size)
{
    size_t index;

    for (index = 0; index < size; index++) {
        switch (elements[index].type) {
        case GATT_PRIMARY_SERVICE:
        case GATT_SECONDARY_SERVICE:
            add_service(&elements[index]);
            break;
        case GATT_CHARACTERISTIC:
            add_characteristic(&elements[index]);
            break;
        case GATT_DESCRIPTOR:
            add_descriptor(&elements[index]);
            break;
        default:
            BT_LOGE("%s, type:%d not handle", __func__, elements[index].type);
            break;
        }
    }

    return register_service();
}

static struct bt_gatt_service* get_primary_service_from_element(gatt_element_t* element)
{
    struct bt_gatt_service* srv;
    union uuid u;

    if (element->type != GATT_PRIMARY_SERVICE) {
        return NULL;
    }

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&element->uuid.val, element->uuid.type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return NULL;
    }

    for (srv = server_svcs; srv < server_svcs + CONFIG_GATT_SERVER_MAX_SERVICES; srv++) {
        if (!srv->attrs) {
            continue;
        }

        if (!bt_uuid_cmp(srv->attrs[0].uuid, &u.uuid)) {
            return srv;
        }
    }

    BT_LOGE("%s", __func__);
    return NULL;
}

bt_status_t bt_sal_gatt_server_remove_elements(gatt_element_t* elements, uint16_t size)
{
    uint16_t i;
    struct bt_gatt_service* srv;
    char uuid_str[40];

    for (i = 0; i < size; i++) {
        srv = get_primary_service_from_element(&elements[i]);
        if (srv) {
            bt_uuid_to_string(&elements[i].uuid, uuid_str, 40);
            BT_LOGD("%s, uuid:%s", __func__, uuid_str);
            bt_gatt_service_unregister(srv);
        }
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

bt_status_t bt_sal_gatt_server_connect(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type)
{
    sal_adapter_req_t* req;
    uint8_t type;

    req = sal_adapter_req(id, addr, STACK_CALL(conn_connect));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
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

static void STACK_CALL(conn_cancel)(void* args)
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

bt_status_t bt_sal_gatt_server_cancel_connection(bt_controller_id_t id, bt_address_t* addr)
{
    sal_adapter_req_t* req;

    req = sal_adapter_req(id, addr, STACK_CALL(conn_cancel));
    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    return sal_send_req(req);
}

bt_status_t bt_sal_gatt_server_send_response(bt_controller_id_t id, bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_gatt_server_set_attr_value(bt_controller_id_t id, bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length)
{
    set_value(request_id, value, length);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_get_attr_value(bt_controller_id_t id, bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length)
{
    return BT_STATUS_UNSUPPORTED;
}

static uint8_t gatt_send_notification(const struct bt_gatt_attr* attr, uint16_t handle, void* user_data)
{
    struct gatt_server_context* context = user_data;
    union uuid u;

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&context->uuid->val, context->uuid->type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    if (bt_uuid_cmp(attr->uuid, &u.uuid)) {
        return BT_GATT_ITER_CONTINUE;
    }

    bt_gatt_notify(context->conn, attr, context->value, context->length);
    return BT_GATT_ITER_STOP;
}

bt_status_t bt_sal_gatt_server_send_notification(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t uuid, uint8_t* value, uint16_t length)
{
    struct gatt_server_context context = {
        .addr = addr,
        .uuid = &uuid,
        .value = value,
        .length = length,
    };

    context.conn = get_le_conn_from_addr(addr);
    if (!context.conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    bt_gatt_foreach_attr(0x0001, 0xffff, gatt_send_notification, (void*)&context);
    return BT_STATUS_SUCCESS;
}

static void send_indication_destory(struct bt_gatt_indicate_params* params)
{
    BT_LOGD("%s, send_indication_destory", __func__);
    free(params);
}

static void send_indication_result(struct bt_conn* conn, struct bt_gatt_indicate_params* params, uint8_t err)
{
    if (err) {
        BT_LOGE("%s, send indication fail", __func__);
    }

    // todo: notify app?
}

static uint8_t gatt_send_indication(const struct bt_gatt_attr* attr, uint16_t handle, void* user_data)
{
    struct gatt_server_context* context = user_data;
    union uuid u;
    struct bt_gatt_indicate_params* params;
    int ret;

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&context->uuid->val, context->uuid->type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    if (bt_uuid_cmp(attr->uuid, &u.uuid)) {
        return BT_GATT_ITER_CONTINUE;
    }

    params = zalloc(sizeof(struct bt_gatt_indicate_params));
    if (!params) {
        BT_LOGE("%s, zalloc fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    params->attr = attr;
    params->data = context->value;
    params->len = context->length;
    params->func = send_indication_result;
    params->destroy = send_indication_destory;
    ret = bt_gatt_indicate(context->conn, params);
    if (ret) {
        BT_LOGE("%s, indicate fail err:%d", __func__, ret);
    }

    return BT_GATT_ITER_STOP;
}

bt_status_t bt_sal_gatt_server_send_indication(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t uuid, uint8_t* value, uint16_t length)
{
    struct gatt_server_context context = {
        .addr = addr,
        .uuid = &uuid,
        .value = value,
        .length = length,
    };

    context.conn = get_le_conn_from_addr(addr);
    if (!context.conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    bt_gatt_foreach_attr(0x0001, 0xffff, gatt_send_indication, (void*)&context);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_read_phy(bt_controller_id_t id, bt_address_t* addr)
{
#if defined(CONFIG_BT_USER_PHY_UPDATE)
    struct bt_conn* conn;
    struct bt_conn_info info;
    int ret;
    ble_phy_type_t tx_mode;
    ble_phy_type_t rx_mode;

    conn = get_le_conn_from_addr(addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        return BT_STATUS_FAIL;
    }

    ret = bt_conn_get_info(conn, &info);
    if (ret) {
        BT_LOGE("%s, conn get info err:%d", __func__, ret);
        return BT_STATUS_FAIL;
    }

    tx_mode = le_phy_convert_from_stack(info.le.phy->tx_phy);
    rx_mode = le_phy_convert_from_stack(info.le.phy->rx_phy);

    BT_LOGD("%s, tx phy:%d, rx phy:%d", __func__, tx_mode, rx_mode);
    if_gatts_on_phy_read(addr, tx_mode, rx_mode);

    return BT_STATUS_SUCCESS;
#else
    SAL_NOT_SUPPORT;
#endif
}

bt_status_t bt_sal_gatt_server_set_phy(bt_controller_id_t id, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    return bt_sal_le_set_phy(id, addr, tx_phy, rx_phy);
}

#endif /* CONFIG_BLUETOOTH_GATT*/
