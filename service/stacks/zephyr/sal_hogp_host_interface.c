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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/hogp_host.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "hid_host_service.h"
#include "sal_hogp_host_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "service_loop.h"
#include "utils/log.h"

#define HOGP_HOST_MAX_CONNECTIONS 3

typedef struct {
    bt_address_t addr;
    struct bt_conn* conn;
    bool in_use;
} hogp_host_conn_t;

typedef void (*sal_hogp_func_t)(void* args);

typedef struct {
    bt_address_t addr;
    sal_hogp_func_t func;
    union {
        struct {
            uint8_t mode;
            uint8_t policy;
            uint16_t iso_interval;
        } set_mode;
    };
} sal_hogp_req_t;

static hogp_host_conn_t g_conns[HOGP_HOST_MAX_CONNECTIONS];
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_conn_cb_registered;

static hogp_host_conn_t* find_by_addr(bt_address_t* addr)
{
    for (int i = 0; i < HOGP_HOST_MAX_CONNECTIONS; i++) {
        if (g_conns[i].in_use && bt_addr_compare(&g_conns[i].addr, addr) == 0) {
            return &g_conns[i];
        }
    }

    return NULL;
}

static hogp_host_conn_t* find_by_conn(struct bt_conn* conn)
{
    for (int i = 0; i < HOGP_HOST_MAX_CONNECTIONS; i++) {
        if (g_conns[i].in_use && g_conns[i].conn == conn) {
            return &g_conns[i];
        }
    }

    BT_LOGW("HOGP Host: find_by_conn conn:%p -> NOT FOUND", conn);
    return NULL;
}

static hogp_host_conn_t* alloc_conn(bt_address_t* addr, struct bt_conn* conn)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);

    for (int i = 0; i < HOGP_HOST_MAX_CONNECTIONS; i++) {
        if (!g_conns[i].in_use) {
            memcpy(&g_conns[i].addr, addr, sizeof(bt_address_t));
            g_conns[i].conn = bt_conn_ref(conn);
            g_conns[i].in_use = true;
            BT_LOGD("HOGP Host: alloc_conn [%s] -> slot %d, conn:%p", addr_str, i, conn);
            return &g_conns[i];
        }
    }

    BT_LOGE("HOGP Host: alloc_conn [%s] FAILED, no free slot", addr_str);
    return NULL;
}

static void free_conn_slot(hogp_host_conn_t* hc)
{
    if (hc && hc->in_use) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

        bt_addr_ba2str(&hc->addr, addr_str);
        BT_LOGD("HOGP Host: free_conn [%s], conn:%p", addr_str, hc->conn);
        if (hc->conn) {
            bt_conn_unref(hc->conn);
            hc->conn = NULL;
        }

        hc->in_use = false;
    }
}

/* ---- HIDS Host callbacks ---- */

static void hogp_host_connected_cb(struct bt_conn* conn, int status,
    uint8_t num_instances, uint8_t protocol_mode)
{
    bt_address_t addr;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("HOGP Host: connected_cb failed to get remote addr");
        return;
    }

    bt_addr_ba2str(&addr, addr_str);
    BT_LOGD("HOGP Host: connected_cb [%s] status=%d instances=%u protocol=%u",
        addr_str, status, num_instances, protocol_mode);

    if (status == 0) {
        hid_host_on_connection_state_changed(&addr, BT_TRANSPORT_BLE, PROFILE_STATE_CONNECTED);
    } else {
        pthread_mutex_lock(&g_mutex);
        hogp_host_conn_t* hc = find_by_conn(conn);
        if (hc) {
            bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            free_conn_slot(hc);
        }

        pthread_mutex_unlock(&g_mutex);
        hid_host_on_connection_state_changed(&addr, BT_TRANSPORT_BLE, PROFILE_STATE_DISCONNECTED);
    }
}

static void hogp_host_disconnected_cb(struct bt_conn* conn, int reason)
{
    bt_address_t addr;
    hogp_host_conn_t* hc;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    BT_LOGD("HOGP Host: disconnected_cb conn:%p reason=0x%02x", conn, reason);

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("HOGP Host: disconnected_cb failed to get remote addr");
        return;
    }

    bt_addr_ba2str(&addr, addr_str);
    BT_LOGD("HOGP Host: disconnected_cb [%s] reason=0x%02x", addr_str, reason);

    pthread_mutex_lock(&g_mutex);
    hc = find_by_conn(conn);
    if (hc) {
        BT_LOGD("HOGP Host: disconnected_cb free_conn [%s]", addr_str);
        free_conn_slot(hc);
    }

    pthread_mutex_unlock(&g_mutex);
    hid_host_on_connection_state_changed(&addr, BT_TRANSPORT_BLE, PROFILE_STATE_DISCONNECTED);
}

static void hogp_host_report_map_cb(struct bt_conn* conn, uint8_t service_index,
    const uint8_t* data, uint16_t len)
{
    bt_address_t addr;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    bt_addr_ba2str(&addr, addr_str);
    BT_LOGD("HOGP Host: report_map_cb [%s] svc=%u len=%u", addr_str, service_index, len);

    hid_host_on_report_map(&addr, service_index, data, len);
}

static void hogp_host_input_report_cb(struct bt_conn* conn, uint8_t service_index,
    uint8_t report_id, const uint8_t* data, uint16_t len)
{
    bt_address_t addr;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("HOGP Host: input_report_cb failed to get remote addr");
        return;
    }

    hid_host_on_input_report(&addr, service_index, report_id, data, len);
}

static void hogp_host_get_report_cb(struct bt_conn* conn, uint8_t report_id,
    uint8_t report_type, const uint8_t* data, uint16_t len)
{
    bt_address_t addr;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    bt_addr_ba2str(&addr, addr_str);
    BT_LOGD("HOGP Host: get_report_cb [%s] id=%u type=%u len=%u", addr_str, report_id, report_type, len);
    hid_host_on_get_report_result(&addr, report_id, report_type, data, len);
}

static void hogp_host_pnp_id_cb(struct bt_conn* conn,
    const struct bt_hogp_host_pnp_id* id)
{
    bt_address_t addr;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    bt_addr_ba2str(&addr, addr_str);
    BT_LOGD("HOGP Host: pnp_id_cb [%s] vid_src=0x%02x vid=0x%04x pid=0x%04x ver=0x%04x",
        addr_str, id->vid_src, id->vid, id->pid, id->version);

    hid_host_on_pnp_id(&addr, id->vid_src, id->vid, id->pid, id->version);
}

static void hogp_host_battery_level_cb(struct bt_conn* conn,
    uint8_t bat_index, uint8_t level)
{
    bt_address_t addr;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    BT_LOGD("HOGP Host: battery_level_cb bat=%u level=%u%%", bat_index, level);
    hid_host_on_battery_level(&addr, bat_index, level);
}

static void hogp_host_mode_changed_cb(struct bt_conn* conn,
    uint8_t mode, int status)
{
    bt_address_t addr;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    BT_LOGD("HOGP Host: mode_changed mode=0x%02x status=%d", mode, status);
    hid_host_on_mode_changed(&addr, mode, status);
}

static const struct bt_hogp_host_cb hogp_host_cb = {
    .connected = hogp_host_connected_cb,
    .disconnected = hogp_host_disconnected_cb,
    .report_map = hogp_host_report_map_cb,
    .input_report = hogp_host_input_report_cb,
    .get_report_result = hogp_host_get_report_cb,
    .pnp_id = hogp_host_pnp_id_cb,
    .battery_level = hogp_host_battery_level_cb,
    .mode_changed = hogp_host_mode_changed_cb,
};

/* ---- BLE connection callback for initiating HOGP discovery ---- */

static void ble_connected_cb(struct bt_conn* conn, uint8_t err)
{
    char a_str[BT_ADDR_STR_LENGTH] = { 0 };
    hogp_host_conn_t* hc;
    bt_address_t addr;
    int ret;

    BT_LOGD("HOGP Host: ble_connected_cb conn:%p err:%d", conn, err);
    if (err) {
        BT_LOGE("HOGP Host: ble_connected_cb err:%d", err);
        pthread_mutex_lock(&g_mutex);
        hc = find_by_conn(conn);
        if (hc)
            free_conn_slot(hc);

        pthread_mutex_unlock(&g_mutex);
        return;
    }

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        return;
    }

    pthread_mutex_lock(&g_mutex);
    hc = find_by_conn(conn);
    pthread_mutex_unlock(&g_mutex);

    if (!hc) {
        BT_LOGE("HOGP Host: ble_connected_cb no connection slot for conn:%p", conn);
        return;
    }

    bt_addr_ba2str(&addr, a_str);
    BT_LOGD("HOGP Host: BLE connected [%s], starting HIDS discovery", a_str);
    hid_host_on_connection_state_changed(&addr, BT_TRANSPORT_BLE, PROFILE_STATE_CONNECTING);

    ret = bt_hogp_host_connect(conn, BT_HID_PROTOCOL_REPORT,
        &hogp_host_cb);
    if (ret) {
        BT_LOGE("HOGP Host: bt_hogp_host_connect failed: %d", ret);
        pthread_mutex_lock(&g_mutex);
        free_conn_slot(hc);
        pthread_mutex_unlock(&g_mutex);
        hid_host_on_connection_state_changed(&addr, BT_TRANSPORT_BLE, PROFILE_STATE_DISCONNECTED);
    }
}

static struct bt_conn_cb hogp_host_conn_cb = {
    .connected = ble_connected_cb,
};

/* ---- Public SAL API ---- */

bt_status_t bt_sal_hogp_host_init(void)
{
    memset(g_conns, 0, sizeof(g_conns));

    BT_LOGD("HOGP Host SAL: init");

    bt_hogp_host_init();

    if (!g_conn_cb_registered) {
        bt_conn_cb_register(&hogp_host_conn_cb);
        g_conn_cb_registered = true;
    }

    return BT_STATUS_SUCCESS;
}

void bt_sal_hogp_host_cleanup(void)
{
    BT_LOGD("HOGP Host SAL: cleanup");

    pthread_mutex_lock(&g_mutex);
    for (int i = 0; i < HOGP_HOST_MAX_CONNECTIONS; i++) {
        if (g_conns[i].in_use && g_conns[i].conn) {
            bt_hogp_host_disconnect(g_conns[i].conn);
        }

        free_conn_slot(&g_conns[i]);
    }

    pthread_mutex_unlock(&g_mutex);
}

/* ---- Async request infrastructure (same pattern as sal_adapter_le) ---- */

static sal_hogp_req_t* sal_hogp_req(bt_address_t* addr, sal_hogp_func_t func)
{
    sal_hogp_req_t* req = calloc(sizeof(sal_hogp_req_t), 1);
    if (req) {
        req->func = func;
        if (addr)
            memcpy(&req->addr, addr, sizeof(bt_address_t));
    }

    return req;
}

static void sal_hogp_invoke_async(service_work_t* work, void* userdata)
{
    sal_hogp_req_t* req = userdata;
    req->func(req);
    free(req);
}

static bt_status_t sal_hogp_send_req(sal_hogp_req_t* req)
{
    if (!req)
        return BT_STATUS_NOMEM;

    if (!service_loop_work(req, sal_hogp_invoke_async, NULL)) {
        BT_LOGE("sal_hogp_send_req: service_loop_work failed");
        free(req);
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

/* ---- Async handlers ---- */

static void hogp_do_connect(void* args)
{
    sal_hogp_req_t* req = args;
    struct bt_conn* conn;
    bt_addr_le_t peer;
    hogp_host_conn_t* hc;

    /* Query addr_type from device manager (recorded during scan/bond) */
    ble_addr_type_t type = adapter_get_le_remote_address_type(&req->addr);
    peer.type = (type != BT_LE_ADDR_TYPE_UNKNOWN) ? type : BT_ADDR_LE_PUBLIC;
    memcpy(peer.a.val, req->addr.addr, 6);

    struct bt_conn_le_create_param create_param = BT_CONN_LE_CREATE_PARAM_INIT(
        BT_CONN_LE_OPT_NONE, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW);
    struct bt_le_conn_param conn_param = BT_LE_CONN_PARAM_INIT(24, 40, 0, 3200);

    int err = bt_conn_le_create(&peer, &create_param, &conn_param, &conn);
    if (err) {
        BT_LOGE("HOGP Host: bt_conn_le_create failed: %d", err);
        return;
    }

    BT_LOGD("HOGP Host: bt_conn_le_create OK, conn=%p", conn);

    pthread_mutex_lock(&g_mutex);
    hc = alloc_conn(&req->addr, conn);
    pthread_mutex_unlock(&g_mutex);

    if (!hc) {
        BT_LOGE("HOGP Host: alloc_conn failed");
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }

    bt_conn_unref(conn); /* release bt_conn_le_create ref; alloc_conn holds its own if success */
}

static void hogp_do_disconnect(void* args)
{
    sal_hogp_req_t* req = args;
    hogp_host_conn_t* hc;
    struct bt_conn* conn;

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(&req->addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        BT_LOGE("HOGP Host: disconnect no connection for addr");
        return;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    bt_conn_unref(conn);
}

static void hogp_do_set_mode(void* args)
{
    sal_hogp_req_t* req = args;
    hogp_host_conn_t* hc;
    struct bt_conn* conn;
    struct bt_hogp_host_mode_param param;
    int err;

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(&req->addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        BT_LOGE("HOGP Host: set_mode no connection for addr");
        return;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    param.mode = req->set_mode.mode;
    param.policy = req->set_mode.policy;
    param.iso_interval = req->set_mode.iso_interval;

    BT_LOGD("HOGP SAL set_mode: mode=0x%02x policy=%u iso_interval=%u",
        param.mode, param.policy, param.iso_interval);

    err = bt_hogp_host_set_mode(conn, &param);
    BT_LOGD("HOGP SAL set_mode result: err=%d", err);
    bt_conn_unref(conn);
}

bt_status_t bt_sal_hogp_host_connect(bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);

    BT_LOGD("HOGP Host SAL: connect [%s]", addr_str);

    pthread_mutex_lock(&g_mutex);
    if (find_by_addr(addr)) {
        pthread_mutex_unlock(&g_mutex);
        BT_LOGE("HOGP Host SAL: connect already connected");
        return BT_STATUS_BUSY;
    }

    pthread_mutex_unlock(&g_mutex);

    return sal_hogp_send_req(sal_hogp_req(addr, hogp_do_connect));
}

bt_status_t bt_sal_hogp_host_disconnect(bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);

    BT_LOGD("HOGP Host SAL: disconnect [%s]", addr_str);

    pthread_mutex_lock(&g_mutex);
    if (!find_by_addr(addr)) {
        pthread_mutex_unlock(&g_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    pthread_mutex_unlock(&g_mutex);

    return sal_hogp_send_req(sal_hogp_req(addr, hogp_do_disconnect));
}

bt_status_t bt_sal_hogp_host_get_report(bt_address_t* addr, uint8_t report_id,
    uint8_t report_type)
{
    hogp_host_conn_t* hc;
    int err;

    BT_LOGD("HOGP Host SAL: get_report id=%u type=%u", report_id, report_type);

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        BT_LOGE("HOGP Host: get_report no connection for addr");
        return BT_STATUS_PARM_INVALID;
    }

    struct bt_conn* conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    err = bt_hogp_host_get_report(conn, report_id, report_type);
    bt_conn_unref(conn);

    return err == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_hogp_host_set_report(bt_address_t* addr, uint8_t report_id,
    uint8_t report_type, const uint8_t* data, uint16_t len)
{
    hogp_host_conn_t* hc;
    struct bt_conn* conn;

    BT_LOGD("HOGP Host SAL: set_report id=%u type=%u len=%u", report_id, report_type, len);

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    int err = bt_hogp_host_set_report(conn, report_id, report_type, data, len);
    bt_conn_unref(conn);

    return err == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_hogp_host_set_protocol(bt_address_t* addr, uint8_t protocol_mode)
{
    hogp_host_conn_t* hc;
    struct bt_conn* conn;
    int err;

    BT_LOGD("HOGP Host SAL: set_protocol mode=%u", protocol_mode);

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    err = bt_hogp_host_set_protocol_mode(conn, 0, protocol_mode);
    bt_conn_unref(conn);

    return err == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_hogp_host_suspend(bt_address_t* addr)
{
    hogp_host_conn_t* hc;
    struct bt_conn* conn;
    int err;

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    err = bt_hogp_host_suspend(conn, 0);
    bt_conn_unref(conn);

    return err == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_hogp_host_exit_suspend(bt_address_t* addr)
{
    hogp_host_conn_t* hc;
    struct bt_conn* conn;
    int err;

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    err = bt_hogp_host_exit_suspend(conn, 0);
    bt_conn_unref(conn);

    return err == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

int bt_sal_hogp_host_get_supported_intervals(bt_address_t* addr,
    uint16_t* intervals, uint8_t max_count)
{
#if defined(CONFIG_BT_HOGP_HOST_ISO)
    hogp_host_conn_t* hc;
    struct bt_conn* conn;
    int ret;

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        return -1;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    ret = bt_hogp_host_get_supported_intervals(conn, intervals, max_count);
    BT_LOGD("SAL get_supported_intervals: ret=%d", ret);

    bt_conn_unref(conn);
    return ret;
#else
    (void)addr;
    (void)intervals;
    (void)max_count;
    return 0; /* CONFIG_BT_HOGP_HOST_ISO disabled: no ISO intervals available */
#endif
}

bt_status_t bt_sal_hogp_host_set_mode(bt_address_t* addr, uint8_t mode,
    uint8_t policy, uint16_t iso_interval)
{
    sal_hogp_req_t* req;

    pthread_mutex_lock(&g_mutex);
    if (!find_by_addr(addr)) {
        pthread_mutex_unlock(&g_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    pthread_mutex_unlock(&g_mutex);

    req = sal_hogp_req(addr, hogp_do_set_mode);
    if (!req)
        return BT_STATUS_NOMEM;

    req->set_mode.mode = mode;
    req->set_mode.policy = policy;
    req->set_mode.iso_interval = iso_interval;

    return sal_hogp_send_req(req);
}

bt_status_t bt_sal_hogp_host_get_mode(bt_address_t* addr, uint8_t* mode)
{
    hogp_host_conn_t* hc;
    struct bt_conn* conn;
    struct bt_hogp_host_mode_param param;
    int err;

    pthread_mutex_lock(&g_mutex);
    hc = find_by_addr(addr);
    if (!hc) {
        pthread_mutex_unlock(&g_mutex);
        return BT_STATUS_PARM_INVALID;
    }

    conn = hc->conn;
    bt_conn_ref(conn);
    pthread_mutex_unlock(&g_mutex);

    err = bt_hogp_host_get_mode(conn, &param);
    bt_conn_unref(conn);
    if (err == 0 && mode)
        *mode = param.mode;

    return err == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}
