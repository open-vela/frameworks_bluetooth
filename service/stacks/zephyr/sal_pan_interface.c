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

/**
 * BNEP (Bluetooth Network Encapsulation Protocol) SAL implementation
 * on top of the zblue BR/EDR L2CAP (PSM 0x000F).
 *
 * Implemented 2026-08-15 (docs_ble round 3): BREDR gate-0 passed on the
 * SF32LB52, so PAN became reachable. Minimal PANU implementation:
 *   - connect to a NAP (dst_role=1, src_role=2), setup handshake
 *   - data path: BNEP_FRAME_ETH (0x00) + 2-byte EtherType + payload
 *     (no MAC extension headers; peer MAC derived from the BT address,
 *      dst MAC = broadcast on receive)
 *   - events reported to the framework profile via pan_on_*_changed()
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>

#include "bt_addr.h"
#include "bt_list.h"
#include "sal_interface.h"
#include "sal_pan_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#define PAN_TX_BUF_SIZE 1600 /* 3 hdr + max eth payload */
#define PAN_TX_BUF_COUNT 6

NET_BUF_POOL_FIXED_DEFINE(pan_tx_pool, PAN_TX_BUF_COUNT,
    PAN_TX_BUF_SIZE, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

typedef enum {
    PAN_CONN_ACL_PENDING = 0,  /* waiting BR/EDR ACL */
    PAN_CONN_L2CAP_PENDING,    /* waiting L2CAP channel */
    PAN_CONN_BNEP_PENDING,     /* waiting BNEP setup response */
    PAN_CONN_CONNECTED,
} pan_conn_state_t;

typedef struct {
    bt_address_t addr;
    pan_conn_state_t state;
    uint8_t dst_role;
    uint8_t src_role;
    struct bt_l2cap_chan chan;
} pan_conn_t;

static struct {
    uint8_t max_connections;
    uint8_t role;
    bool initialized;
    bt_list_t* conn_list;
    struct bt_conn* acl_conn;  /* BR/EDR ACL under setup (unref'd on up/fail) */
    struct bt_conn_cb conn_cb;
} g_pan = {
    .max_connections = 1,
    .role = 0,
    .initialized = false,
    .conn_list = NULL,
    .acl_conn = NULL,
};

/* -- helpers --------------------------------------------------- */

static pan_conn_t* pan_find_conn(const bt_address_t* addr)
{
    bt_list_node_t* node;

    for (node = bt_list_head(g_pan.conn_list); node;
         node = bt_list_next(g_pan.conn_list, node)) {
        pan_conn_t* conn = (pan_conn_t*)bt_list_node(node);
        if (conn && !memcmp(&conn->addr, addr, sizeof(bt_address_t))) {
            return conn;
        }
    }
    return NULL;
}

static void pan_conn_report(pan_conn_t* conn, profile_connection_state_t state)
{
    pan_on_connection_state_changed(&conn->addr, conn->dst_role,
        conn->src_role, state);
}

static void pan_conn_free(pan_conn_t* conn)
{
    bt_list_remove(g_pan.conn_list, conn);
    free(conn);
}

/* -- L2CAP channel callbacks ----------------------------------- */

static void pan_chan_connected(struct bt_l2cap_chan* chan)
{
    pan_conn_t* conn = CONTAINER_OF(chan, pan_conn_t, chan);
    uint8_t req[3];

    if (!conn || conn->state != PAN_CONN_L2CAP_PENDING) {
        BT_LOGW("%s unexpected state", __func__);
        return;
    }

    /* BNEP Setup Connection Request: [0x01][dst_role][src_role] */
    req[0] = BNEP_SETUP_CONN_REQ;
    req[1] = conn->dst_role;
    req[2] = conn->src_role;

    struct net_buf* buf = net_buf_alloc(&pan_tx_pool, K_NO_WAIT);
    if (!buf) {
        BT_LOGE("%s tx pool exhausted", __func__);
        return;
    }
    net_buf_reserve(buf, CONFIG_BT_L2CAP_TX_MTU);
    net_buf_add_mem(buf, req, sizeof(req));

    int ret = bt_l2cap_chan_send(chan, buf);
    if (ret < 0) {
        BT_LOGE("%s send setup req failed: %d", __func__, ret);
        net_buf_unref(buf);
        return;
    }
    conn->state = PAN_CONN_BNEP_PENDING;
}

static void pan_chan_disconnected(struct bt_l2cap_chan* chan)
{
    pan_conn_t* conn = CONTAINER_OF(chan, pan_conn_t, chan);

    if (!conn) {
        return;
    }
    if (conn->state == PAN_CONN_CONNECTED) {
        pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
    }
    pan_conn_free(conn);
}

static int pan_chan_recv(struct bt_l2cap_chan* chan, struct net_buf* buf)
{
    pan_conn_t* conn = CONTAINER_OF(chan, pan_conn_t, chan);
    uint8_t* data = buf->data;
    uint16_t len = buf->len;

    if (!conn || len < 1) {
        return -EINVAL;
    }

    switch (data[0]) {
    case BNEP_SETUP_CONN_RESP: {
        uint16_t resp;
        if (len < 3) {
            return -EINVAL;
        }
        resp = (uint16_t)((data[1] << 8) | data[2]);
        BT_LOGI("%s setup resp 0x%04x", __func__, resp);
        if (resp == BNEP_CONN_RESP_SUCCESS) {
            conn->state = PAN_CONN_CONNECTED;
            pan_conn_report(conn, PROFILE_STATE_CONNECTED);
        } else {
            BT_LOGE("%s setup failed 0x%04x", __func__, resp);
            pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
            pan_conn_free(conn);
        }
        break;
    }
    case BNEP_FRAME_ETH: {
        uint16_t protocol;
        uint8_t eth[6 + 6 + 2];

        if (len < 3) {
            return -EINVAL;
        }
        protocol = (uint16_t)((data[1] << 8) | data[2]);

        /* Rebuild the Ethernet frame for the TAP bridge:
         * src MAC = peer BT address, dst MAC = broadcast. */
        memset(eth, 0xff, 6);
        memcpy(eth + 6, conn->addr.addr, 6);
        pan_on_data_received(&conn->addr, protocol, eth, eth + 6,
            data + 3, len - 3);
        break;
    }
    case BNEP_SETUP_CONN_REQ: {
        /* Incoming connection (we are PANU, not NAP): reject. */
        uint8_t resp[3] = { BNEP_SETUP_CONN_RESP, 0x00, 0x03 };
        struct net_buf* out = net_buf_alloc(&pan_tx_pool, K_NO_WAIT);
        if (out) {
            net_buf_reserve(out, CONFIG_BT_L2CAP_TX_MTU);
            net_buf_add_mem(out, resp, sizeof(resp));
            bt_l2cap_chan_send(chan, out);
        }
        break;
    }
    default:
        BT_LOGW("%s unhandled control type 0x%02x", __func__, data[0]);
        break;
    }

    return 0;
}

static const struct bt_l2cap_chan_ops g_pan_chan_ops = {
    .connected = pan_chan_connected,
    .disconnected = pan_chan_disconnected,
    .recv = pan_chan_recv,
};

/* -- BR/EDR connection callbacks ------------------------------- */

static void pan_br_connected(struct bt_conn* conn, uint8_t err)
{
    pan_conn_t* pconn;
    bt_address_t addr;
    struct bt_conn_info info;

    if (err) {
        BT_LOGE("%s ACL err %d", __func__, err);
        if (g_pan.acl_conn) {
            bt_conn_unref(g_pan.acl_conn);
            g_pan.acl_conn = NULL;
        }
        return;
    }

    if (bt_conn_get_info(conn, &info) != 0 || !info.br.dst) {
        BT_LOGE("%s info fail", __func__);
        return;
    }
    bt_addr_set(&addr, info.br.dst->val);

    if (g_pan.acl_conn) {
        bt_conn_unref(g_pan.acl_conn);
        g_pan.acl_conn = NULL;
    }

    pconn = pan_find_conn(&addr);
    if (!pconn || pconn->state != PAN_CONN_ACL_PENDING) {
        BT_LOGW("%s no pending pan conn", __func__);
        return;
    }

    /* Establish the L2CAP channel on PSM 0x000F */
    pconn->state = PAN_CONN_L2CAP_PENDING;
    int ret = bt_l2cap_chan_connect(conn, &pconn->chan, BT_BNEP_PSM);
    if (ret < 0) {
        BT_LOGE("%s l2cap connect failed: %d", __func__, ret);
        pan_conn_report(pconn, PROFILE_STATE_DISCONNECTED);
        pan_conn_free(pconn);
    }
}

static void pan_br_disconnected(struct bt_conn* conn, uint8_t reason)
{
    bt_address_t addr;
    struct bt_conn_info info;
    pan_conn_t* pconn;

    (void)reason;
    if (bt_conn_get_info(conn, &info) != 0 || !info.br.dst) {
        return;
    }
    bt_addr_set(&addr, info.br.dst->val);

    pconn = pan_find_conn(&addr);
    if (!pconn) {
        return;
    }
    BT_LOGW("%s reason %d", __func__, reason);
    if (pconn->state == PAN_CONN_CONNECTED) {
        pan_conn_report(pconn, PROFILE_STATE_DISCONNECTED);
    }
    pan_conn_free(pconn);
}

/* -- Public API ------------------------------------------------ */

bt_status_t bt_sal_pan_init(uint8_t max_connections, uint8_t role)
{
    if (g_pan.initialized) {
        return BT_STATUS_SUCCESS;
    }
    BT_LOGI("%s max=%u role=%u", __func__, max_connections, role);

    g_pan.max_connections = max_connections;
    g_pan.role = role;
    g_pan.conn_list = bt_list_new(NULL);
    if (!g_pan.conn_list) {
        return BT_STATUS_NOMEM;
    }

    g_pan.conn_cb.connected = pan_br_connected;
    g_pan.conn_cb.disconnected = pan_br_disconnected;
    bt_conn_cb_register(&g_pan.conn_cb);

    g_pan.initialized = true;
    return BT_STATUS_SUCCESS;
}

void bt_sal_pan_cleanup(void)
{
    bt_list_node_t* node;

    if (!g_pan.initialized) {
        return;
    }
    while ((node = bt_list_head(g_pan.conn_list)) != NULL) {
        pan_conn_t* conn = (pan_conn_t*)bt_list_node(node);
        if (conn->state >= PAN_CONN_L2CAP_PENDING) {
            bt_l2cap_chan_disconnect(&conn->chan);
        }
        pan_conn_free(conn);
    }
    if (g_pan.acl_conn) {
        bt_conn_disconnect(g_pan.acl_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        bt_conn_unref(g_pan.acl_conn);
        g_pan.acl_conn = NULL;
    }
    bt_conn_cb_unregister(&g_pan.conn_cb);
    bt_list_free(g_pan.conn_list);
    g_pan.conn_list = NULL;
    g_pan.initialized = false;
}

bt_status_t bt_sal_pan_connect(bt_address_t* addr, uint8_t dst_role,
    uint8_t src_role)
{
    pan_conn_t* conn;
    struct bt_conn* acl;

    if (!addr || !g_pan.initialized) {
        return BT_STATUS_PARM_INVALID;
    }

    if (pan_find_conn(addr)) {
        BT_LOGW("%s already exists", __func__);
        return BT_STATUS_BUSY;
    }

    conn = calloc(1, sizeof(pan_conn_t));
    if (!conn) {
        return BT_STATUS_NOMEM;
    }
    conn->chan.ops = &g_pan_chan_ops;
    memcpy(&conn->addr, addr, sizeof(bt_address_t));
    conn->dst_role = dst_role;
    conn->src_role = src_role;
    conn->state = PAN_CONN_ACL_PENDING;
    bt_list_add_tail(g_pan.conn_list, conn);
    pan_conn_report(conn, PROFILE_STATE_CONNECTING);

    /* Establish the BR/EDR ACL first */
    acl = bt_conn_create_br((const bt_addr_t*)addr, BT_BR_CONN_PARAM_DEFAULT);
    if (!acl) {
        BT_LOGE("%s create_br failed", __func__);
        pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
        pan_conn_free(conn);
        return BT_STATUS_FAIL;
    }
    g_pan.acl_conn = acl; /* unref'd in pan_br_connected/failure */

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pan_disconnect(bt_address_t* addr)
{
    pan_conn_t* conn;

    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }
    conn = pan_find_conn(addr);
    if (!conn || conn->state < PAN_CONN_L2CAP_PENDING) {
        return BT_STATUS_FAIL;
    }
    return bt_l2cap_chan_disconnect(&conn->chan) == 0 ?
        BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_pan_write(bt_address_t* addr, uint16_t protocol,
    uint8_t* dst_addr, uint8_t* src_addr,
    uint8_t* data, uint16_t length)
{
    pan_conn_t* conn;
    struct net_buf* buf;
    uint8_t* p;

    (void)dst_addr;
    (void)src_addr;

    if (!addr || !data || length == 0) {
        return BT_STATUS_PARM_INVALID;
    }
    conn = pan_find_conn(addr);
    if (!conn || conn->state != PAN_CONN_CONNECTED) {
        return BT_STATUS_NOT_READY;
    }
    if (length > PAN_TX_BUF_SIZE - 3) {
        BT_LOGW("%s frame too long %u", __func__, length);
        return BT_STATUS_NOMEM;
    }

    buf = net_buf_alloc(&pan_tx_pool, K_NO_WAIT);
    if (!buf) {
        BT_LOGE("%s tx pool exhausted", __func__);
        return BT_STATUS_NOMEM;
    }
    net_buf_reserve(buf, CONFIG_BT_L2CAP_TX_MTU);
    p = net_buf_tail(buf);
    p[0] = BNEP_FRAME_ETH;
    p[1] = (uint8_t)(protocol >> 8);
    p[2] = (uint8_t)(protocol & 0xff);
    net_buf_add(buf, 3);
    net_buf_add_mem(buf, data, length);

    int ret = bt_l2cap_chan_send(&conn->chan, buf);
    if (ret < 0) {
        BT_LOGE("%s send failed: %d", __func__, ret);
        net_buf_unref(buf);
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}
