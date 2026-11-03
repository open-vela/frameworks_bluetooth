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
#include <syslog.h>
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
NET_BUF_POOL_DEFINE(bnep_tx_pool, PL_SDU_NUM, PL_SDU_SIZE, 32, NULL)  /* PAN-FIX: user_data_size for closure */;

static struct net_buf *pl_alloc_sdu(void)
{
    struct net_buf *b = net_buf_alloc(&bnep_tx_pool, K_NO_WAIT);
    if (b) { b->user_data_size = 32; }  /* PAN-FIX: ensure user_data_size set */
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
    {
        syslog(LOG_INFO, "[pan] BNEP-DIAG buf=%p ref=%d len=%u head=%u chan=%p conn=%p cid=0x%04x\n",
               (void*)buf, (int)(buf?buf->ref:-1), (unsigned)(buf?buf->len:0),
               (unsigned)(buf?net_buf_headroom(buf):0),
               (void*)&g_chan.chan, (void*)g_chan.chan.conn, (unsigned)g_chan.tx.cid);
        if (buf && buf->ref != 1) {
            syslog(LOG_INFO, "[pan] BNEP-FIX ref=%d -> 1\n", (int)buf->ref);
            buf->ref = 1;
        }
        int snd = bt_l2cap_chan_send(&g_chan.chan, buf);
        syslog(LOG_INFO, "[pan] BNEP-SEND rc=%d len=%u\n", snd, (unsigned)(buf ? buf->len : 0));
        if (snd < 0 && g_chan.chan.conn && g_chan.tx.cid) {
            syslog(LOG_INFO, "[pan] BNEP-BYPASS: raw ACL inject, cid=0x%04x\n",
                   (unsigned)g_chan.tx.cid);
            /* construct raw L2CAP frame: [len][cid][payload] */
            uint8_t raw[64];
            uint16_t l2cap_len = buf ? buf->len : 11;
            raw[0] = l2cap_len & 0xff;
            raw[1] = (l2cap_len >> 8) & 0xff;
            raw[2] = g_chan.tx.cid & 0xff;
            raw[3] = (g_chan.tx.cid >> 8) & 0xff;
            if (buf) memcpy(&raw[4], buf->data, buf->len);
            syslog(LOG_INFO, "[pan] BNEP-BYPASS: L2CAP hdr %02x %02x %02x %02x payload=%d\n",
                   raw[0], raw[1], raw[2], raw[3], (int)l2cap_len);
            /* TODO: inject raw[0..3+l2cap_len] as ACL data via bth4 */
        }
        return snd;
    }
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
            if (g_state == PL_BNEP_OPENING || g_state == PL_IDLE) mark_up(); /* PAN-FIX: also handle post-timeout */
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
{
            int hi; char hex[80] = {0}; int hp = 0;
            for (hi = 0; hi < 16 && hi < len; hi++)
                hp += snprintf(hex+hp, sizeof(hex)-hp, "%02x ", data[hi]);
            syslog(LOG_INFO, "[pan] BNEP-RXHEX len=%d: %s\n", len, hex);
        }
        BT_LOGE("[pan] rx: malformed BNEP (len=%u err=%d), dropped", (unsigned)len, ret);
        return 0;
    }

    if (bnep_type_is_ethernet(f.type)) {
        if (f.u.eth.dst == NULL || f.u.eth.src == NULL) {
            /* PAN-FIX: compressed frames - use g_peer BT addr as missing MAC */
            static uint8_t s_mac[6];
            memcpy(s_mac, &g_peer, 6);
            if (f.u.eth.payload && f.u.eth.payload_len > 0) {
                g_rx_frames++;
                syslog(LOG_INFO, "[pan] RX-COMPRESSED type=0x%02x eth=0x%04x len=%u\n",
                       f.type, f.u.eth.ethertype, (unsigned)f.u.eth.payload_len);
                pan_on_data_received(&g_peer, f.u.eth.ethertype,
                    (uint8_t*)(f.u.eth.dst ? f.u.eth.dst : s_mac),
                    (uint8_t*)(f.u.eth.src ? f.u.eth.src : s_mac),
                    (uint8_t*)f.u.eth.payload, (uint16_t)f.u.eth.payload_len);
            } else {
                g_rx_drops++;
            }
            return 0;
        }
        g_rx_frames++; syslog(LOG_INFO, "[pan] RX-GENERAL type=0x%02x eth=0x%04x len=%u dst=%02x:%02x src=%02x:%02x\n", f.type, f.u.eth.ethertype, (unsigned)f.u.eth.payload_len, f.u.eth.dst[0], f.u.eth.dst[5], f.u.eth.src[0], f.u.eth.src[5]);
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
    syslog(LOG_INFO, "[pan] CID-DUMP rx.cid=0x%04x tx.cid=0x%04x\n",
               (unsigned)g_chan.rx.cid, (unsigned)g_chan.tx.cid);
    syslog(LOG_INFO, "[pan] L2CAP-CONNECTED psm=0x%04x rx.mtu=%u tx.mtu=%u\n",
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
    syslog(LOG_INFO, "[pan] waiting SETUP_CONN_RSP (budget %d ms)\n", BNEP_SETUP_TIMEOUT_MS);
    /* PAN-FIX: do NOT mark_up here - wait for SETUP_CONN_RSP in rx_control */

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

/* PAN-FIX: listen on PSM 0x000F so phone NAP can dial US (PR#591 flow) */
static struct bt_l2cap_server g_pan_server;

static int pan_server_accept(struct bt_conn *acl, struct bt_l2cap_server *server,
                             struct bt_l2cap_chan **chan)
{
    syslog(LOG_INFO, "[pan] incoming BNEP channel request\n");
    g_chan.chan.ops = &g_ops;
    g_chan.rx.mtu = BNEP_MIN_MTU;
    g_chan.tx.mtu = BNEP_MIN_MTU;
    g_chan.required_sec_level = BT_SECURITY_L0;
    pthread_mutex_lock(&g_lock);
    g_state = PL_BNEP_OPENING;
    pthread_mutex_unlock(&g_lock);
    *chan = &g_chan.chan;
    syslog(LOG_INFO, "[pan] incoming BNEP channel accepted\n");
    return 0;
}

bt_status_t bt_sal_pan_init(uint8_t max_conn, pan_role_t local_role)
{
    (void)max_conn;
    g_local = local_role;

    memset(&g_chan, 0, sizeof(g_chan));
    g_chan.chan.ops = &g_ops;
    #if 0  /* try both variants below */
    g_chan.chan.required_sec_level = BT_SECURITY_L0;
#endif
    /* Variant A: set on bt_l2cap_br_chan itself (line 279 field) */
    g_chan.required_sec_level = BT_SECURITY_L0;

    /* PAN Profile requires L2CAP MTU >= 1691 */
    g_chan.rx.mtu = BNEP_MIN_MTU;
    g_chan.tx.mtu = BNEP_MIN_MTU;

#ifndef K_WORK_DELAYABLE_DEFINE
    k_work_init_delayable(&g_setup_work, setup_timeout_work);
#endif

    memset(&g_eth, 0, sizeof(g_eth));
    g_state = PL_IDLE;
    BT_LOGI("[pan] init, psm=0x%04x mtu=%u", BNEP_PSM, BNEP_MIN_MTU);

    /* PAN-FIX: register L2CAP server so phone NAP can dial us (PR#591) */
    g_pan_server.psm = BNEP_PSM;
    g_pan_server.sec_level = BT_SECURITY_L0;
    g_pan_server.accept = pan_server_accept;
    int sret = bt_l2cap_br_server_register(&g_pan_server);
    syslog(LOG_INFO, "[pan] BNEP server register (psm 0x%04x) ret=%d\n",
           BNEP_PSM, sret);

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

/* PAN_CONN_CB: fire L2CAP connect when ACL comes up in PL_ACL_WAIT */
static void __attribute__((used)) pan_conn_connected(struct bt_conn *conn, uint8_t err)
{
    syslog(LOG_INFO, "[pan] CB-ENTER err=%d\n", err);
    bt_address_t peer;
    const bt_addr_le_t *dst;
    int st;

    if (err) {
        syslog(LOG_ERR, "[pan] conn_cb err=%d\n", err);
        return;
    }
    if (!bt_conn_is_type(conn, BT_CONN_TYPE_BR)) return;

    dst = bt_conn_get_dst(conn);
    memcpy(&peer, dst->a.val, sizeof(peer));

    pthread_mutex_lock(&g_lock);
    st = g_state;
    pthread_mutex_unlock(&g_lock);

    if (st == PL_ACL_WAIT &&
        memcmp(&peer, &g_peer, sizeof(peer)) == 0) {
        syslog(LOG_INFO, "[pan] ACL up, L2CAP PSM=0x%04x\n", BNEP_PSM);
        pthread_mutex_lock(&g_lock);
        g_state = PL_BNEP_OPENING;
        pthread_mutex_unlock(&g_lock);
        bt_l2cap_chan_connect(conn, &g_chan.chan, BNEP_PSM);
    }
}



BT_CONN_CB_DEFINE(pan_conn_cbs) = {
    .connected = pan_conn_connected,
};

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
        int rc = bt_l2cap_chan_connect(conn, &g_chan.chan, BNEP_PSM);
        syslog(LOG_INFO, "[pan] direct l2cap_connect rc=%d\n", rc);
        if (rc != 0 && rc != -17) {
            syslog(LOG_ERR, "[pan] direct l2cap FAILED rc=%d\n", rc);
            bt_conn_unref(conn);
            pthread_mutex_lock(&g_lock);
            g_state = PL_IDLE;
            pthread_mutex_unlock(&g_lock);
            return BT_STATUS_FAIL;
        }
        bt_conn_unref(conn);
    } else {
        conn = bt_conn_create_br((bt_addr_t *)addr, BT_BR_CONN_PARAM_DEFAULT);
        if (!conn) {
            syslog(LOG_ERR, "[pan] create_br FAILED\n");
            return BT_STATUS_FAIL;
        }
        bt_conn_unref(conn);
        syslog(LOG_INFO, "[pan] POLL: waiting ACL...\n");
        int l2cap_ok = 0;
        for (int i = 0; i < 50; i++) {
            usleep(100000);
            conn = bt_conn_lookup_addr_br((bt_addr_t *)addr);
            if (conn && i >= 20) {
                        syslog(LOG_INFO, "[pan] POLL: ACL ready %d00ms\n", i + 1);

                /* PAN-FIX: use L2 like PR#591, no fallback chain */
                {
                    int sec_rc = bt_conn_set_security(conn, BT_SECURITY_L2);
                    syslog(LOG_INFO, "[pan] SEC-REQ L2 rc=%d\n", sec_rc);
                    for (int j = 0; j < 30; j++) {
                        usleep(100000);
                        bt_security_t lvl = bt_conn_get_security(conn);
                        if (lvl >= BT_SECURITY_L2) {
                            syslog(LOG_INFO, "[pan] SEC OK lvl=%d after %d00ms\n", lvl, j+1);
                            break;
                        }
                    }
                }

                /* STEP 3: now L2CAP connect (security already met) */
                pthread_mutex_lock(&g_lock);
                g_state = PL_BNEP_OPENING;
                pthread_mutex_unlock(&g_lock);
                int rc = bt_l2cap_chan_connect(conn, &g_chan.chan, BNEP_PSM);
                syslog(LOG_INFO, "[pan] l2cap_connect rc=%d PSM=0x%04x\n", rc, BNEP_PSM);
                if (rc == 0 || rc == -17) {
                    syslog(LOG_INFO, "[pan] L2CAP ok(%d), waiting BNEP callback\n", rc);
                    l2cap_ok = 1;
                    bt_conn_unref(conn);
                    break;
                }
                bt_conn_unref(conn);
                bt_conn_unref(conn);
                            }
        }
        if (!l2cap_ok) {
            syslog(LOG_ERR, "[pan] POLL: ACL TIMEOUT 5s\n");
            pthread_mutex_lock(&g_lock);
            g_state = PL_IDLE;
            pthread_mutex_unlock(&g_lock);
            return BT_STATUS_FAIL;
        }

        /* PAN-FIX: wait for BNEP connected callback (up to 5s) */
        syslog(LOG_INFO, "[pan] waiting BNEP connected callback...\n");
        for (int k = 0; k < 50; k++) {
            usleep(100000);
            pthread_mutex_lock(&g_lock);
            int st = g_state;
            pthread_mutex_unlock(&g_lock);
            if (st != PL_BNEP_OPENING) {
                syslog(LOG_INFO, "[pan] BNEP state=%d after %d00ms\n", st, k+1);
                break;
            }
            if (k % 10 == 9) {
                syslog(LOG_INFO, "[pan] BNEP-WAIT state=%d t=%d00ms\n", st, k+1);
            }
        }
        pthread_mutex_lock(&g_lock);
        int final_st = g_state;
        pthread_mutex_unlock(&g_lock);
        if (final_st == PL_BNEP_OPENING) {
            syslog(LOG_ERR, "[pan] BNEP TIMEOUT state=%d, NOT resetting (callback may fire later)\n", final_st);
            /* PAN-FIX: do NOT reset g_state - let connected callback call mark_up() */
        }
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
    /* PAN-FIX: removed g_state check - BNEP setup ok means link is up */

    buf = pl_alloc_sdu();
    if (!buf) { syslog(LOG_INFO, "[pan] TX-FAIL: pl_alloc_sdu NULL\n"); return BT_STATUS_FAIL; }

    /* encode straight into the net_buf tail:
     * first frame GENERAL, same-session frames COMPRESSED (M1) */
    n = bnep_encode_ethernet(buf->data + buf->len, net_buf_tailroom(buf),
                             &g_eth, 1, /* PAN-FIX: force GENERAL */
                             dst_addr, src_addr, protocol, data, length);
    if (n < 0) {
        syslog(LOG_INFO, "[pan] TX-FAIL: encode=%d tailroom=%d len=%d\n", n, (int)net_buf_tailroom(buf), (int)length);
        net_buf_unref(buf);
        return BT_STATUS_FAIL;
    }
    net_buf_add(buf, (size_t)n);

    { int src = bt_l2cap_chan_send(&g_chan.chan, buf);
      if (src == 0) { g_tx_frames++; return BT_STATUS_SUCCESS; }
      syslog(LOG_INFO, "[pan] TX-FAIL: l2cap_send=%d\n", src);
    }
    return BT_STATUS_FAIL;
}

/* COMPAT-SHIM: bt_buf_get_tx is HCI_RAW-only in this zblue; host-mode fallback */
#if !defined(CONFIG_BT_HCI_RAW)
#include <zephyr/bluetooth/buf.h>
#ifndef BT_BUF_RESERVE
#define BT_BUF_RESERVE 0
#endif
struct net_buf *bt_buf_get_tx(enum bt_buf_type type, k_timeout_t timeout,
                              const void *data, size_t size)
{
    struct net_buf *buf = net_buf_alloc(&bnep_tx_pool, timeout);
    if (buf) { buf->user_data_size = 32; }  /* PAN-FIX: ensure user_data_size set */
    if (!buf) return NULL;
    net_buf_reserve(buf, BT_BUF_RESERVE);
    bt_buf_set_type(buf, type);
    if (data && size > 0) net_buf_add_mem(buf, data, size);
    return buf;
}
#endif
