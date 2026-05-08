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

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/uuid.h>

#include "bluetooth.h"
#include "bt_list.h"
#include "bt_status.h"
#include "gatt_define.h"
#include "gatts_service.h"
#include "sal_adapter_le_interface.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "sal_zephyr_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_GATT_SERVER

#ifndef CONFIG_GATT_SERVER_MAX_SERVICES
#define CONFIG_GATT_SERVER_MAX_SERVICES 20
#endif

struct gatt_user_data {
    gatt_element_t* element;
};

#ifndef CONFIG_GATT_SERVER_MAX_ATTRIBUTES
#define CONFIG_GATT_SERVER_MAX_ATTRIBUTES 60
#endif

#define NEXT_DB_ATTR(attr) (attr + 1)
#define LAST_DB_ATTR (server_db + (attr_count - 1))

#define GATT_PERM_MASK (BT_GATT_PERM_READ | BT_GATT_PERM_READ_AUTHEN \
    | BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_READ_LESC             \
    | BT_GATT_PERM_WRITE | BT_GATT_PERM_WRITE_AUTHEN                 \
    | BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_WRITE_LESC | BT_GATT_PERM_PREPARE_WRITE)

#define GATT_PERM_ENC_READ_MASK (BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_READ_AUTHEN)
#define GATT_PERM_ENC_WRITE_MASK (BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_WRITE_AUTHEN)

#define GATT_OPS_WRITE_REQUEST 0
#define GATT_OPS_READ_REQUEST 1
#define GATT_WRITE_FLAGS_RELIABLE_WRITE (BT_GATT_WRITE_FLAG_PREPARE | BT_GATT_WRITE_FLAG_EXECUTE)

#define MAKE_REQUEST_ID(handle, op_type) (((uint32_t)(op_type) << 31) | ((handle) & (0xFFFF)))
#define REQUEST_ID_HANDLE(id) ((uint16_t)((id) & (0xFFFF)))
#define REQUEST_ID_OP_TYPE(id) (((id) >> 31) & 0x1)
#define REQUEST_ID_NORSP ((uint32_t)0xFFFFFFFF)

#define STACK_CALL(func) zblue_##func

typedef enum {
    GATTS_CB_TYPE_ADDED,
    GATTS_CB_TYPE_REMOVED
} sal_gatts_cb_type_t;

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
    gatt_element_t* element;
};

struct add_characteristic {
    uint16_t char_id;
    uint8_t properties;
    uint16_t permissions;
    const struct bt_uuid* uuid;
    uint32_t attr_length;
    uint8_t* attr_data;
    gatt_element_t* element;
};

struct gatt_ccc_wrapper {
    /**
     * NOTE: `ccc` must be the first member!
     * This ensures `&wrapper->ccc == (void *)wrapper`,
     * so that we can safely cast between `_bt_gatt_ccc*` and `gatt_ccc_wrapper*`
     * or free it through `user_data` pointer.
     */
    struct _bt_gatt_ccc ccc;
    gatt_element_t* element;
    uint16_t len;
    uint8_t data[0];
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
    gatt_element_t* element;
};

typedef union {
    bool reason;

    struct {
        uint16_t element_id;
        uint16_t size;
        sal_gatts_cb_type_t type;
    } attr_op;
} sal_adapter_args_t;

typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type;
    sal_func_t func;
    sal_adapter_args_t adpt;
} sal_adapter_req_t;

typedef struct {
    struct bt_gatt_service* srv;
    struct bt_sdp_record* record;
} sal_gatt_sdp_record_t;

/* Request struct for async notify/indicate dispatch.
 * First four fields mirror sal_adapter_req_t so sal_invoke_async can cast safely.
 */
typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type; /* unused, kept for layout compatibility */
    sal_func_t func;
    gatt_element_t* element;
    uint16_t length;
    uint8_t value[];
} sal_gatts_notify_req_t;

/* Request struct for async send_response dispatch. */
typedef struct {
    bt_controller_id_t id;
    bt_address_t addr;
    ble_addr_type_t addr_type; /* unused, kept for layout compatibility */
    sal_func_t func;
    uint32_t request_id;
    uint16_t length;
    uint8_t value[];
} sal_gatts_rsp_req_t;

static size_t attr_count;
static size_t svc_attr_count;

static struct bt_gatt_service server_svcs[CONFIG_GATT_SERVER_MAX_SERVICES];
static struct bt_gatt_attr server_db[CONFIG_GATT_SERVER_MAX_ATTRIBUTES];
static sal_gatt_sdp_record_t gatt_sdp_records[CONFIG_GATT_SERVER_MAX_SERVICES];

static int find_free_service_index(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(server_svcs); i++) {
        if (server_svcs[i].attrs == NULL && server_svcs[i].attr_count == 0) {
            return i;
        }
    }

    return -1;
}

static void remove_from_server_db(const struct bt_gatt_attr* start, size_t count)
{
    size_t index, i;

    if (!start || count == 0) {
        return;
    }

    index = start - server_db;

    if (start < server_db || index >= attr_count) {
        BT_LOGE("%s, invalid start pointer", __func__);
        return;
    }

    if (count > attr_count || index + count > attr_count) {
        BT_LOGE("%s, invalid count: %zu (index=%zu, attr_count=%zu)", __func__, count, index, attr_count);
        return;
    }

    for (i = 0; i < count; i++) {
        free(start[i].user_data);
        free((void*)start[i].uuid);
    }

    if (index + count < attr_count) {
        memmove(&server_db[index], &server_db[index + count],
            (attr_count - index - count) * sizeof(struct bt_gatt_attr));
    }

    memset(&server_db[attr_count - count], 0, count * sizeof(struct bt_gatt_attr));
    attr_count -= count;
}

/* Generic ATT SDP record */
static struct bt_sdp_attribute gatt_attrs_template[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), /* 35 03 */
        BT_SDP_DATA_ELEM_LIST(
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
                BT_SDP_ARRAY_16(BT_SDP_GENERIC_ATTRIB_SVCLASS) /* 18 01 */
            }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 19), /* 35 13 */
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
                BT_SDP_DATA_ELEM_LIST(
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) /* 01 00 */
                    },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
                        BT_SDP_ARRAY_16(BT_L2CAP_PSM_ATT) /* 00 1F */
                    }, ) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 9), /* 35 09 */
                BT_SDP_DATA_ELEM_LIST(
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_ATT) /* 00 07 */
                    },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
                        BT_SDP_ARRAY_16(0) /* 00 00, assigned in gatt_sdp_create_record */
                    },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
                        BT_SDP_ARRAY_16(0) /* 00 00, assigned in gatt_sdp_create_record */
                    }, ) }, )),
};

static ssize_t read_value(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    void* buf, uint16_t len, uint16_t offset)
{
    bt_address_t addr;
    gatt_element_t* element;
    uint32_t request_id;

    if (!attr || !attr->user_data) {
        BT_LOGE("%s, user_data is NULL", __func__);
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    element = (gatt_element_t*)attr->user_data;

    bt_sal_get_remote_address(conn, &addr);

    request_id = MAKE_REQUEST_ID(element->handle, GATT_OPS_READ_REQUEST);
    if_gatts_on_received_element_read_request(&addr, request_id, element->handle);

    return -EINPROGRESS;
}

static ssize_t write_value(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    const void* buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    bt_address_t addr;
    gatt_element_t* element;
    uint32_t request_id;
    int ret;

    if (!attr || !attr->user_data) {
        BT_LOGE("%s, user_data is NULL", __func__);
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    element = (gatt_element_t*)attr->user_data;

    if (flags & GATT_WRITE_FLAGS_RELIABLE_WRITE) {
        BT_LOGE("%s, reliable write is not supported", __func__);
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    if (flags & BT_GATT_WRITE_FLAG_CMD) {
        request_id = REQUEST_ID_NORSP;
        ret = len;
    } else {
        request_id = MAKE_REQUEST_ID(element->handle, GATT_OPS_WRITE_REQUEST);
        ret = -EINPROGRESS;
    }

    bt_sal_get_remote_address(conn, &addr);

    if_gatts_on_received_element_write_request(&addr, request_id, element->handle, (uint8_t*)buf, offset, len);

    return ret;
}

static struct bt_gatt_attr* gatt_db_add(const struct bt_gatt_attr* pattern, size_t user_data_len)
{
    struct bt_gatt_attr* attr = &server_db[attr_count];

    if (attr_count >= CONFIG_GATT_SERVER_MAX_ATTRIBUTES) {
        BT_LOGE("%s, server_db is full", __func__);
        return NULL;
    }

    const union uuid* u = CONTAINER_OF(pattern->uuid, union uuid, uuid);
    size_t uuid_size = (u->uuid.type == BT_UUID_TYPE_16) ? sizeof(u->u16) : sizeof(u->u128);

    memcpy(attr, pattern, sizeof(*attr));

    attr->uuid = malloc(uuid_size);
    if (!attr->uuid) {
        BT_LOGE("%s, uuid malloc failed", __func__);
        return NULL;
    }
    memcpy((void*)attr->uuid, &u->uuid, uuid_size);

    if (user_data_len == 0) {
        attr->user_data = pattern->user_data;
    } else {
        attr->user_data = malloc(user_data_len);
        if (!attr->user_data) {
            BT_LOGE("%s, user_data malloc failed", __func__);
            free((void*)attr->uuid);
            attr->uuid = NULL;
            return NULL;
        }
        memcpy(attr->user_data, pattern->user_data, user_data_len);
    }

    BT_LOGD("user_data 0x%p, user_data_len: %zu", attr->user_data, user_data_len);

    attr_count++;
    svc_attr_count++;

    return attr;
}

static int gatt_sdp_set_srv_cls(struct bt_sdp_attribute* attr, union uuid* uuid)
{
    struct bt_sdp_data_elem *element, *tmp_elem;

    if (!attr || !uuid) {
        BT_LOGE("%s, invalid params", __func__);
        return -EINVAL;
    }

    if (attr->id != BT_SDP_ATTR_SVCLASS_ID_LIST) {
        BT_LOGE("Invalid attribute id: %d, only for BT_SDP_ATTR_SVCLASS_ID_LIST", attr->id);
        return -EINVAL;
    }

    tmp_elem = (struct bt_sdp_data_elem*)zalloc(sizeof(struct bt_sdp_data_elem));
    if (!tmp_elem) {
        BT_LOGE("malloc failed!");
        return -ENOMEM;
    }

    if (uuid->uuid.type == BT_UUID_TYPE_16) {
        /* Modify BT_SDP_ATTR_SVCLASS_ID_LIST attribute (index 1):
         * - Redirect data pointer to the passed uuid16
         */
        element = (struct bt_sdp_data_elem*)&attr->val;

        memcpy(tmp_elem, element->data, sizeof(struct bt_sdp_data_elem));
        tmp_elem->data = &uuid->u16.val;
        element->data = tmp_elem;
    } else if (uuid->uuid.type == BT_UUID_TYPE_128) {
        /* Modify BT_SDP_ATTR_SVCLASS_ID_LIST attribute (index 1):
         * - Change sequence length from 3 to 17 (1 byte type + 16 bytes UUID128)
         * - Change UUID type from BT_SDP_UUID16 to BT_SDP_UUID128
         * - Redirect data pointer to the passed uuid128
         */
        element = (struct bt_sdp_data_elem*)&attr->val;
        element->data_size = BT_UUID_SIZE_128 + 1;
        element->total_size = BIT((element->type & BT_SDP_SIZE_DESC_MASK) - BT_SDP_SIZE_INDEX_OFFSET) + element->data_size + 1;

        element->data = tmp_elem;

        element = (struct bt_sdp_data_elem*)element->data;
        element->type = BT_SDP_UUID128;
        element->data_size = BIT(element->type & BT_SDP_SIZE_DESC_MASK);
        element->total_size = BIT(element->type & BT_SDP_SIZE_DESC_MASK) + 1;
        element->data = uuid->u128.val;
    }

    return 0;
}

static int gatt_sdp_set_hdl(struct bt_sdp_attribute* attr, uint16_t* start_hdl, uint16_t* end_hdl)
{
    struct bt_sdp_data_elem *element, *tmp_element, *start_hdl_elem, *end_hdl_elem;
    struct bt_sdp_data_elem *prot_desc_elem, *handle_desc_elem;

    if (!attr) {
        BT_LOGE("%s, invalid params", __func__);
        return -EINVAL;
    }

    if (attr->id != BT_SDP_ATTR_PROTO_DESC_LIST) {
        BT_LOGE("Invalid attribute id: %d, only for BT_SDP_ATTR_PROTO_DESC_LIST", attr->id);
        return -EINVAL;
    }

    /* Modify BT_SDP_ATTR_PROTO_DESC_LIST attribute (index 1, pointer depth 0):
     * - Redirect data pointer to the passed start_hdl & end_hdl
     */

    /* Get the GATT protocol descriptor list (index 0, pointer depth 1): */
    element = (struct bt_sdp_data_elem*)attr->val.data;
    prot_desc_elem = (struct bt_sdp_data_elem*)zalloc(sizeof(struct bt_sdp_data_elem) * 2);
    if (!prot_desc_elem) {
        BT_LOGE("malloc failed!");
        return -ENOMEM;
    }

    memcpy(prot_desc_elem, element, sizeof(struct bt_sdp_data_elem) * 2);
    attr->val.data = prot_desc_elem;

    /* Get the start_hdl/end_hdl element of GATT protocol descriptor list (index 0, pointer depth 2): */
    tmp_element = (struct bt_sdp_data_elem*)prot_desc_elem[DATA_ELEM_GATT_PROT_DESC_POS].data;
    handle_desc_elem = (struct bt_sdp_data_elem*)zalloc(sizeof(struct bt_sdp_data_elem) * 3);
    if (!handle_desc_elem) {
        BT_LOGE("malloc failed!");
        free(prot_desc_elem);
        return -ENOMEM;
    }

    memcpy(handle_desc_elem, tmp_element, sizeof(struct bt_sdp_data_elem) * 3);
    prot_desc_elem[DATA_ELEM_GATT_PROT_DESC_POS].data = handle_desc_elem;

    /* Modify start handle attribute (index 1, pointer depth 2): */
    start_hdl_elem = &handle_desc_elem[DATA_ELEM_GATT_START_HANDLE_POS];
    start_hdl_elem->data = start_hdl;

    /* Modify end handle attribute (index 2, pointer depth 2): */
    end_hdl_elem = &handle_desc_elem[DATA_ELEM_GATT_END_HANDLE_POS];
    end_hdl_elem->data = end_hdl;

    return 0;
}

static struct bt_sdp_record* gatt_sdp_create_record(struct bt_gatt_service* srv)
{
    struct bt_sdp_record* record;
    size_t attrs_count;
    struct bt_sdp_attribute* attrs;
    sal_gatt_sdp_record_t* gatt_record;
    union uuid* uuid;
    struct bt_sdp_data_elem* prot_desc_elem;
    int err = 0;
    uint32_t* srv_hdl;

    /* First attribute of services is service declaration(primary or secondary) */
    uuid = srv->attrs->user_data;
    if (uuid->uuid.type == BT_UUID_TYPE_32) {
        BT_LOGE("Invalid UUID type: %d, only for UUID16/UUID128", uuid->uuid.type);
        return NULL;
    }

    record = zalloc(sizeof(struct bt_sdp_record));
    if (!record) {
        BT_LOGE("Failed to allocate memory for SDP record");
        return NULL;
    }

    attrs = zalloc(sizeof(gatt_attrs_template));
    if (!attrs) {
        BT_LOGE("Failed to allocate memory for SDP attributes");
        free(record);
        return NULL;
    }

    attrs_count = ARRAY_SIZE(gatt_attrs_template);
    memcpy(attrs, gatt_attrs_template, sizeof(gatt_attrs_template));

    /* Modify SDP service handle */
    srv_hdl = (uint32_t*)malloc(sizeof(uint32_t));
    if (!srv_hdl) {
        BT_LOGE("Failed to allocate memory for SDP service handle");
        goto err_free_attrs;
    }

    attrs[0].val.data = srv_hdl;

    if (gatt_sdp_set_hdl(&attrs[SDP_ATTR_PROT_GATT_POS], &srv->attrs->handle, &srv->attrs[svc_attr_count - 1].handle)) {
        BT_LOGE("Failed to set SDP GATT handle");
        free(srv_hdl);
        goto err_free_attrs;
    }

    err = gatt_sdp_set_srv_cls(&attrs[SDP_ATTR_SVCLS_GATT_POS], uuid);

    if (err) {
        prot_desc_elem = (struct bt_sdp_data_elem*)attrs[SDP_ATTR_PROT_GATT_POS].val.data;
        free(srv_hdl);
        free((struct bt_sdp_data_elem*)prot_desc_elem[DATA_ELEM_GATT_PROT_DESC_POS].data);
        free(prot_desc_elem);
        goto err_free_attrs;
    }

    record->attr_count = attrs_count;
    record->attrs = attrs;

    for (gatt_record = gatt_sdp_records; gatt_record < gatt_sdp_records + CONFIG_GATT_SERVER_MAX_SERVICES; gatt_record++) {
        if (gatt_record->srv) {
            continue;
        }

        gatt_record->srv = srv;
        gatt_record->record = record;
        return record;
    }

err_free_attrs:
    free(attrs);
    free(record);
    return NULL;
}

static void gatt_sdp_free_record_attr(struct bt_sdp_attribute* attrs)
{
    struct bt_sdp_attribute *srv_cls_attr, *prot_desc_attr;
    struct bt_sdp_data_elem *prot_desc_elem, *handle_desc_elem, *srv_cls_elem;
    uint32_t* srv_hdl;

    if (!attrs) {
        BT_LOGE("Invalid SDP attributes");
        return;
    }

    /* free SDP service handle element data */
    srv_hdl = (uint32_t*)attrs[0].val.data;
    free(srv_hdl);

    /* free service class id element data */
    srv_cls_attr = &attrs[SDP_ATTR_SVCLS_GATT_POS];
    srv_cls_elem = (struct bt_sdp_data_elem*)srv_cls_attr->val.data;
    free(srv_cls_elem);

    /* free start/end handle protocol descriptor element data */
    prot_desc_attr = &attrs[SDP_ATTR_PROT_GATT_POS];
    prot_desc_elem = (struct bt_sdp_data_elem*)prot_desc_attr->val.data;
    handle_desc_elem = (struct bt_sdp_data_elem*)prot_desc_elem[DATA_ELEM_GATT_PROT_DESC_POS].data;
    free(handle_desc_elem);
    free(prot_desc_elem);

    free(attrs);
}

static void gatt_sdp_delete_record(struct bt_sdp_record* record)
{
    sal_gatt_sdp_record_t* gatt_record;

    if (!record) {
        BT_LOGE("Invalid SDP record");
        return;
    }

    for (gatt_record = gatt_sdp_records; gatt_record < gatt_sdp_records + CONFIG_GATT_SERVER_MAX_SERVICES; gatt_record++) {
        if (!gatt_record->srv) {
            continue;
        }

        if (gatt_record->record == record) {
            gatt_record->srv = NULL;
            gatt_record->record = NULL;
            break;
        }
    }

    if (record->attrs) {
        gatt_sdp_free_record_attr(record->attrs);
        record->attrs = NULL;
    }

    free(record);
}

static bt_status_t register_service(bool is_over_br)
{
    int err;
    int service_index;
    struct bt_sdp_record* record;
    bt_status_t status = BT_STATUS_SUCCESS;

    service_index = find_free_service_index();
    if (service_index < 0) {
        BT_LOGE("%s, service full", __func__);
        status = BT_STATUS_FAIL;
        goto out;
    }

    server_svcs[service_index].attrs = server_db + (attr_count - svc_attr_count);
    server_svcs[service_index].attr_count = svc_attr_count;

    err = bt_gatt_service_register(&server_svcs[service_index]);
    if (err) {
        server_svcs[service_index].attrs = NULL;
        server_svcs[service_index].attr_count = 0;
        BT_LOGD("%s, gatt service register %d", __func__, err);
        status = BT_STATUS_FAIL;
        goto out;
    }

    if (!is_over_br) {
        goto out;
    }

    record = gatt_sdp_create_record(&server_svcs[service_index]);

    if (!record) {
        BT_LOGE("Failed to create SDP record");
        goto out;
    }

    err = bt_sdp_register_service(record);
    if (err != 0) {
        BT_LOGE("GATT SDP record register fail");
        gatt_sdp_delete_record(record);
        goto out;
    }

out:
    svc_attr_count = 0U;
    if (status != BT_STATUS_SUCCESS) {
        remove_from_server_db(server_db + (attr_count - svc_attr_count), svc_attr_count);
    }
    return status;
}

static void add_service(gatt_element_t* element, bool is_over_br)
{
    struct bt_gatt_attr* attr_svc;
    union uuid u;
    size_t size;

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&element->uuid.val, element->uuid.type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return;
    }

    size = u.uuid.type == BT_UUID_TYPE_16 ? sizeof(u.u16) : sizeof(u.u128);

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
        BT_LOGE("%s, attr_svc is null", __func__);
        return;
    }
}

static int alloc_characteristic(struct add_characteristic* ch)
{
    struct bt_gatt_attr *attr_chrc, *attr_value;
    struct bt_gatt_chrc* chrc_data;

    /* Add Characteristic Declaration */
    attr_chrc = gatt_db_add(&(struct bt_gatt_attr)BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC, BT_GATT_PERM_READ, bt_gatt_attr_read_chrc, NULL, (&(struct bt_gatt_chrc) {})), sizeof(*chrc_data));
    if (!attr_chrc) {
        return -EINVAL;
    }

    if (!attr_chrc) {
        BT_LOGE("%s, attr_chrc allocation failed", __func__);
        return -EINVAL;
    }

    attr_value = gatt_db_add(&(struct bt_gatt_attr)BT_GATT_ATTRIBUTE(ch->uuid, ch->permissions & GATT_PERM_MASK, read_value, write_value, ch->element), 0);
    if (!attr_value) {
        BT_LOGE("%s, attr_value allocation failed", __func__);
        return -EINVAL;
    }

    chrc_data = attr_chrc->user_data;
    chrc_data->properties = ch->properties;
    chrc_data->uuid = attr_value->uuid;

    ch->char_id = attr_chrc->handle;
    return 0;
}

static uint16_t covert_gatt_permission(uint16_t elem_perm)
{
    int chr_perm = 0;

    if (elem_perm & GATT_PERM_READ) {
        chr_perm |= BT_GATT_PERM_READ;
        if (elem_perm & GATT_PERM_AUTHEN_REQUIRED) {
            chr_perm |= BT_GATT_PERM_READ_AUTHEN;
        }
        if (elem_perm & GATT_PERM_ENCRYPT_REQUIRED) {
            chr_perm |= BT_GATT_PERM_READ_ENCRYPT;
        }
        if (elem_perm & GATT_PERM_MITM_REQUIRED) {
            chr_perm |= BT_GATT_PERM_READ_LESC;
        }
    }

    if (elem_perm & GATT_PERM_WRITE) {
        chr_perm |= BT_GATT_PERM_WRITE;
        if (elem_perm & GATT_PERM_AUTHEN_REQUIRED) {
            chr_perm |= BT_GATT_PERM_WRITE_AUTHEN;
        }
        if (elem_perm & GATT_PERM_ENCRYPT_REQUIRED) {
            chr_perm |= BT_GATT_PERM_WRITE_ENCRYPT;
        }
        if (elem_perm & GATT_PERM_MITM_REQUIRED) {
            chr_perm |= BT_GATT_PERM_WRITE_LESC;
        }
    }

    return chr_perm;
}

static void add_characteristic(gatt_element_t* element)
{
    struct add_characteristic chr = { 0 };
    union uuid u = { 0 };

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&element->uuid.val, element->uuid.type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return;
    }

    chr.permissions = covert_gatt_permission(element->permissions);
    chr.properties = element->properties;
    chr.uuid = &u.uuid;
    chr.attr_length = element->attr_length;
    chr.attr_data = element->attr_data;
    chr.element = element;

    if (alloc_characteristic(&chr)) {
        BT_LOGE("%s, alloc characteristic fail", __func__);
        return;
    }
}

static ssize_t bt_sal_on_ccc_written(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    const void* buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    bt_address_t addr;
    struct gatt_ccc_wrapper* wrapper;
    struct _bt_gatt_ccc* ccc;
    gatt_element_t* element;
    uint16_t value;
    ssize_t ret;
    uint8_t index;

    ret = bt_gatt_attr_write_ccc(conn, attr, buf, len, offset, flags);

    if (ret < 0)
        return ret;

    ccc = (struct _bt_gatt_ccc*)attr->user_data;

    wrapper = CONTAINER_OF(ccc, struct gatt_ccc_wrapper, ccc);
    element = wrapper->element;

    index = bt_conn_index(conn);
    if (index >= CONFIG_BT_MAX_CONN) {
        BT_LOGE("%s, invalid conn index = %u", __func__, index);
        return -EINVAL;
    }

    value = ccc->cfg[index].value;

    bt_sal_get_remote_address(conn, &addr);

    if_gatts_on_received_element_write_request(&addr, GATT_OPS_WRITE_REQUEST,
        element->handle, (uint8_t*)&value, 0, sizeof(value));

    // framework response
    return -EINPROGRESS;
}

static int alloc_descriptor(const struct bt_gatt_attr* attr, struct add_descriptor* desc)
{
    struct bt_gatt_attr* attr_desc;
    struct gatt_ccc_wrapper* ccc_wrapper;
    struct bt_gatt_chrc* chrc = attr->user_data;
    struct _bt_gatt_ccc* ccc;
    struct gatt_user_data user_data = { 0 };

    if (bt_uuid_cmp(desc->uuid, BT_UUID_GATT_CCC) == 0) {

        if (!(chrc->properties & (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE))) {
            BT_LOGE("%s, invald properties:0x%0x", __func__, chrc->properties);
            return -EINVAL;
        }

        /* This memory is freed in remove_service() via attr->user_data */
        ccc_wrapper = zalloc(sizeof(struct gatt_ccc_wrapper));
        if (!ccc_wrapper) {
            BT_LOGE("%s, wrapper alloc failed", __func__);
            return -ENOMEM;
        }

        ccc = &ccc_wrapper->ccc;
        ccc_wrapper->element = desc->element;

        attr_desc = gatt_db_add(
            &(struct bt_gatt_attr) {
                .uuid = BT_UUID_GATT_CCC,
                .perm = desc->permissions & GATT_PERM_MASK,
                .read = bt_gatt_attr_read_ccc,
                .write = bt_sal_on_ccc_written,
                .user_data = ccc },
            0);

        if (!attr_desc) {
            free(ccc_wrapper);
            BT_LOGE("%s attr_desc null", __func__);
            return -EINVAL;
        }

        desc->desc_id = attr_desc->handle;
    } else {
        user_data.element = desc->element;

        attr_desc = gatt_db_add(
            &(struct bt_gatt_attr)BT_GATT_ATTRIBUTE(
                desc->uuid,
                desc->permissions & GATT_PERM_MASK,
                read_value,
                write_value,
                &user_data),
            sizeof(user_data));

        if (!attr_desc) {
            BT_LOGE("%s, generic descriptor allocation failed", __func__);
            return -EINVAL;
        }

        desc->desc_id = attr_desc->handle;
    }

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

    desc.permissions = covert_gatt_permission(element->permissions);
    desc.properties = element->properties;
    desc.uuid = &u.uuid;
    desc.element = element;

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

static void zblue_gatts_mtu_updated_callback(struct bt_conn* conn, uint16_t tx, uint16_t rx)
{
    bt_address_t addr;
    uint16_t att_mtu = MIN(tx, rx);
    uint16_t att_payload = (att_mtu >= 23) ? (att_mtu - 3) : 20;

    bt_sal_get_remote_address(conn, &addr);
    if_gatts_on_mtu_changed(&addr, att_payload);
}

static struct bt_gatt_cb zblue_gatt_callbacks = {
    .att_mtu_updated = zblue_gatts_mtu_updated_callback
};

static bt_status_t do_gatts_disconnect(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data)
{
    struct bt_conn* conn;
    int err;

    conn = bt_conn_lookup_addr_br((bt_addr_t*)bd_addr);
    if (!conn) {
        BT_LOGE("No ACL connection found for address: %s", bt_addr_str(bd_addr));
        return BT_STATUS_FAIL;
    }

    err = bt_att_br_disconnect(conn);
    bt_conn_unref(conn);
    if (err) {
        BT_LOGE("%s, disconnect fail err:%d", __func__, err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void zblue_gatts_connected_callback(struct bt_conn* conn)
{
    if (!bt_conn_get_dst_br(conn)) {
        return;
    }

    bt_address_t addr;
    struct bt_conn_info info;
    bt_conn_info_t* slot;

    bt_conn_get_info(conn, &info);
    bt_addr_set(&addr, info.br.dst->val);

    slot = bt_conn_add(&addr, BT_TRANSPORT_BREDR);
    if (!slot) {
        BT_LOGE("%s, conn slot null", __func__);
        return;
    }

    slot->conn = conn;
    if (!slot->role) {
        slot->role |= GATT_ROLE_SERVER;
    }

    if_gatts_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTED);
    bt_sal_cm_profile_connected_callback(&addr, PROFILE_GATTS, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&addr, PROFILE_GATTS, CONN_ID_DEFAULT, PRIMARY_ADAPTER, do_gatts_disconnect, NULL);
}

static void zblue_gatts_disconnected_callback(struct bt_conn* conn)
{
    if (!bt_conn_get_dst_br(conn)) {
        return;
    }

    bt_address_t addr;
    struct bt_conn_info info;
    bt_conn_info_t* slot;

    bt_conn_get_info(conn, &info);
    bt_addr_set(&addr, info.br.dst->val);

    slot = bt_conn_find(&addr, BT_TRANSPORT_BREDR);
    if (!slot) {
        BT_LOGE("%s, conn slot null", __func__);
        return;
    }

    bt_conn_remove(&addr, BT_TRANSPORT_BREDR);
    if_gatts_on_connection_state_changed(&addr, PROFILE_STATE_DISCONNECTED);
    bt_sal_cm_profile_disconnected_callback(&addr, PROFILE_GATTS, CONN_ID_DEFAULT);
}

static struct bt_att_conn_cb zblue_att_callbacks = {
    .connected = zblue_gatts_connected_callback,
    .disconnected = zblue_gatts_disconnected_callback,
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
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void sal_gatts_elements_callback(void* args)
{
    sal_adapter_req_t* req = args;
    if (!args)
        return;

    switch (req->adpt.attr_op.type) {
    case GATTS_CB_TYPE_ADDED:
        if_gatts_on_elements_added(BT_STATUS_SUCCESS, req->adpt.attr_op.element_id, req->adpt.attr_op.size);
        break;
    case GATTS_CB_TYPE_REMOVED:
        if_gatts_on_elements_removed(BT_STATUS_SUCCESS, req->adpt.attr_op.element_id, req->adpt.attr_op.size);
        break;
    default:
        break;
    }
}

bt_status_t bt_sal_gatt_server_enable(void)
{
    bt_gatt_cb_register(&zblue_gatt_callbacks);
    bt_att_conn_cb_register(&zblue_att_callbacks);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_disable(void)
{
    bt_gatt_cb_unregister(&zblue_gatt_callbacks);
    bt_att_conn_cb_unregister(&zblue_att_callbacks);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_add_elements(gatt_element_t* elements, uint16_t size)
{
    size_t index;
    bt_status_t status;
    sal_adapter_req_t* req;
    bool is_over_bredr = false;

    if (!elements || size == 0)
        return BT_STATUS_PARM_INVALID;

    for (index = 0; index < size; index++) {
        switch (elements[index].type) {
        case GATT_PRIMARY_SERVICE:
        case GATT_SECONDARY_SERVICE:
            /* Workaround: BR/EDR services to be registered over BLE as well */
            if (elements[index].properties & GATT_PROP_EXPOSED_OVER_BREDR) {
                is_over_bredr = true;
                BT_LOGD("BR/EDR service to be registered over BLE");
            }
            add_service(&elements[index], is_over_bredr);
            break;
        case GATT_CHARACTERISTIC:
            add_characteristic(&elements[index]);
            break;
        case GATT_DESCRIPTOR:
            add_descriptor(&elements[index]);
            break;
        default:
            BT_LOGE("%s, unsupported type: %d", __func__, elements[index].type);
            break;
        }
    }

    req = calloc(1, sizeof(sal_adapter_req_t));
    if (!req)
        return BT_STATUS_NOMEM;

    status = register_service(is_over_bredr);
    if (status != BT_STATUS_SUCCESS) {
        free(req);
        return status;
    }

    req->func = sal_gatts_elements_callback;
    req->adpt.attr_op.element_id = elements[0].handle;
    req->adpt.attr_op.size = size;
    req->adpt.attr_op.type = GATTS_CB_TYPE_ADDED;

    return sal_send_req((void*)req);
}

static sal_gatt_sdp_record_t* get_sdp_from_service(struct bt_gatt_service* srv)
{
    sal_gatt_sdp_record_t* record;

    if (!srv) {
        return NULL;
    }

    for (record = gatt_sdp_records; record < gatt_sdp_records + CONFIG_GATT_SERVER_MAX_SERVICES; record++) {
        if (!record->srv) {
            continue;
        }
        if (record->srv == srv) {
            return record;
        }
    }

    BT_LOGW("%s not found sdp_record", __func__);
    return NULL;
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
        if (!bt_uuid_cmp(srv->attrs[0].uuid, BT_UUID_GATT_PRIMARY)) {
            const struct bt_uuid* svc_uuid = (const struct bt_uuid*)srv->attrs[0].user_data;
            if (svc_uuid && !bt_uuid_cmp(svc_uuid, &u.uuid)) {
                return srv;
            }
        }
    }

    BT_LOGE("%s", __func__);
    return NULL;
}

static void remove_service(gatt_element_t* element)
{
    size_t i, count, index;
    struct bt_gatt_attr* start;
    struct bt_gatt_service* svc = get_primary_service_from_element(element);
    sal_gatt_sdp_record_t* record;
    if (!svc) {
        BT_LOGW("%s, service not found", __func__);
        return;
    }

    record = get_sdp_from_service(svc);
    if (record && record->record) {
        bt_sdp_unregister_service(record->record);
        gatt_sdp_delete_record(record->record);
    }

    bt_gatt_service_unregister(svc);

    start = svc->attrs;
    count = svc->attr_count;
    index = start - server_db;

    remove_from_server_db(start, count);

    for (i = 0; i < ARRAY_SIZE(server_svcs); i++) {
        struct bt_gatt_service* s = &server_svcs[i];
        if (!s->attrs) {
            continue;
        }

        if (s->attrs > start) {
            s->attrs -= count;
        }
    }

    svc->attrs = NULL;
    svc->attr_count = 0;

    BT_LOGD("%s, removed service at index %zu, attr_count now %u", __func__, index, attr_count);
}

bt_status_t bt_sal_gatt_server_remove_elements(gatt_element_t* elements, uint16_t size)
{
    if (!elements || size == 0)
        return BT_STATUS_PARM_INVALID;

    uint16_t i;
    sal_adapter_req_t* req;
    char uuid_str[BT_UUID_STR_LENGTH];

    req = calloc(1, sizeof(sal_adapter_req_t));
    if (!req)
        return BT_STATUS_NOMEM;

    for (i = 0; i < size; i++) {
        switch (elements[i].type) {
        case GATT_PRIMARY_SERVICE:
            remove_service(&elements[i]);
            break;
        default:
            bt_uuid_to_string(&elements[i].uuid, uuid_str, sizeof(uuid_str));
            BT_LOGW("%s, unknown type %d, uuid=%s", __func__, elements[i].type, uuid_str);
            break;
        }
    }

    req->func = sal_gatts_elements_callback;
    req->adpt.attr_op.element_id = elements[0].handle;
    req->adpt.attr_op.size = size;
    req->adpt.attr_op.type = GATTS_CB_TYPE_REMOVED;

    return sal_send_req((void*)req);
}

static void STACK_CALL(conn_connect)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t address = { 0 };
    struct bt_conn* conn = NULL;
    int err;

    if (bt_conn_set_role(BT_TRANSPORT_BLE, &req->addr, GATT_ROLE_SERVER) != BT_STATUS_SUCCESS) {
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

static bt_status_t gatts_br_profile_connect(bt_controller_id_t id, bt_address_t* addr, void* user_data)
{
    struct bt_conn* conn;
    int err;

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        BT_LOGE("%s, acl not connected", __func__);
        if_gatts_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_GATTS, CONN_ID_DEFAULT);
        return BT_STATUS_FAIL;
    }

    err = bt_att_br_connect(conn);
    if (err) {
        BT_LOGE("%s, ATT over BR connect failed", __func__);
        goto error;
    }

    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;

error:
    if_gatts_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED);
    bt_sal_cm_profile_disconnected_callback(addr, PROFILE_GATTS, CONN_ID_DEFAULT);
    bt_conn_unref(conn);
    return BT_STATUS_FAIL;
}

static void STACK_CALL(conn_br_connect)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_status_t status;

    if (bt_conn_set_role(BT_TRANSPORT_BREDR, &req->addr, GATT_ROLE_SERVER) != BT_STATUS_SUCCESS) {
        return;
    }

    status = bt_sal_profile_connect_request(&req->addr, PROFILE_GATTS, CONN_ID_DEFAULT, req->id, gatts_br_profile_connect, NULL);
    if (status != BT_STATUS_SUCCESS) {
        bt_conn_remove(&req->addr, BT_TRANSPORT_BREDR);
        BT_LOGE("%s, PROFILE_GATTS connect failed", __func__);
    }
}

bt_status_t bt_sal_gatt_server_connect_bear(bt_controller_id_t id, bt_address_t* addr, ble_addr_type_t addr_type, uint8_t bear_type)
{
    sal_adapter_req_t* req;
    uint8_t type;

    switch (bear_type) {
    case ATT_BEAR_TYPE_LE_ATT:
        req = sal_adapter_req(id, addr, STACK_CALL(conn_connect));
        break;
    case ATT_BEAR_TYPE_BR_ATT:
        req = sal_adapter_req(id, addr, STACK_CALL(conn_br_connect));
        break;
    default:
        BT_LOGE("%s, unsupported bear_type:%d", __func__, bear_type);
        return BT_STATUS_UNSUPPORTED;
    }

    if (!req) {
        BT_LOGE("%s, req null", __func__);
        return BT_STATUS_NOMEM;
    }

    if (bear_type == ATT_BEAR_TYPE_BR_ATT) {
        /* ATT_BEAR_TYPE_BR_ATT Skip addr_type convert */
        return sal_send_req(req);
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

static void STACK_CALL(conn_cancel)(void* args)
{
    sal_adapter_req_t* req = args;
    struct bt_conn* conn;
    int err;

    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGE("%s, conn null", __func__);
        goto br_disconn;
    }

    err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        BT_LOGE("%s, disconnect fail err:%d", __func__, err);
        return;
    }

br_disconn:
    bt_sal_profile_disconnect_request(&req->addr, PROFILE_GATTS, CONN_ID_DEFAULT, PRIMARY_ADAPTER, do_gatts_disconnect, NULL);
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

static void zblue_conn_send_response(void* args)
{
    sal_gatts_rsp_req_t* req = args;
    struct bt_conn* conn;
    bt_conn_info_t* info;
    uint16_t handle;
    uint8_t op_type;
    int err;

    /* FIXME: If the LE address matches the BREDR address, only the LE connection will send rsp. */
    conn = get_le_conn_from_addr(&req->addr);
    if (!conn) {
        BT_LOGW("%s, le conn null", __func__);
        info = bt_conn_find(&req->addr, BT_TRANSPORT_BREDR);
        conn = info ? info->conn : NULL;
        if (!conn) {
            BT_LOGE("%s, br conn null", __func__);
            return;
        }
    }

    handle = REQUEST_ID_HANDLE(req->request_id);
    op_type = REQUEST_ID_OP_TYPE(req->request_id);
    switch (op_type) {
    case GATT_OPS_READ_REQUEST:
        err = bt_gatt_send_read_rsp(conn, 0, handle, req->value, req->length);
        break;
    case GATT_OPS_WRITE_REQUEST:
        err = bt_gatt_send_write_rsp(conn, 0, handle);
        break;
    default:
        BT_LOGE("%s, unsupported op_type:%d", __func__, op_type);
        return;
    }
    if (err) {
        BT_LOGE("%s, send rsp failed err:%d", __func__, err);
    }
}

bt_status_t bt_sal_gatt_server_send_response(bt_controller_id_t id, bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length)
{
    sal_gatts_rsp_req_t* req;
    uint8_t op_type;

    if (!addr || request_id == REQUEST_ID_NORSP) {
        return BT_STATUS_PARM_INVALID;
    }

    op_type = REQUEST_ID_OP_TYPE(request_id);
    if (op_type == GATT_OPS_READ_REQUEST && !value) {
        return BT_STATUS_PARM_INVALID;
    }

    req = malloc(sizeof(sal_gatts_rsp_req_t) + length);
    if (!req) {
        BT_LOGE("%s, malloc fail", __func__);
        return BT_STATUS_NOMEM;
    }

    req->id = id;
    memcpy(&req->addr, addr, sizeof(bt_address_t));
    req->func = zblue_conn_send_response;
    req->request_id = request_id;
    req->length = length;
    if (length > 0 && value) {
        memcpy(req->value, value, length);
    }

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void send_notification_result(struct bt_conn* conn, void* user_data)
{
    gatt_element_t* element = (gatt_element_t*)user_data;
    bt_address_t addr;

    if (!element) {
        BT_LOGE("%s, element is NULL", __func__);
        return;
    }

    bt_sal_get_remote_address(conn, &addr);

    if_gatts_on_notification_sent(&addr, element->handle, GATT_STATUS_SUCCESS);
}

static uint8_t gatt_send_notification(const struct bt_gatt_attr* attr, uint16_t handle, void* user_data)
{
    struct gatt_server_context* context = user_data;
    struct bt_gatt_notify_params params;
    union uuid u;

    if (!bt_uuid_create(&u.uuid, (uint8_t*)&context->uuid->val, context->uuid->type)) {
        BT_LOGE("%s, uuid convert fail", __func__);
        return BT_GATT_ITER_STOP;
    }

    if (bt_uuid_cmp(attr->uuid, &u.uuid)) {
        return BT_GATT_ITER_CONTINUE;
    }

    memset(&params, 0, sizeof(params));

    params.attr = attr;
    params.data = context->value;
    params.len = context->length;
    params.func = send_notification_result;
    params.user_data = context->element;
#if defined(CONFIG_BT_EATT)
    params.chan_opt = BT_ATT_CHAN_OPT_NONE;
#endif /* CONFIG_BT_EATT */

    bt_gatt_notify_cb(context->conn, &params);

    return BT_GATT_ITER_STOP;
}

static uint8_t gatt_send_indication(const struct bt_gatt_attr* attr, uint16_t handle, void* user_data);

static void zblue_conn_send_notification(void* args)
{
    sal_gatts_notify_req_t* req = args;
    struct gatt_server_context context = {
        .addr = &req->addr,
        .uuid = &req->element->uuid,
        .value = req->value,
        .length = req->length,
        .element = req->element,
    };

    context.conn = get_le_conn_from_addr(&req->addr);
    if (!context.conn) {
        bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BREDR);
        context.conn = info ? info->conn : NULL;
        if (!context.conn) {
            BT_LOGE("%s, conn null", __func__);
            return;
        }
    }

    bt_gatt_foreach_attr(0x0001, 0xffff, gatt_send_notification, &context);
}

static void zblue_conn_send_indication(void* args)
{
    sal_gatts_notify_req_t* req = args;
    struct gatt_server_context context = {
        .addr = &req->addr,
        .uuid = &req->element->uuid,
        .value = req->value,
        .length = req->length,
        .element = req->element,
    };

    context.conn = get_le_conn_from_addr(&req->addr);
    if (!context.conn) {
        bt_conn_info_t* info = bt_conn_find(&req->addr, BT_TRANSPORT_BREDR);
        context.conn = info ? info->conn : NULL;
        if (!context.conn) {
            BT_LOGE("%s, conn null", __func__);
            return;
        }
    }

    bt_gatt_foreach_attr(0x0001, 0xffff, gatt_send_indication, &context);
}

bt_status_t bt_sal_gatt_server_send_notification(bt_controller_id_t id, bt_address_t* addr, gatt_element_t* element, uint8_t* value, uint16_t length)
{
    sal_gatts_notify_req_t* req = malloc(sizeof(sal_gatts_notify_req_t) + length);

    if (!req) {
        BT_LOGE("%s, malloc fail", __func__);
        return BT_STATUS_NOMEM;
    }

    req->id = id;
    memcpy(&req->addr, addr, sizeof(bt_address_t));
    req->func = zblue_conn_send_notification;
    req->element = element;
    req->length = length;
    memcpy(req->value, value, length);

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        free(req);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void send_indication_destory(struct bt_gatt_indicate_params* params)
{
    BT_LOGD("%s", __func__);
    free(params);
}

static void send_indication_result(struct bt_conn* conn, struct bt_gatt_indicate_params* params, uint8_t err)
{
    gatt_element_t* element;
    bt_address_t addr;
    bt_status_t status = GATT_STATUS_SUCCESS;

    if (!params || !params->attr || !params->attr->user_data) {
        BT_LOGE("%s, params or attr is NULL", __func__);
        return;
    }

    element = (gatt_element_t*)params->attr->user_data;

    if (!element) {
        BT_LOGE("%s, element is NULL", __func__);
        return;
    }

    bt_sal_get_remote_address(conn, &addr);

    if (err) {
        BT_LOGE("%s, send indication failed for handle:0x%04x", __func__, element->handle);
        status = GATT_STATUS_FAILURE;
    }

    if_gatts_on_notification_sent(&addr, element->handle, status);
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

bt_status_t bt_sal_gatt_server_send_indication(bt_controller_id_t id, bt_address_t* addr, gatt_element_t* element, uint8_t* value, uint16_t length)
{
    sal_gatts_notify_req_t* req = malloc(sizeof(sal_gatts_notify_req_t) + length);

    if (!req) {
        BT_LOGE("%s, malloc fail", __func__);
        return BT_STATUS_NOMEM;
    }

    req->id = id;
    memcpy(&req->addr, addr, sizeof(bt_address_t));
    req->func = zblue_conn_send_indication;
    req->element = element;
    req->length = length;
    memcpy(req->value, value, length);

    if (!service_loop_work((void*)req, sal_invoke_async, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        free(req);
        return BT_STATUS_FAIL;
    }

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

void bt_sal_gatt_server_connection_state_changed_callback(bt_controller_id_t id, bt_address_t* addr, profile_connection_state_t state)
{
    if_gatts_on_connection_state_changed(addr, state);
}

#endif /* CONFIG_BLUETOOTH_GATT_SERVER */
