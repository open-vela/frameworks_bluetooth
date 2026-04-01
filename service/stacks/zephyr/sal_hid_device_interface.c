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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/bluetooth/classic/hid_device.h>
#include <zephyr/bluetooth/classic/sdp.h>

#include "bt_addr.h"
#include "hid_device_service.h"
#include "utils/log.h"

#include "sal_connection_manager.h"
#include "sal_hid_device_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "service_loop.h"

#define BT_HID_DEVICE_VERSION 0x0101
#define BT_HID_PARSER_VERSION 0x0111
#define BT_HID_DEVICE_SUBCLASS 0xc0
#define BT_HID_DEVICE_COUNTRY_CODE 0x21
#define BT_HID_PROTO_INTERRUPT 0x0013

#define BT_HID_LANG_ID_ENGLISH 0x0409
#define BT_HID_LANG_ID_OFFSET 0x0100

#define BT_HID_SUPERVISION_TIMEOUT 1000
#define BT_HID_MAX_LATENCY 240
#define BT_HID_MIN_LATENCY 0

#define BT_HID_DEVICE_REPORT_DESC_SIZE 256
#define BT_HID_DEVICE_DESC_VALUE_INDEX 3

typedef struct sal_hid_connection {
    struct bt_hid_device* hid_device;
    bt_address_t addr;
    struct bt_conn* conn;
    bool le_hid;
} sal_hid_connection_t;

typedef struct sal_bt_hid_device_mgr {
    bool registered;
    pthread_mutex_t mutex;
    bt_list_t* connections;
    struct bt_sdp_record* record;
    uint8_t* description;
} sal_bt_hid_device_mgr_t;

static struct bt_sdp_attribute hid_attrs_template[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                BT_SDP_ARRAY_16(BT_SDP_HID_SVCLASS) })),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_HID) }) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_HID) }) }, )),
    BT_SDP_LIST(BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_HID_SVCLASS) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_HID_DEVICE_VERSION) }) })),
    BT_SDP_LIST(
        BT_SDP_ATTR_ADD_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 15),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 13),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                        BT_SDP_DATA_ELEM_LIST(
                            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                                BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) },
                            { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                                BT_SDP_ARRAY_16(BT_HID_PROTO_INTERRUPT) }) },
                    { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
                        BT_SDP_DATA_ELEM_LIST(
                            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                                BT_SDP_ARRAY_16(BT_SDP_PROTO_HID) }) }) })),
    BT_SDP_SERVICE_NAME("HID CONTROL"),
    { BT_SDP_ATTR_HID_PARSER_VERSION,
        { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
            BT_SDP_ARRAY_16(BT_HID_PARSER_VERSION) } },
    { BT_SDP_ATTR_HID_DEVICE_SUBCLASS,
        { BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
            BT_SDP_ARRAY_16(BT_HID_DEVICE_SUBCLASS) } },
    { BT_SDP_ATTR_HID_COUNTRY_CODE,
        { BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
            BT_SDP_ARRAY_16(BT_HID_DEVICE_COUNTRY_CODE) } },
    { BT_SDP_ATTR_HID_VIRTUAL_CABLE,
        { BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
            BT_SDP_ARRAY_8(0x01) } },
    { BT_SDP_ATTR_HID_RECONNECT_INITIATE,
        { BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
            BT_SDP_ARRAY_8(0x01) } },
    BT_SDP_LIST(
        BT_SDP_ATTR_HID_DESCRIPTOR_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, BT_HID_DEVICE_REPORT_DESC_SIZE + 8),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ16, BT_HID_DEVICE_REPORT_DESC_SIZE + 5),
                BT_SDP_DATA_ELEM_LIST(
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
                        BT_SDP_ARRAY_8(0x22),
                    },
                    {
                        BT_SDP_TYPE_SIZE_VAR(BT_SDP_TEXT_STR16,
                            BT_HID_DEVICE_REPORT_DESC_SIZE),
                        NULL,
                    }) })),
    BT_SDP_LIST(
        BT_SDP_ATTR_HID_LANG_ID_BASE_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
            {
                BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_HID_LANG_ID_ENGLISH),
                    },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_HID_LANG_ID_OFFSET),
                    }),
            })),
    { BT_SDP_ATTR_HID_BOOT_DEVICE,
        { BT_SDP_TYPE_SIZE(BT_SDP_BOOL),
            BT_SDP_ARRAY_8(0x01) } },
    { BT_SDP_ATTR_HID_SUPERVISION_TIMEOUT,
        { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
            BT_SDP_ARRAY_16(BT_HID_SUPERVISION_TIMEOUT) } },
    { BT_SDP_ATTR_HID_MAX_LATENCY,
        { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
            BT_SDP_ARRAY_16(BT_HID_MAX_LATENCY) } },
    { BT_SDP_ATTR_HID_MIN_LATENCY,
        { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
            BT_SDP_ARRAY_16(BT_HID_MIN_LATENCY) } },
};

static sal_bt_hid_device_mgr_t g_hid_device_mgr = {
    .registered = false,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .connections = NULL
};

static bt_status_t hid_disconnect_handler(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data);

static inline void hid_conn_lock(void)
{
    pthread_mutex_lock(&g_hid_device_mgr.mutex);
}

static inline void hid_conn_unlock(void)
{
    pthread_mutex_unlock(&g_hid_device_mgr.mutex);
}

static sal_hid_connection_t* hid_find_connection_by_address(bt_address_t* addr)
{
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;

    if (hid_mgr->connections == NULL) {
        return NULL;
    }

    for (bt_list_node_t* node = bt_list_head(hid_mgr->connections); node != NULL; node = bt_list_next(hid_mgr->connections, node)) {
        sal_hid_connection_t* hid_conn = (sal_hid_connection_t*)bt_list_node(node);

        if (bt_addr_compare(&hid_conn->addr, addr) == 0) {
            return hid_conn;
        }
    }

    return NULL;
}

static sal_hid_connection_t* hid_find_connections_by_device(struct bt_hid_device* hid)
{
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;

    if (hid_mgr->connections == NULL) {
        return NULL;
    }

    for (bt_list_node_t* node = bt_list_head(hid_mgr->connections); node != NULL; node = bt_list_next(hid_mgr->connections, node)) {
        sal_hid_connection_t* hid_conn = (sal_hid_connection_t*)bt_list_node(node);

        if (hid_conn->hid_device == hid) {
            return hid_conn;
        }
    }

    return NULL;
}

static sal_hid_connection_t* hid_connection_new(bt_address_t* addr, struct bt_conn* conn)
{
    sal_hid_connection_t* hid_conn;

    hid_conn = zalloc(sizeof(sal_hid_connection_t));
    if (!hid_conn) {
        BT_LOGE("Failed to allocate memory for HID connection");
        return NULL;
    }

    memcpy(&hid_conn->addr, addr, sizeof(bt_address_t));
    hid_conn->conn = conn;
    bt_conn_ref(conn);

    return hid_conn;
}

static void hid_connection_free(void* data)
{
    sal_hid_connection_t* hid_conn = (sal_hid_connection_t*)data;

    if (!data) {
        return;
    }

    if (hid_conn->conn) {
        bt_conn_unref(hid_conn->conn);
        hid_conn->conn = NULL;
    }

    free(hid_conn);
}

static struct bt_sdp_record* hid_sdp_create_record(const uint8_t* description, uint16_t length)
{
    size_t desc_len;
    uint8_t* desc;
    struct bt_sdp_record* record;
    size_t attrs_count;
    struct bt_sdp_attribute* attrs;
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;

    if (!description) {
        BT_LOGE("description is NULL");
        return NULL;
    }

    desc_len = length - BT_HID_DEVICE_DESC_VALUE_INDEX;
    if (desc_len < 1) {
        BT_LOGE("Invalid description length: %zu", desc_len);
        return NULL;
    }

    if (hid_mgr->description) {
        BT_LOGE("HID description already exists");
        return NULL;
    }

    if (hid_mgr->record) {
        BT_LOGE("HID SDP record already exists");
        return NULL;
    }

    desc = zalloc(desc_len);
    if (!desc) {
        BT_LOGE("Failed to allocate memory for description");
        return NULL;
    }

    memcpy(desc, description + BT_HID_DEVICE_DESC_VALUE_INDEX, desc_len);

    BT_DUMPBUFFER("HID Description", desc, desc_len);

    record = zalloc(sizeof(struct bt_sdp_record));
    if (!record) {
        BT_LOGE("Failed to allocate memory for SDP record");
        free(desc);
        return NULL;
    }

    attrs = zalloc(sizeof(hid_attrs_template));
    if (!attrs) {
        BT_LOGE("Failed to allocate memory for SDP attributes");
        free(desc);
        free(record);
        return NULL;
    }

    attrs_count = ARRAY_SIZE(hid_attrs_template);
    memcpy(attrs, hid_attrs_template, sizeof(hid_attrs_template));

    for (int i = 0; i < attrs_count; i++) {
        if (attrs[i].id == BT_SDP_ATTR_HID_DESCRIPTOR_LIST) {
            struct bt_sdp_data_elem* element = (struct bt_sdp_data_elem*)&attrs[i].val;

            element[0].type = BT_SDP_SEQ16;
            element[0].data_size = desc_len + 8;
            element[0].total_size = BIT((BT_SDP_SEQ16 & BT_SDP_SIZE_DESC_MASK) - BT_SDP_SIZE_INDEX_OFFSET) + element[0].data_size + 1;

            element = (struct bt_sdp_data_elem*)element[0].data;
            if (!element) {
                BT_LOGE("HID Descriptor List element1 is NULL");
                goto fail;
            }

            element[0].type = BT_SDP_SEQ16;
            element[0].data_size = desc_len + 5;
            element[0].total_size = BIT((BT_SDP_SEQ16 & BT_SDP_SIZE_DESC_MASK) - BT_SDP_SIZE_INDEX_OFFSET) + element[0].data_size + 1;

            element = (struct bt_sdp_data_elem*)element[0].data;
            if (!element) {
                BT_LOGE("HID Descriptor List element2 is NULL");
                goto fail;
            }

            element[1].type = BT_SDP_TEXT_STR16;
            element[1].data_size = desc_len;
            element[1].total_size = BIT((BT_SDP_TEXT_STR16 & BT_SDP_SIZE_DESC_MASK) - BT_SDP_SIZE_INDEX_OFFSET) + element[1].data_size + 1;
            element[1].data = desc;

            break;
        }
    }

    record->attr_count = attrs_count;
    record->attrs = attrs;

    hid_mgr->record = record;
    hid_mgr->description = desc;

    return record;

fail:
    free(attrs);
    free(desc);
    free(record);
    return NULL;
}

static void hid_sdp_delete_record(struct bt_sdp_record* record)
{
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;

    if ((!record) || (record != hid_mgr->record)) {
        BT_LOGE("Invalid SDP record");
        return;
    }

    if (record->attrs) {
        free(record->attrs);
        record->attrs = NULL;
    }

    if (hid_mgr->description) {
        free(hid_mgr->description);
        hid_mgr->description = NULL;
    }

    if (hid_mgr->record) {
        free(hid_mgr->record);
        hid_mgr->record = NULL;
    }
}

static void hid_accept_callback(struct bt_hid_device* hid)
{
    sal_hid_connection_t* hid_conn;
    bt_address_t addr;

    if (!hid) {
        BT_LOGE("hid not found");
        return;
    }

    BT_LOGD("hid:%p accept", hid);

    if (bt_sal_get_remote_address(hid->conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, failed to get remote address", __func__);
        return;
    }

    hid_conn = hid_connection_new(&addr, hid->conn);
    if (!hid_conn) {
        BT_LOGE("Failed to create HID connection");
        return;
    }

    hid_conn->hid_device = hid;

    hid_conn_lock();
    bt_list_add_tail(g_hid_device_mgr.connections, hid_conn);
    hid_conn_unlock();

    hid_device_on_connection_state_changed(&hid_conn->addr, false, PROFILE_STATE_CONNECTING);
}

static void hid_connect_callback(struct bt_hid_device* hid)
{
    sal_hid_connection_t* hid_conn;

    BT_LOGD("hid:%p connected", hid);

    hid_conn_lock();
    hid_conn = hid_find_connections_by_device(hid);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("hid:%p not found", hid);
        return;
    }

    hid_conn_unlock();
    hid_device_on_connection_state_changed(&hid_conn->addr, false, PROFILE_STATE_CONNECTED);

    bt_sal_cm_profile_connected_callback(&hid_conn->addr, PROFILE_HID_DEV, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&hid_conn->addr, PROFILE_HID_DEV, CONN_ID_DEFAULT, PRIMARY_ADAPTER, hid_disconnect_handler, hid_conn);
}

static void hid_disconnected_callback(struct bt_hid_device* hid)
{
    sal_hid_connection_t* hid_conn;

    BT_LOGD("hid:%p disconnected", hid);

    hid_conn_lock();
    hid_conn = hid_find_connections_by_device(hid);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("hid:%p not found", hid);
        return;
    }

    hid_device_on_connection_state_changed(&hid_conn->addr, false, PROFILE_STATE_DISCONNECTED);
    bt_sal_cm_profile_disconnected_callback(&hid_conn->addr, PROFILE_HID_DEV, CONN_ID_DEFAULT);
    bt_list_remove(g_hid_device_mgr.connections, hid_conn);
    hid_conn_unlock();
}

void hid_set_report_callback(struct bt_hid_device* hid, const uint8_t* data, uint16_t len)
{
    sal_hid_connection_t* hid_conn;

    BT_LOGD("hid:%p set report cb, len:%d", hid, len);

    hid_conn_lock();
    hid_conn = hid_find_connections_by_device(hid);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("hid:%p not found", hid);
        return;
    }

    hid_conn_unlock();
    hid_device_on_set_report(&hid_conn->addr, data[0], len - 1, (uint8_t*)&data[1]);
}

void hid_get_report_callback(struct bt_hid_device* hid, const uint8_t* data, uint16_t len)
{
    sal_hid_connection_t* hid_conn;

    BT_LOGD("hid:%p get report cb, len:%d", hid, len);

    hid_conn_lock();
    hid_conn = hid_find_connections_by_device(hid);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("hid:%p not found", hid);
        return;
    }

    hid_conn_unlock();
    hid_device_on_get_report(&hid_conn->addr, data[0], data[1], data[2]);
}

void hid_set_protocol_callback(struct bt_hid_device* hid, uint8_t protocol)
{
    BT_LOGD("hid:%p set protocol:%d, ", hid, protocol);
}

typedef struct {
    bt_address_t addr;
    uint8_t type;
    uint8_t data[BT_HID_REPORT_DATA_LEN];
    uint16_t len;
} hid_send_data_param_t;

typedef struct {
    bt_address_t addr;
    hid_status_error_t param;
} hid_device_cmd_param_t;

/**
 * Lookup hid_device pointer safely under lock by address.
 * Returns NULL if the connection has already been torn down.
 */
static struct bt_hid_device* hid_device_lookup_by_addr(bt_address_t* addr)
{
    sal_hid_connection_t* hid_conn;
    struct bt_hid_device* hid_device = NULL;

    hid_conn_lock();
    hid_conn = hid_find_connection_by_address(addr);
    if (hid_conn) {
        hid_device = hid_conn->hid_device;
    }
    hid_conn_unlock();

    return hid_device;
}

static void do_hid_send_ctrl_data(service_work_t* work, void* userdata)
{
    hid_send_data_param_t* params = (hid_send_data_param_t*)userdata;
    struct bt_hid_device* hid_device;
    int ret;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    hid_device = hid_device_lookup_by_addr(&params->addr);
    if (!hid_device) {
        BT_LOGW("%s, connection already released, skip", __func__);
        free(params);
        return;
    }

    ret = Z_API(bt_hid_device_send_ctrl_data)(hid_device, params->type,
        params->data, params->len);
    if (ret < 0) {
        BT_LOGE("%s, send ctrl data failed: %d", __func__, ret);
    }

    free(params);
}

static void do_hid_send_intr_data(service_work_t* work, void* userdata)
{
    hid_send_data_param_t* params = (hid_send_data_param_t*)userdata;
    struct bt_hid_device* hid_device;
    int ret;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    hid_device = hid_device_lookup_by_addr(&params->addr);
    if (!hid_device) {
        BT_LOGW("%s, connection already released, skip", __func__);
        free(params);
        return;
    }

    ret = Z_API(bt_hid_device_send_intr_data)(hid_device, params->type,
        params->data, params->len);
    if (ret < 0) {
        BT_LOGE("%s, send intr data failed: %d", __func__, ret);
    }

    free(params);
}

static void do_hid_report_error(service_work_t* work, void* userdata)
{
    hid_device_cmd_param_t* params = (hid_device_cmd_param_t*)userdata;
    struct bt_hid_device* hid_device;
    int ret;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    hid_device = hid_device_lookup_by_addr(&params->addr);
    if (!hid_device) {
        BT_LOGW("%s, connection already released, skip", __func__);
        free(params);
        return;
    }

    ret = Z_API(bt_hid_device_report_error)(hid_device, params->param);
    if (ret < 0) {
        BT_LOGE("%s, report error failed: %d", __func__, ret);
    }

    free(params);
}

static void do_hid_virtual_unplug(service_work_t* work, void* userdata)
{
    hid_device_cmd_param_t* params = (hid_device_cmd_param_t*)userdata;
    struct bt_hid_device* hid_device;
    int ret;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    hid_device = hid_device_lookup_by_addr(&params->addr);
    if (!hid_device) {
        BT_LOGW("%s, connection already released, skip", __func__);
        free(params);
        return;
    }

    ret = Z_API(bt_hid_device_virtual_unplug)(hid_device);
    if (ret < 0) {
        BT_LOGE("%s, virtual unplug failed: %d", __func__, ret);
    }

    free(params);
}

void hid_get_protocol_callback(struct bt_hid_device* hid)
{
    hid_send_data_param_t* params;
    sal_hid_connection_t* hid_conn;

    BT_LOGD("hid:%p get protocol", hid);

    hid_conn_lock();
    hid_conn = hid_find_connections_by_device(hid);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("%s, hid:%p connection not found", __func__, hid);
        return;
    }

    params = zalloc(sizeof(hid_send_data_param_t));
    if (!params) {
        hid_conn_unlock();
        BT_LOGE("%s, Failed to allocate memory", __func__);
        return;
    }

    memcpy(&params->addr, &hid_conn->addr, sizeof(bt_address_t));
    params->type = BT_HID_REPORT_TYPE_OTHER;
    params->data[0] = BT_HID_PROTOCOL_REPORT_MODE;
    params->len = sizeof(uint8_t);
    hid_conn_unlock();

    if (!service_loop_work(params, do_hid_send_ctrl_data, NULL)) {
        BT_LOGE("%s, service_loop_work failed", __func__);
        free(params);
    }
}

void hid_intr_data_callback(struct bt_hid_device* hid, uint8_t* data, uint16_t len)
{
    sal_hid_connection_t* hid_conn;

    BT_LOGD("hid:%p intr data, len:%d", hid, len);

    hid_conn_lock();
    hid_conn = hid_find_connections_by_device(hid);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("hid:%p not found", hid);
        return;
    }

    hid_conn_unlock();
    hid_device_on_receive_report(&hid_conn->addr, data[0], len - 1, &data[1]);
}

void hid_vc_unplug_callback(struct bt_hid_device* hid)
{
    sal_hid_connection_t* hid_conn;
    BT_LOGD("hid:%p unplug", hid);

    hid_conn_lock();
    hid_conn = hid_find_connections_by_device(hid);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("hid:%p not found", hid);
        return;
    }

    hid_conn_unlock();
    hid_device_on_virtual_cable_unplug(&hid_conn->addr);
}

static struct bt_hid_device_cb hid_callback = {
    .accept = hid_accept_callback,
    .connected = hid_connect_callback,
    .disconnected = hid_disconnected_callback,
    .set_report = hid_set_report_callback,
    .get_report = hid_get_report_callback,
    .set_protocol = hid_set_protocol_callback,
    .get_protocol = hid_get_protocol_callback,
    .intr_data = hid_intr_data_callback,
    .vc_unplug = hid_vc_unplug_callback,
};

bt_status_t bt_sal_hid_device_init()
{
    int err;
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;

    err = Z_API(bt_hid_device_register)(&hid_callback);
    if (err != 0) {
        BT_LOGE("HID register cb fail,err:%d", err);
        return BT_STATUS_FAIL;
    }

    hid_mgr->connections = bt_list_new(hid_connection_free);
    if (!hid_mgr->connections) {
        BT_LOGE("Failed to create HID connections list");
        return BT_STATUS_NO_RESOURCES;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_register_app(hid_device_sdp_settings_t* sdp, bool le_hid)
{
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;
    int err;
    struct bt_sdp_record* record;

    if (hid_mgr->registered) {
        BT_LOGE("HID already registered");
        return BT_STATUS_FAIL;
    }

    record = hid_sdp_create_record(sdp->hids_info.dsc_list, sdp->hids_info.dsc_list_length);
    if (!record) {
        BT_LOGE("Failed to create HID SDP record");
        return BT_STATUS_FAIL;
    }

    err = bt_sdp_register_service(record);
    if (err != 0) {
        BT_LOGE("HID SDP record register fail");
        hid_sdp_delete_record(record);
        return BT_STATUS_FAIL;
    }

    hid_mgr->registered = true;

    hid_device_on_app_state_changed(HID_APP_STATE_REGISTERED);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_unregister_app(void)
{
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;

    if (!hid_mgr->registered) {
        BT_LOGE("HID not registered");
        return BT_STATUS_FAIL;
    }

    if (hid_mgr->record) {
        bt_sdp_unregister_service(hid_mgr->record);
        hid_sdp_delete_record(hid_mgr->record);
    }

    hid_mgr->registered = false;
    hid_device_on_app_state_changed(HID_APP_STATE_NOT_REGISTERED);

    return BT_STATUS_SUCCESS;
}

static bt_status_t hid_connect_handler(bt_controller_id_t id, bt_address_t* addr, void* user_data)
{
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;
    struct bt_conn* conn;
    sal_hid_connection_t* hid_conn;
    struct bt_hid_device* hid_device;
    bt_status_t status;

    hid_device_on_connection_state_changed(addr, false, PROFILE_STATE_CONNECTING);

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        status = BT_STATUS_FAIL;
        goto out;
    }

    hid_conn = hid_connection_new(addr, conn);
    bt_conn_unref(conn);

    if (!hid_conn) {
        BT_LOGE("Failed to allocate memory for HID connection");
        status = BT_STATUS_NOMEM;
        goto out;
    }

    BT_LOGD("HID device Connecting, addr:%s", bt_addr_bastr(addr));
    hid_device = Z_API(bt_hid_device_connect)(hid_conn->conn);
    if (!hid_device) {
        BT_LOGE("Failed to connect HID device");
        status = BT_STATUS_FAIL;
        goto out_con;
    }

    hid_conn->hid_device = hid_device;

    hid_conn_lock();
    bt_list_add_tail(hid_mgr->connections, hid_conn);
    hid_conn_unlock();

    return BT_STATUS_SUCCESS;

out_con:
    hid_connection_free(hid_conn);

out:
    hid_device_on_connection_state_changed(addr, false, PROFILE_STATE_DISCONNECTED);
    bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HID_DEV, CONN_ID_DEFAULT);
    return status;
}

bt_status_t bt_sal_hid_device_connect(bt_address_t* addr)
{
    sal_hid_connection_t* hid_conn;
    bt_status_t status;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s, addr:%s", __func__, addr_str);

    hid_conn_lock();
    hid_conn = hid_find_connection_by_address(addr);
    if (hid_conn) {
        hid_conn_unlock();
        BT_LOGE("HID connection already exists for addr: %s", addr_str);
        return BT_STATUS_FAIL;
    }

    hid_conn_unlock();

    status = bt_sal_profile_connect_request(addr, PROFILE_HID_DEV, CONN_ID_DEFAULT, PRIMARY_ADAPTER, hid_connect_handler, NULL);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("Failed to connect HID profile: %d", status);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t hid_disconnect_handler(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data)
{
    sal_hid_connection_t* hid_conn;
    int ret;

    hid_conn = hid_find_connection_by_address(bd_addr);
    if (!hid_conn) {
        BT_LOGE("No HID connection found for addr");
        return BT_STATUS_FAIL;
    }

    BT_LOGD("HID disconnect handler, addr:%s", bt_addr_bastr(bd_addr));

    ret = Z_API(bt_hid_device_disconnect)(hid_conn->hid_device);
    if (ret < 0) {
        BT_LOGE("Failed to disconnect HID device: %d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hid_device_disconnect(bt_address_t* addr)
{
    sal_hid_connection_t* hid_conn;

    hid_conn_lock();
    hid_conn = hid_find_connection_by_address(addr);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("No HID connection found for addr");
        return BT_STATUS_PARM_INVALID;
    }

    hid_conn_unlock();

    return bt_sal_profile_disconnect_request(&hid_conn->addr, PROFILE_HID_DEV, CONN_ID_DEFAULT, PRIMARY_ADAPTER, hid_disconnect_handler, NULL);
}

void bt_sal_hid_device_cleanup()
{
    sal_bt_hid_device_mgr_t* hid_mgr = &g_hid_device_mgr;
    bt_list_node_t* node;

    hid_conn_lock();
    for (node = bt_list_head(hid_mgr->connections); node != NULL; node = bt_list_next(hid_mgr->connections, node)) {
        sal_hid_connection_t* hid_conn = (sal_hid_connection_t*)bt_list_node(node);

        hid_conn_unlock();
        bt_sal_hid_device_disconnect(&hid_conn->addr);
        hid_conn_lock();
    }

    hid_conn_unlock();

    bt_sal_hid_device_unregister_app();
    hid_sdp_delete_record(hid_mgr->record);

    hid_conn_lock();
    bt_list_free(hid_mgr->connections);
    hid_mgr->connections = NULL;
    hid_conn_unlock();
}

/*
 * NOTE: Async via service_loop_work(). Returns BT_STATUS_SUCCESS once
 * the work is queued, not when the Z_API call completes. Actual send
 * failures are logged in the worker callback.
 */
bt_status_t bt_sal_hid_device_get_report_response(bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
{
    sal_hid_connection_t* hid_conn;
    hid_send_data_param_t* params;

    if (rpt_size < 0 || rpt_size > BT_HID_REPORT_DATA_LEN) {
        BT_LOGE("Invalid report size %d (max %d)", rpt_size, BT_HID_REPORT_DATA_LEN);
        return BT_STATUS_PARM_INVALID;
    }

    if (rpt_size > 0 && !rpt_data) {
        BT_LOGE("rpt_data is NULL but rpt_size is %d", rpt_size);
        return BT_STATUS_PARM_INVALID;
    }

    hid_conn_lock();
    hid_conn = hid_find_connection_by_address(addr);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("No HID connection found for addr");
        return BT_STATUS_PARM_INVALID;
    }

    params = zalloc(sizeof(hid_send_data_param_t));
    if (!params) {
        hid_conn_unlock();
        BT_LOGE("Failed to allocate memory");
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, &hid_conn->addr, sizeof(bt_address_t));
    params->type = rpt_type;
    if (rpt_data && rpt_size > 0) {
        memcpy(params->data, rpt_data, rpt_size);
    }
    params->len = (uint16_t)rpt_size;
    hid_conn_unlock();

    if (!service_loop_work(params, do_hid_send_ctrl_data, NULL)) {
        BT_LOGE("service_loop_work failed");
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

/* NOTE: Async via service_loop_work(). See bt_sal_hid_device_get_report_response. */
bt_status_t bt_sal_hid_device_report_error(bt_address_t* addr, hid_status_error_t error)
{
    sal_hid_connection_t* hid_conn;
    hid_device_cmd_param_t* params;

    hid_conn_lock();
    hid_conn = hid_find_connection_by_address(addr);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("No HID connection found for addr");
        return BT_STATUS_PARM_INVALID;
    }

    params = zalloc(sizeof(hid_device_cmd_param_t));
    if (!params) {
        hid_conn_unlock();
        BT_LOGE("Failed to allocate memory");
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, &hid_conn->addr, sizeof(bt_address_t));
    params->param = error;
    hid_conn_unlock();

    if (!service_loop_work(params, do_hid_report_error, NULL)) {
        BT_LOGE("service_loop_work failed");
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

/* NOTE: Async via service_loop_work(). See bt_sal_hid_device_get_report_response. */
bt_status_t bt_sal_hid_device_send_report(bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size)
{
    sal_hid_connection_t* hid_conn;
    hid_send_data_param_t* params;

    if (rpt_size < 0 || rpt_size > BT_HID_REPORT_DATA_LEN) {
        BT_LOGE("Invalid report size %d (max %d)", rpt_size, BT_HID_REPORT_DATA_LEN);
        return BT_STATUS_PARM_INVALID;
    }

    if (rpt_size > 0 && !rpt_data) {
        BT_LOGE("rpt_data is NULL but rpt_size is %d", rpt_size);
        return BT_STATUS_PARM_INVALID;
    }

    hid_conn_lock();
    hid_conn = hid_find_connection_by_address(addr);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("No HID connection found for addr");
        return BT_STATUS_PARM_INVALID;
    }

    params = zalloc(sizeof(hid_send_data_param_t));
    if (!params) {
        hid_conn_unlock();
        BT_LOGE("Failed to allocate memory");
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, &hid_conn->addr, sizeof(bt_address_t));
    params->type = BT_HID_REPORT_TYPE_INPUT;
    if (rpt_data && rpt_size > 0) {
        memcpy(params->data, rpt_data, rpt_size);
    }
    params->len = (uint16_t)rpt_size;
    hid_conn_unlock();

    if (!service_loop_work(params, do_hid_send_intr_data, NULL)) {
        BT_LOGE("service_loop_work failed");
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

/* NOTE: Async via service_loop_work(). See bt_sal_hid_device_get_report_response. */
bt_status_t bt_sal_hid_device_virtual_unplug(bt_address_t* addr)
{
    sal_hid_connection_t* hid_conn;
    hid_device_cmd_param_t* params;

    hid_conn_lock();
    hid_conn = hid_find_connection_by_address(addr);
    if (!hid_conn) {
        hid_conn_unlock();
        BT_LOGE("No HID connection found for addr");
        return BT_STATUS_PARM_INVALID;
    }

    params = zalloc(sizeof(hid_device_cmd_param_t));
    if (!params) {
        hid_conn_unlock();
        BT_LOGE("Failed to allocate memory");
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, &hid_conn->addr, sizeof(bt_address_t));
    hid_conn_unlock();

    if (!service_loop_work(params, do_hid_virtual_unplug, NULL)) {
        BT_LOGE("service_loop_work failed");
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}