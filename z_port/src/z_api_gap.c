/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <debug.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>

#include "z_api.h"
#include "z_api_manager.h"
#include "bt_adapter.h"
#include "bluetooth.h"
#include "bt_le_advertiser.h"
#include "bt_le_scan.h"
#include "bt_device.h"
#include "bt_gattc.h"
#include "bt_gatts.h"
#include "utils/log.h"

/* Mirror of Zephyr's struct bt_conn_cb field layout.
 * Must stay in sync with zephyr/bluetooth/conn.h.
 * Fields guarded by CONFIG_* are included because the zblue build
 * enables them (CONFIG_BT_CONN_REQ_AUTO_HANDLE is NOT defined,
 * CONFIG_BT_SMP and CONFIG_BT_CLASSIC are defined).
 */
struct z_conn_cb_mirror {
    void (*connect_req)(void *, uint8_t, uint8_t *);  /* CONFIG_BT_CONN_REQ_AUTO_HANDLE not defined */
    void (*connected)(void *, uint8_t);
    void (*disconnected)(void *, uint8_t);
    void (*recycled)(void);
    bool (*le_param_req)(void *, void *);
    void (*le_param_updated)(void *, uint16_t, uint16_t, uint16_t);
    void (*identity_resolved)(void *, const void *, const void *);  /* CONFIG_BT_SMP */
    void (*security_changed)(void *, uint8_t, uint8_t);             /* CONFIG_BT_SMP */
};

/*
 * Dispatch mechanism: route BT Framework API calls to bt_ipc_thread.
 * All socket IPC must happen in the thread that created the BT instance.
 */
typedef int (*z_api_func_t)(void *arg);
extern int z_api_dispatch(z_api_func_t func, void *arg);

#define ADV_DATA_LEN_MAX 31

/* ================================================================
 * Connection management
 *
 * Framework uses gattc/gatts for BLE connections, not GAP-level connect.
 * We maintain a connection table that maps Zephyr bt_conn* to Framework
 * gattc_handle_t / gatts_handle_t.
 * ================================================================ */

#define MAX_CONNECTIONS 4

typedef struct {
    bool in_use;
    uint8_t ref_count;
    uint8_t role;           /* 0=central, 1=peripheral */
    uint8_t sec_level;      /* bt_security_t: 1=L1, 2=L2, ... */
    uint8_t enc_key_size;
    uint8_t addr[7];        /* Zephyr bt_addr_le_t: { type(1), a.val[6] } */
    uint16_t interval;
    uint16_t latency;
    uint16_t timeout;
    bt_address_t fw_addr;   /* Framework address (big-endian) */
    gattc_handle_t gattc_handle;  /* For central role */
    bool connected;
    bool connect_notified;  /* true after notify_connected sent */
    bool disconnect_notified; /* true after notify_disconnected sent */
} z_conn_t;

z_conn_t g_conns[MAX_CONNECTIONS];

/* Registered Zephyr connection callbacks (linked list) */
typedef struct z_conn_cb_node {
    void *cb;
    struct z_conn_cb_node *next;
} z_conn_cb_node_t;

static z_conn_cb_node_t *g_conn_cb_list;
static const void *g_auth_cb;
static void *g_auth_info_cb;

/* GATTS handle for peripheral role (registered once) */
gatts_handle_t g_gatts_handle;

/* GATTC handle for connection parameter updates (registered once) */
gattc_handle_t g_gattc_handle;

static void *le_gap_handle;
static bt_advertiser_t *le_advertiser;
static bt_scanner_t *scanner_handle;
static void *user_scan_cb;
static bool is_scanning;
static bool is_enabled;
static bool bondable_mode = true;
static bool sc_oob_flag;
static bool legacy_oob_flag;

/* Non-static: set by main.c bt_ipc_thread after z_bt_manager_init */
bt_instance_t *local_bt_ins;

static bt_instance_t *get_ins(void)
{
    if (local_bt_ins) return local_bt_ins;
    return (bt_instance_t *)z_api(bt_svc_ins_get)();
}

/* ---- Address conversion helpers ---- */

static void fw_addr_to_zephyr(const bt_address_t *fw, uint8_t addr_type, uint8_t *z_addr)
{
    /* bt_addr_le_t layout: { type(1), a.val[6] }
     * Framework bt_address_t.addr from adapter callbacks is already
     * in the same byte order as Zephyr a.val (LSB first), so just copy.
     */
    z_addr[0] = addr_type;
    memcpy(&z_addr[1], fw->addr, 6);
}

static void zephyr_addr_to_fw(const uint8_t *z_addr, bt_address_t *fw)
{
    memcpy(fw->addr, &z_addr[1], 6);
}

/* ---- Connection table helpers ---- */

static z_conn_t *conn_alloc(void)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!g_conns[i].in_use) {
            memset(&g_conns[i], 0, sizeof(z_conn_t));
            g_conns[i].in_use = true;
            g_conns[i].ref_count = 1;
            g_conns[i].enc_key_size = 16;
            g_conns[i].sec_level = 1;
            g_conns[i].interval = 48;
            g_conns[i].latency = 0;
            g_conns[i].timeout = 400;
            return &g_conns[i];
        }
    }
    return NULL;
}

static z_conn_t *conn_lookup_by_fw_addr(const bt_address_t *addr)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (g_conns[i].in_use &&
            memcmp(&g_conns[i].fw_addr, addr, sizeof(bt_address_t)) == 0)
            return &g_conns[i];
    }
    return NULL;
}

static z_conn_t *conn_lookup_by_zephyr_addr(const uint8_t *z_addr)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (g_conns[i].in_use &&
            memcmp(g_conns[i].addr, z_addr, 7) == 0)
            return &g_conns[i];
    }
    return NULL;
}

static z_conn_t *conn_lookup_by_gattc(gattc_handle_t h)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (g_conns[i].in_use && g_conns[i].gattc_handle == h)
            return &g_conns[i];
    }
    return NULL;
}

/* ---- Notify Zephyr bt_conn_cb callbacks ---- */

/*
 * bt_conn_cb layout (from Zephyr):
 *   [0] void (*connected)(struct bt_conn *conn, uint8_t err);
 *   [1] void (*disconnected)(struct bt_conn *conn, uint8_t reason);
 *   [2] void (*identity_resolved)(conn, rpa, identity);
 *   [3] bool (*le_param_req)(conn, param);
 *   [4] void (*le_param_updated)(conn, interval, latency, timeout);
 *   [5] void (*security_changed)(conn, level, err);
 */

static void notify_connected(z_conn_t *conn, uint8_t err)
{
    _info("[z_api][conn_dbg] notify_connected: conn=%p err=%d cb_list=%p\n",
          conn, err, g_conn_cb_list);
    int count = 0;
    for (z_conn_cb_node_t *n = g_conn_cb_list; n; n = n->next) {
        struct z_conn_cb_mirror *cb = (struct z_conn_cb_mirror *)n->cb;
        _info("[z_api][conn_dbg] notify_connected: cb=%p connected=%p\n", cb, cb->connected);
        if (cb->connected) cb->connected((void *)conn, err);
        count++;
    }
    _info("[z_api][conn_dbg] notify_connected: dispatched to %d callbacks\n", count);
}

static void notify_disconnected(z_conn_t *conn, uint8_t reason)
{
    for (z_conn_cb_node_t *n = g_conn_cb_list; n; n = n->next) {
        struct z_conn_cb_mirror *cb = (struct z_conn_cb_mirror *)n->cb;
        if (cb->disconnected) cb->disconnected((void *)conn, reason);
    }
}

static void notify_security_changed(z_conn_t *conn, uint8_t level, uint8_t err)
{
    for (z_conn_cb_node_t *n = g_conn_cb_list; n; n = n->next) {
        struct z_conn_cb_mirror *cb = (struct z_conn_cb_mirror *)n->cb;
        if (cb->security_changed) cb->security_changed((void *)conn, level, err);
    }
}

/* ---- GATTC callbacks (central role) ---- */

static void gattc_on_connected(gattc_handle_t h, bt_address_t *addr)
{
    _info("[z_api] gattc_on_connected: addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
          addr->addr[0], addr->addr[1], addr->addr[2],
          addr->addr[3], addr->addr[4], addr->addr[5]);

    z_conn_t *conn = conn_lookup_by_gattc(h);
    if (!conn) {
        _info("[z_api] gattc_on_connected: no conn for handle\n");
        return;
    }
    memcpy(&conn->fw_addr, addr, sizeof(bt_address_t));

    bt_instance_t *ins = get_ins();
    ble_addr_type_t atype = BT_LE_ADDR_TYPE_UNKNOWN;
    if (ins) atype = bt_device_get_address_type(ins, addr);
    fw_addr_to_zephyr(addr, (atype == BT_LE_ADDR_TYPE_PUBLIC) ? 0 : 1, conn->addr);

    conn->connected = true;
    if (!conn->connect_notified) {
        conn->connect_notified = true;
        notify_connected(conn, 0);
    }
}

static void gattc_on_disconnected(gattc_handle_t h, bt_address_t *addr)
{
    _info("[z_api] gattc_on_disconnected\n");
    z_conn_t *conn = conn_lookup_by_gattc(h);
    if (!conn) return;
    conn->connected = false;
    if (!conn->disconnect_notified) {
        conn->disconnect_notified = true;
        notify_disconnected(conn, 0x13); /* REMOTE_USER_TERM_CONN */
        /* Don't set in_use=false here; on_conn_state_changed_cb will do it */
    }
}

static void gattc_on_mtu_updated(gattc_handle_t h, gatt_status_t status, uint32_t mtu)
{
    _info("[z_api] gattc_on_mtu_updated: mtu=%u status=%d\n", mtu, status);
}

static void gattc_on_conn_param_updated(gattc_handle_t h, bt_status_t status,
    uint16_t interval, uint16_t latency, uint16_t timeout)
{
    _info("[z_api] gattc_on_conn_param_updated: interval=%d latency=%d timeout=%d\n",
          interval, latency, timeout);
    z_conn_t *conn = conn_lookup_by_gattc(h);
    if (!conn) return;
    conn->interval = interval;
    conn->latency = latency;
    conn->timeout = timeout;

    /* Notify le_param_updated callback */
    for (z_conn_cb_node_t *n = g_conn_cb_list; n; n = n->next) {
        struct z_conn_cb_mirror *cb = (struct z_conn_cb_mirror *)n->cb;
        if (cb->le_param_updated) cb->le_param_updated((void *)conn, interval, latency, timeout);
    }
}

gattc_callbacks_t g_gattc_cbs = {
    .size = sizeof(gattc_callbacks_t),
    .on_connected = gattc_on_connected,
    .on_disconnected = gattc_on_disconnected,
    .on_mtu_updated = gattc_on_mtu_updated,
    .on_conn_param_updated = gattc_on_conn_param_updated,
};

/* ---- GATTS callbacks (peripheral role) ---- */

void gatts_on_connected(gatts_handle_t h, bt_address_t *addr)
{
    _info("[z_api] gatts_on_connected: addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
          addr->addr[0], addr->addr[1], addr->addr[2],
          addr->addr[3], addr->addr[4], addr->addr[5]);

    z_conn_t *conn = conn_lookup_by_fw_addr(addr);
    if (!conn) {
        conn = conn_alloc();
        if (!conn) return;
        conn->role = 1; /* peripheral */
        memcpy(&conn->fw_addr, addr, sizeof(bt_address_t));

        bt_instance_t *ins = get_ins();
        ble_addr_type_t atype = BT_LE_ADDR_TYPE_UNKNOWN;
        if (ins) atype = bt_device_get_address_type(ins, addr);
        fw_addr_to_zephyr(addr, (atype == BT_LE_ADDR_TYPE_PUBLIC) ? 0 : 1, conn->addr);
    }
    conn->connected = true;
    if (!conn->connect_notified) {
        conn->connect_notified = true;
        notify_connected(conn, 0);
    }
}

void gatts_on_disconnected(gatts_handle_t h, bt_address_t *addr)
{
    _info("[z_api] gatts_on_disconnected\n");
    z_conn_t *conn = conn_lookup_by_fw_addr(addr);
    if (!conn) return;
    conn->connected = false;
    if (!conn->disconnect_notified) {
        conn->disconnect_notified = true;
        notify_disconnected(conn, 0x13);
        /* Don't set in_use=false here; on_conn_state_changed_cb will do it */
    }
}

static void gatts_on_mtu_changed(gatts_handle_t h, bt_address_t *addr, uint32_t mtu)
{
    _info("[z_api] gatts_on_mtu_changed: mtu=%u\n", mtu);
}

static void gatts_on_conn_param_changed(gatts_handle_t h, bt_address_t *addr,
    uint16_t interval, uint16_t latency, uint16_t timeout)
{
    _info("[z_api] gatts_on_conn_param_changed: interval=%d\n", interval);
    z_conn_t *conn = conn_lookup_by_fw_addr(addr);
    if (!conn) return;
    conn->interval = interval;
    conn->latency = latency;
    conn->timeout = timeout;
}

gatts_callbacks_t g_gatts_cbs = {
    .size = sizeof(gatts_callbacks_t),
    .on_connected = gatts_on_connected,
    .on_disconnected = gatts_on_disconnected,
    .on_mtu_changed = gatts_on_mtu_changed,
    .on_conn_param_changed = gatts_on_conn_param_changed,
};

/* ---- Adapter callbacks (for pairing/bonding) ---- */

static void on_conn_state_changed_cb(void *cookie, bt_address_t *addr,
    bt_transport_t transport, connection_state_t state)
{
    _info("[z_api] adapter conn_state: state=%d transport=%d addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
          state, transport,
          addr->addr[0], addr->addr[1], addr->addr[2],
          addr->addr[3], addr->addr[4], addr->addr[5]);

    /* Handle BR/EDR transport connections */
    if (transport == BT_TRANSPORT_BREDR) {
        z_conn_t *conn = conn_lookup_by_fw_addr(addr);

        if (state == CONNECTION_STATE_CONNECTED) {
            if (!conn) {
                conn = conn_alloc();
                if (!conn) {
                    _info("[z_api] conn_state(bredr): no free conn slot!\n");
                    return;
                }
                conn->role = 0; /* BR/EDR initiator */
                memcpy(&conn->fw_addr, addr, sizeof(bt_address_t));
                /* BR/EDR uses public address type (0x00) */
                fw_addr_to_zephyr(addr, 0x00, conn->addr);
            }
            conn->connected = true;
            if (!conn->connect_notified) {
                conn->connect_notified = true;
                _info("[z_api] conn_state(bredr): CONNECTED, notify conn=%p\n", conn);
                notify_connected(conn, 0);
            }
        } else if (state == CONNECTION_STATE_DISCONNECTED) {
            for (int i = 0; i < MAX_CONNECTIONS; i++) {
                if (g_conns[i].in_use &&
                    memcmp(&g_conns[i].fw_addr, addr, sizeof(bt_address_t)) == 0) {
                    g_conns[i].connected = false;
                    if (!g_conns[i].disconnect_notified) {
                        g_conns[i].disconnect_notified = true;
                        _info("[z_api] conn_state(bredr): DISCONNECTED, notify conn[%d]=%p\n",
                              i, &g_conns[i]);
                        notify_disconnected(&g_conns[i], 0x13);
                    }
                    g_conns[i].in_use = false;
                }
            }
        } else if (state == CONNECTION_STATE_ENCRYPTED_BREDR) {
            if (conn) {
                conn->sec_level = 2;
                _info("[z_api] conn_state(bredr): ENCRYPTED, notify conn=%p\n", conn);
                notify_security_changed(conn, conn->sec_level, 0);
            }
        }
        return;
    }

    if (transport != BT_TRANSPORT_BLE) return;

    z_conn_t *conn = conn_lookup_by_fw_addr(addr);

    if (state == CONNECTION_STATE_CONNECTED) {
        if (!conn) {
            /* Incoming connection (peripheral role) */
            conn = conn_alloc();
            if (!conn) {
                _info("[z_api] conn_state: no free conn slot!\n");
                return;
            }
            conn->role = 1; /* peripheral */
            memcpy(&conn->fw_addr, addr, sizeof(bt_address_t));

            bt_instance_t *ins = get_ins();
            ble_addr_type_t atype = BT_LE_ADDR_TYPE_UNKNOWN;
            if (ins) atype = bt_device_get_address_type(ins, addr);
            fw_addr_to_zephyr(addr, (atype == BT_LE_ADDR_TYPE_PUBLIC) ? 0 : 1, conn->addr);

            conn->interval = 48;
            conn->latency = 0;
            conn->timeout = 400;
        }
        conn->connected = true;
        if (!conn->connect_notified) {
            conn->connect_notified = true;
            _info("[z_api] conn_state: CONNECTED, calling notify_connected conn=%p role=%d\n",
                  conn, conn->role);
            notify_connected(conn, 0);

            /* For peripheral role, auto-request preferred connection parameters.
             * Must be done via k_work to run in Zephyr's syswq thread,
             * since L2CAP operations are not thread-safe.
             */
            /* For peripheral role, auto-request preferred connection parameters.
             * Use deferred work with short delay (500ms) to run in Zephyr's syswq.
             * PTS expects param update within ~3 seconds of connection.
             */
            if (conn->role == 1) {
                extern void *bt_conn_lookup_addr_le_mc(uint8_t dev_id, uint8_t id,
                    const void *peer);
                extern void bt_conn_unref(void *conn);
                extern void bt_conn_le_param_update_set(void *conn,
                    uint16_t min, uint16_t max, uint16_t latency, uint16_t timeout);

                uint8_t z_addr[7];
                fw_addr_to_zephyr(addr, 0, z_addr);

                void *zconn = bt_conn_lookup_addr_le_mc(0, 0, z_addr);
                if (zconn) {
                    bt_conn_le_param_update_set(zconn, 24, 40, 0, 400);
                    _info("[z_api] conn_state: scheduled param update for zephyr bt_conn=%p\n", zconn);
                    bt_conn_unref(zconn);
                } else {
                    _info("[z_api] conn_state: no zephyr bt_conn for param update\n");
                }
            }
        }
    } else if (state == CONNECTION_STATE_DISCONNECTED) {
        /* Release ALL conn slots matching this address (handles slot leaks from conn_create) */
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (g_conns[i].in_use &&
                memcmp(&g_conns[i].fw_addr, addr, sizeof(bt_address_t)) == 0) {
                g_conns[i].connected = false;
                if (!g_conns[i].disconnect_notified) {
                    g_conns[i].disconnect_notified = true;
                    _info("[z_api] conn_state: DISCONNECTED, notify conn[%d]=%p\n", i, &g_conns[i]);
                    notify_disconnected(&g_conns[i], 0x13);
                }
                g_conns[i].in_use = false;
            }
        }
    } else if (state == CONNECTION_STATE_ENCRYPTED_LE) {
        if (conn) {
            conn->sec_level = 2;
            _info("[z_api] conn_state: ENCRYPTED, calling notify_security_changed conn=%p\n", conn);
            notify_security_changed(conn, conn->sec_level, 0);
        }
    }
}

static void on_bond_state_changed_cb(void *cookie, bt_address_t *addr,
    bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    _info("[z_api] bond_state: state=%d transport=%d\n", state, transport);

    z_conn_t *conn = conn_lookup_by_fw_addr(addr);
    if (!conn) return;

    if (state == BOND_STATE_BONDED && g_auth_info_cb) {
        typedef struct {
            void (*pairing_complete)(void *conn, bool bonded);
            void (*pairing_failed)(void *conn, uint8_t reason);
        } auth_info_cb_t;
        auth_info_cb_t *aicb = (auth_info_cb_t *)g_auth_info_cb;
        if (aicb->pairing_complete)
            aicb->pairing_complete((void *)conn, true);
    }
}

static void on_pair_request_cb(void *cookie, bt_address_t *addr)
{
    _info("[z_api] pair_request_cb\n");
    bt_device_pair_request_reply(get_ins(), addr, bondable_mode);
}

static void on_ssp_request_cb(void *cookie, bt_address_t *addr,
    bt_transport_t transport, bt_pair_type_t type, uint32_t passkey)
{
    _info("[z_api] ssp_request: type=%d passkey=%u transport=%d\n", type, passkey, transport);

    z_conn_t *conn = conn_lookup_by_fw_addr(addr);
    if (!conn) return;

    typedef void (*passkey_display_t)(void *, unsigned int);
    typedef void (*passkey_entry_t)(void *);
    typedef void (*passkey_confirm_t)(void *, unsigned int);
    typedef void (*pairing_confirm_t)(void *);
    typedef void (*generic_t)(void);

    if (!g_auth_cb) {
        bt_device_set_pairing_confirmation(get_ins(), addr, transport, true);
        return;
    }

    generic_t *cbs = (generic_t *)g_auth_cb;
    /* bt_conn_auth_cb: [0]passkey_display [1]passkey_entry [2]passkey_confirm
     *                  [3]oob_data_request [4]cancel [5]pairing_confirm */
    switch (type) {
    case PAIR_TYPE_PASSKEY_NOTIFICATION:
        if (cbs[0]) ((passkey_display_t)cbs[0])((void *)conn, passkey);
        break;
    case PAIR_TYPE_PASSKEY_ENTRY:
        if (cbs[1]) ((passkey_entry_t)cbs[1])((void *)conn);
        break;
    case PAIR_TYPE_PASSKEY_CONFIRMATION:
        if (cbs[2]) ((passkey_confirm_t)cbs[2])((void *)conn, passkey);
        break;
    case PAIR_TYPE_CONSENT:
        if (cbs[5]) ((pairing_confirm_t)cbs[5])((void *)conn);
        else bt_device_set_pairing_confirmation(get_ins(), addr, transport, true);
        break;
    default:
        bt_device_set_pairing_confirmation(get_ins(), addr, transport, true);
        break;
    }
}

static void on_connect_request_cb(void *cookie, bt_address_t *addr)
{
    _info("[z_api] on_connect_request_cb: auto-accepting incoming BR/EDR connection\n");
    extern bt_status_t bt_sal_acl_connection_reply(bt_controller_id_t id,
                                                   bt_address_t *addr, bool accept);
    bt_sal_acl_connection_reply(PRIMARY_ADAPTER, addr, true);
}

static adapter_callbacks_t gap_cbs = {
    .on_connection_state_changed = on_conn_state_changed_cb,
    .on_bond_state_changed = on_bond_state_changed_cb,
    .on_pair_request = on_pair_request_cb,
    .on_pair_display = on_ssp_request_cb,
    .on_connect_request = on_connect_request_cb,
};

/* ---- Advertising callbacks ---- */
static void on_adv_start_cb(bt_advertiser_t *adv, uint8_t adv_id, uint8_t status)
{
    _info("[z_api] adv_start_cb: adv_id=%d status=%d adv=%p le_advertiser=%p\n",
          adv_id, status, adv, le_advertiser);
    if (status != BT_ADV_STATUS_SUCCESS) {
        if (adv == le_advertiser)
            le_advertiser = NULL;
    }
}

static void on_adv_stopped_cb(bt_advertiser_t *adv, uint8_t adv_id)
{
    _info("[z_api] adv_stopped_cb: adv_id=%d adv=%p le_advertiser=%p\n",
          adv_id, adv, le_advertiser);
    if (adv == le_advertiser)
        le_advertiser = NULL;
}

static advertiser_callback_t adv_cbs = {
    sizeof(advertiser_callback_t),
    on_adv_start_cb,
    on_adv_stopped_cb
};

/* ---- Scan callbacks ---- */
static void on_scan_result_cb(bt_scanner_t *scanner, ble_scan_result_t *result)
{
    (void)scanner;
    typedef void (*scan_cb_fn_t)(const void *addr, int8_t rssi,
                                  uint8_t adv_type, void *buf);
    scan_cb_fn_t scan_cb = (scan_cb_fn_t)user_scan_cb;
    if (!scan_cb || !result) return;

    /* Step 1: Build Zephyr bt_addr_le_t: { type(1), a.val[6] } */
    uint8_t z_addr[7];
    switch (result->addr_type) {
    case BT_LE_ADDR_TYPE_PUBLIC:
        z_addr[0] = 0; /* BT_ADDR_LE_PUBLIC */
        break;
    case BT_LE_ADDR_TYPE_RANDOM:
        z_addr[0] = 1; /* BT_ADDR_LE_RANDOM */
        break;
    default:
        z_addr[0] = 1; /* default to random */
        break;
    }
    memcpy(&z_addr[1], result->addr.addr, 6);

    /* Step 2: Map Framework adv_type to Zephyr BT_GAP_ADV_TYPE_* */
    uint8_t evtype;
    switch (result->adv_type) {
    case BT_LE_ADV_IND:
        evtype = 0x00; /* BT_GAP_ADV_TYPE_ADV_IND */
        break;
    case BT_LE_ADV_DIRECT_IND:
        evtype = 0x01; /* BT_GAP_ADV_TYPE_ADV_DIRECT_IND */
        break;
    case BT_LE_ADV_SCAN_IND:
        evtype = 0x02; /* BT_GAP_ADV_TYPE_ADV_SCAN_IND */
        break;
    case BT_LE_ADV_NONCONN_IND:
        evtype = 0x03; /* BT_GAP_ADV_TYPE_ADV_NONCONN_IND */
        break;
    case 4: /* BT_LE_ADV_SCAN_RSP */
        evtype = 0x04; /* BT_GAP_ADV_TYPE_SCAN_RSP */
        break;
    default:
        evtype = 0x00;
        break;
    }

    /* Step 3: Wrap adv_data in a net_buf_simple on the stack.
     * Layout must match Zephyr's struct net_buf_simple:
     *   uint8_t *data; uint16_t len; uint16_t size; uint8_t *__buf; */
    struct {
        uint8_t *data;
        uint16_t len;
        uint16_t size;
        uint8_t *__buf;
    } buf;
    buf.data = result->adv_data;
    buf.len = result->length;
    buf.size = result->length;
    buf.__buf = result->adv_data;

    /* Step 4: Invoke Zephyr scan callback */
    scan_cb((const void *)z_addr, result->rssi, evtype, (void *)&buf);
}

static void on_scan_start_cb(bt_scanner_t *scanner, uint8_t status)
{
    is_scanning = (status == 0);
    if (status != 0 && scanner_handle == scanner) scanner_handle = NULL;
}

static void on_scan_stopped_cb(bt_scanner_t *scanner)
{
    is_scanning = false;
    (void)scanner;
}

static scanner_callbacks_t scan_cbs = {
    .on_scan_result = on_scan_result_cb,
    .on_scan_start_status = on_scan_start_cb,
    .on_scan_stopped = on_scan_stopped_cb,
};

/* ================================================================
 * GAP init/deinit
 * ================================================================ */

static int gap_init_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    _info("[z_api] gap_init_in_ipc: ins=%p le_gap_handle=%p\n", ins, le_gap_handle);
    if (!ins) return -EIO;

    if (le_gap_handle) {
        _info("[z_api] gap_init_in_ipc: already init\n");
        return -EALREADY;
    }
    le_gap_handle = bt_adapter_register_callback(ins, &gap_cbs);
    _info("[z_api] gap_init: adapter cb=%p\n", le_gap_handle);

    /* Register GATTS for peripheral role connections */
    if (!g_gatts_handle) {
        bt_status_t ret = bt_gatts_register_service(ins, &g_gatts_handle, &g_gatts_cbs);
        _info("[z_api] gap_init: gatts register=%d handle=%p\n", ret, g_gatts_handle);
    }

    /* Register GATTC for connection parameter updates and central role */
    if (!g_gattc_handle) {
        bt_status_t ret = bt_gattc_create_connect(ins, &g_gattc_handle, &g_gattc_cbs);
        _info("[z_api] gap_init: gattc register=%d handle=%p\n", ret, g_gattc_handle);
    }

    /* Install GATT layer callbacks (after handles are registered) */
    extern void z_api_gatt_init(void);
    z_api_gatt_init();

    _info("[z_api] gap_init_in_ipc: done, handle=%p\n", le_gap_handle);
    return le_gap_handle ? 0 : -EIO;
}

int z_api(bt_gap_init)(void)
{
    _info("[z_api] >>> z_bt_gap_init\n");
    return z_api_dispatch(gap_init_in_ipc, NULL);
}

static int gap_deinit_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (le_gap_handle) {
        bt_adapter_unregister_callback(ins, le_gap_handle);
        le_gap_handle = NULL;
    }
    if (g_gatts_handle) {
        bt_gatts_unregister_service(g_gatts_handle);
        g_gatts_handle = NULL;
    }
    return 0;
}

int z_api(bt_gap_deinit)(void)
{
    _info("[z_api] >>> z_bt_gap_deinit\n");
    return z_api_dispatch(gap_deinit_in_ipc, NULL);
}

/* ================================================================
 * bt_enable / bt_disable
 * ================================================================ */

int z_api(bt_enable)(void *cb)
{
    typedef void (*bt_ready_cb_t)(int err);
    bt_ready_cb_t ready_cb = (bt_ready_cb_t)cb;

    _info("[z_api] >>> z_bt_enable: cb=%p is_enabled=%d\n", cb, is_enabled);

    /* Auto-init GAP (register adapter callbacks, GATTS for peripheral) */
    if (!le_gap_handle) {
        _info("[z_api] z_bt_enable: auto-calling gap_init\n");
        int gap_err = z_api_dispatch(gap_init_in_ipc, NULL);
        _info("[z_api] z_bt_enable: gap_init returned %d\n", gap_err);
    }

    /* Enable the adapter through Framework API */
    bt_instance_t *ins = get_ins();
    if (ins) {
        bt_adapter_state_t state = bt_adapter_get_state(ins);
        _info("[z_api] z_bt_enable: adapter state=%d\n", state);
        if (state < BT_ADAPTER_STATE_ON) {
            _info("[z_api] z_bt_enable: calling bt_adapter_enable\n");
            bt_status_t ret = bt_adapter_enable(ins);
            _info("[z_api] z_bt_enable: bt_adapter_enable returned %d\n", ret);

            /* Wait for adapter to reach ON state (up to 10 seconds) */
            for (int i = 0; i < 100; i++) {
                usleep(100000); /* 100ms */
                state = bt_adapter_get_state(ins);
                if (state >= BT_ADAPTER_STATE_ON) break;
            }
            _info("[z_api] z_bt_enable: adapter state after wait=%d\n", state);
        }

        /* Set BR/EDR connectable + discoverable for PTS testing */
        state = bt_adapter_get_state(ins);
        if (state >= BT_ADAPTER_STATE_ON) {
            bt_status_t sm = bt_adapter_set_scan_mode(ins,
                BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
            _info("[z_api] z_bt_enable: set_scan_mode returned %d\n", sm);
        }
    }

    is_enabled = true;
    if (ready_cb) {
        _info("[z_api] z_bt_enable: calling ready_cb(0)\n");
        ready_cb(0);
        _info("[z_api] z_bt_enable: ready_cb returned\n");
    }
    return 0;
}

int z_api(bt_disable)(void)
{
    _info("[z_api] >>> z_bt_disable\n");
    if (!is_enabled) return -EALREADY;
    is_enabled = false;
    return 0;
}

/* ================================================================
 * Advertising
 * ================================================================ */

/* Zephyr BT_LE_ADV_OPT flags */
#define Z_BT_LE_ADV_OPT_CONN           0x03  /* BIT(0) | BIT(1) */
#define Z_BT_LE_ADV_OPT_USE_IDENTITY   0x04  /* BIT(2) */
#define Z_BT_LE_ADV_OPT_DIR_MODE_LOW   0x10  /* BIT(4) */
#define Z_BT_LE_ADV_OPT_FILTER_CONN    0x40  /* BIT(6) */
#define Z_BT_LE_ADV_OPT_FILTER_SCAN    0x80  /* BIT(7) */
#define Z_BT_LE_ADV_OPT_SCANNABLE      0x200 /* BIT(9) */

/* Zephyr bt_le_adv_param layout (must match Zephyr conn.h) */
struct z_bt_le_adv_param {
    uint8_t  id;
    uint8_t  sid;
    uint8_t  secondary_max_skip;
    uint32_t options;
    uint32_t interval_min;
    uint32_t interval_max;
    const void *peer;  /* bt_addr_le_t* */
};

/* Zephyr bt_data layout */
struct z_bt_data {
    uint8_t type;
    uint8_t data_len;
    const uint8_t *data;
};

typedef struct {
    ble_adv_params_t fw_params;
    uint8_t adv_raw[ADV_DATA_LEN_MAX];
    uint8_t sd_raw[ADV_DATA_LEN_MAX];
    uint16_t adv_raw_len;
    uint16_t sd_raw_len;
} adv_start_args_t;

/*
 * Convert Zephyr bt_le_adv_param + bt_data[] into Framework
 * ble_adv_params_t + raw AD/SD bytes.
 */
static void zephyr_adv_param_to_fw(const void *z_param,
                                   const void *ad, size_t ad_len,
                                   const void *sd, size_t sd_len,
                                   adv_start_args_t *out)
{
    const struct z_bt_le_adv_param *p = (const struct z_bt_le_adv_param *)z_param;
    memset(out, 0, sizeof(*out));

    uint32_t opts = p->options;

    /* Determine advertising type */
    if (p->peer != NULL) {
        out->fw_params.adv_type = BT_LE_ADV_DIRECT_IND;
        /* Copy peer address: bt_addr_le_t = { type(1), a.val[6] } */
        const uint8_t *peer_bytes = (const uint8_t *)p->peer;
        out->fw_params.peer_addr_type = (peer_bytes[0] == 0) ?
            BT_LE_ADDR_TYPE_PUBLIC : BT_LE_ADDR_TYPE_RANDOM;
        memcpy(out->fw_params.peer_addr.addr, &peer_bytes[1], 6);
    } else if (opts & Z_BT_LE_ADV_OPT_CONN) {
        out->fw_params.adv_type = BT_LE_ADV_IND;
    } else if (opts & Z_BT_LE_ADV_OPT_SCANNABLE) {
        out->fw_params.adv_type = BT_LE_ADV_SCAN_IND;
    } else {
        out->fw_params.adv_type = BT_LE_ADV_NONCONN_IND;
    }

    /* Own address type */
    if (opts & Z_BT_LE_ADV_OPT_USE_IDENTITY) {
        out->fw_params.own_addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    } else {
        out->fw_params.own_addr_type = BT_LE_ADDR_TYPE_UNKNOWN;
    }

    /* Filter policy */
    if ((opts & Z_BT_LE_ADV_OPT_FILTER_CONN) &&
        (opts & Z_BT_LE_ADV_OPT_FILTER_SCAN)) {
        out->fw_params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_ALL;
    } else if (opts & Z_BT_LE_ADV_OPT_FILTER_CONN) {
        out->fw_params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_CONNECTION;
    } else if (opts & Z_BT_LE_ADV_OPT_FILTER_SCAN) {
        out->fw_params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_SCAN;
    } else {
        out->fw_params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE;
    }

    /* Interval and other params */
    out->fw_params.interval = p->interval_min;
    out->fw_params.duration = 0;
    out->fw_params.channel_map = BT_LE_ADV_CHANNEL_DEFAULT;
    out->fw_params.tx_power = 0;

    /* Serialize AD data: bt_data[] → raw TLV bytes */
    if (ad && ad_len > 0) {
        const struct z_bt_data *ad_arr = (const struct z_bt_data *)ad;
        uint16_t pos = 0;
        for (size_t i = 0; i < ad_len; i++) {
            if (pos + 2 + ad_arr[i].data_len > ADV_DATA_LEN_MAX)
                break;
            out->adv_raw[pos++] = ad_arr[i].data_len + 1;
            out->adv_raw[pos++] = ad_arr[i].type;
            if (ad_arr[i].data_len > 0 && ad_arr[i].data) {
                memcpy(&out->adv_raw[pos], ad_arr[i].data, ad_arr[i].data_len);
                pos += ad_arr[i].data_len;
            }
        }
        out->adv_raw_len = pos;
    }

    /* Serialize SD data */
    if (sd && sd_len > 0) {
        const struct z_bt_data *sd_arr = (const struct z_bt_data *)sd;
        uint16_t pos = 0;
        for (size_t i = 0; i < sd_len; i++) {
            if (pos + 2 + sd_arr[i].data_len > ADV_DATA_LEN_MAX)
                break;
            out->sd_raw[pos++] = sd_arr[i].data_len + 1;
            out->sd_raw[pos++] = sd_arr[i].type;
            if (sd_arr[i].data_len > 0 && sd_arr[i].data) {
                memcpy(&out->sd_raw[pos], sd_arr[i].data, sd_arr[i].data_len);
                pos += sd_arr[i].data_len;
            }
        }
        out->sd_raw_len = pos;
    }
}

static int adv_start_in_ipc(void *arg)
{
    adv_start_args_t *a = (adv_start_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    /* Stop any existing advertising first */
    if (le_advertiser) {
        _info("[z_api] adv_start_in_ipc: stopping existing adv=%p\n", le_advertiser);
        bt_le_stop_advertising(ins, le_advertiser);
        le_advertiser = NULL;
    }

    _info("[z_api] adv_start_in_ipc: type=%d interval=%u adv_len=%u sd_len=%u\n",
          a->fw_params.adv_type, a->fw_params.interval,
          a->adv_raw_len, a->sd_raw_len);

    le_advertiser = bt_le_start_advertising(ins, &a->fw_params,
        a->adv_raw, a->adv_raw_len,
        a->sd_raw, a->sd_raw_len,
        &adv_cbs);

    _info("[z_api] adv_start_in_ipc: le_advertiser=%p\n", le_advertiser);
    return le_advertiser ? 0 : -EIO;
}

int z_api(bt_le_adv_start)(const void *param,
                           const void *ad, size_t ad_len,
                           const void *sd, size_t sd_len)
{
    _info("[z_api] >>> z_bt_le_adv_start: ad_len=%zu sd_len=%zu\n", ad_len, sd_len);

    adv_start_args_t args;
    zephyr_adv_param_to_fw(param, ad, ad_len, sd, sd_len, &args);

    return z_api_dispatch(adv_start_in_ipc, &args);
}

static int adv_stop_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (le_advertiser) {
        _info("[z_api] adv_stop_in_ipc: stopping adv=%p\n", le_advertiser);
        bt_le_stop_advertising(ins, le_advertiser);
        le_advertiser = NULL;
    }
    return 0;
}

int z_api(bt_le_adv_stop)(void)
{
    _info("[z_api] >>> z_bt_le_adv_stop\n");
    return z_api_dispatch(adv_stop_in_ipc, NULL);
}

/* ================================================================
 * Scanning
 * ================================================================ */

/* Zephyr bt_le_scan_param layout (must match Zephyr bluetooth/gap.h) */
struct z_bt_le_scan_param {
    uint8_t  type;       /* BT_LE_SCAN_TYPE_PASSIVE=0, BT_LE_SCAN_TYPE_ACTIVE=1 */
    uint32_t options;
    uint16_t interval;   /* N * 0.625ms */
    uint16_t window;     /* N * 0.625ms */
    uint16_t timeout;    /* N * 10ms, 0 = no timeout */
};

/* Zephyr scan callback: void (*)(const bt_addr_le_t*, int8_t, uint8_t, struct net_buf_simple*) */
typedef void (*z_bt_le_scan_cb_t)(const void *addr, int8_t rssi,
                                  uint8_t adv_type, void *buf);

/* Zephyr net_buf_simple layout (must match zephyr/net_buf.h) */
struct z_net_buf_simple {
    uint8_t *data;
    uint16_t len;
    uint16_t size;
    uint8_t *__buf;
};

typedef struct {
    const struct z_bt_le_scan_param *param;
    z_bt_le_scan_cb_t cb;
} scan_start_args_t;

static int scan_start_in_ipc(void *arg)
{
    scan_start_args_t *a = (scan_start_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    /* Stop existing scan if active */
    if (scanner_handle) {
        _info("[z_api] scan_start_in_ipc: stopping existing scan=%p\n", scanner_handle);
        bt_le_stop_scan(ins, scanner_handle);
        scanner_handle = NULL;
        is_scanning = false;
    }

    /* Convert Zephyr scan params to Framework settings */
    ble_scan_settings_t settings;
    memset(&settings, 0, sizeof(settings));

    /* Map scan type: Zephyr active/passive → Framework scan_type */
    if (a->param->type == 1) { /* BT_LE_SCAN_TYPE_ACTIVE */
        settings.scan_type = BT_LE_SCAN_TYPE_ACTIVE;
    } else {
        settings.scan_type = BT_LE_SCAN_TYPE_PASSIVE;
    }

    /* Use low-latency mode for best responsiveness in PTS testing */
    settings.scan_mode = BT_SCAN_MODE_LOW_LATENCY;
    settings.legacy = 1;
    settings.scan_phy = 1; /* BT_LE_1M_PHY */

    /* Store user callback */
    user_scan_cb = (void *)a->cb;

    _info("[z_api] scan_start_in_ipc: type=%d mode=%d\n",
          settings.scan_type, settings.scan_mode);

    scanner_handle = bt_le_start_scan_settings(ins, &settings, &scan_cbs);

    _info("[z_api] scan_start_in_ipc: scanner_handle=%p\n", scanner_handle);

    if (!scanner_handle) {
        user_scan_cb = NULL;
        return -EIO;
    }

    is_scanning = true;
    return 0;
}

int z_api(bt_le_scan_start)(const void *param, void *cb)
{
    _info("[z_api] >>> z_bt_le_scan_start\n");

    scan_start_args_t args = {
        .param = (const struct z_bt_le_scan_param *)param,
        .cb = (z_bt_le_scan_cb_t)cb,
    };

    return z_api_dispatch(scan_start_in_ipc, &args);
}

static int scan_stop_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    if (scanner_handle) {
        _info("[z_api] scan_stop_in_ipc: stopping scan=%p\n", scanner_handle);
        bt_le_stop_scan(ins, scanner_handle);
        scanner_handle = NULL;
    }
    is_scanning = false;
    user_scan_cb = NULL;
    return 0;
}

int z_api(bt_le_scan_stop)(void)
{
    _info("[z_api] >>> z_bt_le_scan_stop\n");
    return z_api_dispatch(scan_stop_in_ipc, NULL);
}

/* ================================================================
 * OOB
 * ================================================================ */

typedef struct {
    uint8_t id;
    void *oob;
    int result;
} oob_get_args_t;

static int oob_get_local_in_ipc(void *arg)
{
    oob_get_args_t *a = (oob_get_args_t *)arg;
    bt_instance_t *ins = get_ins();
    uint8_t *oob_bytes = (uint8_t *)a->oob;

    _info("[z_api] oob_get_local_in_ipc: ins=%p\n", ins);

    /* We set a public BD_ADDR via VS_WRITE_BD_ADDR in hci_init,
     * so report it as public here. */
    _info("[z_api] oob: using public addr C0:AA:BB:CC:DD:EE\n");
    oob_bytes[0] = 0;    /* BT_ADDR_LE_PUBLIC */
    oob_bytes[1] = 0xEE; oob_bytes[2] = 0xDD; oob_bytes[3] = 0xCC;
    oob_bytes[4] = 0xBB; oob_bytes[5] = 0xAA; oob_bytes[6] = 0xC0;
    return 0;
}

int z_api(bt_le_oob_get_local)(uint8_t id, void *oob)
{
    _info("[z_api] >>> z_bt_le_oob_get_local\n");
    if (!oob) return -EINVAL;
    oob_get_args_t args = { .id = id, .oob = oob };
    return z_api_dispatch(oob_get_local_in_ipc, &args);
}

/* ================================================================
 * Bondable
 * ================================================================ */

static int set_bondable_in_ipc(void *arg)
{
    bool enable = *(bool *)arg;
    bt_instance_t *ins = get_ins();
    if (ins) bt_device_set_bondable_le(ins, enable);
    return 0;
}

int z_api(bt_set_bondable)(bool enable)
{
    _info("[z_api] >>> z_bt_set_bondable: %d\n", enable);
    bondable_mode = enable;
    return z_api_dispatch(set_bondable_in_ipc, &enable);
}

/* ================================================================
 * Connection APIs - using gattc for central, gatts for peripheral
 * ================================================================ */

typedef struct {
    uint8_t z_addr[7];  /* Zephyr bt_addr_le_t */
    void **ret_conn;
    int result;
} conn_create_args_t;

static int conn_create_in_ipc(void *arg)
{
    conn_create_args_t *a = (conn_create_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    /* Allocate connection slot */
    z_conn_t *conn = conn_alloc();
    if (!conn) return -ENOMEM;

    conn->role = 0; /* central */
    memcpy(conn->addr, a->z_addr, 7);
    zephyr_addr_to_fw(a->z_addr, &conn->fw_addr);

    /* Use bt_device_connect_le (same as bttool) */
    ble_addr_type_t addr_type = (a->z_addr[0] == 0) ?
        BT_LE_ADDR_TYPE_PUBLIC : BT_LE_ADDR_TYPE_RANDOM;

    ble_connect_params_t params = {
        .use_default_params = 0,
        .filter_policy = 0, /* BT_LE_CONNECT_FILTER_POLICY_ADDR */
        .init_phy = 1,      /* BT_LE_1M_PHY */
        .scan_interval = 20,
        .scan_window = 20,
        .connection_interval_min = 24,  /* 30ms */
        .connection_interval_max = 24,  /* 30ms */
        .connection_latency = 0,
        .supervision_timeout = 400,     /* 4000ms */
        .min_ce_length = 0,
        .max_ce_length = 0,
    };

    _info("[z_api] conn_create: calling bt_device_connect_le addr=%02x:%02x:%02x:%02x:%02x:%02x type=%d\n",
          conn->fw_addr.addr[0], conn->fw_addr.addr[1], conn->fw_addr.addr[2],
          conn->fw_addr.addr[3], conn->fw_addr.addr[4], conn->fw_addr.addr[5], addr_type);

    bt_status_t ret = bt_device_connect_le(ins, &conn->fw_addr, addr_type, &params);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api] conn_create: bt_device_connect_le failed=%d\n", ret);
        conn->in_use = false;
        return -EIO;
    }

    _info("[z_api] conn_create: connect_le initiated, conn=%p\n", conn);
    if (a->ret_conn) *(a->ret_conn) = (void *)conn;
    return 0;
}

int z_api(bt_conn_le_create)(const void *peer, const void *create_param,
                             const void *conn_param, void **ret_conn)
{
    _info("[z_api] >>> z_bt_conn_le_create\n");
    if (!peer || !ret_conn) return -EINVAL;

    conn_create_args_t args;
    memcpy(args.z_addr, peer, 7);
    args.ret_conn = ret_conn;
    return z_api_dispatch(conn_create_in_ipc, &args);
}

static int conn_create_auto_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    /* Allocate connection slot for auto connection */
    z_conn_t *conn = conn_alloc();
    if (!conn) return -ENOMEM;

    conn->role = 0; /* central */
    /* Address will be filled in when connected callback fires */
    memset(conn->addr, 0, sizeof(conn->addr));

    /* Use filter accept list (white list) policy - controller will
     * connect to any device in the accept list that is advertising */
    bt_address_t dummy_addr;
    memset(&dummy_addr, 0, sizeof(dummy_addr));

    ble_connect_params_t params = {
        .use_default_params = 0,
        .filter_policy = 1, /* BT_LE_CONNECT_FILTER_POLICY_WHITE_LIST */
        .init_phy = 0,      /* BT_LE_1M_PHY */
        .scan_interval = 20,
        .scan_window = 20,
        .connection_interval_min = 48,  /* 60ms */
        .connection_interval_max = 48,  /* 60ms */
        .connection_latency = 0,
        .supervision_timeout = 400,     /* 4000ms */
        .min_ce_length = 0,
        .max_ce_length = 0,
    };

    _info("[z_api] conn_create_auto: calling bt_device_connect_le with filter_policy=WHITE_LIST\n");

    bt_status_t ret = bt_device_connect_le(ins, &dummy_addr, BT_LE_ADDR_TYPE_PUBLIC, &params);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api] conn_create_auto: bt_device_connect_le failed=%d\n", ret);
        conn->in_use = false;
        return -EIO;
    }

    _info("[z_api] conn_create_auto: auto connect initiated, conn=%p\n", conn);
    return 0;
}

int z_api(bt_conn_le_create_auto)(const void *create_param, const void *conn_param)
{
    _info("[z_api] >>> z_bt_conn_le_create_auto\n");
    return z_api_dispatch(conn_create_auto_in_ipc, NULL);
}

typedef struct {
    z_conn_t *conn;
    uint8_t reason;
} conn_disconnect_args_t;

static int conn_disconnect_in_ipc(void *arg)
{
    conn_disconnect_args_t *a = (conn_disconnect_args_t *)arg;
    z_conn_t *conn = a->conn;

    _info("[z_api] conn_disconnect: role=%d addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
          conn->role, conn->fw_addr.addr[0], conn->fw_addr.addr[1],
          conn->fw_addr.addr[2], conn->fw_addr.addr[3],
          conn->fw_addr.addr[4], conn->fw_addr.addr[5]);

    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_status_t ret = bt_device_disconnect_le(ins, &conn->fw_addr);
    _info("[z_api] conn_disconnect: ret=%d\n", ret);
    return (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
}

int z_api(bt_conn_disconnect)(void *conn, uint8_t reason)
{
    _info("[z_api] >>> z_bt_conn_disconnect\n");
    if (!conn) return -EINVAL;
    conn_disconnect_args_t args = { .conn = (z_conn_t *)conn, .reason = reason };
    return z_api_dispatch(conn_disconnect_in_ipc, &args);
}

void *z_api(bt_conn_lookup_addr_le)(uint8_t id, const void *peer)
{
    _info("[z_api] >>> z_bt_conn_lookup_addr_le\n");
    if (!peer) return NULL;
    const uint8_t *z_addr = (const uint8_t *)peer;
    _info("[z_api] lookup addr: type=%d %02x:%02x:%02x:%02x:%02x:%02x\n",
          z_addr[0], z_addr[6], z_addr[5], z_addr[4], z_addr[3], z_addr[2], z_addr[1]);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (g_conns[i].in_use) {
            _info("[z_api] conn[%d]: in_use=%d connected=%d type=%d %02x:%02x:%02x:%02x:%02x:%02x\n",
                  i, g_conns[i].in_use, g_conns[i].connected,
                  g_conns[i].addr[0],
                  g_conns[i].addr[6], g_conns[i].addr[5], g_conns[i].addr[4],
                  g_conns[i].addr[3], g_conns[i].addr[2], g_conns[i].addr[1]);
        }
    }
    z_conn_t *conn = conn_lookup_by_zephyr_addr(z_addr);
    if (conn) {
        conn->ref_count++;
        _info("[z_api] lookup: FOUND conn=%p\n", conn);
    } else {
        _info("[z_api] lookup: NOT FOUND\n");
    }
    return (void *)conn;
}

void z_api(bt_conn_unref)(void *conn)
{
    if (!conn) return;
    z_conn_t *c = (z_conn_t *)conn;
    if (c->ref_count > 0) c->ref_count--;
}

void *z_api(bt_conn_ref)(void *conn)
{
    if (!conn) return NULL;
    z_conn_t *c = (z_conn_t *)conn;
    c->ref_count++;
    return conn;
}

int z_api(bt_conn_get_info)(void *conn, void *info)
{
    _info("[z_api] >>> z_bt_conn_get_info: conn=%p\n", conn);
    if (!conn || !info) return -EINVAL;

    z_conn_t *c = (z_conn_t *)conn;

    /*
     * bt_conn_info layout (from Zephyr conn.h):
     *   uint16_t handle;
     *   enum bt_conn_type type;  // uint8_t, BT_CONN_TYPE_LE=1
     *   uint8_t role;
     *   uint8_t id;
     *   // padding to align union
     *   union {
     *     struct bt_conn_le_info {
     *       const bt_addr_le_t *src;
     *       const bt_addr_le_t *dst;
     *       const bt_addr_le_t *local;
     *       const bt_addr_le_t *remote;
     *       uint16_t interval;
     *       uint16_t latency;
     *       uint16_t timeout;
     *       // phy info pointer (if CONFIG_BT_USER_PHY_UPDATE)
     *     } le;
     *   };
     *   enum bt_conn_state state;
     *   struct bt_security_info security; // { uint8_t level; uint8_t enc_key_size; enum flags; }
     */
    typedef struct {
        uint16_t handle;
        uint8_t type;
        uint8_t role;
        uint8_t id;
        uint8_t _pad[3]; /* align to pointer */
        /* le sub-struct */
        const void *src;
        const void *dst;
        const void *local;
        const void *remote;
        uint16_t interval;
        uint16_t latency;
        uint16_t timeout;
        uint16_t _pad2;
        const void *phy;
        /* state */
        uint8_t state;
        uint8_t _pad3[3];
        /* security */
        uint8_t security_level;
        uint8_t enc_key_size;
        uint8_t security_flags;
    } z_conn_info_t;

    z_conn_info_t *ci = (z_conn_info_t *)info;
    memset(ci, 0, sizeof(z_conn_info_t));
    ci->handle = 0;
    ci->type = 1;  /* BT_CONN_TYPE_LE */
    ci->role = c->role;
    ci->id = 0;
    ci->src = NULL;
    ci->dst = (const void *)c->addr;
    ci->local = NULL;
    ci->remote = (const void *)c->addr;
    ci->interval = c->interval;
    ci->latency = c->latency;
    ci->timeout = c->timeout;
    ci->state = c->connected ? 1 : 0; /* BT_CONN_STATE_CONNECTED=1 */
    ci->security_level = c->sec_level;
    ci->enc_key_size = c->enc_key_size;

    _info("[z_api] bt_conn_get_info: role=%d interval=%d dst=%p\n",
          ci->role, ci->interval, ci->dst);

    return 0;
}

const void *z_api(bt_conn_get_dst)(const void *conn)
{
    if (!conn) return NULL;
    const z_conn_t *c = (const z_conn_t *)conn;
    return (const void *)c->addr;
}

int z_api(bt_conn_get_security)(void *conn)
{
    if (!conn) return 1;
    return ((z_conn_t *)conn)->sec_level;
}

void z_api(bt_conn_cb_register)(void *cb)
{
    _info("[z_api] >>> z_bt_conn_cb_register\n");
    if (!cb) return;
    /* Check if already registered to avoid duplicates */
    for (z_conn_cb_node_t *n = g_conn_cb_list; n; n = n->next) {
        if (n->cb == cb) {
            _info("[z_api] bt_conn_cb_register: already registered, skip\n");
            return;
        }
    }
    z_conn_cb_node_t *node = (z_conn_cb_node_t *)malloc(sizeof(z_conn_cb_node_t));
    if (!node) return;
    node->cb = cb;
    node->next = g_conn_cb_list;
    g_conn_cb_list = node;
}

void z_api(bt_conn_cb_unregister)(void *cb)
{
    _info("[z_api] >>> z_bt_conn_cb_unregister\n");
    z_conn_cb_node_t **pp = &g_conn_cb_list;
    while (*pp) {
        if ((*pp)->cb == cb) {
            z_conn_cb_node_t *tmp = *pp;
            *pp = tmp->next;
            free(tmp);
            return;
        }
        pp = &(*pp)->next;
    }
}

typedef struct {
    z_conn_t *conn;
    uint16_t interval_min;
    uint16_t interval_max;
    uint16_t latency;
    uint16_t timeout;
} param_update_args_t;

static int param_update_in_ipc(void *arg)
{
    param_update_args_t *a = (param_update_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    gattc_handle_t handle = a->conn->gattc_handle ? a->conn->gattc_handle : g_gattc_handle;
    if (!handle) {
        _info("[z_api] param_update: no gattc handle available\n");
        return -EIO;
    }

    _info("[z_api] param_update_in_ipc: handle=%p min=%d max=%d lat=%d to=%d\n",
          handle, a->interval_min, a->interval_max, a->latency, a->timeout);

    bt_status_t ret = bt_gattc_update_connection_parameter(handle,
        a->interval_min, a->interval_max, a->latency, a->timeout, 0, 0);

    _info("[z_api] param_update_in_ipc: ret=%d\n", ret);
    return (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
}

int z_api(bt_conn_le_param_update)(void *conn, const void *param)
{
    _info("[z_api] >>> z_bt_conn_le_param_update\n");
    if (!conn || !param) return -EINVAL;

    /* bt_le_conn_param: { interval_min, interval_max, latency, timeout } */
    const uint16_t *p = (const uint16_t *)param;

    param_update_args_t args = {
        .conn = (z_conn_t *)conn,
        .interval_min = p[0],
        .interval_max = p[1],
        .latency = p[2],
        .timeout = p[3],
    };

    _info("[z_api] param_update: min=%d max=%d latency=%d timeout=%d\n",
          args.interval_min, args.interval_max, args.latency, args.timeout);

    return z_api_dispatch(param_update_in_ipc, &args);
}

/* ================================================================
 * Security / Auth APIs
 * ================================================================ */

typedef struct {
    z_conn_t *conn;
    int sec;
} set_security_args_t;

static int set_security_in_ipc(void *arg)
{
    set_security_args_t *a = (set_security_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    /* Map Zephyr security level to Framework:
     * BT_SECURITY_L2 = authenticated pairing
     * BT_SECURITY_L3 = authenticated + encrypted
     * BT_SECURITY_L4 = authenticated + encrypted + SC */
    int level = a->sec & 0x0F;
    if (level >= 2) {
        bt_device_set_security_level(ins, level, BT_TRANSPORT_BLE);
        bt_device_create_bond(ins, &a->conn->fw_addr, BT_TRANSPORT_BLE);
    }
    return 0;
}

int z_api(bt_conn_set_security)(void *conn, int sec)
{
    _info("[z_api] >>> z_bt_conn_set_security: sec=%d\n", sec);
    if (!conn) return -EINVAL;
    set_security_args_t args = { .conn = (z_conn_t *)conn, .sec = sec };
    return z_api_dispatch(set_security_in_ipc, &args);
}

uint8_t z_api(bt_conn_enc_key_size)(void *conn)
{
    if (!conn) return 16;
    return ((z_conn_t *)conn)->enc_key_size;
}

int z_api(bt_conn_auth_cb_register)(const void *cb)
{
    _info("[z_api] >>> z_bt_conn_auth_cb_register\n");
    g_auth_cb = cb;
    return 0;
}

int z_api(bt_conn_auth_info_cb_register)(void *cb)
{
    _info("[z_api] >>> z_bt_conn_auth_info_cb_register\n");
    g_auth_info_cb = cb;
    return 0;
}

typedef struct {
    z_conn_t *conn;
    unsigned int passkey;
} passkey_args_t;

static int passkey_entry_in_ipc(void *arg)
{
    passkey_args_t *a = (passkey_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;
    bt_device_set_pass_key(ins, &a->conn->fw_addr, BT_TRANSPORT_BLE, true, a->passkey);
    return 0;
}

int z_api(bt_conn_auth_passkey_entry)(void *conn, unsigned int passkey)
{
    _info("[z_api] >>> z_bt_conn_auth_passkey_entry: %u\n", passkey);
    if (!conn) return -EINVAL;
    passkey_args_t args = { .conn = (z_conn_t *)conn, .passkey = passkey };
    return z_api_dispatch(passkey_entry_in_ipc, &args);
}

static int passkey_confirm_in_ipc(void *arg)
{
    z_conn_t *conn = (z_conn_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;
    bt_device_set_pairing_confirmation(ins, &conn->fw_addr, BT_TRANSPORT_BLE, true);
    return 0;
}

int z_api(bt_conn_auth_passkey_confirm)(void *conn)
{
    _info("[z_api] >>> z_bt_conn_auth_passkey_confirm\n");
    if (!conn) return -EINVAL;
    return z_api_dispatch(passkey_confirm_in_ipc, conn);
}

static int auth_cancel_in_ipc(void *arg)
{
    z_conn_t *conn = (z_conn_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;
    bt_device_set_pairing_confirmation(ins, &conn->fw_addr, BT_TRANSPORT_BLE, false);
    return 0;
}

int z_api(bt_conn_auth_cancel)(void *conn)
{
    _info("[z_api] >>> z_bt_conn_auth_cancel\n");
    if (!conn) return -EINVAL;
    return z_api_dispatch(auth_cancel_in_ipc, conn);
}

/* ================================================================
 * Unpair / Bond check
 * ================================================================ */

typedef struct {
    bt_address_t fw_addr;
    bool has_addr;
} unpair_args_t;

static int unpair_in_ipc(void *arg)
{
    unpair_args_t *a = (unpair_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;
    if (a->has_addr)
        bt_device_remove_bond(ins, &a->fw_addr, BT_TRANSPORT_BLE);
    return 0;
}

int z_api(bt_unpair)(uint8_t id, const void *addr)
{
    _info("[z_api] >>> z_bt_unpair\n");
    unpair_args_t args;
    memset(&args, 0, sizeof(args));
    if (addr) {
        const uint8_t *z_addr = (const uint8_t *)addr;
        zephyr_addr_to_fw(z_addr, &args.fw_addr);
        args.has_addr = true;
    }
    return z_api_dispatch(unpair_in_ipc, &args);
}

bool z_api(bt_addr_le_is_bonded)(uint8_t id, const void *addr)
{
    _info("[z_api] >>> z_bt_addr_le_is_bonded\n");
    if (!addr) return false;
    bt_instance_t *ins = get_ins();
    if (!ins) return false;

    bt_address_t fw_addr;
    zephyr_addr_to_fw((const uint8_t *)addr, &fw_addr);
    return bt_device_is_bonded(ins, &fw_addr, BT_TRANSPORT_BLE);
}

/* ================================================================
 * OOB SC/Legacy stubs
 * ================================================================ */

int z_api(bt_le_oob_set_sc_data)(void *conn, const void *oobd_local, const void *oobd_remote)
{
    _info("[z_api] >>> z_bt_le_oob_set_sc_data\n");
    if (!conn) return -EINVAL;
    (void)conn;
    /* TODO: call bt_device_set_le_sc_remote_oob_data */
    return 0;
}

int z_api(bt_le_oob_set_legacy_tk)(void *conn, const uint8_t *tk)
{
    _info("[z_api] >>> z_bt_le_oob_set_legacy_tk\n");
    if (!conn || !tk) return -EINVAL;
    (void)conn; (void)tk;
    /* TODO: call bt_device_set_le_legacy_tk */
    return 0;
}

int z_api(bt_le_oob_set_sc_flag)(bool enable)
{
    sc_oob_flag = enable;
    return 0;
}

int z_api(bt_le_oob_set_legacy_flag)(bool enable)
{
    legacy_oob_flag = enable;
    return 0;
}

#define MAX_FILTER_LIST_SIZE 8
static bt_address_t filter_list_addrs[MAX_FILTER_LIST_SIZE];
static int filter_list_count = 0;

static int filter_accept_list_clear_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    for (int i = 0; i < filter_list_count; i++) {
        bt_adapter_le_remove_whitelist(ins, &filter_list_addrs[i]);
    }
    filter_list_count = 0;
    _info("[z_api] filter_accept_list_clear done\n");
    return 0;
}

int z_api(bt_le_filter_accept_list_clear)(void)
{
    _info("[z_api] >>> bt_le_filter_accept_list_clear\n");
    return z_api_dispatch(filter_accept_list_clear_in_ipc, NULL);
}

static int filter_accept_list_add_in_ipc(void *arg)
{
    bt_address_t *fw_addr = (bt_address_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_status_t ret = bt_adapter_le_add_whitelist(ins, fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api] filter_accept_list_add: failed=%d\n", ret);
        return -EIO;
    }

    if (filter_list_count < MAX_FILTER_LIST_SIZE) {
        memcpy(&filter_list_addrs[filter_list_count], fw_addr, sizeof(bt_address_t));
        filter_list_count++;
    }

    _info("[z_api] filter_accept_list_add: addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
          fw_addr->addr[0], fw_addr->addr[1], fw_addr->addr[2],
          fw_addr->addr[3], fw_addr->addr[4], fw_addr->addr[5]);
    return 0;
}

int z_api(bt_le_filter_accept_list_add)(const void *addr)
{
    _info("[z_api] >>> bt_le_filter_accept_list_add\n");
    if (!addr) return -EINVAL;

    const uint8_t *z_addr = (const uint8_t *)addr;
    bt_address_t fw_addr;
    zephyr_addr_to_fw(z_addr, &fw_addr);

    return z_api_dispatch(filter_accept_list_add_in_ipc, &fw_addr);
}

/* ================================================================
 * BR/EDR GAP APIs
 * ================================================================ */

typedef struct {
    uint8_t z_addr[7];
    void **ret_conn;
} br_conn_create_args_t;

static int br_conn_create_in_ipc(void *arg)
{
    br_conn_create_args_t *a = (br_conn_create_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    z_conn_t *conn = conn_alloc();
    if (!conn) return -ENOMEM;

    conn->role = 0; /* initiator */
    memcpy(conn->addr, a->z_addr, 7);
    zephyr_addr_to_fw(a->z_addr, &conn->fw_addr);

    _info("[z_api] br_conn_create: calling bt_device_connect addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
          conn->fw_addr.addr[0], conn->fw_addr.addr[1], conn->fw_addr.addr[2],
          conn->fw_addr.addr[3], conn->fw_addr.addr[4], conn->fw_addr.addr[5]);

    bt_status_t ret = bt_device_connect(ins, &conn->fw_addr);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api] br_conn_create: bt_device_connect failed=%d\n", ret);
        conn->in_use = false;
        return -EIO;
    }

    _info("[z_api] br_conn_create: connect initiated, conn=%p\n", conn);
    if (a->ret_conn) *(a->ret_conn) = (void *)conn;
    return 0;
}

int z_bt_conn_create(const void *peer, void **ret_conn)
{
    _info("[z_api] >>> z_bt_conn_create\n");
    if (!peer) return -EINVAL;

    br_conn_create_args_t args;
    memcpy(args.z_addr, peer, 7);
    args.ret_conn = ret_conn;
    return z_api_dispatch(br_conn_create_in_ipc, &args);
}

typedef struct {
    uint32_t timeout;
} br_discovery_args_t;

static int br_discovery_start_in_ipc(void *arg)
{
    br_discovery_args_t *a = (br_discovery_args_t *)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_status_t ret = bt_adapter_start_discovery(ins, a->timeout);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api] br_discovery_start: failed=%d\n", ret);
        return -EIO;
    }

    _info("[z_api] br_discovery_start: started, timeout=%u\n", a->timeout);
    return 0;
}

int z_bt_br_discovery_start(uint32_t timeout)
{
    _info("[z_api] >>> z_bt_br_discovery_start: timeout=%u\n", timeout);

    br_discovery_args_t args;
    args.timeout = timeout;
    return z_api_dispatch(br_discovery_start_in_ipc, &args);
}

static int br_discovery_stop_in_ipc(void *arg)
{
    (void)arg;
    bt_instance_t *ins = get_ins();
    if (!ins) return -EIO;

    bt_status_t ret = bt_adapter_cancel_discovery(ins);
    if (ret != BT_STATUS_SUCCESS) {
        _info("[z_api] br_discovery_stop: failed=%d\n", ret);
        return -EIO;
    }

    _info("[z_api] br_discovery_stop: stopped\n");
    return 0;
}

int z_bt_br_discovery_stop(void)
{
    _info("[z_api] >>> z_bt_br_discovery_stop\n");
    return z_api_dispatch(br_discovery_stop_in_ipc, NULL);
}
