/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "L2CAP"
/****************************************************************************
 * Included Files
 ****************************************************************************/
// stdlib
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
// nuttx
#include <debug.h>
// libuv
#include "uv.h"

#include "adapter_internel.h"
#include "bluetooth.h"
#include "l2cap_service.h"
#include "sal_l2cap_interface.h"
#include "service_loop.h"

#include "euv_pty.h"
#include "openpty.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * \def Check adapter is enabled
 */
#define CHECK_ADAPTER_ENABLED(ret)    \
    do {                              \
        if (!adapter_is_le_enabled()) \
            return ret;               \
    } while (0)

/**
 * \def L2CAP_CBACK_FOREACH(_list, _cback) Description
 */
#define L2CAP_CBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, l2cap_callbacks_t, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    bt_address_t addr;
    bt_transport_t transport;
    uint16_t cid;
    uint16_t psm;
    l2cap_endpoint_param_t incoming;
    l2cap_endpoint_param_t outgoing;
    uint16_t tx_mtu;
    euv_pty_t* pty;
    int mfd;
    char pty_name[64];
} l2cap_channel_t;

typedef struct {
    callbacks_list_t* callbacks;
    bt_list_t* channel_list;
    pthread_mutex_t l2cap_lock;

} l2cap_manager_t;

typedef struct {
    enum {
        CHANNEL_CONNECTED_EVT,
        CHANNEL_DISCONNECTED_EVT,
        PACKET_RECEVIED_EVT,
        PACKET_SENT_EVT,
    } event;

    union {
        /**
         * @brief CHANNEL_CONNECTED_EVT
         */
        struct channel_connected_evt_param {
            bt_address_t addr;
            l2cap_channel_param_t param;
        } channel_connected;

        /**
         * @brief CHANNEL_DISCONNECTED_EVT
         */
        struct channel_disconnected_evt_param {
            bt_address_t addr;
            uint16_t cid;
            uint32_t reason;
        } channel_disconnected;

        /**
         * @brief PACKET_RECEVIED_EVT
         */
        struct packet_received_evt_param {
            bt_address_t addr;
            uint16_t cid;
            uint16_t size;
            uint8_t* data;
        } packet_received;

        /**
         * @brief PACKET_SENT_EVT
         */
        struct packet_sent_evt_param {
            bt_address_t addr;
            uint16_t cid;
        } packet_sent;
    };

} l2cap_msg_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static l2cap_manager_t g_l2cap_manager;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static l2cap_channel_t* find_l2cap_channel_by_cid(uint16_t cid)
{
    bt_list_node_t* node;
    bt_list_t* list = g_l2cap_manager.channel_list;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        l2cap_channel_t* channel = (l2cap_channel_t*)bt_list_node(node);
        if (channel->cid == cid) {
            return channel;
        }
    }

    return NULL;
}

static l2cap_channel_t* find_l2cap_channel_by_handle(euv_pty_t* handle)
{
    bt_list_node_t* node;
    bt_list_t* list = g_l2cap_manager.channel_list;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        l2cap_channel_t* channel = (l2cap_channel_t*)bt_list_node(node);
        if (channel->pty == handle) {
            return channel;
        }
    }

    return NULL;
}

static int l2cap_channel_pty_open(l2cap_channel_t* channel)
{
    int ret;

    ret = open_pty(&channel->mfd, channel->pty_name);
    if (ret != 0) {
        BT_LOGE("pty create failed");
        goto error;
    }

    channel->pty = euv_pty_init(get_service_uv_loop(), channel->mfd, UV_TTY_MODE_IO);
    if (!channel->pty) {
        ret = -1;
        goto error;
    }

    BT_LOGD("pty create success, name: %s, master: %d", channel->pty_name, channel->mfd);
    return 0;
error:
    close(channel->mfd);
    return ret;
}

static int l2cap_channel_pty_close(l2cap_channel_t* channel)
{
    if (channel->pty) {
        euv_pty_close(channel->pty);
        channel->pty = NULL;
        channel->mfd = -1;
    }

    return 0;
}

static void euv_read_complete(euv_pty_t* handle, const uint8_t* buf, ssize_t size)
{
    l2cap_channel_t* channel;

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);
    channel = find_l2cap_channel_by_handle(handle);
    if (!channel || !buf)
        goto exit;

    if (size < 0) {
        bt_sal_l2cap_disconnect_channel(channel->cid);
        goto exit;
    }

    bt_sal_l2cap_send_packet(channel->cid, (uint8_t*)buf, size);
exit:
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);
}

static void euv_write_complete(euv_pty_t* handle, uint8_t* buf, int status)
{
    free(buf);
}

static void handle_channel_conneted(bt_address_t* addr, l2cap_channel_param_t* param)
{
    l2cap_channel_t* channel;
    l2cap_connect_params_t conn_param;

    channel = find_l2cap_channel_by_cid(param->cid);
    if (channel) {
        BT_LOGW("L2CAP channel(cid:0x%x) already exists", channel->cid);
        return;
    }

    channel = calloc(1, sizeof(l2cap_channel_t));
    if (!channel) {
        return;
    }

    memcpy(&channel->addr, addr, sizeof(channel->addr));
    channel->transport = param->transport;
    channel->cid = param->cid;
    channel->psm = param->psm;
    memcpy(&channel->incoming, &param->incoming, sizeof(channel->incoming));
    memcpy(&channel->outgoing, &param->outgoing, sizeof(channel->outgoing));
    channel->tx_mtu = MIN(param->outgoing.mtu, CONFIG_BLUETOOTH_L2CAP_OUTGOING_MTU);
    bt_list_add_tail(g_l2cap_manager.channel_list, channel);

    if (l2cap_channel_pty_open(channel) != 0) {
        BT_LOGE("L2CAP channel(psm:0x%x/cid:0x%x) pty open failed!", channel->psm, channel->cid);
        bt_sal_l2cap_disconnect_channel(channel->cid);
        return;
    }

    int ret = euv_pty_read_start(channel->pty, channel->tx_mtu, euv_read_complete);
    if (ret != 0) {
        BT_LOGE("L2CAP channel(cid:0x%x) pty(%s) read start failed!", channel->cid, channel->pty_name);
        bt_sal_l2cap_disconnect_channel(channel->cid);
        return;
    }

    memcpy(&conn_param.addr, &channel->addr, sizeof(conn_param.addr));
    conn_param.transport = channel->transport;
    conn_param.cid = channel->cid;
    conn_param.psm = channel->psm;
    conn_param.incoming_mtu = channel->incoming.mtu;
    conn_param.outgoing_mtu = channel->outgoing.mtu;
    conn_param.pty_name = channel->pty_name;

    L2CAP_CBACK_FOREACH(g_l2cap_manager.callbacks, on_connected, &conn_param);
}

static void handle_channel_disconneted(bt_address_t* addr, uint16_t cid, uint32_t reason)
{
    l2cap_channel_t* channel;

    channel = find_l2cap_channel_by_cid(cid);
    if (channel) {
        l2cap_channel_pty_close(channel);
        bt_list_remove(g_l2cap_manager.channel_list, channel);
    }
    L2CAP_CBACK_FOREACH(g_l2cap_manager.callbacks, on_disconnected, addr, cid, reason);
}

static void handle_packet_received(bt_address_t* addr, uint16_t cid, uint8_t* packet_data, uint16_t packet_size)
{
    l2cap_channel_t* channel;

    channel = find_l2cap_channel_by_cid(cid);
    if (channel && channel->pty) {
        int ret = euv_pty_write(channel->pty, packet_data, packet_size, euv_write_complete);
        if (ret != 0) {
            BT_LOGE("L2CAP channel(cid:0x%x) pty(%s) write failed!", channel->cid, channel->pty_name);
            bt_sal_l2cap_disconnect_channel(channel->cid);
        }
    }
}

static void handle_packet_sent(bt_address_t* addr, uint16_t cid)
{
    l2cap_channel_t* channel;

    channel = find_l2cap_channel_by_cid(cid);
    if (channel && channel->pty) {
        int ret = euv_pty_read_start(channel->pty, channel->tx_mtu, euv_read_complete);
        if (ret != 0) {
            BT_LOGE("L2CAP channel(cid:0x%x) pty(%s) read start failed!", channel->cid, channel->pty_name);
            bt_sal_l2cap_disconnect_channel(channel->cid);
        }
    }
}

static void handle_l2cap_event(void* data)
{
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    if (!msg) {
        return;
    }

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);

    switch (msg->event) {
    case CHANNEL_CONNECTED_EVT:
        handle_channel_conneted(&msg->channel_connected.addr, &msg->channel_connected.param);
        break;
    case CHANNEL_DISCONNECTED_EVT:
        handle_channel_disconneted(&msg->channel_disconnected.addr,
            msg->channel_disconnected.cid,
            msg->channel_disconnected.reason);
        break;
    case PACKET_RECEVIED_EVT:
        handle_packet_received(&msg->packet_received.addr,
            msg->packet_received.cid,
            msg->packet_received.data,
            msg->packet_received.size);
        break;
    case PACKET_SENT_EVT:
        handle_packet_sent(&msg->packet_sent.addr,
            msg->packet_sent.cid);
        break;
    default:
        break;
    }

    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);
    free(msg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void l2cap_on_channel_connected(bt_address_t* addr, l2cap_channel_param_t* param)
{
    l2cap_msg_t* msg = malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        return;
    }

    msg->event = CHANNEL_CONNECTED_EVT;
    memcpy(&msg->channel_connected.addr, addr, sizeof(msg->channel_connected.addr));
    memcpy(&msg->channel_connected.param, param, sizeof(msg->channel_connected.param));
    do_in_service_loop(handle_l2cap_event, msg);
}

void l2cap_on_channel_disconnected(bt_address_t* addr, uint16_t cid, uint32_t reason)
{
    l2cap_msg_t* msg = malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        return;
    }

    msg->event = CHANNEL_DISCONNECTED_EVT;
    memcpy(&msg->channel_disconnected.addr, addr, sizeof(msg->channel_disconnected.addr));
    msg->channel_disconnected.cid = cid;
    msg->channel_disconnected.reason = reason;
    do_in_service_loop(handle_l2cap_event, msg);
}

void l2cap_on_packet_received(bt_address_t* addr, uint16_t cid, uint8_t* packet_data, uint16_t packet_size)
{
    l2cap_msg_t* msg = malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        return;
    }

    msg->packet_received.data = malloc(packet_size);
    if (!msg->packet_received.data) {
        free(msg);
        return;
    }

    msg->event = PACKET_RECEVIED_EVT;
    memcpy(&msg->packet_received.addr, addr, sizeof(msg->packet_received.addr));
    msg->packet_received.cid = cid;
    msg->packet_received.size = packet_size;
    memcpy(msg->packet_received.data, packet_data, packet_size);
    do_in_service_loop(handle_l2cap_event, msg);
}

void l2cap_on_packet_sent(bt_address_t* addr, uint16_t cid)
{
    l2cap_msg_t* msg = malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        return;
    }

    msg->event = PACKET_SENT_EVT;
    memcpy(&msg->packet_sent.addr, addr, sizeof(msg->packet_sent.addr));
    msg->packet_sent.cid = cid;
    do_in_service_loop(handle_l2cap_event, msg);
}

void* l2cap_register_callbacks(void* remote, const l2cap_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_l2cap_manager.callbacks, remote, (void*)callbacks);
}

bool l2cap_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_l2cap_manager.callbacks, remote, (remote_callback_t*)cookie);
}

bt_status_t l2cap_listen_channel(l2cap_config_option_t* option)
{
    if (!option) {
        return BT_STATUS_PARM_INVALID;
    }

    CHECK_ADAPTER_ENABLED(BT_STATUS_NOT_ENABLED);

    return bt_sal_l2cap_listen_channel(option);
}

bt_status_t l2cap_connect_channel(bt_address_t* addr, l2cap_config_option_t* option)
{
    if ((!addr) || (!option)) {
        return BT_STATUS_PARM_INVALID;
    }

    CHECK_ADAPTER_ENABLED(BT_STATUS_NOT_ENABLED);

    return bt_sal_l2cap_connect_channel(addr, option);
}

bt_status_t l2cap_disconnect_channel(uint16_t cid)
{
    bt_status_t status;

    CHECK_ADAPTER_ENABLED(BT_STATUS_NOT_ENABLED);

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);
    if (!find_l2cap_channel_by_cid(cid)) {
        status = BT_STATUS_NOT_FOUND;
        goto exit;
    }

    status = bt_sal_l2cap_disconnect_channel(cid);

exit:
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);
    return status;
}

bt_status_t l2cap_service_init(void)
{
    pthread_mutexattr_t attr;

    memset(&g_l2cap_manager, 0, sizeof(g_l2cap_manager));

    g_l2cap_manager.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    if (!g_l2cap_manager.callbacks) {
        return BT_STATUS_NOMEM;
    }

    g_l2cap_manager.channel_list = bt_list_new(free);
    if (!g_l2cap_manager.channel_list) {
        bt_callbacks_list_free(g_l2cap_manager.callbacks);
        return BT_STATUS_NOMEM;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_l2cap_manager.l2cap_lock, &attr);

    return BT_STATUS_SUCCESS;
}

void l2cap_service_cleanup(void)
{
    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);

    bt_callbacks_list_free(g_l2cap_manager.callbacks);
    g_l2cap_manager.callbacks = NULL;
    bt_list_free(g_l2cap_manager.channel_list);
    g_l2cap_manager.channel_list = NULL;
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);

    pthread_mutex_destroy(&g_l2cap_manager.l2cap_lock);
}
