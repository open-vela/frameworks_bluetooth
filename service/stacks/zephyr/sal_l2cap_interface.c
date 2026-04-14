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
#define LOG_TAG "sal_l2cap"

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/net/buf.h>

#include "bt_addr.h"
#include "l2cap_service.h"
#include "sal_interface.h"
#include "sal_l2cap_interface.h"
#include "sal_zblue.h"
#include "sal_zephyr_interface.h"
#include "service_loop.h"
#include "utils/log.h"

/* Forward declaration - defined in zblue l2cap_br.c */
extern int bt_l2cap_br_send_echo_req(struct bt_conn *conn);
extern int bt_l2cap_br_send_conf_req(struct bt_conn *conn, uint16_t cid);
extern int bt_l2cap_br_send_data(struct bt_conn *conn, uint16_t cid,
    const uint8_t *data, uint16_t len);

#ifdef CONFIG_BLUETOOTH_L2CAP

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define L2CAP_MAX_SERVERS 8
#define L2CAP_MAX_CHANNELS 20
#define L2CAP_DEFAULT_CREDITS 10
#define L2CAP_TX_BUF_COUNT 10
#define L2CAP_TX_MTU CONFIG_BT_L2CAP_TX_MTU

#ifdef CONFIG_BLUETOOTH_PTS_TEST
#define L2CAP_BR_MAX_SERVERS 4
#define L2CAP_BR_DEFAULT_MTU 672
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    struct bt_l2cap_server server;
    l2cap_config_option_t config;
    bool in_use;
} sal_l2cap_server_t;

typedef struct {
    struct bt_l2cap_le_chan le_chan;
    bt_address_t addr;
    uint16_t psm;
    bool is_server;
    bool in_use;
    sal_l2cap_server_t* server; /* back reference for server-accepted channels */
} sal_l2cap_channel_t;

#ifdef CONFIG_BLUETOOTH_PTS_TEST
typedef struct {
    struct bt_l2cap_server server;
    l2cap_config_option_t config;
    bool in_use;
} sal_l2cap_br_server_t;

typedef struct {
    struct bt_l2cap_br_chan br_chan;
    bt_address_t addr;
    uint16_t psm;
    bool is_server;
    sal_l2cap_br_server_t* server;
    struct list_node node;
} sal_l2cap_br_channel_t;
#endif

#ifdef CONFIG_BLUETOOTH_PTS_TEST
typedef struct {
    bt_address_t addr;
    uint16_t psm;
    uint16_t mtu;
} sal_l2cap_br_connect_param_t;
#endif

typedef struct {
    sal_l2cap_server_t servers[L2CAP_MAX_SERVERS];
    sal_l2cap_channel_t channels[L2CAP_MAX_CHANNELS];
#ifdef CONFIG_BLUETOOTH_PTS_TEST
    sal_l2cap_br_server_t br_servers[L2CAP_BR_MAX_SERVERS];
    struct list_node br_channel_list;
#endif
    pthread_mutex_t lock;
} sal_l2cap_manager_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

NET_BUF_POOL_FIXED_DEFINE(l2cap_tx_pool, L2CAP_TX_BUF_COUNT,
    BT_L2CAP_SDU_BUF_SIZE(L2CAP_TX_MTU),
    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static sal_l2cap_manager_t g_l2cap_mgr = {
#ifdef CONFIG_BLUETOOTH_PTS_TEST
    .br_channel_list = LIST_INITIAL_VALUE(g_l2cap_mgr.br_channel_list),
#endif
    .lock = PTHREAD_MUTEX_INITIALIZER,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void sal_l2cap_lock(void)
{
    pthread_mutex_lock(&g_l2cap_mgr.lock);
}

static inline void sal_l2cap_unlock(void)
{
    pthread_mutex_unlock(&g_l2cap_mgr.lock);
}

static sal_l2cap_server_t* find_server_by_psm(uint16_t psm)
{
    for (int i = 0; i < L2CAP_MAX_SERVERS; i++) {
        if (g_l2cap_mgr.servers[i].in_use && g_l2cap_mgr.servers[i].server.psm == psm) {
            return &g_l2cap_mgr.servers[i];
        }
    }
    return NULL;
}

static sal_l2cap_server_t* alloc_server(void)
{
    for (int i = 0; i < L2CAP_MAX_SERVERS; i++) {
        if (!g_l2cap_mgr.servers[i].in_use) {
            memset(&g_l2cap_mgr.servers[i], 0, sizeof(sal_l2cap_server_t));
            g_l2cap_mgr.servers[i].in_use = true;
            return &g_l2cap_mgr.servers[i];
        }
    }
    return NULL;
}

static sal_l2cap_channel_t* find_channel_by_le_chan(struct bt_l2cap_le_chan* le_chan)
{
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (g_l2cap_mgr.channels[i].in_use && &g_l2cap_mgr.channels[i].le_chan == le_chan) {
            return &g_l2cap_mgr.channels[i];
        }
    }
    return NULL;
}

static sal_l2cap_channel_t* find_channel_by_cid(uint16_t cid)
{
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (g_l2cap_mgr.channels[i].in_use && g_l2cap_mgr.channels[i].le_chan.rx.cid == cid) {
            return &g_l2cap_mgr.channels[i];
        }
    }
    return NULL;
}

static sal_l2cap_channel_t* alloc_channel(void)
{
    for (int i = 0; i < L2CAP_MAX_CHANNELS; i++) {
        if (!g_l2cap_mgr.channels[i].in_use) {
            memset(&g_l2cap_mgr.channels[i], 0, sizeof(sal_l2cap_channel_t));
            g_l2cap_mgr.channels[i].in_use = true;
            return &g_l2cap_mgr.channels[i];
        }
    }
    return NULL;
}

static void free_channel(sal_l2cap_channel_t* ch)
{
    if (ch) {
        ch->in_use = false;
    }
}

/****************************************************************************
 * zblue L2CAP Callbacks (LE)
 ****************************************************************************/

static void sal_l2cap_connected_cb(struct bt_l2cap_chan* chan)
{
    struct bt_l2cap_le_chan* le_chan = BT_L2CAP_LE_CHAN(chan);
    sal_l2cap_channel_t* sal_ch;
    bt_address_t addr;
    l2cap_channel_param_t param;

    if (bt_sal_get_remote_address(chan->conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get remote address failed", __func__);
        return;
    }

    sal_l2cap_lock();
    sal_ch = find_channel_by_le_chan(le_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, channel not found", __func__);
        return;
    }

    memcpy(&sal_ch->addr, &addr, sizeof(bt_address_t));

    memset(&param, 0, sizeof(param));
    param.local_cid = le_chan->rx.cid;
    param.remote_cid = le_chan->tx.cid;
    param.psm = sal_ch->psm;
    param.is_client = !sal_ch->is_server;
    param.transport = BT_TRANSPORT_BLE;
    param.incoming.mtu = le_chan->rx.mtu;
    param.incoming.le_mps = le_chan->rx.mps;
    param.incoming.credits = (uint16_t)atomic_get(&le_chan->rx.credits);
    param.outgoing.mtu = le_chan->tx.mtu;
    param.outgoing.le_mps = le_chan->tx.mps;
    param.outgoing.credits = (uint16_t)atomic_get(&le_chan->tx.credits);

    sal_l2cap_unlock();

    l2cap_on_channel_connected(&addr, &param);
}

static void sal_l2cap_disconnected_cb(struct bt_l2cap_chan* chan)
{
    struct bt_l2cap_le_chan* le_chan = BT_L2CAP_LE_CHAN(chan);
    sal_l2cap_channel_t* sal_ch;
    bt_address_t addr;
    uint16_t cid;

    sal_l2cap_lock();
    sal_ch = find_channel_by_le_chan(le_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, channel not found", __func__);
        return;
    }

    memcpy(&addr, &sal_ch->addr, sizeof(bt_address_t));
    cid = le_chan->rx.cid;
    free_channel(sal_ch);
    sal_l2cap_unlock();

    l2cap_on_channel_disconnected(&addr, cid, 0);
}

static int sal_l2cap_recv_cb(struct bt_l2cap_chan* chan, struct net_buf* buf)
{
    struct bt_l2cap_le_chan* le_chan = BT_L2CAP_LE_CHAN(chan);
    sal_l2cap_channel_t* sal_ch;
    bt_address_t addr;

    sal_l2cap_lock();
    sal_ch = find_channel_by_le_chan(le_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, channel not found", __func__);
        return -ENOENT;
    }
    memcpy(&addr, &sal_ch->addr, sizeof(bt_address_t));
    sal_l2cap_unlock();

    l2cap_on_packet_received(&addr, le_chan->rx.cid, buf->data, buf->len);
    return 0;
}

static void sal_l2cap_sent_cb(struct bt_l2cap_chan* chan)
{
    struct bt_l2cap_le_chan* le_chan = BT_L2CAP_LE_CHAN(chan);
    sal_l2cap_channel_t* sal_ch;
    bt_address_t addr;

    sal_l2cap_lock();
    sal_ch = find_channel_by_le_chan(le_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        return;
    }
    memcpy(&addr, &sal_ch->addr, sizeof(bt_address_t));
    sal_l2cap_unlock();

    l2cap_on_packet_sent(&addr, le_chan->rx.cid);
}

static struct net_buf* sal_l2cap_alloc_buf_cb(struct bt_l2cap_chan* chan)
{
    return net_buf_alloc(&l2cap_tx_pool, K_NO_WAIT);
}

static const struct bt_l2cap_chan_ops g_l2cap_chan_ops = {
    .connected = sal_l2cap_connected_cb,
    .disconnected = sal_l2cap_disconnected_cb,
    .recv = sal_l2cap_recv_cb,
    .sent = sal_l2cap_sent_cb,
    .alloc_buf = sal_l2cap_alloc_buf_cb,
};

static int sal_l2cap_server_accept_cb(struct bt_conn* conn,
    struct bt_l2cap_server* server, struct bt_l2cap_chan** chan)
{
    bt_address_t addr;
    sal_l2cap_server_t* sal_srv;
    sal_l2cap_channel_t* sal_ch;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get remote address failed", __func__);
        return -ENXIO;
    }

    sal_l2cap_lock();
    sal_srv = CONTAINER_OF(server, sal_l2cap_server_t, server);

    sal_ch = alloc_channel();
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, no free channel", __func__);
        return -ENOMEM;
    }

    sal_ch->psm = server->psm;
    sal_ch->is_server = true;
    sal_ch->server = sal_srv;
    memcpy(&sal_ch->addr, &addr, sizeof(bt_address_t));

    sal_ch->le_chan.chan.ops = &g_l2cap_chan_ops;
    sal_ch->le_chan.rx.mtu = sal_srv->config.mtu;
    if (sal_srv->config.le_mps) {
        sal_ch->le_chan.rx.mps = sal_srv->config.le_mps;
    }

    *chan = &sal_ch->le_chan.chan;
    sal_l2cap_unlock();

    l2cap_on_cid_allocated(&addr, server->psm, 0);

    return 0;
}

#ifdef CONFIG_BLUETOOTH_PTS_TEST

/****************************************************************************
 * BR L2CAP Private Functions
 ****************************************************************************/

static sal_l2cap_br_server_t* find_br_server_by_psm(uint16_t psm)
{
    int i;

    for (i = 0; i < L2CAP_BR_MAX_SERVERS; i++) {
        if (g_l2cap_mgr.br_servers[i].in_use &&
            g_l2cap_mgr.br_servers[i].server.psm == psm) {
            return &g_l2cap_mgr.br_servers[i];
        }
    }

    return NULL;
}

static sal_l2cap_br_server_t* alloc_br_server(void)
{
    int i;

    for (i = 0; i < L2CAP_BR_MAX_SERVERS; i++) {
        if (!g_l2cap_mgr.br_servers[i].in_use) {
            memset(&g_l2cap_mgr.br_servers[i], 0,
                   sizeof(sal_l2cap_br_server_t));
            g_l2cap_mgr.br_servers[i].in_use = true;
            return &g_l2cap_mgr.br_servers[i];
        }
    }

    return NULL;
}

static sal_l2cap_br_channel_t* find_br_channel_by_chan(
    struct bt_l2cap_br_chan* br_chan)
{
    struct list_node* node;
    sal_l2cap_br_channel_t* ch;

    list_for_every(&g_l2cap_mgr.br_channel_list, node) {
        ch = CONTAINER_OF(node, sal_l2cap_br_channel_t, node);
        if (&ch->br_chan == br_chan) {
            return ch;
        }
    }

    return NULL;
}

static sal_l2cap_br_channel_t* find_br_channel_by_cid(uint16_t cid)
{
    struct list_node* node;
    sal_l2cap_br_channel_t* ch;

    list_for_every(&g_l2cap_mgr.br_channel_list, node) {
        ch = CONTAINER_OF(node, sal_l2cap_br_channel_t, node);
        if (ch->br_chan.rx.cid == cid) {
            return ch;
        }
    }

    return NULL;
}

static sal_l2cap_br_channel_t* alloc_br_channel(void)
{
    sal_l2cap_br_channel_t* ch;

    ch = zalloc(sizeof(sal_l2cap_br_channel_t));
    if (ch) {
        list_add_tail(&g_l2cap_mgr.br_channel_list, &ch->node);
    }

    return ch;
}

static void free_br_channel(sal_l2cap_br_channel_t* ch)
{
    if (ch) {
        list_delete(&ch->node);
        free(ch);
    }
}

/****************************************************************************
 * BR L2CAP zblue Callbacks
 ****************************************************************************/

static void sal_l2cap_br_connected_cb(struct bt_l2cap_chan* chan)
{
    struct bt_l2cap_br_chan* br_chan = BT_L2CAP_BR_CHAN(chan);
    sal_l2cap_br_channel_t* sal_ch;
    bt_address_t addr;
    l2cap_channel_param_t param;

    if (bt_sal_get_remote_address(chan->conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get remote address failed", __func__);
        return;
    }

    sal_l2cap_lock();
    sal_ch = find_br_channel_by_chan(br_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, channel not found", __func__);
        return;
    }

    memcpy(&sal_ch->addr, &addr, sizeof(bt_address_t));

    memset(&param, 0, sizeof(param));
    param.local_cid = br_chan->rx.cid;
    param.remote_cid = br_chan->tx.cid;
    param.psm = sal_ch->psm;
    param.is_client = !sal_ch->is_server;
    param.transport = BT_TRANSPORT_BREDR;
    param.incoming.mtu = br_chan->rx.mtu;
    param.outgoing.mtu = br_chan->tx.mtu;

    sal_l2cap_unlock();

    l2cap_on_channel_connected(&addr, &param);
}

static void sal_l2cap_br_disconnected_cb(struct bt_l2cap_chan* chan)
{
    struct bt_l2cap_br_chan* br_chan = BT_L2CAP_BR_CHAN(chan);
    sal_l2cap_br_channel_t* sal_ch;
    bt_address_t addr;
    uint16_t cid;

    sal_l2cap_lock();
    sal_ch = find_br_channel_by_chan(br_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, channel not found", __func__);
        return;
    }

    memcpy(&addr, &sal_ch->addr, sizeof(bt_address_t));
    cid = br_chan->rx.cid;
    free_br_channel(sal_ch);
    sal_l2cap_unlock();

    l2cap_on_channel_disconnected(&addr, cid, 0);
}

static int sal_l2cap_br_recv_cb(struct bt_l2cap_chan* chan,
                                struct net_buf* buf)
{
    struct bt_l2cap_br_chan* br_chan = BT_L2CAP_BR_CHAN(chan);
    sal_l2cap_br_channel_t* sal_ch;
    bt_address_t addr;

    sal_l2cap_lock();
    sal_ch = find_br_channel_by_chan(br_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, channel not found", __func__);
        return -ENOENT;
    }

    memcpy(&addr, &sal_ch->addr, sizeof(bt_address_t));
    sal_l2cap_unlock();

    l2cap_on_packet_received(&addr, br_chan->rx.cid, buf->data, buf->len);
    return 0;
}

static void sal_l2cap_br_sent_cb(struct bt_l2cap_chan* chan)
{
    struct bt_l2cap_br_chan* br_chan = BT_L2CAP_BR_CHAN(chan);
    sal_l2cap_br_channel_t* sal_ch;
    bt_address_t addr;

    sal_l2cap_lock();
    sal_ch = find_br_channel_by_chan(br_chan);
    if (!sal_ch) {
        sal_l2cap_unlock();
        return;
    }

    memcpy(&addr, &sal_ch->addr, sizeof(bt_address_t));
    sal_l2cap_unlock();

    l2cap_on_packet_sent(&addr, br_chan->rx.cid);
}

static struct net_buf* sal_l2cap_br_alloc_buf_cb(struct bt_l2cap_chan* chan)
{
    return net_buf_alloc(&l2cap_tx_pool, K_NO_WAIT);
}

static const struct bt_l2cap_chan_ops g_l2cap_br_chan_ops = {
    .connected = sal_l2cap_br_connected_cb,
    .disconnected = sal_l2cap_br_disconnected_cb,
    .recv = sal_l2cap_br_recv_cb,
    .sent = sal_l2cap_br_sent_cb,
    .alloc_buf = sal_l2cap_br_alloc_buf_cb,
};

static int sal_l2cap_br_server_accept_cb(struct bt_conn* conn,
    struct bt_l2cap_server* server, struct bt_l2cap_chan** chan)
{
    bt_address_t addr;
    sal_l2cap_br_server_t* sal_srv;
    sal_l2cap_br_channel_t* sal_ch;

    if (bt_sal_get_remote_address(conn, &addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get remote address failed", __func__);
        return -ENXIO;
    }

    sal_l2cap_lock();
    sal_srv = CONTAINER_OF(server, sal_l2cap_br_server_t, server);

    sal_ch = alloc_br_channel();
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, no free channel", __func__);
        return -ENOMEM;
    }

    sal_ch->psm = server->psm;
    sal_ch->is_server = true;
    sal_ch->server = sal_srv;
    memcpy(&sal_ch->addr, &addr, sizeof(bt_address_t));

    sal_ch->br_chan.chan.ops = &g_l2cap_br_chan_ops;
    sal_ch->br_chan.rx.mtu = sal_srv->config.mtu ?
        sal_srv->config.mtu : L2CAP_BR_DEFAULT_MTU;

    *chan = &sal_ch->br_chan.chan;
    sal_l2cap_unlock();

    l2cap_on_cid_allocated(&addr, server->psm, 0);

    return 0;
}

#endif /* CONFIG_BLUETOOTH_PTS_TEST */

/****************************************************************************
 * Public Functions - SAL L2CAP Interface
 ****************************************************************************/

bt_status_t bt_sal_l2cap_listen_channel(l2cap_config_option_t* option)
{
    sal_l2cap_server_t* sal_srv;
    int err;

    SAL_CHECK_PARAM(option);

    if (option->transport == BT_TRANSPORT_BLE) {
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        sal_l2cap_lock();
        sal_srv = find_server_by_psm(option->psm);
        if (sal_srv) {
            sal_l2cap_unlock();
            BT_LOGE("%s, psm 0x%x already registered", __func__, option->psm);
            return BT_STATUS_BUSY;
        }

        sal_srv = alloc_server();
        if (!sal_srv) {
            sal_l2cap_unlock();
            BT_LOGE("%s, no free server slot", __func__);
            return BT_STATUS_NOMEM;
        }

        memcpy(&sal_srv->config, option, sizeof(l2cap_config_option_t));
        sal_srv->server.psm = option->psm;
        sal_srv->server.accept = sal_l2cap_server_accept_cb;

        err = bt_l2cap_server_register(&sal_srv->server);
        if (err) {
            sal_srv->in_use = false;
            sal_l2cap_unlock();
            BT_LOGE("%s, register server failed: %d", __func__, err);
            return BT_STATUS_FAIL;
        }

        /* psm may be dynamically allocated by zblue */
        option->psm = sal_srv->server.psm;
        sal_srv->config.psm = sal_srv->server.psm;
        sal_l2cap_unlock();
#else
        return BT_STATUS_NOT_SUPPORTED;
#endif
    } else if (option->transport == BT_TRANSPORT_BREDR) {
#ifdef CONFIG_BLUETOOTH_PTS_TEST
        sal_l2cap_br_server_t* br_srv;

        sal_l2cap_lock();
        br_srv = find_br_server_by_psm(option->psm);
        if (br_srv) {
            sal_l2cap_unlock();
            BT_LOGE("%s, BR psm 0x%x already registered",
                     __func__, option->psm);
            return BT_STATUS_BUSY;
        }

        br_srv = alloc_br_server();
        if (!br_srv) {
            sal_l2cap_unlock();
            BT_LOGE("%s, no free BR server slot", __func__);
            return BT_STATUS_NOMEM;
        }

        memcpy(&br_srv->config, option, sizeof(l2cap_config_option_t));
        br_srv->server.psm = option->psm;
        br_srv->server.accept = sal_l2cap_br_server_accept_cb;

        err = bt_l2cap_br_server_register(&br_srv->server);
        if (err) {
            br_srv->in_use = false;
            sal_l2cap_unlock();
            BT_LOGE("%s, BR register server failed: %d",
                     __func__, err);
            return BT_STATUS_FAIL;
        }

        option->psm = br_srv->server.psm;
        br_srv->config.psm = br_srv->server.psm;
        sal_l2cap_unlock();
#else
        return BT_STATUS_NOT_SUPPORTED;
#endif
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_stop_listen_channel(uint16_t psm)
{
    sal_l2cap_server_t* sal_srv;

    sal_l2cap_lock();
    sal_srv = find_server_by_psm(psm);
    if (!sal_srv) {
        sal_l2cap_unlock();
        BT_LOGE("%s, server psm 0x%x not found", __func__, psm);
        return BT_STATUS_NOT_FOUND;
    }

    /* TODO: zblue does not provide bt_l2cap_server_unregister yet,
     * mark server as not in use to prevent new accepts */
    sal_srv->in_use = false;
    sal_l2cap_unlock();

    return BT_STATUS_SUCCESS;
}

#ifdef CONFIG_BLUETOOTH_PTS_TEST
static void do_br_l2cap_connect(service_work_t* work, void* userdata)
{
    sal_l2cap_br_connect_param_t* params = userdata;
    struct bt_conn* conn;
    sal_l2cap_br_channel_t* br_ch;
    int err;

    conn = bt_conn_lookup_addr_br((bt_addr_t*)&params->addr);
    if (!conn) {
        BT_LOGE("%s, no BR connection for addr", __func__);
        free(params);
        return;
    }

    sal_l2cap_lock();
    br_ch = alloc_br_channel();
    if (!br_ch) {
        bt_conn_unref(conn);
        sal_l2cap_unlock();
        BT_LOGE("%s, no free BR channel", __func__);
        free(params);
        return;
    }

    br_ch->psm = params->psm;
    br_ch->is_server = false;
    memcpy(&br_ch->addr, &params->addr, sizeof(bt_address_t));

    br_ch->br_chan.chan.ops = &g_l2cap_br_chan_ops;
    br_ch->br_chan.rx.mtu = params->mtu ?
        params->mtu : L2CAP_BR_DEFAULT_MTU;

    err = bt_l2cap_chan_connect(conn, &br_ch->br_chan.chan,
                               params->psm);
    bt_conn_unref(conn);
    if (err) {
        free_br_channel(br_ch);
        sal_l2cap_unlock();
        BT_LOGE("%s, BR connect failed: %d", __func__, err);
        free(params);
        return;
    }

    l2cap_on_cid_allocated(&params->addr, params->psm,
                           br_ch->br_chan.rx.cid);
    sal_l2cap_unlock();
    free(params);
}
#endif

bt_status_t bt_sal_l2cap_connect_channel(bt_address_t* addr, l2cap_config_option_t* option)
{
    struct bt_conn* conn;
    sal_l2cap_channel_t* sal_ch;
    int err;

    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(option);

    if (option->transport == BT_TRANSPORT_BLE) {
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        conn = get_le_conn_from_addr(addr);
        if (!conn) {
            BT_LOGE("%s, no LE connection for addr", __func__);
            return BT_STATUS_NOT_FOUND;
        }

        sal_l2cap_lock();
        sal_ch = alloc_channel();
        if (!sal_ch) {
            sal_l2cap_unlock();
            BT_LOGE("%s, no free channel", __func__);
            return BT_STATUS_NOMEM;
        }

        sal_ch->psm = option->psm;
        sal_ch->is_server = false;
        memcpy(&sal_ch->addr, addr, sizeof(bt_address_t));

        sal_ch->le_chan.chan.ops = &g_l2cap_chan_ops;
        sal_ch->le_chan.rx.mtu = option->mtu;
        if (option->le_mps) {
            sal_ch->le_chan.rx.mps = option->le_mps;
        }

        err = bt_l2cap_chan_connect(conn, &sal_ch->le_chan.chan, option->psm);
        if (err) {
            free_channel(sal_ch);
            sal_l2cap_unlock();
            BT_LOGE("%s, connect failed: %d", __func__, err);
            return BT_STATUS_FAIL;
        }

        /* notify cid allocated for client */
        l2cap_on_cid_allocated(addr, option->psm, sal_ch->le_chan.rx.cid);

        sal_l2cap_unlock();
#else
        return BT_STATUS_NOT_SUPPORTED;
#endif
    } else if (option->transport == BT_TRANSPORT_BREDR) {
#ifdef CONFIG_BLUETOOTH_PTS_TEST
        sal_l2cap_br_connect_param_t* params;

        params = malloc(sizeof(sal_l2cap_br_connect_param_t));
        if (!params) {
            BT_LOGE("%s, alloc BR connect params failed", __func__);
            return BT_STATUS_NOMEM;
        }

        memcpy(&params->addr, addr, sizeof(bt_address_t));
        params->psm = option->psm;
        params->mtu = option->mtu;

        if (!service_loop_work(params, do_br_l2cap_connect, NULL)) {
            BT_LOGE("%s, service_loop_work submit failed", __func__);
            free(params);
            return BT_STATUS_FAIL;
        }
#else
        return BT_STATUS_NOT_SUPPORTED;
#endif
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_disconnect_channel(uint16_t cid)
{
    sal_l2cap_channel_t* sal_ch;
    int err;

    sal_l2cap_lock();
    sal_ch = find_channel_by_cid(cid);
    if (sal_ch) {
        err = bt_l2cap_chan_disconnect(&sal_ch->le_chan.chan);
        sal_l2cap_unlock();
        if (err) {
            BT_LOGE("%s, LE disconnect failed: %d", __func__, err);
            return BT_STATUS_FAIL;
        }

        return BT_STATUS_SUCCESS;
    }

#ifdef CONFIG_BLUETOOTH_PTS_TEST
    {
        sal_l2cap_br_channel_t* br_ch;

        br_ch = find_br_channel_by_cid(cid);
        if (br_ch) {
            err = bt_l2cap_chan_disconnect(&br_ch->br_chan.chan);
            sal_l2cap_unlock();
            if (err) {
                BT_LOGE("%s, BR disconnect failed: %d",
                         __func__, err);
                return BT_STATUS_FAIL;
            }

            return BT_STATUS_SUCCESS;
        }
    }
#endif

    sal_l2cap_unlock();
    BT_LOGE("%s, channel cid 0x%x not found", __func__, cid);
    return BT_STATUS_NOT_FOUND;
}

bt_status_t bt_sal_l2cap_send_packet(uint16_t cid, uint8_t* packet_data,
    uint16_t packet_size)
{
    sal_l2cap_channel_t* sal_ch;
    struct net_buf* buf;
    int err;

    SAL_CHECK_PARAM(packet_data);

    sal_l2cap_lock();
    sal_ch = find_channel_by_cid(cid);
    if (sal_ch) {
        buf = net_buf_alloc(&l2cap_tx_pool, K_NO_WAIT);
        if (!buf) {
            sal_l2cap_unlock();
            BT_LOGE("%s, alloc tx buf failed", __func__);
            return BT_STATUS_NOMEM;
        }

        net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE);
        net_buf_add_mem(buf, packet_data, packet_size);

        err = bt_l2cap_chan_send(&sal_ch->le_chan.chan, buf);
        sal_l2cap_unlock();

        if (err < 0) {
            net_buf_unref(buf);
            BT_LOGE("%s, LE send failed: %d", __func__, err);
            return BT_STATUS_FAIL;
        }

        return BT_STATUS_SUCCESS;
    }

#ifdef CONFIG_BLUETOOTH_PTS_TEST
    {
        sal_l2cap_br_channel_t* br_ch;

        br_ch = find_br_channel_by_cid(cid);
        if (br_ch) {
            buf = net_buf_alloc(&l2cap_tx_pool, K_NO_WAIT);
            if (!buf) {
                sal_l2cap_unlock();
                BT_LOGE("%s, alloc tx buf failed", __func__);
                return BT_STATUS_NOMEM;
            }

            net_buf_reserve(buf, BT_L2CAP_CHAN_SEND_RESERVE);
            net_buf_add_mem(buf, packet_data, packet_size);

            err = bt_l2cap_chan_send(&br_ch->br_chan.chan, buf);
            sal_l2cap_unlock();

            if (err < 0) {
                net_buf_unref(buf);
                BT_LOGE("%s, BR send failed: %d", __func__, err);
                return BT_STATUS_FAIL;
            }

            return BT_STATUS_SUCCESS;
        }
    }
#endif

    sal_l2cap_unlock();
    BT_LOGE("%s, channel cid 0x%x not found", __func__, cid);
    return BT_STATUS_NOT_FOUND;
}

bt_status_t bt_sal_l2cap_give_incoming_credits(bt_address_t* addr, uint16_t cid, uint16_t credits)
{
#if defined(CONFIG_BT_L2CAP_SEG_RECV)
    sal_l2cap_channel_t* sal_ch;
    int err;

    sal_l2cap_lock();
    sal_ch = find_channel_by_cid(cid);
    if (!sal_ch) {
        sal_l2cap_unlock();
        BT_LOGE("%s, channel cid 0x%x not found", __func__, cid);
        return BT_STATUS_NOT_FOUND;
    }

    err = bt_l2cap_chan_give_credits(&sal_ch->le_chan.chan, credits);
    sal_l2cap_unlock();

    if (err) {
        BT_LOGE("%s, give credits failed: %d", __func__, err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
#else
    /* Credits are managed automatically by zblue when SEG_RECV is not enabled */
    return BT_STATUS_SUCCESS;
#endif
}

bt_status_t bt_sal_l2cap_send_echo_req(bt_address_t* addr)
{
    struct bt_conn* conn;
    int err;

    SAL_CHECK_PARAM(addr);

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    err = bt_l2cap_br_send_echo_req(conn);
    bt_conn_unref(conn);

    if (err) {
        BT_LOGE("%s, send echo req failed: %d", __func__, err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_send_conf_req(bt_address_t* addr, uint16_t cid)
{
    struct bt_conn* conn;
    int err;

    SAL_CHECK_PARAM(addr);

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    err = bt_l2cap_br_send_conf_req(conn, cid);
    bt_conn_unref(conn);

    if (err) {
        BT_LOGE("%s, send conf req failed: %d", __func__, err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_send_br_data(bt_address_t* addr, uint16_t cid,
    uint8_t* data, uint16_t len)
{
    struct bt_conn* conn;
    int err;

    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(data);

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    err = bt_l2cap_br_send_data(conn, cid, data, len);
    bt_conn_unref(conn);

    if (err) {
        BT_LOGE("%s, send data failed: %d", __func__, err);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_L2CAP */
