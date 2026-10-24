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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/net/buf.h>
#include <zephyr/sys/byteorder.h>

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "sal_interface.h"
#include "sal_pan_interface.h"
#include "service_loop.h"
#include "utils/log.h"

/* BNEP TX uses zblue's ACL pool through bt_l2cap_create_pdu_timeout(NULL).
 *
 * Do not define a private net_buf pool here: any pool passed to
 * net_buf_alloc() must also be listed in zblue's _net_buf_pool_list[]
 * (external/zblue/zblue/port/sections/defines.c). pool_id() silently
 * resolves an unlisted pool to index 0, so the buffer gets discardable_pool's
 * max_alloc_size/storage (249 bytes tailroom -> BNEP ENOSPC, or memory
 * corruption). The ACL pool already honours CONFIG_BT_L2CAP_TX_MTU=1691 on
 * this board and keeps the PR self-contained.
 *
 * There is no compile-time TX ceiling: the BR/EDR L2CAP send path checks
 * buf->len against the negotiated br_chan->tx.mtu (zblue
 * classic/l2cap_br.c:1906), never CONFIG_BT_L2CAP_TX_MTU directly.
 *
 * PAN_ETH_FRAME_MAX is the largest Ethernet frame we ever handle; it must
 * match CONFIG_NET_ETH_PKTSIZE on the TAP side so neither direction can
 * be the short end. */
#define PAN_ETH_FRAME_MAX 1514
/* A whole Ethernet frame becomes BNEP General Ethernet: the 15-byte BNEP
 * header replaces the 14-byte Ethernet header, so the frame grows by 1. */
#define PAN_BNEP_OVERHEAD 1

/* bt_l2cap_create_pdu_timeout is the exported zblue entry (l2cap.c,
 * unconditionally compiled); bt_l2cap_create_pdu is only a macro in the
 * internal l2cap_internal.h, so call the timeout variant directly. */
struct net_buf* bt_l2cap_create_pdu_timeout(struct net_buf_pool* pool,
    size_t reserve,
    k_timeout_t timeout);

typedef enum {
    PAN_CONN_ACL_PENDING = 0, /* waiting BR/EDR ACL */
    PAN_CONN_L2CAP_PENDING, /* waiting L2CAP channel */
    PAN_CONN_BNEP_PENDING, /* waiting BNEP setup response */
    PAN_CONN_CONNECTED,
    PAN_CONN_FAILED, /* a failure has been reported */
    PAN_CONN_RELEASED, /* stack released the channel; free pending */
} pan_conn_state_t;

typedef struct {
    bt_address_t addr;
    pan_conn_state_t state;
    uint8_t dst_role;
    uint8_t src_role;
    struct bt_l2cap_br_chan chan; /* must be br_chan: BR_CHAN() walks past chan */
    uint64_t setup_sent_ms; /* for the 5s handshake timeout */
    service_timer_t* setup_timer; /* owns one ref while armed */
    uint8_t rx_eth[PAN_ETH_FRAME_MAX];
    int refs; /* list ref + transient users */
    bool linked; /* still on g_pan.conn_list */
    bool reported; /* DISCONNECTED already delivered */
    bool l2cap_started; /* connect/disconnect may own a zblue chan */
    bool released; /* zblue .released callback ran */
    bool in_worker; /* worker holds this pointer */
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
static volatile bool g_pan_worker_run;
static bool g_pan_worker_running; /* thread is alive */
static bool g_pan_acl_ready; /* set by pan_br_connected (stack thread) */

/* Protects g_pan.conn_list, conn_count and every pan_conn_t lifetime field. */
static pthread_mutex_t g_pan_conn_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_pan_conn_cond = PTHREAD_COND_INITIALIZER;

static void* pan_worker_thread(void* arg);
static void pan_br_security_changed(struct bt_conn* conn, bt_security_t level,
    enum bt_security_err err);
static void pan_l2cap_connect(struct bt_conn* conn);

static struct {
    uint8_t max_connections;
    uint8_t role;
    bool initialized;
    bool closing;
    bool server_registered;
    bool sdp_registered;
    bool conn_cb_registered;
    bt_list_t* conn_list;
    int conn_count;
    struct bt_conn* acl_conn; /* BR/EDR ACL under setup (unref'd on up/fail) */
    struct bt_conn_cb conn_cb;
    uint64_t last_disconnect_ms;
    uint8_t local_mac[6]; /* BD_ADDR byte-reversed; filled lazily */
    bool local_mac_valid;
    uint32_t tx_oversize; /* must stay 0 in normal operation */
    uint32_t tx_nobuf; /* ACL TX pool exhausted */
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

static void pan_conn_lock(void)
{
    pthread_mutex_lock(&g_pan_conn_lock);
}

static void pan_conn_unlock(void)
{
    pthread_mutex_unlock(&g_pan_conn_lock);
}

/* Caller must hold g_pan_conn_lock. */
static pan_conn_t* pan_find_conn_locked(const bt_address_t* addr)
{
    bt_list_node_t* node;

    if (!g_pan.conn_list) {
        return NULL;
    }
    for (node = bt_list_head(g_pan.conn_list); node;
         node = bt_list_next(g_pan.conn_list, node)) {
        pan_conn_t* conn = (pan_conn_t*)bt_list_node(node);
        if (conn && conn->linked
            && !memcmp(&conn->addr, addr, sizeof(bt_address_t))) {
            return conn;
        }
    }
    return NULL;
}

/* Returned pointer owns one reference; always pair with pan_conn_put(). */
static pan_conn_t* pan_find_conn(const bt_address_t* addr)
{
    pan_conn_t* conn;

    pan_conn_lock();
    conn = pan_find_conn_locked(addr);
    if (conn) {
        conn->refs++;
    }
    pan_conn_unlock();
    return conn;
}

static void pan_conn_report(pan_conn_t* conn, profile_connection_state_t state)
{
    pan_on_connection_state_changed(&conn->addr, conn->dst_role,
        conn->src_role, state);
}

/* Report a failed/aborted connection exactly once. The caller keeps the
 * reference it already owns; this function never drops the list ref. */
static void pan_conn_fail(pan_conn_t* conn, int reason)
{
    bool report = false;

    if (!conn) {
        return;
    }

    pan_conn_lock();
    if (!conn->reported) {
        conn->reported = true;
        conn->state = PAN_CONN_FAILED;
        report = true;
    }
    pan_conn_unlock();

    if (report) {
        syslog(LOG_WARNING, "[pan] conn failed reason=%d\n", reason);
        pan_conn_report(conn, PROFILE_STATE_DISCONNECTED);
    }
}

static void pan_conn_put(pan_conn_t* conn);

static void pan_conn_get_ptr(pan_conn_t* conn)
{
    if (!conn) {
        return;
    }
    pan_conn_lock();
    conn->refs++;
    pan_conn_unlock();
}

static void pan_conn_worker_done(pan_conn_t* conn)
{
    if (!conn) {
        return;
    }
    pan_conn_lock();
    conn->in_worker = false;
    pan_conn_unlock();
    pan_conn_put(conn);
}

/* Drop one reference; the last one frees the object. */
static void pan_conn_put(pan_conn_t* conn)
{
    if (!conn) {
        return;
    }

    pan_conn_lock();
    if (conn->refs <= 0) {
        pan_conn_unlock();
        return;
    }
    conn->refs--;
    if (conn->refs == 0) {
        if (conn->linked) {
            bt_list_remove(g_pan.conn_list, conn);
            conn->linked = false;
            if (g_pan.conn_count > 0) {
                g_pan.conn_count--;
            }
        }
        free(conn);
        pthread_cond_broadcast(&g_pan_conn_cond);
    }
    pan_conn_unlock();
}

/* Remove from the active list and drop the list's reference. */
static void pan_conn_drop_list_ref(pan_conn_t* conn)
{
    bool drop = false;

    if (!conn) {
        return;
    }
    pan_conn_lock();
    if (conn->linked) {
        bt_list_remove(g_pan.conn_list, conn);
        conn->linked = false;
        if (g_pan.conn_count > 0) {
            g_pan.conn_count--;
        }
        drop = true;
    }
    pan_conn_unlock();
    if (drop) {
        pan_conn_put(conn);
    }
}

/* Allocate but do NOT publish: callers must finish initialising all fields
 * before pan_conn_link(), so cleanup can never see a half-built conn. */
static pan_conn_t* pan_conn_alloc(const bt_address_t* addr)
{
    pan_conn_t* conn = calloc(1, sizeof(pan_conn_t));

    if (!conn) {
        return NULL;
    }
    memcpy(&conn->addr, addr, sizeof(bt_address_t));
    conn->refs = 1;
    return conn;
}

static bool pan_conn_link(pan_conn_t* conn)
{
    pan_conn_lock();
    if (!g_pan.conn_list) {
        pan_conn_unlock();
        return false;
    }
    bt_list_add_tail(g_pan.conn_list, conn);
    conn->linked = true;
    g_pan.conn_count++;
    pan_conn_unlock();
    return true;
}

static void pan_setup_timeout(service_timer_t* timer, void* data);

/* Timer APIs are only safe on the service-loop thread. Callbacks on the
 * zblue stack thread must marshal arm/cancel through do_in_service_loop(). */
static void pan_setup_timer_cancel_cb(void* data)
{
    pan_conn_t* conn = data;
    service_timer_t* timer;

    pan_conn_lock();
    timer = conn->setup_timer;
    conn->setup_timer = NULL;
    pan_conn_unlock();

    if (timer) {
        service_loop_cancel_timer(timer);
        pan_conn_put(conn); /* timer reference */
    }
    pan_conn_put(conn); /* message reference */
}

static void pan_setup_timer_cancel(pan_conn_t* conn)
{
    if (!conn) {
        return;
    }
    pan_conn_get_ptr(conn);
    do_in_service_loop(pan_setup_timer_cancel_cb, conn);
}

/* Cleanup runs on the service loop, so it can release a one-shot timer
 * synchronously. The async cancel path must not outlive adapter shutdown. */
static void pan_setup_timer_cancel_now(pan_conn_t* conn)
{
    service_timer_t* timer;

    if (!conn) {
        return;
    }
    pan_conn_lock();
    timer = conn->setup_timer;
    conn->setup_timer = NULL;
    pan_conn_unlock();
    if (timer) {
        service_loop_cancel_timer(timer);
        pan_conn_put(conn); /* timer reference */
    }
}

static void pan_setup_timeout(service_timer_t* timer, void* data)
{
    pan_conn_t* conn = data;
    bool pending;

    pan_conn_lock();
    if (conn->setup_timer != timer) {
        pan_conn_unlock();
        pan_conn_put(conn);
        return;
    }
    conn->setup_timer = NULL;
    pending = (conn->state == PAN_CONN_BNEP_PENDING);
    pan_conn_unlock();

    if (pending) {
        syslog(LOG_WARNING, "[pan] BNEP setup timeout (5s)\n");
        pan_conn_fail(conn, -ETIMEDOUT);
        bt_l2cap_chan_disconnect(&conn->chan.chan);
    }
    service_loop_cancel_timer(timer); /* one-shot handle must be closed */
    pan_conn_put(conn); /* timer reference */
}

static void pan_setup_timer_arm_cb(void* data)
{
    pan_conn_t* conn = data;
    service_timer_t* timer;
    service_timer_t* old_timer = NULL;
    bool pending;

    bool closing;

    pan_conn_lock();
    closing = (g_pan.closing || !g_pan.initialized);
    pending = !closing && (conn->state == PAN_CONN_BNEP_PENDING);
    if (pending) {
        old_timer = conn->setup_timer;
        conn->setup_timer = NULL;
    }
    pan_conn_unlock();
    if (closing) {
        pan_conn_put(conn); /* message reference; cleanup owns cancellation */
        return;
    }
    if (old_timer) {
        service_loop_cancel_timer(old_timer);
        pan_conn_put(conn); /* old timer reference */
    }
    if (!pending) {
        pan_conn_put(conn); /* message reference */
        return;
    }

    pan_conn_get_ptr(conn); /* timer reference */
    timer = service_loop_timer_no_repeating(5000, pan_setup_timeout, conn);
    if (!timer) {
        syslog(LOG_ERR, "[pan] setup timeout timer alloc failed\n");
        pan_conn_put(conn); /* timer reference */
        pan_conn_put(conn); /* message reference */
        return;
    }

    pan_conn_lock();
    conn->setup_timer = timer;
    pan_conn_unlock();
    pan_conn_put(conn); /* message reference */
}

static void pan_setup_timer_arm(pan_conn_t* conn)
{
    pan_setup_timer_cancel(conn);
    pan_conn_get_ptr(conn);
    do_in_service_loop(pan_setup_timer_arm_cb, conn);
}

/* Worker is about to drop its reference. If cleanup started while it was
 * still inside a synchronous HCI command, the worker (not cleanup) owns
 * the channel teardown; this avoids touching br_chan concurrently. */
static void pan_conn_worker_finish(pan_conn_t* conn)
{
    bool closing;

    pan_conn_lock();
    closing = g_pan.closing;
    pan_conn_unlock();

    if (closing) {
        bool started;
        bool released;

        pan_conn_fail(conn, 0);
        pan_setup_timer_cancel(conn);

        pan_conn_lock();
        started = conn->l2cap_started;
        released = conn->released;
        pan_conn_unlock();

        if (started && !released) {
            if (conn->chan.chan.conn) {
                bt_l2cap_chan_disconnect(&conn->chan.chan);
            }
        } else {
            pan_conn_drop_list_ref(conn);
        }
    }

    pan_conn_worker_done(conn);
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
    uint16_t dst_uuid = (conn->dst_role == 1) ? BNEP_UUID16_NAP : BNEP_UUID16_GN;
    uint16_t src_uuid = (conn->src_role == 2) ? BNEP_UUID16_PANU : BNEP_UUID16_GN;
    n = bnep_encode_setup_req(req, sizeof(req), dst_uuid, src_uuid);
    if (n <= 0) {
        syslog(LOG_ERR, "[pan] send_setup: encode failed %d\n", n);
        pan_conn_fail(conn, n);
        bt_l2cap_chan_disconnect(&conn->chan.chan);
        return;
    }

    buf = bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);
    if (!buf) {
        syslog(LOG_ERR, "[pan] send_setup: tx pool exhausted\n");
        pan_conn_fail(conn, -ENOBUFS);
        bt_l2cap_chan_disconnect(&conn->chan.chan);
        return;
    }
    net_buf_add_mem(buf, req, (size_t)n);
    ret = bt_l2cap_chan_send(&conn->chan.chan, buf);
    if (ret < 0) {
        syslog(LOG_ERR, "[pan] send_setup: send failed %d\n", ret);
        net_buf_unref(buf);
        pan_conn_fail(conn, ret);
        bt_l2cap_chan_disconnect(&conn->chan.chan);
        return;
    }
    pan_conn_lock();
    conn->state = PAN_CONN_BNEP_PENDING;
    conn->setup_sent_ms = pan_now_ms();
    pan_conn_unlock();
    pan_setup_timer_arm(conn);
    syslog(LOG_INFO, "[pan] setup req sent (%d bytes, dst=NAP src=PANU)\n", n);
}

static void pan_chan_disconnected(struct bt_l2cap_chan* chan)
{
    pan_conn_t* conn = CONTAINER_OF(BT_L2CAP_BR_CHAN(chan), pan_conn_t, chan);

    if (!conn) {
        return;
    }

    pan_conn_lock();
    conn->state = PAN_CONN_RELEASED;
    pan_conn_unlock();

    pan_setup_timer_cancel(conn);

    /* Report every state transition, not only CONNECTED: the profile's
     * auto-connect state machine has no other way to leave CONNECTING. */
    pan_conn_fail(conn, 0);

    /* Do NOT free here: zblue's bt_l2cap_br_chan_del() still writes
     * chan->conn/state/psm and calls chan->destroy after this callback.
     * The .released callback drops the list reference instead. */
}

static void pan_chan_released(struct bt_l2cap_chan* chan)
{
    pan_conn_t* conn = CONTAINER_OF(BT_L2CAP_BR_CHAN(chan), pan_conn_t, chan);

    if (!conn) {
        return;
    }

    pan_setup_timer_cancel(conn);

    pan_conn_lock();
    conn->released = true;
    conn->state = PAN_CONN_RELEASED;
    pan_conn_unlock();

    pan_conn_drop_list_ref(conn);
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

    int pret = bnep_parse_control(ctrl, ctrl_len, &info);
    if (pret != BNEP_OK) {
        if (pret == BNEP_ERR_BADTYPE && ctrl_len >= 1) {
            /* Unknown but well-formed control type: BNEP requires a
             * Command Not Understood response. info.unknown_type is zero
             * here because bnep_parse_control memset before returning, so
             * use the raw type byte. */
            n = bnep_encode_cmd_not_understood(out, sizeof(out), ctrl[0]);
            if (n > 0) {
                pan_send_ctrl(conn, out, (size_t)n);
            }
            syslog(LOG_WARNING, "[pan] unknown ctrl 0x%02x, replied "
                                "Command Not Understood\n",
                ctrl[0]);
        } else if (pret == BNEP_ERR_BADUUID && ctrl_len >= 1
            && ctrl[0] == BNEP_CTRL_SETUP_CONN_REQ) {
            n = bnep_encode_setup_rsp(out, sizeof(out),
                BNEP_RSP_INVALID_UUID_SIZE);
            if (n > 0) {
                pan_send_ctrl(conn, out, (size_t)n);
            }
            syslog(LOG_WARNING, "[pan] setup req invalid UUID size\n");
        } else {
            syslog(LOG_WARNING, "[pan] malformed control frame (%u bytes)\n",
                (unsigned)ctrl_len);
        }
        return;
    }

    switch (info.msg_type) {
    case BNEP_CTRL_SETUP_CONN_RSP:
        /* Strict: only 0x0000 is success. The old code treated every
         * non-zero status as success "anyway", which turned a rejected
         * handshake into a half-open link that silently ate all data. */
        pan_setup_timer_cancel(conn);
        if (!info.is_success) {
            syslog(LOG_ERR, "[pan] setup rejected, rsp=0x%04x\n",
                info.rsp_code);
            pan_conn_fail(conn, info.rsp_code);
            bt_l2cap_chan_disconnect(&conn->chan.chan);
            return;
        }
        if (conn->state != PAN_CONN_BNEP_PENDING) {
            syslog(LOG_WARNING, "[pan] setup rsp in state %d, ignored\n",
                (int)conn->state);
            return;
        }
        pan_conn_lock();
        conn->state = PAN_CONN_CONNECTED;
        pan_conn_unlock();
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
            if (n > 0) {
                pan_send_ctrl(conn, out, (size_t)n);
            }
            return;
        }
        n = bnep_encode_setup_rsp(out, sizeof(out), BNEP_RSP_SUCCESS);
        if (n > 0) {
            pan_send_ctrl(conn, out, (size_t)n);
        }
        pan_setup_timer_cancel(conn);
        pan_conn_lock();
        bool was_connected = (conn->state == PAN_CONN_CONNECTED);
        if (!was_connected) {
            conn->state = PAN_CONN_CONNECTED;
        }
        pan_conn_unlock();
        if (!was_connected) {
            syslog(LOG_INFO, "[pan] BNEP setup OK (peer initiated)\n");
            pan_conn_report(conn, PROFILE_STATE_CONNECTED);
        }
        return;

    case BNEP_CTRL_FILTER_NET_TYPE_SET:
        /* We support no filters (spec 10: YAGNI). Answering
         * "Unsupported" is the correct, spec-legal reply. */
        n = bnep_encode_filter_rsp(out, sizeof(out),
            BNEP_CTRL_FILTER_NET_TYPE_RSP, BNEP_FILTER_RSP_UNSUPPORTED);
        if (n > 0) {
            pan_send_ctrl(conn, out, (size_t)n);
        }
        return;

    case BNEP_CTRL_FILTER_MULTI_ADDR_SET:
        n = bnep_encode_filter_rsp(out, sizeof(out),
            BNEP_CTRL_FILTER_MULTI_ADDR_RSP, BNEP_FILTER_RSP_UNSUPPORTED);
        if (n > 0) {
            pan_send_ctrl(conn, out, (size_t)n);
        }
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
        if (n > 0) {
            pan_send_ctrl(conn, out, (size_t)n);
        }
        syslog(LOG_WARNING, "[pan] unknown ctrl 0x%02x, replied "
                            "Command Not Understood\n",
            info.unknown_type);
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
        return 0; /* a bad frame is not a channel error */
    }
    if (conn->state != PAN_CONN_CONNECTED) {
        syslog(LOG_WARNING, "[pan] rx data before setup done, dropped\n");
        return 0;
    }

    /* Compatibility with the existing service profile: deliver the
     * Ethernet payload plus L2 header fields instead of the whole frame.
     * The service layer owns its own ETH header reconstruction. */
    if (n < 14) {
        syslog(LOG_WARNING, "[pan] rx eth too short %d\n", n);
        return 0;
    }
    uint16_t proto = (uint16_t)((conn->rx_eth[12] << 8) | conn->rx_eth[13]);
    pan_on_data_received(&conn->addr, proto, conn->rx_eth, conn->rx_eth + 6,
        conn->rx_eth + 14, (uint16_t)(n - 14));
    return 0;
}

static const struct bt_l2cap_chan_ops g_pan_chan_ops = {
    .connected = pan_chan_connected,
    .disconnected = pan_chan_disconnected,
    .released = pan_chan_released,
    .recv = pan_chan_recv,
};

/* -- Incoming BNEP (phone NAP dials us, xiaozhi-sf32 style) ----- */

static struct bt_l2cap_server g_pan_server;

/* R89: SDP PANU service record - minimal set to fit one SDP PDU.
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
    pan_conn_t* old;
    bt_address_t addr;
    struct bt_conn_info info;

    (void)server;

    if (!g_pan.initialized || g_pan.closing) {
        return -ENOMEM;
    }
    if (bt_conn_get_info(acl, &info) != 0 || !info.br.dst) {
        return -EINVAL;
    }
    bt_addr_set(&addr, info.br.dst->val);

    old = pan_find_conn(&addr);
    if (old) {
        bool stale;

        pan_conn_lock();
        stale = old->released || old->state == PAN_CONN_RELEASED
            || old->state == PAN_CONN_FAILED
            || (!old->l2cap_started && !old->chan.chan.conn);
        pan_conn_unlock();
        if (stale) {
            syslog(LOG_WARNING, "[pan] incoming conn: dropping stale entry\n");
            pan_conn_drop_list_ref(old);
            pan_conn_put(old);
        } else {
            pan_conn_put(old);
            /* The external/zblue companion patch rejects only this L2CAP
             * attempt on accept<0 (NO_RESOURCES) and no longer tears down
             * the shared ACL. Keep -EALREADY for an active duplicate. */
            syslog(LOG_WARNING, "[pan] incoming conn for known addr, busy\n");
            return -EALREADY;
        }
    }

    pan_conn_lock();
    bool full = (g_pan.conn_count >= (int)g_pan.max_connections);
    pan_conn_unlock();
    if (full) {
        syslog(LOG_WARNING, "[pan] incoming conn rejected: max connections\n");
        return -ENOMEM;
    }

    conn = pan_conn_alloc(&addr);
    if (!conn) {
        return -ENOMEM;
    }
    conn->chan.chan.ops = &g_pan_chan_ops;
    /* Same MTU rules as the outbound path (Android NAP wants >=1691). */
    conn->chan.rx.mtu = 1691;
    conn->chan.required_sec_level = BT_SECURITY_L2;
    conn->dst_role = 1; /* NAP */
    conn->src_role = 2; /* PANU */
    conn->state = PAN_CONN_L2CAP_PENDING; /* setup handshake on recv */
    conn->l2cap_started = true; /* zblue will add this channel */
    if (!pan_conn_link(conn)) {
        pan_conn_put(conn);
        return -ENOMEM;
    }
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
        if (bt_conn_get_info(conn, &info) == 0 && info.br.dst) {
            bt_addr_set(&addr, info.br.dst->val);
            pconn = pan_find_conn(&addr);
            if (pconn) {
                pan_conn_fail(pconn, err);
                pan_conn_drop_list_ref(pconn);
                pan_conn_put(pconn);
            }
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
        pan_conn_put(pconn);
        return;
    }
    pan_conn_put(pconn);
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
        if (bt_conn_get_info(conn, &info) == 0 && info.br.dst) {
            bt_addr_set(&addr, info.br.dst->val);
            pconn = pan_find_conn(&addr);
            if (pconn) {
                pan_conn_fail(pconn, err);
                if (!pconn->chan.chan.conn) {
                    pan_conn_drop_list_ref(pconn);
                }
                pan_conn_put(pconn);
            }
        }
        return;
    }
    syslog(LOG_INFO, "[pan] security level=%d\n", (int)level);
    if (bt_conn_get_info(conn, &info) != 0 || !info.br.dst) {
        return;
    }
    bt_addr_set(&addr, info.br.dst->val);

    if (level < BT_SECURITY_L2) {
        syslog(LOG_WARNING, "[pan] security level %d below L2\n", (int)level);
        pconn = pan_find_conn(&addr);
        if (pconn) {
            pan_conn_fail(pconn, -EACCES);
            if (!pconn->chan.chan.conn) {
                pan_conn_drop_list_ref(pconn);
            }
            pan_conn_put(pconn);
        }
        return;
    }

    pconn = pan_find_conn(&addr);
    if (!pconn || pconn->state != PAN_CONN_ACL_PENDING) {
        pan_conn_put(pconn);
        return;
    }

    /* R88: encryption is done - now send L2CAP CONN_REQ to the phone's
     * NAP service (PSM 0x000F).  The phone will not connect to our PANU
     * server on its own; the PANU is the initiator.  The R88 deadlock
     * scenario (outbound CONFIG_REQ blocking the signaling channel) only
     * happens when both sides dial simultaneously; here only the PANU
     * dials, so the signaling channel is free for the phone's CONFIG_REQ.
     * pan_l2cap_connect sets state = L2CAP_PENDING, so the worker's
     * wait loop (which watches for state != ACL_PENDING) exits cleanly. */
    syslog(LOG_INFO, "[pan] security ok, connecting L2CAP to phone NAP\n");
    pan_conn_put(pconn);
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
    if (!pconn) {
        return;
    }

    /* CAS: the security callback and the worker can both reach here. Only
     * one of them may call bt_l2cap_chan_connect(); the loser must not
     * report a failure for a channel the winner is mounting. */
    pan_conn_lock();
    if (pconn->state != PAN_CONN_ACL_PENDING) {
        pan_conn_unlock();
        pan_conn_put(pconn);
        return;
    }
    pconn->state = PAN_CONN_L2CAP_PENDING;
    pconn->l2cap_started = true;
    pan_conn_unlock();

    syslog(LOG_INFO, "[pan] L2CAP connect (psm 0x%04x)\n", BT_BNEP_PSM);
    int ret = bt_l2cap_chan_connect(conn, &pconn->chan.chan, BT_BNEP_PSM);
    syslog(LOG_INFO, "[pan] L2CAP connect ret=%d\n", ret);
    if (ret < 0 && ret != -EEXIST && ret != -EISCONN) {
        bool released;

        BT_LOGE("%s l2cap connect failed: %d", __func__, ret);
        pan_conn_fail(pconn, ret);

        pan_conn_lock();
        released = pconn->released;
        if (!released && ret != -EIO) {
            /* Pre-add failure: no channel can appear later. -EIO is
             * excluded because zblue's synchronous cleanup may still be
             * delivering .released. */
            pconn->l2cap_started = false;
        }
        pan_conn_unlock();
        if (!released && ret != -EIO) {
            pan_conn_drop_list_ref(pconn);
        }
    } else if (ret < 0) {
        syslog(LOG_WARNING, "[pan] L2CAP connect benign error %d\n", ret);
    }
    pan_conn_put(pconn);
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

        pan_conn_lock();
        conn->in_worker = true;
        pan_conn_unlock();

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
            pan_conn_fail(conn, -ENOLINK);
            pan_conn_drop_list_ref(conn);
            pan_conn_worker_finish(conn);
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
            if (!g_pan_worker_run) {
                break;
            }
            pan_conn_lock();
            bool aborted = (conn->state == PAN_CONN_FAILED
                || conn->state == PAN_CONN_RELEASED);
            pan_conn_unlock();
            if (aborted) {
                break;
            }
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
            pan_conn_fail(conn, -ETIMEDOUT);
            pan_conn_drop_list_ref(conn);
            pan_conn_worker_finish(conn);
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

        if (sret < 0) {
            pan_conn_fail(conn, sret);
            pan_conn_drop_list_ref(conn);
            bt_conn_unref(acl);
            pan_conn_worker_finish(conn);
            continue;
        }

        if (!g_pan_worker_run) {
            pan_conn_fail(conn, -ECANCELED);
            pan_conn_drop_list_ref(conn);
            bt_conn_unref(acl);
            pan_conn_worker_finish(conn);
            continue;
        }

        /* Already-encrypted ACL: set_security returns 0 without a
         * security_changed callback; do not sit in the 30s loop. */
        if (bt_conn_get_security(acl) >= BT_SECURITY_L2) {
            pan_l2cap_connect(acl);
            bt_conn_unref(acl);
            pan_conn_worker_finish(conn);
            continue;
        }

        /* Wait for encryption to complete before connecting L2CAP.
         * BNEP requires an encrypted link (BT_SECURITY_L2). A full SSP
         * pairing + encrypt cycle can take 5-10 s on slower phones;
         * a reconnect with a stored link key is typically < 1 s.
         * 30 s covers all cases without blocking reconnects forever. */
        for (int i = 0; i < 300; i++) {
            if (!g_pan_worker_run) {
                break;
            }
            pan_conn_lock();
            bool done = (conn->state != PAN_CONN_ACL_PENDING);
            bool aborted = (conn->state == PAN_CONN_FAILED
                || conn->state == PAN_CONN_RELEASED);
            pan_conn_unlock();
            if (done || aborted) {
                break;
            }
            usleep(100000); /* 100ms */
        }
        pan_conn_lock();
        still_pending = (conn->state == PAN_CONN_ACL_PENDING);
        pan_conn_unlock();
#endif
        if (still_pending && g_pan_worker_run) {
            syslog(LOG_INFO, "[pan] worker: timeout, L2CAP anyway\n");
            pan_l2cap_connect(acl);
        }
        bt_conn_unref(acl);
        pan_conn_worker_finish(conn);
    }

    pthread_mutex_lock(&g_pan_worker_lock);
    g_pan_worker_running = false;
    pthread_cond_broadcast(&g_pan_worker_cond);
    pthread_mutex_unlock(&g_pan_worker_lock);
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

    pan_conn_lock();
    g_pan.last_disconnect_ms = pan_now_ms();
    pan_conn_unlock();

    pconn = pan_find_conn(&addr);
    if (!pconn) {
        /* zblue calls bt_l2cap_disconnected before notify_disconnected;
         * the chan callback may already have dropped the active conn.
         * Always record the cooldown above, even in that case. */
        return;
    }
    syslog(LOG_WARNING, "[pan] ACL disconnected reason=%d\n", reason);
    BT_LOGW("%s reason %d", __func__, reason);

    pan_conn_fail(pconn, reason);
    pan_setup_timer_cancel(pconn);

    /* Never free the conn here: the L2CAP chan may still be owned by the
     * stack. If connect() started, only .released may drop the list ref. */
    bool started;
    bool released;

    pan_conn_lock();
    started = pconn->l2cap_started;
    released = pconn->released;
    pan_conn_unlock();

    if (started && !released) {
        if (pconn->chan.chan.conn) {
            bt_l2cap_chan_disconnect(&pconn->chan.chan);
        }
    } else {
        pan_conn_drop_list_ref(pconn);
    }
    pan_conn_put(pconn);
}

/* -- Public API ------------------------------------------------ */

bt_status_t bt_sal_pan_init(uint8_t max_connections, uint8_t role)
{
    if (g_pan.initialized) {
        return BT_STATUS_SUCCESS;
    }

    pan_conn_lock();
    bool conns_left = (g_pan.conn_count > 0);
    pan_conn_unlock();
    pthread_mutex_lock(&g_pan_worker_lock);
    bool worker_alive = g_pan_worker_running;
    pthread_mutex_unlock(&g_pan_worker_lock);
    if (conns_left || worker_alive) {
        syslog(LOG_WARNING, "[pan] init: previous cleanup still running\n");
        return BT_STATUS_BUSY;
    }
    pan_conn_lock();
    g_pan.closing = false;
    pan_conn_unlock();

    BT_LOGI("%s max=%u role=%u", __func__, max_connections, role);

    /* The caller-side contract for bt_list_*() is a valid list (they assert
     * on NULL).  A soft reset keeps RAM, so g_pan can survive from the
     * previous incarnation while the list it pointed at no longer exists;
     * make sure init leaves a usable list behind instead of relying on the
     * library to tolerate NULL. */
    if (g_pan.conn_list == NULL) {
        g_pan.conn_list = bt_list_new(NULL);
        if (g_pan.conn_list == NULL) {
            return BT_STATUS_NOMEM;
        }
    }

    g_pan.max_connections = max_connections;
    g_pan.role = role;
    g_pan.last_disconnect_ms = 0;
    g_pan.tx_oversize = 0;
    g_pan.tx_nobuf = 0;

    /* R72: listen on PSM 0x000F so a phone NAP can dial US after bonding
     * (xiaozhi-sf32 flow: phone with "Bluetooth tethering" on connects
     * to the device; the device only accepts). */
    if (!g_pan.server_registered) {
        g_pan_server.psm = BT_BNEP_PSM;
        g_pan_server.sec_level = BT_SECURITY_L2;
        g_pan_server.accept = pan_server_accept;
        int sret = bt_l2cap_br_server_register(&g_pan_server);
        syslog(LOG_INFO, "[pan] BNEP server register (psm 0x%04x) ret=%d\n",
            BT_BNEP_PSM, sret);
        if (sret == 0 || sret == -EADDRINUSE) {
            g_pan.server_registered = true;
        } else {
            return BT_STATUS_FAIL;
        }
    }

    if (!g_pan.sdp_registered) {
        int sret = bt_sdp_register_service(&g_panu_sdp_record);
        syslog(LOG_INFO, "[pan] PANU SDP record register ret=%d\n", sret);
        if (sret == 0) {
            g_pan.sdp_registered = true;
        } else {
            return BT_STATUS_FAIL;
        }
    }

    if (!g_pan.conn_cb_registered) {
        g_pan.conn_cb.connected = pan_br_connected;
        g_pan.conn_cb.disconnected = pan_br_disconnected;
        g_pan.conn_cb.security_changed = pan_br_security_changed;
        bt_conn_cb_register(&g_pan.conn_cb);
        g_pan.conn_cb_registered = true;
    }

    pthread_attr_t wattr;
    pthread_attr_init(&wattr);
    pthread_attr_setstacksize(&wattr, 8192);
    pthread_attr_setdetachstate(&wattr, PTHREAD_CREATE_DETACHED);

    pthread_mutex_lock(&g_pan_worker_lock);
    g_pan_worker_run = true;
    g_pan_worker_conn = NULL;
    g_pan_worker_running = true;
    pthread_mutex_unlock(&g_pan_worker_lock);

    if (pthread_create(&g_pan_worker, &wattr, pan_worker_thread, NULL) != 0) {
        syslog(LOG_ERR, "[pan] worker thread create failed\n");
        pthread_mutex_lock(&g_pan_worker_lock);
        g_pan_worker_run = false;
        g_pan_worker_running = false;
        pthread_mutex_unlock(&g_pan_worker_lock);
        pthread_attr_destroy(&wattr);
        if (g_pan.conn_cb_registered) {
            bt_conn_cb_unregister(&g_pan.conn_cb);
            g_pan.conn_cb_registered = false;
        }
        return BT_STATUS_NOMEM;
    }
    pthread_attr_destroy(&wattr);

    g_pan.initialized = true;
    return BT_STATUS_SUCCESS;
}

void bt_sal_pan_cleanup(void)
{
    pan_conn_t* snapshot[8];
    int count = 0;

    if (!g_pan.initialized) {
        return;
    }

    pan_conn_lock();
    g_pan.closing = true;
    g_pan.initialized = false;
    pan_conn_unlock();

    /* Stop the worker but do NOT join it here: this runs on the service
     * loop, which also polls /dev/ttyHCI0. The worker may be inside a
     * synchronous HCI command and needs that poll thread to finish. */
    pthread_mutex_lock(&g_pan_worker_lock);
    g_pan_worker_run = false;
    pan_conn_t* queued = g_pan_worker_conn;
    g_pan_worker_conn = NULL;
    pthread_cond_broadcast(&g_pan_worker_cond);
    pthread_mutex_unlock(&g_pan_worker_lock);
    pan_conn_put(queued); /* slot reference, if any */

    /* Snapshot up to 8 conns (PAN_MAX_CONNECTIONS is 1) and take a ref
     * for each, so callbacks may remove/free them while we work. */
    pan_conn_lock();
    bt_list_node_t* node = bt_list_head(g_pan.conn_list);
    while (node && count < (int)(sizeof(snapshot) / sizeof(snapshot[0]))) {
        pan_conn_t* conn = (pan_conn_t*)bt_list_node(node);
        if (conn) {
            conn->refs++;
            snapshot[count++] = conn;
        }
        node = bt_list_next(g_pan.conn_list, node);
    }
    pan_conn_unlock();

    for (int i = 0; i < count; i++) {
        pan_conn_t* conn = snapshot[i];
        bool in_worker;
        bool started;
        bool released;

        pan_conn_lock();
        in_worker = conn->in_worker;
        started = conn->l2cap_started;
        released = conn->released;
        pan_conn_unlock();

        pan_setup_timer_cancel_now(conn);

        if (in_worker) {
            /* The worker is inside a synchronous HCI/L2CAP call. Do not
             * touch br_chan concurrently; it will run worker_finish()
             * after seeing g_pan.closing. */
            pan_conn_put(conn);
            continue;
        }

        pan_conn_fail(conn, 0);
        if (started && !released) {
            if (conn->chan.chan.conn) {
                bt_l2cap_chan_disconnect(&conn->chan.chan);
            }
        } else {
            pan_conn_drop_list_ref(conn);
        }
        pan_conn_put(conn);
    }

    if (g_pan.acl_conn) {
        bt_conn_disconnect(g_pan.acl_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        bt_conn_unref(g_pan.acl_conn);
        g_pan.acl_conn = NULL;
    }
    if (g_pan.sdp_registered) {
        bt_sdp_unregister_service(&g_panu_sdp_record);
        g_pan.sdp_registered = false;
    }
    if (g_pan.conn_cb_registered) {
        bt_conn_cb_unregister(&g_pan.conn_cb);
        g_pan.conn_cb_registered = false;
    }

    /* Keep g_pan.conn_list allocated: disconnected/released callbacks may
     * still arrive. Init reuses it once conn_count reaches zero. */
}

bt_status_t bt_sal_pan_connect(bt_address_t* addr, uint8_t dst_role,
    uint8_t src_role)
{
    pan_conn_t* conn;

    if (!addr || !g_pan.initialized || g_pan.closing) {
        syslog(LOG_ERR, "[pan] connect: bad param/not init\n");
        return BT_STATUS_PARM_INVALID;
    }

    conn = pan_find_conn(addr);
    if (conn) {
        pan_conn_put(conn);
        syslog(LOG_WARNING, "[pan] connect: already exists\n");
        BT_LOGW("%s already exists", __func__);
        return BT_STATUS_BUSY;
    }

    pan_conn_lock();
    bool full = (g_pan.conn_count >= (int)g_pan.max_connections);
    pan_conn_unlock();
    if (full) {
        syslog(LOG_WARNING, "[pan] connect: max connections reached\n");
        return BT_STATUS_NO_RESOURCES;
    }

    /* Reconnect cooldown: the LCPU controller keeps stale state after an
     * ACL teardown; reconnecting within the window makes CREATE_CONN time
     * out and zblue asserts. Reject early instead. */
    pan_conn_lock();
    uint64_t last_disconnect_ms = g_pan.last_disconnect_ms;
    pan_conn_unlock();
    if (last_disconnect_ms
        && pan_now_ms() - last_disconnect_ms < PAN_RECONNECT_COOLDOWN_MS) {
        syslog(LOG_WARNING, "[pan] connect: cooldown active\n");
        BT_LOGW("%s in reconnect cooldown, retry later", __func__);
        return BT_STATUS_BUSY;
    }

    conn = pan_conn_alloc(addr);
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
    conn->dst_role = dst_role;
    conn->src_role = src_role;
    conn->state = PAN_CONN_ACL_PENDING;
    pan_conn_get_ptr(conn); /* candidate worker-slot reference */
    if (!pan_conn_link(conn)) {
        pan_conn_put(conn); /* candidate */
        pan_conn_put(conn); /* allocation */
        return BT_STATUS_NOMEM;
    }
    pan_conn_report(conn, PROFILE_STATE_CONNECTING);

    /* Hand the connection sequence to the worker thread: synchronous
     * HCI commands (create_br / set_security) must not run on the
     * service-loop thread (self-deadlock on HCI event polling). */
    pthread_mutex_lock(&g_pan_worker_lock);
    if (!g_pan_worker_running || g_pan_worker_conn) {
        pthread_mutex_unlock(&g_pan_worker_lock);
        pan_conn_fail(conn, -ENOSYS);
        pan_conn_drop_list_ref(conn);
        pan_conn_put(conn); /* candidate slot reference */
        return BT_STATUS_NO_RESOURCES;
    }
    g_pan_worker_conn = conn; /* candidate slot ref is transferred */
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
    if (!conn) {
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    bool started;
    bool released;

    pan_conn_lock();
    started = conn->l2cap_started;
    released = conn->released;
    pan_conn_unlock();

    if (!started || released) {
        /* No channel was ever handed to zblue: cancel locally. */
        pan_conn_fail(conn, -ECANCELED);
        pan_conn_drop_list_ref(conn);
        pan_conn_put(conn);
        return BT_STATUS_SUCCESS;
    }

    if (!conn->chan.chan.conn) {
        /* connect() has started but zblue has not mounted the channel yet.
         * Dropping the list ref here would free memory that the stack is
         * about to own; let the worker/released path finish instead. */
        pan_conn_put(conn);
        return BT_STATUS_BUSY;
    }

    int ret = bt_l2cap_chan_disconnect(&conn->chan.chan);
    pan_conn_put(conn);
    return ret == 0 ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

/* Compatibility with the existing service profile, which still hands the
 * stack L2 fields plus payload. The stack owns the Ethernet header. */
bt_status_t bt_sal_pan_write(const bt_address_t* addr, uint16_t protocol,
    const uint8_t* dst_addr, const uint8_t* src_addr,
    const uint8_t* data, uint16_t length)
{
    if (!addr || !dst_addr || !src_addr || (length && !data)
        || length > PAN_ETH_FRAME_MAX - 14) {
        return BT_STATUS_PARM_INVALID;
    }

    uint16_t total = (uint16_t)(14 + length);
    uint8_t* frame = malloc(total);
    if (!frame) {
        return BT_STATUS_NOMEM;
    }
    memcpy(frame, dst_addr, 6);
    memcpy(frame + 6, src_addr, 6);
    frame[12] = (uint8_t)(protocol >> 8);
    frame[13] = (uint8_t)protocol;
    if (length) {
        memcpy(frame + 14, data, length);
    }

    bt_status_t ret = bt_sal_pan_write_eth(addr, frame, total);
    free(frame);
    return ret;
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
    if (!conn) {
        return BT_STATUS_NOT_READY;
    }
    if (conn->state != PAN_CONN_CONNECTED) {
        pan_conn_put(conn);
        return BT_STATUS_NOT_READY;
    }
    local_mac = pan_local_mac();
    if (!local_mac) {
        pan_conn_put(conn);
        return BT_STATUS_FAIL;
    }

    /* The only real ceiling is the negotiated L2CAP MTU. If we ever exceed
     * it the MTU derivation chain is broken (the TAP MTU should have been
     * clamped to tx.mtu - 15 at ifup), so count it loudly
     * instead of silently dropping - g_pan.tx_oversize must stay 0. */
    limit = conn->chan.tx.mtu;
    if ((uint32_t)eth_len + PAN_BNEP_OVERHEAD > (uint32_t)limit) {
        g_pan.tx_oversize++;
        syslog(LOG_ERR, "[pan] tx %u > tx_mtu %u (oversize=%lu)\n",
            eth_len, limit, (unsigned long)g_pan.tx_oversize);
        pan_conn_put(conn);
        return BT_STATUS_NOMEM;
    }

    buf = bt_l2cap_create_pdu_timeout(NULL, 0, K_NO_WAIT);
    if (!buf) {
        g_pan.tx_nobuf++;
        pan_conn_put(conn);
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
        pan_conn_put(conn);
        return BT_STATUS_FAIL;
    }
    net_buf_add(buf, (size_t)n);

    ret = bt_l2cap_chan_send(&conn->chan.chan, buf);
    if (ret < 0) {
        syslog(LOG_ERR, "[pan] tx send failed %d (len=%d tx_mtu=%u)\n",
            ret, n, conn->chan.tx.mtu);
        net_buf_unref(buf);
        pan_conn_put(conn);
        return BT_STATUS_FAIL;
    }
    pan_conn_put(conn);
    return BT_STATUS_SUCCESS;
}

uint16_t bt_sal_pan_get_tx_mtu(const bt_address_t* addr)
{
    pan_conn_t* conn;
    bt_list_node_t* node;

    if (!addr) {
        return 0;
    }

    conn = pan_find_conn(addr);
    if (conn) {
        if (conn->state == PAN_CONN_CONNECTED) {
            uint16_t mtu = conn->chan.tx.mtu;
            pan_conn_put(conn);
            return mtu;
        }
        pan_conn_put(conn);
    }

    /* Address lookup missed.  PAN_MAX_CONNECTIONS is 1, so there is at most
     * one real candidate, and the two establishment paths store the peer
     * address from different sources (outbound bt_sal_pan_connect() copies
     * the caller's bytes, the inbound pan_server_accept() builds it from
     * the stack's own representation) - a mismatch here used to return 0,
     * which made the profile abort ifup with "no_negotiated_mtu" and drop a
     * link that had just finished the BNEP handshake.  Fall back to the
     * only connected entry instead of failing. */
    pan_conn_lock();
    for (node = bt_list_head(g_pan.conn_list); node;
         node = bt_list_next(g_pan.conn_list, node)) {
        pan_conn_t* c = (pan_conn_t*)bt_list_node(node);
        if (c && c->linked && c->state == PAN_CONN_CONNECTED) {
            uint16_t mtu = c->chan.tx.mtu;
            pan_conn_unlock();
            syslog(LOG_WARNING, "[pan] tx_mtu: addr lookup missed, using the "
                                "single connected entry (tx_mtu=%u)\n",
                mtu);
            return mtu;
        }
    }
    pan_conn_unlock();
    return 0;
}
