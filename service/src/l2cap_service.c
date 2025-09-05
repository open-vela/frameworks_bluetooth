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
#include "euv_pipe.h"
#include "index_allocator.h"
#include "l2cap_service.h"
#include "sal_l2cap_interface.h"
#include "service_loop.h"
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
 * \def L2CAP connection maximum limitation
 */
#define L2CAP_CHANNEL_MAX_NUM 20

/**
 * \def L2CAP socket server pipe name prefix
 */
#define L2CAP_SRVPIPE_NAME_PREF "l-srvpipe"

/**
 * \def L2CAP socket pipe default read size
 */
#define L2CAP_PIPE_DEF_READ_SIZE 1024

/**
 * \def L2CAP LE Dynamic PSM number limitation
 *
 * \note 0x0080 ~ 0x00FF is for L2CAP LE Dynamic PSM
 */
#define L2CAP_LE_DYNAMIC_PSM_NUM 64

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef enum {
    L2CAP_CHANNEL_ROLE_SERVER,
    L2CAP_CHANNEL_ROLE_ACCEPT,
    L2CAP_CHANNEL_ROLE_CLIENT,
} l2cap_channel_role_t;

typedef struct {
    bt_address_t addr;
    bt_transport_t transport;
    uint16_t local_cid;
    uint16_t remote_cid;
    uint16_t psm;
    l2cap_endpoint_param_t incoming;
    l2cap_endpoint_param_t outgoing;
    uint16_t tx_mtu;
    uint16_t id;
    l2cap_channel_role_t role;
    bool channel_connected;
    euv_pipe_t* pipe;
    char proxy_name[16];
    bool proxy_connected;
    remote_callback_t* app_handle;
} l2cap_channel_t;

typedef struct {
    callbacks_list_t* callbacks;
    bt_list_t* channel_list;
    index_allocator_t* id_allocator; // allocate id
    uint64_t psm_map;
    pthread_mutex_t l2cap_lock;

} l2cap_manager_t;

typedef struct {
    enum {
        CID_ALLOCATED_EVT,
        CHANNEL_CONNECTED_EVT,
        CHANNEL_DISCONNECTED_EVT,
        PACKET_RECEVIED_EVT,
        PACKET_SENT_EVT,
    } event;

    union {
        /**
         * @brief CID_ALLOCATED_EVT
         */
        struct cid_allocated_evt_param {
            bt_address_t addr;
            uint16_t psm;
            uint16_t cid;
        } cid_allocated;
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

static inline void l2cap_notify_connected(l2cap_channel_t* channel, l2cap_connect_params_t* param)
{
    l2cap_callbacks_t* cbs;

    if (channel && channel->app_handle && channel->app_handle->remote && channel->app_handle->callbacks) {
        cbs = (l2cap_callbacks_t*)channel->app_handle->callbacks;
        if (cbs->on_connected) {
            cbs->on_connected(channel->app_handle->remote, param);
        }
    } else {
        BT_LOGE("%s, channel or callbacks is NULL", __func__);
    }
}

static inline void l2cap_notify_disconnected(l2cap_channel_t* channel, uint32_t reason)
{
    l2cap_callbacks_t* cbs;

    if (channel && channel->app_handle && channel->app_handle->remote && channel->app_handle->callbacks) {
        cbs = (l2cap_callbacks_t*)channel->app_handle->callbacks;
        if (cbs->on_disconnected) {
            cbs->on_disconnected(channel->app_handle->remote, &channel->addr, channel->id, reason);
        }
    } else {
        BT_LOGE("%s, channel or callbacks is NULL", __func__);
    }
}

static l2cap_channel_t* alloc_free_channel(void* handle, bt_address_t* addr, uint16_t psm, l2cap_channel_role_t role)
{
    int id;
    l2cap_channel_t* channel;

    if (addr && role == L2CAP_CHANNEL_ROLE_SERVER) {
        // this check is not necessary?
        BT_LOGW("%s, server channel remote addr is not NULL", __func__);
        return NULL;
    }

    id = index_alloc(g_l2cap_manager.id_allocator);
    if (id < 0) {
        BT_LOGE("%s, alloc l2cap channel id failed", __func__);
        return NULL;
    }

    channel = (l2cap_channel_t*)calloc(1, sizeof(l2cap_channel_t));
    if (!channel) {
        BT_LOGE("%s, alloc l2cap channel failed", __func__);
        return NULL;
    }

    channel->app_handle = (remote_callback_t*)handle;
    if (addr)
        memcpy(&channel->addr, addr, sizeof(bt_address_t)); // copy address

    channel->psm = psm;
    channel->id = id;
    channel->role = role;
    channel->channel_connected = false;
    channel->proxy_connected = false;

    bt_list_add_tail(g_l2cap_manager.channel_list, (void*)channel);

    return channel;
}

static bool check_psm_available(uint16_t psm)
{
    if (psm < LE_PSM_DYNAMIC_MIN || psm >= LE_PSM_DYNAMIC_MIN + L2CAP_LE_DYNAMIC_PSM_NUM) {
        BT_LOGE("%s, psm %" PRIx16 " is not support", __func__, psm);
        return false;
    }

    return !(g_l2cap_manager.psm_map & (1 << (psm - LE_PSM_DYNAMIC_MIN)));
}

static uint16_t alloc_le_dynamic_psm(void)
{
    uint16_t psm;
    uint8_t i;

    // Reserved PSM range: 0x00080 - 0x0089
    for (i = 10; i < L2CAP_LE_DYNAMIC_PSM_NUM; i++) {
        if (!(g_l2cap_manager.psm_map & (1 << i))) {
            psm = LE_PSM_DYNAMIC_MIN + i;
            g_l2cap_manager.psm_map |= (1 << i);
            BT_LOGI("%s, alloc psm %" PRIx16, __func__, psm);
            return psm;
        }
    }

    BT_LOGE("%s, no dynamic PSM available", __func__);
    return 0;
}

static l2cap_channel_t* find_l2cap_channel_by_cid(uint16_t cid)
{
    bt_list_node_t* node;
    bt_list_t* list = g_l2cap_manager.channel_list;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        l2cap_channel_t* channel = (l2cap_channel_t*)bt_list_node(node);
        if (channel->local_cid == cid) {
            return channel;
        }
    }

    return NULL;
}

static l2cap_channel_t* find_l2cap_channel_by_pipe(euv_pipe_t* pipe)
{
    bt_list_node_t* node;
    bt_list_t* list = g_l2cap_manager.channel_list;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        l2cap_channel_t* channel = (l2cap_channel_t*)bt_list_node(node);
        if (channel->pipe == pipe) {
            return channel;
        }
    }

    return NULL;
}

static l2cap_channel_t* find_l2cap_channel_by_id(uint16_t id)
{
    bt_list_node_t* node;
    bt_list_t* list = g_l2cap_manager.channel_list;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        l2cap_channel_t* channel = (l2cap_channel_t*)bt_list_node(node);
        if (channel->id == id) {
            return channel;
        }
    }

    return NULL;
}

static l2cap_channel_t* find_l2cap_channel_by_conn_param(bt_address_t* addr, uint16_t psm,
    l2cap_channel_role_t role, bool is_connected)
{
    bt_list_node_t* node;
    bt_list_t* list = g_l2cap_manager.channel_list;

    switch (role) {
    case L2CAP_CHANNEL_ROLE_CLIENT: {
        // client find by psm and addr
        if (!addr) {
            BT_LOGE("%s, invalid arg", __func__);
            return NULL;
        }

        for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
            l2cap_channel_t* channel = (l2cap_channel_t*)bt_list_node(node);
            if (channel->psm == psm
                && channel->role == role
                && channel->channel_connected == is_connected
                && !bt_addr_compare(&channel->addr, addr)) {
                return channel;
            }
        }
        break;
    }
    case L2CAP_CHANNEL_ROLE_SERVER: {
        // server find by psm
        for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
            l2cap_channel_t* channel = (l2cap_channel_t*)bt_list_node(node);
            if (channel->psm == psm
                && channel->role == role
                && channel->channel_connected == is_connected) {
                return channel;
            }
        }
        break;
    }
    case L2CAP_CHANNEL_ROLE_ACCEPT:
    default:
        break;
    }

    return NULL;
}

static void free_l2cap_channel(l2cap_channel_t* channel)
{
    if (!channel) {
        BT_LOGE("%s, channel is NULL", __func__);
        return;
    }

    if (channel->pipe)
        euv_pipe_close(channel->pipe);

    index_free(g_l2cap_manager.id_allocator, channel->id);

    bt_list_remove(g_l2cap_manager.channel_list, (void*)channel);
}

static void l2cap_receive_data_from_app(euv_pipe_t* pipe, const uint8_t* buf, ssize_t size)
{
    l2cap_channel_t* channel;

    if (!pipe || !buf) {
        BT_LOGE("%s, invalid arg", __func__);
        return;
    }

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);
    channel = find_l2cap_channel_by_pipe(pipe);
    if (!channel) {
        BT_LOGE("%s, find L2CAP channel null", __func__);
        goto unlock;
    }

    if (!size) {
        // maybe data path disconnect.
        BT_LOGD("read size is 0");
    } else if (size < 0) {
        BT_LOGD("%s, data path for L2CAP connnection %" PRIu16 " close, reason: %zd", __func__, channel->id, size);
        if (channel->channel_connected) {
            bt_sal_l2cap_disconnect_channel(channel->local_cid);
        }

        euv_pipe_close(pipe); // may be read stop, free it at l2cap disconnected.
        channel->pipe = NULL;
        channel->proxy_connected = false;
    } else {
        if (!channel->channel_connected) {
            BT_LOGW("%s, L2CAP channel not connected", __func__);
        } else {
            bt_sal_l2cap_send_packet(channel->local_cid, (uint8_t*)buf, size);
        }
    }

unlock:
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);
}

static void proxy_connected_cb(euv_pipe_t* pipe, int status, void* data)
{
    int ret;
    l2cap_channel_t* channel;

    if (!pipe || !data) {
        BT_LOGE("%s, invalid arg", __func__);
        return;
    }

    channel = (l2cap_channel_t*)data;
    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);
    if (status) {
        BT_LOGE("%s, data path for L2CAP connnection %" PRIu16 " establish failed: %s", __func__, channel->id, uv_strerror(status));
        goto fail;
    }

    BT_LOGI("%s, data path for L2CAP connnection %" PRIu16 " established", __func__, channel->id);
    channel->proxy_connected = true;
#ifdef CONFIG_NET_RPMSG
    // TBD: API specific socket protocol.
    // Close unused pipe.
    euv_pipe_close2(channel->pipe);
#endif
    // If keep unconnected pipe alive, service need to release two pipes on disconnection.

    // start reading pipe after L2CAP Channel connected? read size unknown now
    // start read for monitoring pipe
    ret = euv_pipe_read_start(channel->pipe, L2CAP_PIPE_DEF_READ_SIZE, l2cap_receive_data_from_app, NULL);
    if (ret) {
        BT_LOGE("%s, start read pipe failed", __func__);
        goto fail;
    }

    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);

    return;

fail:
    // TBD: cancel l2cap channel listen or connect immediately?
    euv_pipe_close(channel->pipe);
    channel->pipe = NULL;
    channel->proxy_connected = false;
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);
}

static bool prepare_data_path(l2cap_channel_t* channel)
{
    snprintf(channel->proxy_name, sizeof(channel->proxy_name), "%s-%d", L2CAP_SRVPIPE_NAME_PREF, channel->id);
    channel->pipe = euv_pipe_open(get_service_uv_loop(), channel->proxy_name, proxy_connected_cb, channel);
    if (!channel->pipe) {
        BT_LOGE("%s, open server pipe %s failed", __func__, channel->proxy_name);
        return false;
    }

    BT_LOGD("%s, open server pipe %s success", __func__, channel->proxy_name);
    return true;
}

static void euv_write_complete(euv_pipe_t* handle, uint8_t* buf, int status)
{
    free(buf);
}

static void handle_cid_allocated(bt_address_t* addr, uint16_t psm, uint16_t cid)
{
    l2cap_channel_t* channel;

    // handle_cid_allocated is for client only.
    channel = find_l2cap_channel_by_conn_param(addr, psm, L2CAP_CHANNEL_ROLE_CLIENT, false);
    if (channel) {
        channel->local_cid = cid;
        BT_LOGI("L2CAP connection %" PRIu16 " get local CID: 0x%" PRIx16, channel->id, cid);
    } else {
        BT_LOGE("record allocated CID: 0x%x failed!", cid);
    }
}

static void handle_channel_conneted(bt_address_t* addr, l2cap_channel_param_t* param)
{
    int ret;
    l2cap_channel_t* channel;
    l2cap_channel_t* new_listen_channel = NULL;
    l2cap_connect_params_t conn_param;
    l2cap_channel_role_t role;

    if (!addr || !param) {
        BT_LOGE("%s, invalid arg", __func__);
        return;
    }

    role = param->is_client ? L2CAP_CHANNEL_ROLE_CLIENT : L2CAP_CHANNEL_ROLE_SERVER;
    channel = find_l2cap_channel_by_conn_param(addr, param->psm, role, false);
    if (!channel) {
        BT_LOGE("%s, find L2CAP channel null, local cid: 0x%" PRIx16, __func__, param->local_cid);
        bt_sal_l2cap_disconnect_channel(param->local_cid);

        return;
    }

    if (role == L2CAP_CHANNEL_ROLE_SERVER) {
        memcpy(&channel->addr, addr, sizeof(channel->addr));
        channel->local_cid = param->local_cid;
        channel->role = L2CAP_CHANNEL_ROLE_ACCEPT;
        new_listen_channel = alloc_free_channel(NULL, channel->psm, L2CAP_CHANNEL_ROLE_SERVER);
        if (!new_listen_channel) {
            BT_LOGE("%s, allocate new listen channel for psm: %" PRIx16 "failed", __func__, channel->psm);
            // TBD: stop server?
            return;
        }

        if (!prepare_data_path(new_listen_channel)) {
            BT_LOGE("%s, prepare data path failed", __func__);
            // TBD: free new listen channel
            // TBD: stop server?
            return;
        }
    }

    memcpy(&channel->incoming, &param->incoming, sizeof(channel->incoming));
    memcpy(&channel->outgoing, &param->outgoing, sizeof(channel->outgoing));
    channel->tx_mtu = MIN(param->outgoing.mtu, CONFIG_BLUETOOTH_L2CAP_OUTGOING_MTU);
    if (!channel->proxy_connected) {
        BT_LOGE("L2CAP channel(id:%" PRIu16 "/cid:0x%x) data path is not prepared", channel->id, channel->local_cid);
        bt_sal_l2cap_disconnect_channel(channel->local_cid);
        return;
    }

    // restart read pipe to adjust mtu
    ret = euv_pipe_read_stop(channel->pipe);
    if (ret != 0) {
        BT_LOGE("L2CAP channel(id: %" PRIu16 "/cid: 0x%" PRIx16 ") read stop failed!", channel->id, channel->local_cid);
        bt_sal_l2cap_disconnect_channel(channel->local_cid);
        return;
    }

    ret = euv_pipe_read_start(channel->pipe, channel->tx_mtu, l2cap_receive_data_from_app, NULL);
    if (ret != 0) {
        BT_LOGE("L2CAP channel(id: %" PRIu16 "/cid: 0x%" PRIx16 ") read start failed!", channel->id, channel->local_cid);
        bt_sal_l2cap_disconnect_channel(channel->local_cid);
        return;
    }

    BT_LOGI("L2CAP channel(id: %" PRIu16 "/cid: 0x%" PRIx16 ") connected", channel->id, channel->local_cid);
    BT_LOGD("L2CAP channel(id: %" PRIu16 "/cid: 0x%" PRIx16 ") mtu: %" PRIu16, channel->id, channel->local_cid, channel->tx_mtu);
    channel->channel_connected = true;

    // notify app
    memcpy(&conn_param.addr, &channel->addr, sizeof(conn_param.addr));
    conn_param.transport = channel->transport;
    conn_param.cid = channel->local_cid;
    conn_param.psm = channel->psm;
    conn_param.incoming_mtu = channel->incoming.mtu;
    conn_param.outgoing_mtu = channel->outgoing.mtu;
    conn_param.id = channel->id;
    if (new_listen_channel) {
        conn_param.listen_id = new_listen_channel->id;
        strlcpy(conn_param.proxy_name, new_listen_channel->proxy_name, sizeof(new_listen_channel->proxy_name));
    }

    l2cap_notify_connected(channel, &conn_param);
}

static void handle_channel_disconneted(bt_address_t* addr, uint16_t cid, uint32_t reason)
{
    l2cap_channel_t* channel;
    uint16_t id;

    // Note:
    // If clinet get cid fail during connecing, it won't be removed in this callback.
    channel = find_l2cap_channel_by_cid(cid);
    if (!channel) {
        BT_LOGE("%s, find L2CAP channel null, local cid: 0x%" PRIx16, __func__, cid);
        return;
    }

    id = channel->id;
    BT_LOGI("L2CAP channel(id:%" PRIu16 "/cid:0x%" PRIx16 ") disconnected, reason: 0x%" PRIx32 "", channel->id, cid, reason);
    // Notice:
    // The app will be aware of the data path disconnected first, pay attention to multithreading conflicts.
    free_l2cap_channel(channel);

    l2cap_notify_disconnected(channel, reason);
}

static void handle_packet_received(bt_address_t* addr, uint16_t cid, uint8_t* packet_data, uint16_t packet_size)
{
    l2cap_channel_t* channel;

    channel = find_l2cap_channel_by_cid(cid);
    if (channel && channel->pipe) {
        int ret = euv_pipe_write(channel->pipe, packet_data, packet_size, euv_write_complete);
        if (ret != 0) {
            BT_LOGE("L2CAP channel(id:%" PRIu16 "/cid:0x%" PRIx16 ") write failed!", channel->id, channel->local_cid);
            euv_pipe_close(channel->pipe);
            channel->proxy_connected = false;
            channel->pipe = NULL;
            bt_sal_l2cap_disconnect_channel(channel->local_cid);
        }
    }
}

static void handle_packet_sent(bt_address_t* addr, uint16_t cid)
{
    // do nothing for now.
    UNUSED(addr);
    UNUSED(cid);
}

static void handle_l2cap_event(void* data)
{
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    if (!msg) {
        return;
    }

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);

    switch (msg->event) {
    case CID_ALLOCATED_EVT:
        handle_cid_allocated(&msg->cid_allocated.addr,
            msg->cid_allocated.psm,
            msg->cid_allocated.cid);
        break;
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

void l2cap_on_cid_allocated(bt_address_t* addr, uint16_t psm, uint16_t cid)
{
    l2cap_msg_t* msg = malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        return;
    }

    msg->event = CID_ALLOCATED_EVT;
    memcpy(&msg->cid_allocated.addr, addr, sizeof(bt_address_t));
    msg->cid_allocated.psm = psm;
    msg->cid_allocated.cid = cid;
    do_in_service_loop(handle_l2cap_event, msg);
}

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
    if (!adapter_is_le_enabled()) {
        BT_LOGE("%s, adapter is not enabled", __func__);
        return NULL;
    }

    return bt_remote_callbacks_register(g_l2cap_manager.callbacks, remote, (void*)callbacks);
}

bool l2cap_unregister_callbacks(void** remote, void* cookie)
{
    if (!adapter_is_le_enabled()) {
        BT_LOGI("%s, adapter is not enabled", __func__);
        return true;
    }

    return bt_remote_callbacks_unregister(g_l2cap_manager.callbacks, remote, (remote_callback_t*)cookie);
}

bt_status_t l2cap_listen_channel(void* handle, l2cap_config_option_t* option)
{
    bt_status_t status;
    l2cap_channel_t* channel;

    if ((!handle) || (!option)) {
        return BT_STATUS_PARM_INVALID;
    }

    CHECK_ADAPTER_ENABLED(BT_STATUS_NOT_ENABLED);

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);
    if (option->psm == 0) {
        option->psm = alloc_le_dynamic_psm();
        if (option->psm == 0) {
            BT_LOGW("%s, allocate psm failed", __func__);
            status = BT_STATUS_NOMEM;
            goto out;
        }
    } else {
        if (check_psm_available(option->psm)) {
            g_l2cap_manager.psm_map |= (1 << (option->psm - LE_PSM_DYNAMIC_MIN));
        } else {
            BT_LOGE("%s, psm: 0x%" PRIx16 " is not available", __func__, option->psm);
            status = BT_STATUS_NOMEM;
            goto out;
        }
    }

    channel = alloc_free_channel(handle, NULL, option->psm, L2CAP_CHANNEL_ROLE_SERVER);
    if (!channel) {
        status = BT_STATUS_NOMEM;
        goto out;
    }

    if (!prepare_data_path(channel)) {
        bt_list_remove(g_l2cap_manager.channel_list, (void*)channel);
        status = BT_STATUS_NOMEM; // maybe use other status
        goto out;
    }

    BT_LOGI("%s, L2CAP(id: %" PRIu16 ", psm: 0x%" PRIx16 ") listen", __func__, channel->id, channel->psm);
    option->id = channel->id;
    strlcpy(option->proxy_name, channel->proxy_name, sizeof(option->proxy_name));
    status = bt_sal_l2cap_listen_channel(option);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, L2CAP(id: %" PRIu16 ", psm: 0x%" PRIx16 " listen failed", __func__, channel->id, channel->psm);
        bt_list_remove(g_l2cap_manager.channel_list, (void*)channel);
    }

out:
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);

    return status;
}

bt_status_t l2cap_connect_channel(void* handle, bt_address_t* addr, l2cap_config_option_t* option)
{
    bt_status_t status;
    l2cap_channel_t* channel;
    char addr_str[BT_ADDR_STR_LENGTH];

    if ((!handle) || (!addr) || (!option)) {
        return BT_STATUS_PARM_INVALID;
    }

    CHECK_ADAPTER_ENABLED(BT_STATUS_NOT_ENABLED);

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);
    channel = alloc_free_channel(handle, addr, option->psm, L2CAP_CHANNEL_ROLE_CLIENT);
    if (!channel) {
        status = BT_STATUS_NOMEM;
        goto out;
    }

    if (!prepare_data_path(channel)) {
        bt_list_remove(g_l2cap_manager.channel_list, (void*)channel);
        status = BT_STATUS_NOMEM; // maybe use other status
        goto out;
    }

    bt_addr_ba2str(addr, addr_str);
    BT_LOGI("%s, L2CAP(id: %" PRIu16 ", psm: 0x%" PRIx16 ") connect remote: %s", __func__, channel->id, channel->psm, addr_str);
    option->id = channel->id;
    strlcpy(option->proxy_name, channel->proxy_name, sizeof(option->proxy_name));
    status = bt_sal_l2cap_connect_channel(addr, option);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, L2CAP(id: %" PRIu16 ", psm: 0x%" PRIx16 ") connect failed", __func__, channel->id, channel->psm);
        bt_list_remove(g_l2cap_manager.channel_list, (void*)channel);
    } else {
        // TBD: timeout for connection initiation.
    }

out:
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);

    return status;
}

bt_status_t l2cap_disconnect_channel(void* handle, uint16_t id)
{
    bt_status_t status;
    l2cap_channel_t* channel;

    CHECK_ADAPTER_ENABLED(BT_STATUS_NOT_ENABLED);

    pthread_mutex_lock(&g_l2cap_manager.l2cap_lock);
    channel = find_l2cap_channel_by_id(id);
    if (!channel) {
        status = BT_STATUS_NOT_FOUND;
        BT_LOGE("%s, L2CAP(id: %" PRIu16 ") not found", __func__, id);
        goto exit;
    }

    if (channel->app_handle != handle) {
        status = BT_STATUS_UNHANDLED;
        BT_LOGW("%s, L2CAP(id: %" PRIu16 ") not belong to this app", __func__, id);
        goto exit;
    }

    // TBD: add channel stm: connecting/listening, connected, disconnecting, disconnected
    if (!channel->local_cid) {
        // bug: if cid not allocated, disconnect failed
        status = BT_STATUS_NOT_READY;
        BT_LOGE("%s, L2CAP(id: %" PRIu16 ") not connected", __func__, id);
        goto exit;
    }

    BT_LOGI("%s, L2CAP(id: %" PRIu16 ", cid: 0x%" PRIx16 ") disconnect", __func__, id, channel->local_cid);
    status = bt_sal_l2cap_disconnect_channel(channel->local_cid);

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

    g_l2cap_manager.id_allocator = index_allocator_create(L2CAP_CHANNEL_MAX_NUM);
    g_l2cap_manager.psm_map = 0;
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
    index_allocator_delete(&g_l2cap_manager.id_allocator);
    g_l2cap_manager.psm_map = 0;
    pthread_mutex_unlock(&g_l2cap_manager.l2cap_lock);

    pthread_mutex_destroy(&g_l2cap_manager.l2cap_lock);
}
