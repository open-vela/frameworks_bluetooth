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
 *   - data path: whole Ethernet frames cross this boundary intact and
 *     bnep_codec.c does all framing (General Ethernet on TX, every frame
 *     type plus extension headers on RX)
 *   - events reported to the framework profile via pan_on_*_changed()
 */

#include <errno.h>
#include <stdbool.h>
#include <syslog.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "sal_interface.h"
#include "sal_pan_interface.h"
#include "service_loop.h"
#include "utils/log.h"

/* BNEP TX pool, sized for the BNEP standard MTU 1691 (Android NAP negotiates
 * 1691 in its L2CAP CONFIG_REQ).
 *
 * Any pool handed to net_buf_alloc() on this port must also be listed in
 * _net_buf_pool_list[] (external/zblue/zblue/port/sections/defines.c).
 * pool_id() resolves a pool pointer by scanning that list and returns 0 for an
 * unlisted pool, so its buffers would take discardable_pool's max_alloc_size
 * (258 -> 249 bytes of tailroom, hence the historical BNEP encode -ENOSPC) and
 * would write into discardable_pool's storage. */
#define PAN_TX_MTU 1691
#define PAN_TX_BUF_SIZE BT_L2CAP_BUF_SIZE(PAN_TX_MTU)
/* Two buffers: the TAP reader sends one frame at a time and the buffer is
 * released on ACL completion, so depth only buys pipelining. Each buffer is
 * ~1.7 KB of BSS and the heap has under 30 KB left after .bss, so a deeper
 * pool would come straight out of malloc(). */
#define PAN_TX_BUF_COUNT 2
NET_BUF_POOL_FIXED_DEFINE(pan_tx_pool, PAN_TX_BUF_COUNT,
    PAN_TX_BUF_SIZE, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
/* BNEP frame size limits.
 *
 * There is no compile-time TX limit: the BR/EDR L2CAP send path checks
 * buf->len against the *negotiated* br_chan->tx.mtu (zblue
 * classic/l2cap_br.c:1906) and never looks at CONFIG_BT_L2CAP_TX_MTU.
 * pan_tx_pool is already sized for PAN_TX_MTU (1691), so the only real
 * ceiling is what the peer agreed to in its CONFIG_REQ.
 *
 * PAN_ETH_FRAME_MAX is the largest Ethernet frame we ever handle; it must
 * match CONFIG_NET_ETH_PKTSIZE on the TAP side so neither direction can
 * be the short end. */
#define PAN_ETH_FRAME_MAX 1514
/* Worst-case BNEP header: General Ethernet = type(1)+dst(6)+src(6)+proto(2) */
#define PAN_BNEP_HDR_MAX  15

/* bt_l2cap_create_pdu_timeout is the exported zblue entry (l2cap.c,
 * unconditionally compiled); bt_l2cap_create_pdu is only a macro in the
 * internal l2cap_internal.h, so call the timeout variant directly. */
struct net_buf *bt_l2cap_create_pdu_timeout(struct net_buf_pool *pool,
                                            size_t reserve,
                                            k_timeout_t timeout);

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
    struct bt_l2cap_br_chan chan;  /* must be br_chan: BR_CHAN() walks past chan */
    uint64_t setup_sent_ms;        /* for the 5s handshake timeout */
    uint8_t rx_eth[PAN_ETH_FRAME_MAX];
} pan_conn_t;

/* Cooldown after a disconnect: the LCPU controller keeps stale state
 * right after an ACL teardown, and an immediate bt_conn_create_br()
 * makes HCI CREATE_CONN time out -> zblue asserts (observed 2026-08-15).
 */
#define PAN_RECONNECT_COOLDOWN_MS 5000

/* Synchronous HCI commands (bt_conn_create_br, bt_conn_set_security)
 * must never run on the bluetoothd service-loop thread: that thread
 * also polls /dev/ttyHCI0 and delivers the Command Status events the
 * command is waiting for - a self-deadlock that times out and asserts
 * after 10s (observed 2026-08-15). All connection work runs on a
 * dedicated worker thread instead.
 */
static pthread_t g_pan_worker;
static pthread_mutex_t g_pan_worker_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_pan_worker_cond = PTHREAD_COND_INITIALIZER;
static pan_conn_t* g_pan_worker_conn;
static bool g_pan_worker_run;
static bool g_pan_acl_ready; /* set by pan_br_connected (stack thread) */

static void* pan_worker_thread(void* arg);
static void pan_br_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err);
static void pan_l2cap_connect(struct bt_conn* conn);

static struct {
    uint8_t max_connections;
    uint8_t role;
    bool initialized;
    bt_list_t* conn_list;
    struct bt_conn* acl_conn;  /* BR/EDR ACL under setup (unref'd on up/fail) */
    struct bt_conn_cb conn_cb;
    uint64_t last_disconnect_ms;
    uint8_t local_mac[6];      /* BD_ADDR byte-reversed; filled lazily */
    bool local_mac_valid;
    uint32_t tx_oversize;      /* must stay 0 in normal operation */
    uint32_t tx_nobuf;         /* pan_tx_pool exhausted for >100ms */
} g_pan = {
    .max_connections = 1,
    .role = 0,
    .initialized = false,
    .conn_list = NULL,
    .acl_conn = NULL,
    .last_disconnect_ms = 0,
    .local_mac_valid = false,
    .tx_oversize = 0,
    .tx_nobuf = 0,
};

static uint64_t pan_now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000u);
}

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

/* R85: the board's BNEP channel (SCID) enters CONNECTED state before
 * the phone's BNEP channel (DCID) has completed its L2CAP config.
 * Sending BNEP data on the un-configured phone channel causes the
 * phone to reject it and disconnect. Defer BNEP setup until the
 * phone's channel is also CONNECTED. */
static void pan_try_send_setup(pan_conn_t* conn);

static void pan_chan_connected(struct bt_l2cap_chan* chan)
{
    pan_conn_t* conn = CONTAINER_OF(BT_L2CAP_BR_CHAN(chan), pan_conn_t, chan);

    if (!conn || conn->state != PAN_CONN_L2CAP_PENDING) {
        syslog(LOG_WARNING, "[pan] chan_connected: unexpected state=%d\n",
            conn ? (int)conn->state : -1);
        return;
    }

    pan_try_send_setup(conn);
}

static void pan_try_send_setup(pan_conn_t* conn)
{
    uint8_t req[16];
    struct net_buf* buf;
    int n, ret;

    if (!conn || conn->state != PAN_CONN_L2CAP_PENDING) {
        syslog(LOG_WARNING, "[pan] send_setup: unexpected state=%d\n",
            conn ? (int)conn->state : -1);
        return;
    }

    /* R85: both directions must have finished L2CAP config. Our outbound
     * channel reaches BT_L2CAP_CONNECTED when the phone's CONFIG_REQ
     * arrives; sending BNEP earlier makes the phone drop the link. */
    if (conn->chan.state != BT_L2CAP_CONNECTED) {
        syslog(LOG_INFO, "[pan] send_setup: deferred (L2CAP state=%u)\n",
            conn->chan.state);
        return;
    }

    /* 7 bytes: 01 01 02 11 16 11 15. The UUID Size field is ONE byte
     * (BT Core Vol 3 Part B 3.2.2.1). The old code wrote it as two bytes
     * and the phone answered 0x0003 = Invalid Service UUID Size, which
     * earlier rounds misread as "needs a 128-bit UUID". */
    n = bnep_encode_setup_req(req, sizeof(req),
        BNEP_UUID16_NAP, BNEP_UUID16_PANU);
    if (n <= 0) {
        syslog(LOG_ERR, "[pan] send_setup: encode failed %d\n", n);
        return;
    }

    buf = bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);
    if (!buf) {
        syslog(LOG_ERR, "[pan] send_setup: tx pool exhausted\n");
        return;
    }
    net_buf_add_mem(buf, req, (size_t)n);
    ret = bt_l2cap_chan_send(&conn->chan.chan, buf);
    if (ret < 0) {
        syslog(LOG_ERR, "[pan] send_setup: send failed %d\n", ret);
        net_buf_unref(buf);
        return;
    }
    conn->state = PAN_CONN_BNEP_PENDING;
    conn->setup_sent_ms = pan_now_ms();
    syslog(LOG_INFO, "[pan] setup req sent (%d bytes, dst=NAP src=PANU)\n", n);
}

static void pan_chan_disconnected(struct bt_l2cap_chan* chan)
{
    pan_conn_t* conn = CONTAINER_OF(BT_L2CAP_BR_CHAN(chan), pan_conn_t, chan);

    if (!conn) {
        return;
    }
    if (conn->state == PAN_CONN_CONNECTED) {
        pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
    }
    pan_conn_free(conn);
}

/* bt_sal_get_address asserts if the controller has not reported an
 * address yet, so resolve it lazily on the first frame instead of during
 * bt_sal_pan_init(). By the time any BNEP frame moves, the ACL is up and
 * the address is definitely valid. */
static const uint8_t* pan_local_mac(void)
{
    if (!g_pan.local_mac_valid) {
        bt_address_t local;
        if (bt_sal_get_address(PRIMARY_ADAPTER, &local) != BT_STATUS_SUCCESS) {
            syslog(LOG_ERR, "[pan] local address unavailable\n");
            return NULL;
        }
        bnep_mac_from_le48(g_pan.local_mac, local.addr);
        g_pan.local_mac_valid = true;
    }
    return g_pan.local_mac;
}

static void pan_send_ctrl(pan_conn_t* conn, const uint8_t* frame, size_t len)
{
    struct net_buf* buf = bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);

    if (!buf) {
        syslog(LOG_ERR, "[pan] ctrl tx pool exhausted\n");
        return;
    }
    net_buf_add_mem(buf, frame, len);
    if (bt_l2cap_chan_send(&conn->chan.chan, buf) < 0) {
        net_buf_unref(buf);
    }
}

static void pan_handle_control(pan_conn_t* conn, const uint8_t* ctrl,
    size_t ctrl_len)
{
    struct bnep_control info;
    uint8_t out[16];
    int n;

    if (bnep_parse_control(ctrl, ctrl_len, &info) != BNEP_OK) {
        syslog(LOG_WARNING, "[pan] malformed control frame (%u bytes)\n",
            (unsigned)ctrl_len);
        return;
    }

    switch (info.msg_type) {
    case BNEP_CTRL_SETUP_CONN_RSP:
        /* Strict: only 0x0000 is success. The old code treated every
         * non-zero status as success "anyway", which turned a rejected
         * handshake into a half-open link that silently ate all data. */
        if (!info.is_success) {
            syslog(LOG_ERR, "[pan] setup rejected, rsp=0x%04x\n",
                info.rsp_code);
            pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
            bt_l2cap_chan_disconnect(&conn->chan.chan);
            return;
        }
        if (conn->state != PAN_CONN_BNEP_PENDING) {
            syslog(LOG_WARNING, "[pan] setup rsp in state %d, ignored\n",
                (int)conn->state);
            return;
        }
        conn->state = PAN_CONN_CONNECTED;
        syslog(LOG_INFO, "[pan] BNEP setup OK, tx_mtu=%u\n",
            conn->chan.tx.mtu);
        pan_conn_report(conn, PROFILE_STATE_CONNECTED);
        return;

    case BNEP_CTRL_SETUP_CONN_REQ:
        /* Phone-initiated setup (it dialed our PSM). Answer with the
         * 4-byte standard response; anything else and Android tears the
         * channel down. */
        syslog(LOG_INFO, "[pan] setup req from peer dst=0x%04x src=0x%04x\n",
            info.dst_uuid16, info.src_uuid16);
        if (info.dst_uuid16 != BNEP_UUID16_PANU) {
            n = bnep_encode_setup_rsp(out, sizeof(out),
                BNEP_RSP_INVALID_DST_UUID);
            if (n > 0) { pan_send_ctrl(conn, out, (size_t)n); }
            return;
        }
        n = bnep_encode_setup_rsp(out, sizeof(out), BNEP_RSP_SUCCESS);
        if (n > 0) { pan_send_ctrl(conn, out, (size_t)n); }
        if (conn->state != PAN_CONN_CONNECTED) {
            conn->state = PAN_CONN_CONNECTED;
            syslog(LOG_INFO, "[pan] BNEP setup OK (peer initiated)\n");
            pan_conn_report(conn, PROFILE_STATE_CONNECTED);
        }
        return;

    case BNEP_CTRL_FILTER_NET_TYPE_SET:
        /* We support no filters (spec 10: YAGNI). Answering
         * "Unsupported" is the correct, spec-legal reply. */
        n = bnep_encode_filter_rsp(out, sizeof(out),
            BNEP_CTRL_FILTER_NET_TYPE_RSP, BNEP_FILTER_RSP_UNSUPPORTED);
        if (n > 0) { pan_send_ctrl(conn, out, (size_t)n); }
        return;

    case BNEP_CTRL_FILTER_MULTI_ADDR_SET:
        n = bnep_encode_filter_rsp(out, sizeof(out),
            BNEP_CTRL_FILTER_MULTI_ADDR_RSP, BNEP_FILTER_RSP_UNSUPPORTED);
        if (n > 0) { pan_send_ctrl(conn, out, (size_t)n); }
        return;

    case BNEP_CTRL_FILTER_NET_TYPE_RSP:
    case BNEP_CTRL_FILTER_MULTI_ADDR_RSP:
    case BNEP_CTRL_CMD_NOT_UNDERSTOOD:
        syslog(LOG_INFO, "[pan] ctrl 0x%02x rsp=0x%04x\n",
            info.msg_type, info.rsp_code);
        return;

    default:
        n = bnep_encode_cmd_not_understood(out, sizeof(out),
            info.unknown_type);
        if (n > 0) { pan_send_ctrl(conn, out, (size_t)n); }
        syslog(LOG_WARNING, "[pan] unknown ctrl 0x%02x, replied "
            "Command Not Understood\n", info.unknown_type);
        return;
    }
}

/* One reassembly buffer per connection, living in the pan_conn_t object
 * rather than on the stack: CONFIG_BT_RX_STACK_SIZE is 1200 bytes, so a
 * 1514-byte local array would overflow the zblue RX thread stack, and only
 * for large frames - every small-packet test would still pass. Only that
 * RX thread runs this callback, one channel PDU at a time. */
static int pan_chan_recv(struct bt_l2cap_chan* chan, struct net_buf* buf)
{
    pan_conn_t* conn = CONTAINER_OF(BT_L2CAP_BR_CHAN(chan), pan_conn_t, chan);
    const uint8_t* ctrl = NULL;
    size_t ctrl_len = 0;
    const uint8_t* local_mac;
    uint8_t peer_mac[6];
    int n;

    if (!conn || buf->len < 1) {
        return -EINVAL;
    }

    local_mac = pan_local_mac();
    if (!local_mac) {
        return 0;
    }
    bnep_mac_from_le48(peer_mac, conn->addr.addr);

    n = bnep_decode_eth(conn->rx_eth, sizeof(conn->rx_eth),
        buf->data, buf->len, local_mac, peer_mac, &ctrl, &ctrl_len);

    if (n == BNEP_DECODE_IS_CONTROL) {
        pan_handle_control(conn, ctrl, ctrl_len);
        return 0;
    }
    if (n < 0) {
        syslog(LOG_WARNING, "[pan] rx decode failed %d (type=0x%02x len=%u)\n",
            n, buf->data[0], buf->len);
        return 0;   /* a bad frame is not a channel error */
    }
    if (conn->state != PAN_CONN_CONNECTED) {
        syslog(LOG_WARNING, "[pan] rx data before setup done, dropped\n");
        return 0;
    }

    pan_on_eth_received(&conn->addr, conn->rx_eth, (uint16_t)n);
    return 0;
}


static const struct bt_l2cap_chan_ops g_pan_chan_ops = {
    .connected = pan_chan_connected,
    .disconnected = pan_chan_disconnected,
    .recv = pan_chan_recv,
};

/* -- Incoming BNEP (phone NAP dials us, xiaozhi-sf32 style) ----- */

static struct bt_l2cap_server g_pan_server;

/* R89: SDP PANU service record — minimal set to fit one SDP PDU.
 * The zblue SDP server's continuation state (03 f0) is malformed for
 * Android's SDP client, causing the phone to abandon the PANU query
 * without activating NAP/BNEP. Removing SupportedNetworkAccessTypeList
 * and SupportedFeatures shrinks the response below the continuation
 * threshold so Android gets the full record in a single response. */
#define BT_SDP_PROTO_BNEP 0x000f
#define PAN_SDP_VERSION 0x0100

static struct bt_sdp_attribute g_panu_sdp_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                BT_SDP_ARRAY_16(BT_SDP_PANU_SVCLASS) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_BNEP_PSM) }, ) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_BNEP) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(PAN_SDP_VERSION) }, ) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PANU_SVCLASS) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(PAN_SDP_VERSION) }, ) }, )),
    BT_SDP_SERVICE_NAME("PANU"),
};

static struct bt_sdp_record g_panu_sdp_record = {
    .attrs = g_panu_sdp_attrs,
    .attr_count = ARRAY_SIZE(g_panu_sdp_attrs),
};

static int pan_server_accept(struct bt_conn* acl, struct bt_l2cap_server* server,
    struct bt_l2cap_chan** chan)
{
    pan_conn_t* conn;
    bt_address_t addr;
    struct bt_conn_info info;

    if (bt_conn_get_info(acl, &info) != 0 || !info.br.dst) {
        return -ENOMEM;
    }
    bt_addr_set(&addr, info.br.dst->val);

    if (pan_find_conn(&addr)) {
        syslog(LOG_WARNING, "[pan] incoming conn for known addr, replacing\n");
        /* drop the stale one; the phone re-dials the PSM after ACL up */
        return -ENOMEM;
    }

    conn = calloc(1, sizeof(pan_conn_t));
    if (!conn) {
        return -ENOMEM;
    }
    conn->chan.chan.ops = &g_pan_chan_ops;
    /* Same MTU rules as the outbound path (Android NAP wants >=1691). */
    conn->chan.rx.mtu = 1691;
    conn->chan.required_sec_level = BT_SECURITY_L2;
    memcpy(&conn->addr, &addr, sizeof(bt_address_t));
    conn->dst_role = 1; /* NAP */
    conn->src_role = 2; /* PANU */
    conn->state = PAN_CONN_L2CAP_PENDING; /* setup handshake on recv */
    bt_list_add_tail(g_pan.conn_list, conn);
    pan_conn_report(conn, PROFILE_STATE_CONNECTING);

    *chan = &conn->chan.chan;
    syslog(LOG_INFO, "[pan] incoming BNEP channel accepted\n");
    return 0;
}

/* -- BR/EDR connection callbacks ------------------------------- */

static void pan_br_connected(struct bt_conn* conn, uint8_t err)
{
    pan_conn_t* pconn;
    bt_address_t addr;
    struct bt_conn_info info;

    if (err) {
        syslog(LOG_ERR, "[pan] ACL connect err %d\n", err);
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
        syslog(LOG_WARNING, "[pan] no pending conn for ACL\n");
        BT_LOGW("%s no pending pan conn", __func__);
        return;
    }
    syslog(LOG_INFO, "[pan] ACL up\n");
    /* Wake the worker: it waits for this before calling
     * set_security() (conn state must be CONNECTED first). */
    pthread_mutex_lock(&g_pan_worker_lock);
    g_pan_acl_ready = true;
    pthread_cond_signal(&g_pan_worker_cond);
    pthread_mutex_unlock(&g_pan_worker_lock);
    /* No synchronous HCI here: the worker thread drives
     * set_security() and L2CAP (see pan_worker_thread). */
}

static void pan_br_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err)
{
    bt_address_t addr;
    struct bt_conn_info info;
    pan_conn_t* pconn;

    if (err != BT_SECURITY_ERR_SUCCESS) {
        syslog(LOG_WARNING, "[pan] security err %d\n", err);
        BT_LOGW("%s security err %d", __func__, err);
        return;
    }
    syslog(LOG_INFO, "[pan] security level=%d\n", (int)level);
    if (bt_conn_get_info(conn, &info) != 0 || !info.br.dst) {
        return;
    }
    bt_addr_set(&addr, info.br.dst->val);

    pconn = pan_find_conn(&addr);
    if (!pconn || pconn->state != PAN_CONN_ACL_PENDING) {
        return;
    }

    /* R88: encryption is done — now send L2CAP CONN_REQ to the phone's
     * NAP service (PSM 0x000F).  The phone will not connect to our PANU
     * server on its own; the PANU is the initiator.  The R88 deadlock
     * scenario (outbound CONFIG_REQ blocking the signaling channel) only
     * happens when both sides dial simultaneously; here only the PANU
     * dials, so the signaling channel is free for the phone's CONFIG_REQ.
     * pan_l2cap_connect sets state = L2CAP_PENDING, so the worker's
     * wait loop (which watches for state != ACL_PENDING) exits cleanly. */
    syslog(LOG_INFO, "[pan] security ok, connecting L2CAP to phone NAP\n");
    pan_l2cap_connect(conn);
}

static void pan_l2cap_connect(struct bt_conn* conn)
{
    bt_address_t addr;
    struct bt_conn_info info;
    pan_conn_t* pconn;

    if (bt_conn_get_info(conn, &info) != 0 || !info.br.dst) {
        return;
    }
    bt_addr_set(&addr, info.br.dst->val);

    pconn = pan_find_conn(&addr);
    if (!pconn || pconn->state != PAN_CONN_ACL_PENDING) {
        return;
    }
    pconn->state = PAN_CONN_L2CAP_PENDING;
    syslog(LOG_INFO, "[pan] L2CAP connect (psm 0x%04x)\n", BT_BNEP_PSM);
    int ret = bt_l2cap_chan_connect(conn, &pconn->chan.chan, BT_BNEP_PSM);
    syslog(LOG_INFO, "[pan] L2CAP connect ret=%d\n", ret);
    if (ret < 0) {
        BT_LOGE("%s l2cap connect failed: %d", __func__, ret);
        pan_conn_report(pconn, PROFILE_STATE_DISCONNECTED);
        bt_l2cap_chan_disconnect(&pconn->chan.chan);
    }
}

/* Dedicated worker: runs the synchronous HCI sequence (create_br,
 * set_security) off the service-loop thread so Command Status events
 * keep being processed. */
static void* pan_worker_thread(void* arg)
{
    (void)arg;
    pan_conn_t* conn;
    struct bt_conn* acl;
    bt_address_t addr;

    for (;;) {
        pthread_mutex_lock(&g_pan_worker_lock);
        while (g_pan_worker_run && !g_pan_worker_conn) {
            pthread_cond_wait(&g_pan_worker_cond, &g_pan_worker_lock);
        }
        if (!g_pan_worker_run) {
            pthread_mutex_unlock(&g_pan_worker_lock);
            break;
        }
        conn = g_pan_worker_conn;
        g_pan_worker_conn = NULL;
        pthread_mutex_unlock(&g_pan_worker_lock);

        if (!conn) {
            continue;
        }

        /* Reuse the ACL if already up, else create one (synchronous:
         * blocks this worker thread only, service loop keeps polling
         * HCI events -> no deadlock). */
        memcpy(&addr, &conn->addr, sizeof(bt_address_t));
        acl = bt_conn_lookup_addr_br((const bt_addr_t*)&addr);
        if (!acl) {
            syslog(LOG_INFO, "[pan] worker: create_br...\n");
            acl = bt_conn_create_br((const bt_addr_t*)&addr,
                BT_BR_CONN_PARAM_DEFAULT);
        } else {
            syslog(LOG_INFO, "[pan] worker: reuse ACL\n");
        }
        if (!acl) {
            syslog(LOG_ERR, "[pan] worker: create_br FAILED\n");
            pthread_mutex_lock(&g_pan_worker_lock);
            if (conn->state == PAN_CONN_ACL_PENDING) {
                pthread_mutex_unlock(&g_pan_worker_lock);
                pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
                pan_conn_free(conn);
            } else {
                pthread_mutex_unlock(&g_pan_worker_lock);
            }
            continue;
        }

        /* Wait for the ACL-up callback (conn state CONNECTED) before
         * requesting encryption: set_security returns ENOTCONN if the
         * link is not yet up from zblue's point of view. A reused ACL
         * that is already CONNECTED skips the wait (its connected
         * callback fired before we reset the flag). */
        struct bt_conn_info cinfo;
        bool already_connected = (bt_conn_get_info(acl, &cinfo) == 0
            && cinfo.state == BT_CONN_STATE_CONNECTED);

        pthread_mutex_lock(&g_pan_worker_lock);
        g_pan_acl_ready = false;
        pthread_mutex_unlock(&g_pan_worker_lock);

        /* Poll both the callback flag and the conn state: the remote
         * can take >3s to complete page/connection setup. */
        bool acl_ok = already_connected;
        for (int i = 0; i < 100 && !acl_ok; i++) {
            pthread_mutex_lock(&g_pan_worker_lock);
            acl_ok = g_pan_acl_ready;
            pthread_mutex_unlock(&g_pan_worker_lock);
            if (!acl_ok) {
                already_connected = (bt_conn_get_info(acl, &cinfo) == 0
                    && cinfo.state == BT_CONN_STATE_CONNECTED);
                acl_ok = already_connected;
            }
            if (!acl_ok) {
                usleep(100000); /* 100ms, total up to 10s */
            }
        }

        if (!acl_ok) {
            syslog(LOG_WARNING, "[pan] worker: ACL up timeout\n");
            bt_conn_unref(acl);
            /* Release pending conn so retry is not rejected as already-exists */
            pthread_mutex_lock(&g_pan_worker_lock);
            bool still_pending = (conn->state == PAN_CONN_ACL_PENDING);
            pthread_mutex_unlock(&g_pan_worker_lock);
            if (still_pending) {
                pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
                pan_conn_free(conn);
            }
            continue;
        }

        /* R55: encryption via set_security is SKIPPED by default. The LCPU
         * controller has no working SSP path: after Auth_Requested it issues
         * a legacy Link_Key_Request, zblue neg-replies (no key), and the
         * controller fails authentication (0x05) and disconnects the ACL
         * before BNEP can start. BNEP itself does not require an encrypted
         * link, so connect L2CAP directly and let the phone decide.
         * Set PAN_SAL_SKIP_SECURITY=0 to restore the old behavior. */
#ifndef PAN_SAL_SKIP_SECURITY
/* R74 (2026-08-18): encryption now WORKS (Set_Event_Mask bit-7 fix, see
 * bth4 R70) - encrypt_change arrives, security level 2 observed both
 * directions. Android NAP requires an encrypted ACL before BNEP, so do
 * NOT skip set_security anymore. Set 1 only to debug the raw path. */
#define PAN_SAL_SKIP_SECURITY 0
#endif
        bool still_pending;
#if PAN_SAL_SKIP_SECURITY
        syslog(LOG_INFO, "[pan] worker: skip encryption (R55)\n");
        still_pending = true;
#else
        /* Request encryption (also synchronous - fine on this thread) */
        syslog(LOG_INFO, "[pan] worker: request encryption\n");
        int sret = bt_conn_set_security(acl, BT_SECURITY_L2);
        syslog(LOG_INFO, "[pan] worker: set_security ret=%d\n", sret);

        /* Wait for encryption to complete before connecting L2CAP.
         * BNEP requires an encrypted link (BT_SECURITY_L2). A full SSP
         * pairing + encrypt cycle can take 5-10 s on slower phones;
         * a reconnect with a stored link key is typically < 1 s.
         * 30 s covers all cases without blocking reconnects forever. */
        for (int i = 0; i < 300; i++) {
            pthread_mutex_lock(&g_pan_worker_lock);
            bool done = (conn->state != PAN_CONN_ACL_PENDING);
            pthread_mutex_unlock(&g_pan_worker_lock);
            if (done) {
                break;
            }
            usleep(100000); /* 100ms */
        }
        pthread_mutex_lock(&g_pan_worker_lock);
        still_pending = (conn->state == PAN_CONN_ACL_PENDING);
        pthread_mutex_unlock(&g_pan_worker_lock);
#endif
        if (still_pending) {
            syslog(LOG_INFO, "[pan] worker: timeout, L2CAP anyway\n");
            pan_l2cap_connect(acl);
        }
        bt_conn_unref(acl);
    }
    return NULL;
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
    syslog(LOG_WARNING, "[pan] ACL disconnected reason=%d\n", reason);
    BT_LOGW("%s reason %d", __func__, reason);
    if (pconn->state == PAN_CONN_CONNECTED) {
        pan_conn_report(pconn, PROFILE_STATE_DISCONNECTED);
    }
    /* Never free the conn here: the L2CAP chan is still owned by the
     * stack; freeing it early corrupts memory and hangs the system
     * (observed 2026-08-15 after phone dropped the ACL). Let the
     * chan disconnected callback free it. For a pending (not yet
     * mounted) chan, tear down through the stack as well. */
    if (pconn->state >= PAN_CONN_L2CAP_PENDING) {
        bt_l2cap_chan_disconnect(&pconn->chan.chan);
    } else {
        pan_conn_free(pconn);
    }
    g_pan.last_disconnect_ms = pan_now_ms();
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
    g_pan.conn_cb.security_changed = pan_br_security_changed;
    bt_conn_cb_register(&g_pan.conn_cb);

    /* R72: listen on PSM 0x000F so a phone NAP can dial US after bonding
     * (xiaozhi-sf32 flow: phone with "Bluetooth tethering" on connects
     * to the device; the device only accepts). */
    g_pan_server.psm = BT_BNEP_PSM;
    g_pan_server.sec_level = BT_SECURITY_L2;
    g_pan_server.accept = pan_server_accept;
    int sret = bt_l2cap_br_server_register(&g_pan_server);
    syslog(LOG_INFO, "[pan] BNEP server register (psm 0x%04x) ret=%d\n",
        BT_BNEP_PSM, sret);

    sret = bt_sdp_register_service(&g_panu_sdp_record);
    syslog(LOG_INFO, "[pan] PANU SDP record register ret=%d\n", sret);
    g_pan_worker_run = true;
    g_pan_worker_conn = NULL;
    pthread_attr_t wattr;
    pthread_attr_init(&wattr);
    pthread_attr_setstacksize(&wattr, 8192);
    if (pthread_create(&g_pan_worker, &wattr, pan_worker_thread, NULL) != 0) {
        syslog(LOG_ERR, "[pan] worker thread create failed\n");
    }
    pthread_attr_destroy(&wattr);

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
            bt_l2cap_chan_disconnect(&conn->chan.chan);
        }
        pan_conn_free(conn);
    }
    if (g_pan.acl_conn) {
        bt_conn_disconnect(g_pan.acl_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        bt_conn_unref(g_pan.acl_conn);
        g_pan.acl_conn = NULL;
    }
    pthread_mutex_lock(&g_pan_worker_lock);
    g_pan_worker_run = false;
    g_pan_worker_conn = NULL;
    pthread_cond_signal(&g_pan_worker_cond);
    pthread_mutex_unlock(&g_pan_worker_lock);
    pthread_join(g_pan_worker, NULL);
    bt_sdp_unregister_service(&g_panu_sdp_record);
    bt_conn_cb_unregister(&g_pan.conn_cb);
    bt_list_free(g_pan.conn_list);
    g_pan.conn_list = NULL;
    g_pan.initialized = false;
}

bt_status_t bt_sal_pan_connect(bt_address_t* addr, uint8_t dst_role,
    uint8_t src_role)
{
    pan_conn_t* conn;

    if (!addr || !g_pan.initialized) {
        syslog(LOG_ERR, "[pan] connect: bad param/not init\n");
        return BT_STATUS_PARM_INVALID;
    }

    if (pan_find_conn(addr)) {
        syslog(LOG_WARNING, "[pan] connect: already exists\n");
        BT_LOGW("%s already exists", __func__);
        return BT_STATUS_BUSY;
    }

    /* Reconnect cooldown: the LCPU controller keeps stale state after an
     * ACL teardown; reconnecting within the window makes CREATE_CONN time
     * out and zblue asserts. Reject early instead. */
    if (g_pan.last_disconnect_ms
        && pan_now_ms() - g_pan.last_disconnect_ms < PAN_RECONNECT_COOLDOWN_MS) {
        syslog(LOG_WARNING, "[pan] connect: cooldown active\n");
        BT_LOGW("%s in reconnect cooldown, retry later", __func__);
        return BT_STATUS_BUSY;
    }

    conn = calloc(1, sizeof(pan_conn_t));
    if (!conn) {
        return BT_STATUS_NOMEM;
    }
    conn->chan.chan.ops = &g_pan_chan_ops;
    /* br_chan fields are calloc'd zero: zblue sends CONFIG_REQ with
     * MTU=0 unless rx.mtu is set (l2cap_br_conf checks != 672), which
     * the phone rejects and disconnects. Android NAP additionally
     * rejects MTU<1691 (CONF_RSP UNACCEPTABLE_PARAMS + MTU=1691
     * observed on Redmi Note 12 Turbo); 1691 is the BNEP standard. */
    conn->chan.rx.mtu = 1691;
    conn->chan.required_sec_level = BT_SECURITY_L2;
    memcpy(&conn->addr, addr, sizeof(bt_address_t));
    conn->dst_role = dst_role;
    conn->src_role = src_role;
    conn->state = PAN_CONN_ACL_PENDING;
    bt_list_add_tail(g_pan.conn_list, conn);
    pan_conn_report(conn, PROFILE_STATE_CONNECTING);

    /* Hand the connection sequence to the worker thread: synchronous
     * HCI commands (create_br / set_security) must not run on the
     * service-loop thread (self-deadlock on HCI event polling). */
    pthread_mutex_lock(&g_pan_worker_lock);
    g_pan_worker_conn = conn;
    pthread_cond_signal(&g_pan_worker_cond);
    pthread_mutex_unlock(&g_pan_worker_lock);

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
    return bt_l2cap_chan_disconnect(&conn->chan.chan) == 0 ?
        BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

bt_status_t bt_sal_pan_write_eth(const bt_address_t* addr,
    const uint8_t* eth_frame, uint16_t eth_len)
{
    pan_conn_t* conn;
    struct net_buf* buf;
    const uint8_t* local_mac;
    uint8_t peer_mac[6];
    uint16_t limit;
    int n, ret;

    if (!addr || !eth_frame || eth_len < BNEP_ETH_HDR_LEN
        || eth_len > PAN_ETH_FRAME_MAX) {
        return BT_STATUS_PARM_INVALID;
    }
    conn = pan_find_conn(addr);
    if (!conn || conn->state != PAN_CONN_CONNECTED) {
        return BT_STATUS_NOT_READY;
    }
    local_mac = pan_local_mac();
    if (!local_mac) {
        return BT_STATUS_FAIL;
    }

    /* The only real ceiling is the negotiated L2CAP MTU. If we ever exceed
     * it the MTU derivation chain is broken (the TAP MTU should have been
     * clamped to tx.mtu - 14 at ifup, see Task 7), so count it loudly
     * instead of silently dropping - g_pan.tx_oversize must stay 0. */
    limit = conn->chan.tx.mtu;
    if ((uint32_t)eth_len + PAN_BNEP_HDR_MAX > (uint32_t)limit) {
        g_pan.tx_oversize++;
        syslog(LOG_ERR, "[pan] tx %u > tx_mtu %u (oversize=%lu)\n",
            eth_len, limit, (unsigned long)g_pan.tx_oversize);
        return BT_STATUS_NOMEM;
    }

    buf = bt_l2cap_create_pdu_timeout(&pan_tx_pool, 0, K_MSEC(100));
    if (!buf) {
        g_pan.tx_nobuf++;
        return BT_STATUS_NOMEM;
    }

    bnep_mac_from_le48(peer_mac, conn->addr.addr);
    /* compress=false in v1: always General Ethernet. Compression only saves
     * 12 bytes and every wrong-compression bug is a silent blackhole. */
    n = bnep_encode_eth(net_buf_tail(buf), net_buf_tailroom(buf),
        eth_frame, eth_len, local_mac, peer_mac, false);
    if (n <= 0) {
        syslog(LOG_ERR, "[pan] tx encode failed %d\n", n);
        net_buf_unref(buf);
        return BT_STATUS_FAIL;
    }
    net_buf_add(buf, (size_t)n);

    ret = bt_l2cap_chan_send(&conn->chan.chan, buf);
    if (ret < 0) {
        syslog(LOG_ERR, "[pan] tx send failed %d (len=%d tx_mtu=%u)\n",
            ret, n, conn->chan.tx.mtu);
        net_buf_unref(buf);
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

uint16_t bt_sal_pan_get_tx_mtu(const bt_address_t* addr)
{
    pan_conn_t* conn;

    if (!addr) {
        return 0;
    }
    conn = pan_find_conn(addr);
    if (!conn || conn->state != PAN_CONN_CONNECTED) {
        return 0;
    }
    return conn->chan.tx.mtu;
}
