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

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#include "bt_addr.h"
#include "l2cap_service.h"
#include "sal_interface.h"
#include "sal_l2cap_interface.h"
#include "sal_zblue.h"
#include "service_loop.h"
#include "utils/log.h"

#define L2CAP_MAX_CHANNELS 20
#define L2CAP_MAX_SERVERS 8
#define L2CAP_SDU_BUF_SIZE CONFIG_BLUETOOTH_L2CAP_OUTGOING_MTU
#define L2CAP_TX_BUF_COUNT 8

NET_BUF_POOL_FIXED_DEFINE(l2cap_tx_pool, L2CAP_TX_BUF_COUNT,
    BT_L2CAP_SDU_BUF_SIZE(L2CAP_SDU_BUF_SIZE), 0, NULL);

typedef struct {
    struct bt_l2cap_le_chan le_chan;
    bt_address_t addr;
    uint16_t local_cid;
    uint16_t psm;
    uint16_t id;
    bool is_server;
} sal_l2cap_channel_t;

typedef struct {
    struct bt_l2cap_server server;
    uint16_t psm;
    uint16_t mtu;
    uint16_t mps;
    uint16_t credits;
    bool in_use;
} sal_l2cap_server_t;

static sal_l2cap_channel_t* g_channels[L2CAP_MAX_CHANNELS];
static sal_l2cap_server_t* g_servers[L2CAP_MAX_SERVERS];

static sal_l2cap_channel_t* find_channel_by_cid(uint16_t local_cid)
{
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (g_channels[i] && g_channels[i]->local_cid == local_cid) {
            return g_channels[i];
        }
    }
    return NULL;
}

static sal_l2cap_channel_t* find_channel_by_le_chan(struct bt_l2cap_le_chan* le_chan)
{
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (g_channels[i] && &g_channels[i]->le_chan == le_chan) {
            return g_channels[i];
        }
    }
    return NULL;
}

static sal_l2cap_channel_t* alloc_channel(void)
{
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (!g_channels[i]) {
            g_channels[i] = calloc(1, sizeof(sal_l2cap_channel_t));
            return g_channels[i];
        }
    }
    return NULL;
}

static void free_channel(sal_l2cap_channel_t* ch)
{
    if (!ch) {
        return;
    }
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (g_channels[i] == ch) {
            free(g_channels[i]);
            g_channels[i] = NULL;
            return;
        }
    }
}

static sal_l2cap_server_t* find_server_by_psm(uint16_t psm)
{
    for (int i = 0; i < L2CAP_MAX_SERVERS; i++) {
        if (g_servers[i] && g_servers[i]->psm == psm) {
            return g_servers[i];
        }
    }
    return NULL;
}

static sal_l2cap_server_t* alloc_server(void)
{
    for (int i = 0; i < L2CAP_MAX_SERVERS; i++) {
        if (!g_servers[i]) {
            g_servers[i] = calloc(1, sizeof(sal_l2cap_server_t));
            if (!g_servers[i]) {
                return NULL;
            }
            g_servers[i]->in_use = true;
            return g_servers[i];
        }
    }
    return NULL;
}

static void get_addr_from_conn(struct bt_conn* conn, bt_address_t* addr)
{
    const bt_addr_le_t* peer = bt_conn_get_dst(conn);
    memcpy(addr->addr, peer->a.val, sizeof(addr->addr));
}

static int l2cap_recv_cb(struct bt_l2cap_chan* chan, struct net_buf* buf)
{
    sal_l2cap_channel_t* ch = find_channel_by_le_chan(BT_L2CAP_LE_CHAN(chan));
    if (!ch) {
        return -ENOENT;
    }

    l2cap_on_packet_received(&ch->addr, ch->local_cid, buf->data, buf->len);
    return 0;
}

static void l2cap_connected_cb(struct bt_l2cap_chan* chan)
{
    struct bt_l2cap_le_chan* le_chan = BT_L2CAP_LE_CHAN(chan);
    sal_l2cap_channel_t* ch = find_channel_by_le_chan(le_chan);
    if (!ch) {
        return;
    }

    ch->local_cid = le_chan->tx.cid;
    get_addr_from_conn(chan->conn, &ch->addr);

    l2cap_channel_param_t param = {
        .local_cid = le_chan->tx.cid,
        .remote_cid = le_chan->rx.cid,
        .psm = ch->psm,
        .is_client = !ch->is_server,
        .transport = BT_TRANSPORT_BLE,
        .incoming = {
            .mtu = le_chan->rx.mtu,
            .le_mps = le_chan->rx.mps,
            .credits = atomic_get(&le_chan->rx.credits),
        },
        .outgoing = {
            .mtu = le_chan->tx.mtu,
            .le_mps = le_chan->tx.mps,
            .credits = atomic_get(&le_chan->tx.credits),
        },
    };

    l2cap_on_channel_connected(&ch->addr, &param);
}

static void l2cap_disconnected_cb(struct bt_l2cap_chan* chan)
{
    sal_l2cap_channel_t* ch = find_channel_by_le_chan(BT_L2CAP_LE_CHAN(chan));
    if (!ch) {
        return;
    }

    l2cap_on_channel_disconnected(&ch->addr, ch->local_cid, 0);
    free_channel(ch);
}

static void l2cap_sent_cb(struct bt_l2cap_chan* chan)
{
    sal_l2cap_channel_t* ch = find_channel_by_le_chan(BT_L2CAP_LE_CHAN(chan));
    if (!ch) {
        return;
    }

    l2cap_on_packet_sent(&ch->addr, ch->local_cid);
}

static const struct bt_l2cap_chan_ops l2cap_chan_ops = {
    .recv = l2cap_recv_cb,
    .connected = l2cap_connected_cb,
    .disconnected = l2cap_disconnected_cb,
    .sent = l2cap_sent_cb,
};

static int l2cap_accept_cb(struct bt_conn* conn, struct bt_l2cap_server* server,
    struct bt_l2cap_chan** chan)
{
    sal_l2cap_channel_t* ch = alloc_channel();
    if (!ch) {
        return -ENOMEM;
    }

    sal_l2cap_server_t* srv = find_server_by_psm(server->psm);

    ch->is_server = true;
    ch->psm = server->psm;
    get_addr_from_conn(conn, &ch->addr);

    ch->le_chan.chan.ops = &l2cap_chan_ops;
    if (srv) {
        ch->le_chan.rx.mtu = srv->mtu;
        ch->le_chan.rx.mps = srv->mps;
    }

    *chan = &ch->le_chan.chan;

    return 0;
}

bt_status_t bt_sal_l2cap_init(void)
{
    memset(g_channels, 0, sizeof(g_channels));
    memset(g_servers, 0, sizeof(g_servers));
    return BT_STATUS_SUCCESS;
}

void bt_sal_l2cap_cleanup(void)
{
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (g_channels[i]) {
            bt_l2cap_chan_disconnect(&g_channels[i]->le_chan.chan);
            free(g_channels[i]);
            g_channels[i] = NULL;
        }
    }
    for (int i = 0; i < L2CAP_MAX_SERVERS; i++) {
        if (g_servers[i]) {
            free(g_servers[i]);
            g_servers[i] = NULL;
        }
    }
}

bt_status_t bt_sal_l2cap_listen_channel(l2cap_config_option_t* option)
{
    if (!option) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_l2cap_server_t* srv = find_server_by_psm(option->psm);
    if (srv) {
        /* Server already registered in zblue, reuse it */
        srv->mtu = option->mtu;
        srv->mps = option->le_mps;
        srv->credits = option->init_credits;
        return BT_STATUS_SUCCESS;
    }

    srv = alloc_server();
    if (!srv) {
        return BT_STATUS_NOMEM;
    }

    srv->psm = option->psm;
    srv->mtu = option->mtu;
    srv->mps = option->le_mps;
    srv->credits = option->init_credits;
    srv->server.psm = option->psm;
    srv->server.accept = l2cap_accept_cb;

    int err = bt_l2cap_server_register(&srv->server);
    if (err) {
        for (int i = 0; i < L2CAP_MAX_SERVERS; i++) {
            if (g_servers[i] == srv) {
                free(g_servers[i]);
                g_servers[i] = NULL;
                break;
            }
        }
        BT_LOGE("L2CAP server register failed: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_stop_listen_channel(uint16_t psm)
{
    sal_l2cap_server_t* srv = find_server_by_psm(psm);
    if (!srv) {
        return BT_STATUS_NOT_FOUND;
    }

    /* Keep the server struct allocated since zblue has no unregister API.
     * Mark as inactive so it can be reused on re-listen. */
    srv->in_use = false;
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_connect_channel(bt_address_t* addr, l2cap_config_option_t* option)
{
    if (!addr || !option) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_l2cap_channel_t* ch = alloc_channel();
    if (!ch) {
        return BT_STATUS_NOMEM;
    }

    ch->is_server = false;
    ch->psm = option->psm;
    ch->id = option->id;
    memcpy(&ch->addr, addr, sizeof(bt_address_t));

    ch->le_chan.chan.ops = &l2cap_chan_ops;
    ch->le_chan.rx.mtu = option->mtu;
    ch->le_chan.rx.mps = option->le_mps;

    bt_addr_le_t le_addr;
    le_addr.type = BT_ADDR_LE_PUBLIC;
    memcpy(le_addr.a.val, addr->addr, sizeof(le_addr.a.val));

    struct bt_conn* conn = bt_conn_lookup_addr_le(BT_ID_DEFAULT, &le_addr);
    if (!conn) {
        free_channel(ch);
        return BT_STATUS_NOT_FOUND;
    }

    int err = bt_l2cap_chan_connect(conn, &ch->le_chan.chan, option->psm);
    bt_conn_unref(conn);
    if (err) {
        free_channel(ch);
        BT_LOGE("L2CAP connect failed: %d", err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_disconnect_channel(uint16_t local_cid)
{
    sal_l2cap_channel_t* ch = find_channel_by_cid(local_cid);
    if (!ch) {
        return BT_STATUS_NOT_FOUND;
    }

    int err = bt_l2cap_chan_disconnect(&ch->le_chan.chan);
    if (err) {
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_send_packet(uint16_t local_cid, const uint8_t* data, uint16_t size)
{
    sal_l2cap_channel_t* ch = find_channel_by_cid(local_cid);
    if (!ch) {
        return BT_STATUS_NOT_FOUND;
    }

    struct net_buf* buf = net_buf_alloc_len(&l2cap_tx_pool, size, K_NO_WAIT);
    if (!buf) {
        return BT_STATUS_NOMEM;
    }

    net_buf_add_mem(buf, data, size);

    int err = bt_l2cap_chan_send(&ch->le_chan.chan, buf);
    if (err < 0) {
        net_buf_unref(buf);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_give_incoming_credits(bt_address_t* addr, uint16_t local_cid, uint16_t credits)
{
    sal_l2cap_channel_t* ch = find_channel_by_cid(local_cid);
    if (!ch) {
        return BT_STATUS_NOT_FOUND;
    }

    /* Credits are managed automatically by the zblue stack when using
     * the standard recv callback. Manual credit management via
     * bt_l2cap_chan_give_credits is only needed with seg_recv. */
    return BT_STATUS_SUCCESS;
}
