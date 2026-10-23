/* BNEP/L2CAP binding for PANU.
 *
 * Implemented from Bluetooth SIG "BNEP Specification v1.0" and
 * "Personal Area Networking (PAN) Profile v1.0".  No third-party code used.
 *
 * Wire sequence (we = PANU 0x1115, peer = NAP 0x1116):
 *   -> 01 01 02 11 16 11 15      SETUP_CONN_REQ
 *   <- 01 02 00 00               SETUP_CONN_RSP  (0x0000 = OK)
 *   <- 01 03 ..  /  01 05 ..     FILTER_*_SET    (must answer)
 *   -> 01 04 00 00 / 01 06 00 00 FILTER_*_RSP    (0x0000 = OK)
 *   <-> 00 / 02 / 03 / 04 ...    Ethernet
 */
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/net_buf.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bnep_codec.h"
#include "sal_pan_interface.h"
#include "utils/log.h"

#define LOG_TAG "pan"

#define BNEP_PSM               0x000f
#define BNEP_UUID_PANU         0x1115
#define BNEP_UUID_NAP          0x1116

#define BNEP_MIN_MTU           1691
#define BNEP_SETUP_TIMEOUT_MS  5000
#define BNEP_REQUIRE_MIN_MTU   1

#define PL_SDU_SIZE   (BT_L2CAP_CHAN_SEND_RESERVE + BNEP_MAX_FRAME)
#define PL_SDU_NUM    8

/* FIX B1: BR/EDR channels must be bt_l2cap_br_chan (l2cap.h:264) */
static struct bt_l2cap_br_chan g_chan;
static struct bt_l2cap_chan_ops g_ops;

enum pl_state { PL_IDLE, PL_ACL_WAIT, PL_BNEP_OPENING, PL_UP, PL_CLOSING };
static enum pl_state g_state = PL_IDLE;

static bt_address_t    g_peer;
static pan_role_t      g_local  = PAN_ROLE_PANU;
static pan_role_t      g_remote = PAN_ROLE_NAP;
static struct bnep_ctx g_eth;
static uint16_t        g_mtu;
static uint16_t        g_rx_frames, g_tx_frames, g_rx_drops, g_bad_ctrl;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

extern void pan_on_connection_state_changed(bt_address_t *addr,
    pan_role_t remote_role, pan_role_t local_role, profile_connection_state_t state);
extern void pan_on_data_received(bt_address_t *addr, uint16_t protocol,
    uint8_t *dst_addr, uint8_t *src_addr, uint8_t *data, uint16_t length);

/* ---- zblue net_buf ------------------------------------------------
 * TX SDU + RX SDU buffers come from zblue's net_buf mechanism.
 * alloc_seg() pulls ACL-sized segment buffers from bt_buf_get_tx(); SDU
 * buffers must be >= 1691 B and therefore need a net_buf pool, exactly as
 * zblue's own SDP/RFCOMM channels do (no custom allocator).
 * ------------------------------------------------------------------ */
NET_BUF_POOL_DEFINE(pl_sdu_pool, PL_SDU_NUM, PL_SDU_SIZE, 0, NULL);

static struct net_buf *pl_alloc_sdu(void)
{
    struct net_buf *b = net_buf_alloc(&pl_sdu_pool, K_NO_WAIT);
    if (b) net_buf_reserve(b, BT_L2CAP_CHAN_SEND_RESERVE);
    return b;
}

static struct net_buf *chan_alloc_seg(struct bt_l2cap_chan *chan)
{
    /* VERIFY-BUF: 3rd arg of bt_buf_get_tx() differs between zblue drops.
     *   NULL                       -> zblue reserves internally
     *   BT_L2CAP_CHAN_SEND_RESERVE -> older signature takes a reserve
     */
    (void)chan;
    return bt_buf_get_tx(BT_BUF_ACL_OUT, K_NO_WAIT, NULL, 0);
}

static struct net_buf *chan_alloc_buf(struct bt_l2cap_chan *chan)
{
    /* FIX B6: without this the SDU RX MTU is truncated to
     * BT_L2CAP_SDU_RX_MTU and 1691-byte frames are dropped. */
    (void)chan;
    return pl_alloc_sdu();
}

static int l2cap_send(const uint8_t *data, uint16_t len)
{
    struct net_buf *buf = pl_alloc_sdu();
    if (!buf) { BT_LOGE("[pan] tx: net_buf_alloc failed len=%u", len); return -ENOMEM; }
    if (len > net_buf_tailroom(buf)) { net_buf_unref(buf); return -EMSGSIZE; }
    net_buf_add_mem(buf, data, len);
    return bt_l2cap_chan_send(&g_chan.chan, buf);
}

static void role_uuids(uint8_t *local, uint8_t *remote)
{
    uint16_t l = (g_local  == PAN_ROLE_PANU) ? BNEP_UUID_PANU : BNEP_UUID_NAP;
    uint16_t r = (g_remote == PAN_ROLE_PANU) ? BNEP_UUID_PANU : BNEP_UUID_NAP;
    local[0]  = (uint8_t)(l >> 8);  local[1]  = (uint8_t)l;
    remote[0] = (uint8_t)(r >> 8);  remote[1] = (uint8_t)r;
}

static void bnep_send_setup_req(void)
{
    uint8_t f[8], lu[2], ru[2];
    role_uuids(lu, ru);
    /* FIX B3: [uuid_size=2][dst=remote][src=local] */
    int n = bnep_encode_setup_req(f, sizeof(f), ru, lu, 2);
    if (n > 0) {
        BT_LOGI("[pan] -> SETUP_CONN_REQ %02x %02x %02x %02x %02x %02x %02x",
                f[0], f[1], f[2], f[3], f[4], f[5], f[6]);
        l2cap_send(f, (uint16_t)n);
    }
}

static void bnep_send_rsp(int (*enc)(uint8_t *, size_t, uint16_t),
                          uint16_t code, const char *what)
{
    uint8_t f[4];
    int n = enc(f, sizeof(f), code);
    if (n > 0) { BT_LOGI("[pan] -> %s 0x%04x", what, code); l2cap_send(f, (uint16_t)n); }
}

static void bnep_send_cmd_not_understood(uint8_t offending)
{
    uint8_t f[3];
    int n = bnep_encode_cmd_not_understood(f, sizeof(f), offending);
    if (n > 0) {
        BT_LOGW("[pan] unknown ctrl 0x%02x -> COMMAND_NOT_UNDERSTOOD", offending);
        l2cap_send(f, (uint16_t)n);
    }
}

/* ---- 5 s setup budget -------------------------------------------- */
static void setup_timeout_work(struct k_work *work);

#ifdef K_WORK_DELAYABLE_DEFINE
K_WORK_DELAYABLE_DEFINE(g_setup_work, setup_timeout_work);
#else
static struct k_work_delayable g_setup_work;
#endif

static void setup_timeout_work(struct k_work *work)
{
    (void)work;
    pthread_mutex_lock(&g_lock);
    if (g_state != PL_BNEP_OPENING) { pthread_mutex_unlock(&g_lock); return; }
    BT_LOGE("[pan] SETUP_CONN_RSP timeout after %d ms -> close channel & retry",
            BNEP_SETUP_TIMEOUT_MS);
    g_state = PL_CLOSING;
    pthread_mutex_unlock(&g_lock);
    bt_l2cap_chan_disconnect(&g_chan.chan);
}

static void setup_timer_start(void) { k_work_schedule(&g_setup_work, K_MSEC(BNEP_SETUP_TIMEOUT_MS)); }
static void setup_timer_stop(void)  { k_work_cancel_delayable(&g_setup_work); }

/* ---- up / down ---------------------------------------------------- */
static void mark_up(void)
{
    BT_LOGI("[pan] bnep setup ok, mtu=%u", g_mtu);
    g_state = PL_UP;
    memset(&g_eth, 0, sizeof(g_eth));
    pan_on_connection_state_changed(&g_peer, g_remote, g_local, PROFILE_STATE_CONNECTED);
}

static void mark_down(void)
{
    if (g_state == PL_UP) {
        pan_on_connection_state_changed(&g_peer, g_remote, g_local,
                                        PROFILE_STATE_DISCONNECTED);
    }
    g_state = PL_IDLE;
    memset(&g_eth, 0, sizeof(g_eth));
}

/* ---- RX control plane --------------------------------------------- */
static void rx_control(const struct bnep_ctrl *c)
{
    switch (c->type) {
    case BNEP_CTRL_SETUP_CONN_RSP:
        if (c->u.rsp.response == BNEP_SETUP_OK) {
            setup_timer_stop();
            pthread_mutex_lock(&g_lock);
            if (g_state == PL_BNEP_OPENING) mark_up();
            pthread_mutex_unlock(&g_lock);
        } else {
            BT_LOGE("[pan] SETUP_CONN_RSP rejected 0x%04x -> close", c->u.rsp.response);
            setup_timer_stop();
            mark_down();
            bt_l2cap_chan_disconnect(&g_chan.chan);
        }
        break;

    case BNEP_CTRL_FILTER_NET_TYPE_SET:
        BT_LOGI("[pan] FILTER_NET_TYPE_SET entries=%u -> rsp 0x0000", c->u.filter_net.count);
        bnep_send_rsp(bnep_encode_filter_net_rsp, BNEP_FILTER_OK, "FILTER_NET_TYPE_RSP");
        break;

    case BNEP_CTRL_FILTER_MULTI_ADDR_SET:
        BT_LOGI("[pan] FILTER_MULTI_ADDR_SET entries=%u -> rsp 0x0000", c->u.filter_multi.count);
        bnep_send_rsp(bnep_encode_filter_multi_rsp, BNEP_FILTER_OK, "FILTER_MULTI_ADDR_RSP");
        break;

    case BNEP_CTRL_SETUP_CONN_REQ:
        BT_LOGI("[pan] peer SETUP_CONN_REQ -> rsp 0x0000");
        bnep_send_rsp(bnep_encode_setup_rsp, BNEP_SETUP_OK, "SETUP_CONN_RSP");
        break;

    case BNEP_CTRL_CMD_NOT_UNDERSTOOD:
        BT_LOGW("[pan] peer CNU offending=0x%02x", c->u.cmd_not_understood.offending_type);
        break;

    default:
        bnep_send_cmd_not_understood(c->type);
        break;
    }
}

/* ---- RX entry ------------------------------------------------------ */
static int chan_recv(struct bt_l2cap_chan *chan, struct net_buf *buf)
{
    static uint8_t linear[PL_SDU_SIZE];
    const uint8_t *data;
    size_t len;
    struct bnep_frame f;
    int ret;

    (void)chan;
    if (buf == NULL || buf->len == 0) return 0;

    if (buf->frags == NULL) {
        data = buf->data;
        len  = buf->len;
    } else {
        len = net_buf_linearize(linear, sizeof(linear), buf, 0, buf->len);
        if (len != buf->len) { g_rx_drops++; return 0; }
        data = linear;
    }

    ret = bnep_decode(data, len, &g_eth, &f);

    if (ret == BNEP_ERR_BADUUID) {
        g_bad_ctrl++;
        BT_LOGE("[pan] SETUP_CONN_REQ uuid_size=%u illegal -> rsp 0x0003",
                f.u.ctrl.u.setup_req.raw_size);
        bnep_send_rsp(bnep_encode_setup_rsp, BNEP_SETUP_ERR_INVALID_UUID_SIZE,
                      "SETUP_CONN_RSP");
        return 0;
    }
    if (ret == BNEP_ERR_BADTYPE && f.type == 0x01u) {
        g_bad_ctrl++;
        bnep_send_cmd_not_understood(f.u.ctrl.type);
        return 0;
    }
    if (ret != BNEP_OK) {
        g_rx_drops++;
        BT_LOGE("[pan] rx: malformed BNEP (len=%u err=%d), dropped", (unsigned)len, ret);
        return 0;
    }

    if (bnep_type_is_ethernet(f.type)) {
        if (f.u.eth.dst == NULL || f.u.eth.src == NULL) {
            g_rx_drops++;
            BT_LOGE("[pan] rx: compressed frame without MAC ctx, dropped");
            return 0;
        }
        g_rx_frames++;
        pan_on_data_received(&g_peer, f.u.eth.ethertype,
                             (uint8_t *)f.u.eth.dst, (uint8_t *)f.u.eth.src,
                             (uint8_t *)f.u.eth.payload, (uint16_t)f.u.eth.payload_len);
    } else {
        rx_control(&f.u.ctrl);
    }

    /* FIX B8: synchronous processing -> 0, stack owns/unrefs buf. */
    return 0;
}

static void chan_connected(struct bt_l2cap_chan *chan)
{
    struct bt_l2cap_br_chan *br = BT_L2CAP_BR_CHAN(chan);

    g_mtu = (br->rx.mtu < br->tx.mtu) ? br->rx.mtu : br->tx.mtu;
    BT_LOGI("[pan] l2cap connected psm=0x%04x rx.mtu=%u tx.mtu=%u",
            BNEP_PSM, br->rx.mtu, br->tx.mtu);

#if BNEP_REQUIRE_MIN_MTU
    if (g_mtu < BNEP_MIN_MTU) {
        BT_LOGE("[pan] peer mtu=%u < %u (PAN min) -> close", g_mtu, BNEP_MIN_MTU);
        bt_l2cap_chan_disconnect(chan);
        return;
    }
#else
    if (g_mtu < BNEP_MIN_MTU)
        BT_LOGW("[pan] peer mtu=%u < %u, continuing", g_mtu, BNEP_MIN_MTU);
#endif

    pthread_mutex_lock(&g_lock);
    g_state = PL_BNEP_OPENING;
    pthread_mutex_unlock(&g_lock);

    bnep_send_setup_req();
    setup_timer_start();
    BT_LOGI("[pan] waiting SETUP_CONN_RSP (budget %d ms)", BNEP_SETUP_TIMEOUT_MS);
}

static void chan_disconnected(struct bt_l2cap_chan *chan)
{
    (void)chan;
    setup_timer_stop();
    BT_LOGI("[pan] l2cap disconnected rx=%u tx=%u drops=%u bad_ctrl=%u",
            g_rx_frames, g_tx_frames, g_rx_drops, g_bad_ctrl);
    pthread_mutex_lock(&g_lock);
    mark_down();
    pthread_mutex_unlock(&g_lock);
}

static void chan_sent(struct bt_l2cap_chan *chan) { (void)chan; }

static void chan_encrypt_change(struct bt_l2cap_chan *chan, uint8_t hci_status)
{
    (void)chan;
    BT_LOGI("[pan] encrypt_change status=0x%02x", hci_status);
}

static struct bt_l2cap_chan_ops g_ops = {
    .connected       = chan_connected,
    .disconnected    = chan_disconnected,
    .encrypt_change  = chan_encrypt_change,
    .alloc_seg       = chan_alloc_seg,
    .alloc_buf       = chan_alloc_buf,
    .recv            = chan_recv,
    .sent            = chan_sent,      /* 若 ops 无此成员则删除本行 */
};

static void conn_connected(struct bt_conn *conn, uint8_t err)
{
    if (err == 0 && g_state == PL_ACL_WAIT) {
        pthread_mutex_lock(&g_lock);
        g_state = PL_BNEP_OPENING;
        pthread_mutex_unlock(&g_lock);
        bt_l2cap_chan_connect(conn, &g_chan.chan, BNEP_PSM);
    }
}

void sal_pan_conn_cb(struct bt_conn *conn, uint8_t err)
{
    BT_LOGI("[pan] sal_pan_conn_cb: err=%d", err);
    conn_connected(conn, err);
}

bt_status_t bt_sal_pan_init(uint8_t max_conn, pan_role_t local_role)
{
    (void)max_conn;
    g_local = local_role;

    memset(&g_chan, 0, sizeof(g_chan));
    g_chan.chan.ops = &g_ops;

    /* PAN Profile requires L2CAP MTU >= 1691 */
    g_chan.rx.mtu = BNEP_MIN_MTU;
    g_chan.tx.mtu = BNEP_MIN_MTU;

#ifndef K_WORK_DELAYABLE_DEFINE
    k_work_init_delayable(&g_setup_work, setup_timeout_work);
#endif

    memset(&g_eth, 0, sizeof(g_eth));
    g_state = PL_IDLE;
    BT_LOGI("[pan] init, psm=0x%04x mtu=%u", BNEP_PSM, BNEP_MIN_MTU);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pan_cleanup(void)
{
    if (g_state == PL_UP || g_state == PL_BNEP_OPENING) {
        setup_timer_stop();
        bt_l2cap_chan_disconnect(&g_chan.chan);
    }
    mark_down();
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pan_connect(bt_address_t *addr, uint8_t dst_role, uint8_t src_role)
{
    struct bt_conn *conn;

    pthread_mutex_lock(&g_lock);
    if (g_state != PL_IDLE) { pthread_mutex_unlock(&g_lock); return BT_STATUS_FAIL; }
    memcpy(&g_peer, addr, sizeof(g_peer));
    g_remote = (pan_role_t)dst_role;
    g_local  = (pan_role_t)src_role;
    pthread_mutex_unlock(&g_lock);

    conn = bt_conn_lookup_addr_br((bt_addr_t *)addr);
    BT_LOGI("[pan] sal_pan_connect: lookup=%p", conn);
    if (conn) {
        pthread_mutex_lock(&g_lock);
        g_state = PL_BNEP_OPENING;
        pthread_mutex_unlock(&g_lock);
        bt_l2cap_chan_connect(conn, &g_chan.chan, BNEP_PSM);
        bt_conn_unref(conn);
    } else {
        conn = bt_conn_create_br((bt_addr_t *)addr, BT_BR_CONN_PARAM_DEFAULT);
        if (!conn) {
            BT_LOGE("[pan] bt_conn_create_br failed -> retry later");
            return BT_STATUS_FAIL;
        }
        pthread_mutex_lock(&g_lock);
        g_state = PL_ACL_WAIT;
        pthread_mutex_unlock(&g_lock);
        bt_conn_unref(conn);
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pan_disconnect(bt_address_t *addr)
{
    (void)addr;
    setup_timer_stop();
    bt_l2cap_chan_disconnect(&g_chan.chan);
    mark_down();
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_pan_write(bt_address_t *addr, uint16_t protocol,
    uint8_t *dst_addr, uint8_t *src_addr, uint8_t *data, uint16_t length)
{
    struct net_buf *buf;
    int n;

    (void)addr;
    if (g_state != PL_UP) return BT_STATUS_FAIL;

    buf = pl_alloc_sdu();
    if (!buf) { BT_LOGW("[pan] tx: no net_buf, frame dropped"); return BT_STATUS_FAIL; }

    /* encode straight into the net_buf tail:
     * first frame GENERAL, same-session frames COMPRESSED (M1) */
    n = bnep_encode_ethernet(buf->data + buf->len, net_buf_tailroom(buf),
                             &g_eth, 0,
                             dst_addr, src_addr, protocol, data, length);
    if (n < 0) {
        BT_LOGE("[pan] tx: encode failed %d, dropped", n);
        net_buf_unref(buf);
        return BT_STATUS_FAIL;
    }
    net_buf_add(buf, (size_t)n);

    if (bt_l2cap_chan_send(&g_chan.chan, buf) == 0) {
        g_tx_frames++;
        return BT_STATUS_SUCCESS;
    }
    return BT_STATUS_FAIL;
}
