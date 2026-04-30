/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hogp_device.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "hid_defs.h"
#include "hid_device_service.h"
#include "utils/log.h"

#include "bt_hid_host.h"
#include "sal_hogp_device_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"

#define DSC_LIST_HEADER_SIZE 3
#ifndef HID_SDP_DESCRIPTOR_REPORT
#define HID_SDP_DESCRIPTOR_REPORT 0x22
#endif

#define HOGP_MAX_CONNECTIONS 1

typedef struct hid_item {
    uint8_t type;
    uint8_t tag;
    uint32_t value;
    uint8_t data_size;
} hid_item_t;

typedef struct {
    bt_address_t addr;
    struct bt_conn* conn;
    bool in_use;
} hogp_connection_t;

static hogp_connection_t g_hogp_conns[HOGP_MAX_CONNECTIONS];
static bool g_hogp_registered;
static pthread_mutex_t g_hogp_mutex = PTHREAD_MUTEX_INITIALIZER;

static const uint8_t* extract_report_map(const uint8_t* dsc_list,
    uint16_t dsc_list_length, uint16_t* out_len)
{
    const uint8_t* p = dsc_list;
    uint16_t remaining = dsc_list_length;

    while (remaining > DSC_LIST_HEADER_SIZE) {
        uint8_t type = p[0];
        uint16_t len = p[1] | (p[2] << 8);

        if (len > remaining - DSC_LIST_HEADER_SIZE) {
            BT_LOGE("Invalid descriptor length: %u > remaining: %u",
                len, remaining - DSC_LIST_HEADER_SIZE);
            break;
        }

        if (type == HID_SDP_DESCRIPTOR_REPORT) {
            *out_len = len;
            return p + DSC_LIST_HEADER_SIZE;
        }

        p += DSC_LIST_HEADER_SIZE + len;
        remaining -= DSC_LIST_HEADER_SIZE + len;
    }

    *out_len = 0;
    return NULL;
}

static const uint8_t* hid_next_item(const uint8_t* pos, const uint8_t* end,
    struct hid_item* item)
{
    uint8_t header;
    uint8_t i;

    if (pos >= end)
        return NULL;

    header = *pos;
    item->data_size = HID_ITEM_SIZE(header);
    item->type = HID_ITEM_TYPE(header);
    item->tag = HID_ITEM_TAG(header);

    if (item->data_size == HID_ITEM_BSIZE_LONG)
        item->data_size = HID_ITEM_BSIZE_LONG_ACTUAL;

    if (pos + 1 + item->data_size > end)
        return NULL;

    item->value = 0;
    for (i = 0; i < item->data_size; i++)
        item->value |= (uint32_t)pos[1 + i] << (8 * i);

    return pos + 1 + item->data_size;
}

static int find_or_add_report(struct bt_hogp_device_report* reports,
    uint8_t* count, uint8_t max, uint8_t report_id, uint8_t report_type)
{
    int i;

    for (i = 0; i < *count; i++) {
        if (reports[i].id == report_id && reports[i].type == report_type)
            return i;
    }

    if (*count >= max)
        return -1;

    i = *count;
    reports[i].id = report_id;
    reports[i].type = report_type;
    (*count)++;
    return i;
}

static int parse_report_descriptor(const uint8_t* desc, uint16_t len,
    struct bt_hogp_device_report* reports, uint8_t max_reports,
    uint8_t* report_sizes)
{
    const uint8_t* pos = desc;
    const uint8_t* end = desc + len;
    struct hid_item item;
    uint32_t bit_totals[BT_HOGP_DEVICE_MAX_REPORTS];
    uint8_t count = 0;
    uint8_t current_report_id = 0;
    uint32_t report_size = 0;
    uint32_t report_count = 0;
    uint8_t report_type;
    int idx;

    memset(bit_totals, 0, sizeof(bit_totals));
    memset(report_sizes, 0, max_reports);

    while ((pos = hid_next_item(pos, end, &item)) != NULL) {
        if (item.type == HID_ITEM_TYPE_GLOBAL) {
            switch (item.tag) {
            case HID_ITEM_TAG_REPORT_ID:
                current_report_id = (uint8_t)item.value;
                break;
            case HID_ITEM_TAG_REPORT_SIZE:
                report_size = item.value;
                break;
            case HID_ITEM_TAG_REPORT_COUNT:
                report_count = item.value;
                break;
            }
        } else if (item.type == HID_ITEM_TYPE_MAIN) {
            switch (item.tag) {
            case HID_ITEM_TAG_INPUT:
                report_type = BT_HID_REPORT_TYPE_INPUT;
                break;
            case HID_ITEM_TAG_OUTPUT:
                report_type = BT_HID_REPORT_TYPE_OUTPUT;
                break;
            case HID_ITEM_TAG_FEATURE:
                report_type = BT_HID_REPORT_TYPE_FEATURE;
                break;
            default:
                continue;
            }

            idx = find_or_add_report(reports, &count, max_reports,
                current_report_id, report_type);

            if (idx >= 0)
                bit_totals[idx] += report_size * report_count;
        }
    }

    for (idx = 0; idx < count && idx < max_reports; idx++) {
        if (reports[idx].type == BT_HID_REPORT_TYPE_INPUT)
            report_sizes[idx] = (uint8_t)((bit_totals[idx] + 7) / 8);
    }

    if (count == 0 && max_reports > 0) {
        reports[0].id = 0;
        reports[0].type = BT_HID_REPORT_TYPE_INPUT;
        report_sizes[0] = 0;
        count = 1;
    }

    return count;
}

static hogp_connection_t* hogp_find_conn_by_addr(bt_address_t* addr)
{
    for (int i = 0; i < HOGP_MAX_CONNECTIONS; i++) {
        if (g_hogp_conns[i].in_use && bt_addr_compare(&g_hogp_conns[i].addr, addr) == 0)
            return &g_hogp_conns[i];
    }

    return NULL;
}

static hogp_connection_t* hogp_find_conn_by_bt_conn(struct bt_conn* conn)
{
    for (int i = 0; i < HOGP_MAX_CONNECTIONS; i++) {
        if (g_hogp_conns[i].in_use && g_hogp_conns[i].conn == conn)
            return &g_hogp_conns[i];
    }

    return NULL;
}

static hogp_connection_t* hogp_alloc_conn(bt_address_t* addr, struct bt_conn* conn)
{
    for (int i = 0; i < HOGP_MAX_CONNECTIONS; i++) {
        if (!g_hogp_conns[i].in_use) {
            memcpy(&g_hogp_conns[i].addr, addr, sizeof(bt_address_t));
            g_hogp_conns[i].conn = bt_conn_ref(conn);
            g_hogp_conns[i].in_use = true;
            return &g_hogp_conns[i];
        }
    }

    return NULL;
}

static void hogp_free_conn(hogp_connection_t* hconn)
{
    if (hconn && hconn->in_use) {
        if (hconn->conn) {
            bt_conn_unref(hconn->conn);
            hconn->conn = NULL;
        }

        hconn->in_use = false;
    }
}

static void hogp_dev_connected_cb(struct bt_conn* conn)
{
    bt_address_t addr;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("HOGP: connected_cb failed to get remote addr");
        return;
    }

    pthread_mutex_lock(&g_hogp_mutex);
    hogp_connection_t* hconn = hogp_alloc_conn(&addr, conn);
    pthread_mutex_unlock(&g_hogp_mutex);

    if (!hconn) {
        BT_LOGE("HOGP: no free connection slot, disconnecting");
        bt_conn_disconnect(conn, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
        return;
    }

    hid_device_on_connection_state_changed(&addr, true, PROFILE_STATE_CONNECTED);
}

static void hogp_dev_disconnected_cb(struct bt_conn* conn, uint8_t reason)
{
    pthread_mutex_lock(&g_hogp_mutex);
    hogp_connection_t* hconn = hogp_find_conn_by_bt_conn(conn);

    if (hconn) {
        bt_address_t addr;

        memcpy(&addr, &hconn->addr, sizeof(bt_address_t));
        hogp_free_conn(hconn);
        pthread_mutex_unlock(&g_hogp_mutex);

        BT_LOGD("HOGP: disconnected reason=0x%02x", reason);
        hid_device_on_connection_state_changed(&addr, true, PROFILE_STATE_DISCONNECTED);
        return;
    }

    pthread_mutex_unlock(&g_hogp_mutex);
}

static void hogp_dev_get_report_cb(struct bt_conn* conn, uint8_t report_type,
    uint8_t report_id, uint16_t buf_size)
{
    hogp_connection_t* hconn = hogp_find_conn_by_bt_conn(conn);

    if (!hconn)
        return;

    BT_LOGD("HOGP: get_report type=%u id=%u buf=%u", report_type, report_id, buf_size);
    hid_device_on_get_report(&hconn->addr, report_type, report_id, buf_size);
}

static void hogp_dev_set_report_cb(struct bt_conn* conn, uint8_t report_type,
    uint8_t report_id, const uint8_t* data, uint16_t len)
{
    hogp_connection_t* hconn = hogp_find_conn_by_bt_conn(conn);

    if (!hconn)
        return;

    BT_LOGD("HOGP: set_report type=%u len=%u", report_type, len);
    hid_device_on_set_report(&hconn->addr, report_type, len, (uint8_t*)data);
}

static void hogp_dev_set_protocol_cb(struct bt_conn* conn, uint8_t protocol)
{
    BT_LOGD("HOGP Device: set_protocol %u", protocol);
}

static void hogp_dev_mode_changed_cb(struct bt_conn* conn, uint8_t mode)
{
    bt_address_t addr;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("HOGP: mode_changed_cb failed to get remote addr");
        return;
    }

    BT_LOGD("HOGP: mode_changed 0x%02x", mode);
    hid_device_on_mode_changed(&addr, mode);
}

static void hogp_dev_ctrl_point_cb(struct bt_conn* conn, uint8_t value)
{
    bt_address_t addr;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    BT_LOGD("HOGP: ctrl_point %s", value == 0 ? "suspend" : "exit_suspend");
    hid_device_on_suspend(&addr, value == 0);
}

static const struct bt_hogp_device_cb hogp_dev_cb = {
    .connected = hogp_dev_connected_cb,
    .disconnected = hogp_dev_disconnected_cb,
    .get_report = hogp_dev_get_report_cb,
    .set_report = hogp_dev_set_report_cb,
    .set_protocol = hogp_dev_set_protocol_cb,
    .mode_changed = hogp_dev_mode_changed_cb,
    .ctrl_point = hogp_dev_ctrl_point_cb,
};

/* ---- Public SAL API ---- */

bt_status_t bt_sal_hogp_device_init(void)
{
    BT_LOGD("HOGP: init");

    memset(g_hogp_conns, 0, sizeof(g_hogp_conns));
    g_hogp_registered = false;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hogp_device_register_app(hid_device_sdp_settings_t* sdp)
{
    struct bt_hogp_device_init_param param;
    struct bt_hogp_device_report reports[BT_HOGP_DEVICE_MAX_REPORTS];
    uint8_t report_sizes[BT_HOGP_DEVICE_MAX_REPORTS];
    const uint8_t* rmap;
    uint16_t rmap_len;
    int report_cnt;
    int err;

    if (g_hogp_registered) {
        BT_LOGE("HOGP: already registered");
        return BT_STATUS_FAIL;
    }

    /* Parse HID descriptor */
    BT_LOGD("HOGP: register_app, dsc_list_len=%u", sdp->hids_info.dsc_list_length);
    rmap = extract_report_map(sdp->hids_info.dsc_list,
        sdp->hids_info.dsc_list_length, &rmap_len);

    if (!rmap || rmap_len == 0) {
        BT_LOGE("HOGP: no report map in dsc_list");
        return BT_STATUS_FAIL;
    }

    report_cnt = parse_report_descriptor(rmap, rmap_len, reports,
        BT_HOGP_DEVICE_MAX_REPORTS, report_sizes);
    BT_LOGD("HOGP: parsed %d reports, rmap_len=%u", report_cnt, rmap_len);

    memset(&param, 0, sizeof(param));
    param.info.bcd_hid = 0x0111;
    param.info.b_country_code = sdp->hids_info.country_code;
    param.info.flags = 0x02;
    param.report_map = rmap;
    param.report_map_len = rmap_len;
    param.reports = reports;
    param.report_count = report_cnt;
    param.cb = &hogp_dev_cb;
#if defined(CONFIG_BT_SHORTER_CONNECTION_INTERVALS)
    param.sci_supported = true;
    param.sci_lp_supported = true;
#endif

    err = bt_hogp_device_register(&param);
    if (err)
        return BT_STATUS_FAIL;

    g_hogp_registered = true;
    hid_device_on_app_state_changed(HID_APP_STATE_REGISTERED);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hogp_device_unregister_app(void)
{
    if (!g_hogp_registered)
        return BT_STATUS_FAIL;

    bt_hogp_device_unregister();

    g_hogp_registered = false;
    hid_device_on_app_state_changed(HID_APP_STATE_NOT_REGISTERED);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hogp_device_connect(bt_address_t* addr)
{
    bt_addr_le_t peer;
    int err;

    /* Query addr_type from device manager (recorded during scan/bond) */
    ble_addr_type_t type = adapter_get_le_remote_address_type(addr);
    peer.type = (type != BT_LE_ADDR_TYPE_UNKNOWN) ? type : BT_ADDR_LE_PUBLIC;
    memcpy(peer.a.val, addr->addr, 6);

    BT_LOGD("HOGP: connect %02x:%02x:%02x:%02x:%02x:%02x",
        addr->addr[5], addr->addr[4], addr->addr[3],
        addr->addr[2], addr->addr[1], addr->addr[0]);

    err = bt_hogp_device_connect(&peer);
    if (err == -EALREADY)
        return BT_STATUS_BUSY;

    if (err)
        return BT_STATUS_FAIL;

    hid_device_on_connection_state_changed(addr, true, PROFILE_STATE_CONNECTING);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hogp_device_disconnect(bt_address_t* addr)
{
    BT_LOGD("HOGP: disconnect");
    hogp_connection_t* hconn;
    struct bt_conn* conn;

    pthread_mutex_lock(&g_hogp_mutex);
    hconn = hogp_find_conn_by_addr(addr);
    if (!hconn) {
        pthread_mutex_unlock(&g_hogp_mutex);
        BT_LOGE("HOGP: disconnect no connection for addr");
        return BT_STATUS_PARM_INVALID;
    }

    conn = hconn->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_hogp_mutex);

    bt_hogp_device_disconnect(conn);
    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hogp_device_send_report(bt_address_t* addr, uint8_t rpt_id,
    uint8_t* rpt_data, int rpt_size)
{
    hogp_connection_t* hconn;
    struct bt_conn* conn;
    int err;

    pthread_mutex_lock(&g_hogp_mutex);
    hconn = hogp_find_conn_by_addr(addr);
    if (!hconn) {
        pthread_mutex_unlock(&g_hogp_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    conn = hconn->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_hogp_mutex);

    err = bt_hogp_device_send_report(conn, rpt_id, rpt_data, rpt_size);
    bt_conn_unref(conn);
    return err == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_hogp_device_get_report_response(bt_address_t* addr,
    uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
{
    hogp_connection_t* hconn;
    struct bt_conn* conn;
    uint8_t report_id;
    int ret;

    if (!rpt_data || rpt_size < 1) {
        return BT_STATUS_PARM_INVALID;
    }

    BT_LOGD("HOGP: get_report_response type=%u len=%d", rpt_type, rpt_size);

    pthread_mutex_lock(&g_hogp_mutex);
    hconn = hogp_find_conn_by_addr(addr);
    if (!hconn || !hconn->conn) {
        pthread_mutex_unlock(&g_hogp_mutex);
        BT_LOGE("HOGP: get_report_response no connection for addr");
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    conn = hconn->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_hogp_mutex);

    report_id = rpt_data[0];
    ret = bt_hogp_device_get_report_response(conn, report_id, rpt_type,
        rpt_data + 1, rpt_size - 1);
    bt_conn_unref(conn);
    return ret ? BT_STATUS_FAIL : BT_STATUS_SUCCESS;
}

static uint8_t hid_error_to_att_error(hid_status_error_t error)
{
    switch (error) {
    case HID_STATUS_OK:
        return 0;
    case HID_STATUS_HANDSHAKE_INVALID_REPORT_ID:
        return BT_ATT_ERR_INVALID_OFFSET;
    case HID_STATUS_HANDSHAKE_UNSUPPORTED_REQ:
        return BT_ATT_ERR_NOT_SUPPORTED;
    case HID_STATUS_HANDSHAKE_INVALID_PARAM:
        return BT_ATT_ERR_INVALID_ATTRIBUTE_LEN;
    default:
        return BT_ATT_ERR_UNLIKELY;
    }
}

bt_status_t bt_sal_hogp_device_report_error(bt_address_t* addr,
    hid_status_error_t error)
{
    hogp_connection_t* hconn;
    struct bt_conn* conn;
    uint8_t att_err;
    int ret;

    BT_LOGD("HOGP: report_error %d", error);

    att_err = hid_error_to_att_error(error);
    if (att_err == 0)
        return BT_STATUS_SUCCESS;

    pthread_mutex_lock(&g_hogp_mutex);
    hconn = hogp_find_conn_by_addr(addr);
    if (!hconn) {
        pthread_mutex_unlock(&g_hogp_mutex);
        BT_LOGE("HOGP: report_error no connection for addr");
        return BT_STATUS_PARM_INVALID;
    }

    conn = hconn->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_hogp_mutex);

    ret = bt_hogp_device_report_error(conn, att_err);
    bt_conn_unref(conn);
    return ret == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_hogp_device_virtual_unplug(bt_address_t* addr)
{
    hogp_connection_t* hconn;
    struct bt_conn* conn;
    int ret;

    BT_LOGD("HOGP: virtual_unplug");

    pthread_mutex_lock(&g_hogp_mutex);
    hconn = hogp_find_conn_by_addr(addr);
    if (!hconn) {
        pthread_mutex_unlock(&g_hogp_mutex);
        BT_LOGE("HOGP: virtual_unplug no connection for addr");
        return BT_STATUS_PARM_INVALID;
    }

    conn = hconn->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_hogp_mutex);

    ret = bt_hogp_device_virtual_cable_unplug(conn);
    bt_conn_unref(conn);
    return ret == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

void bt_sal_hogp_device_cleanup(void)
{
    pthread_mutex_lock(&g_hogp_mutex);

    for (int i = 0; i < HOGP_MAX_CONNECTIONS; i++)
        hogp_free_conn(&g_hogp_conns[i]);

    pthread_mutex_unlock(&g_hogp_mutex);

    if (g_hogp_registered) {
        bt_hogp_device_unregister();
        g_hogp_registered = false;
    }
}
