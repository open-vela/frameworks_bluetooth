/****************************************************************************
 * service/ipc/socket/src/bt_socket_client.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "bt_socket_client"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>

#ifndef __NuttX__
#include <linux/un.h>
#else
#include <sys/un.h>
#endif

#include "bt_config.h"
#ifdef CONFIG_NET_RPMSG
#include <netpacket/rpmsg.h>
#endif

#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_debug.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "utils/log.h"
#include "uv_thread_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CLIENT_MAX_RETRY 10
#define CLIENT_MIN_RETRY_DELAY_MS 100
#define CLIENT_DELAY_MS(retry) ((CLIENT_MAX_RETRY - retry) * CLIENT_MIN_RETRY_DELAY_MS)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct _work_msg {
    struct list_node node;
    bt_instance_t* ins;
    bt_message_packet_t packet;
} bt_client_msg_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

typedef void (*bt_socket_callback_t)(void*, int, bt_instance_t*, bt_message_packet_t*);

static void bt_socket_client_callback_process(bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    static const struct {
        int start;
        int end;
        bt_socket_callback_t callback;
    } callback_map[] = {
        { BT_ADAPTER_CALLBACK_START, BT_ADAPTER_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_adapter_callback },
#ifdef CONFIG_BLUETOOTH_HFP_AG
        { BT_HFP_AG_CALLBACK_START, BT_HFP_AG_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_hfp_ag_callback },
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
        { BT_HFP_HF_CALLBACK_START, BT_HFP_HF_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_hfp_hf_callback },
#endif
#ifdef CONFIG_BLUETOOTH_A2DP
        { BT_A2DP_SINK_CALLBACK_START, BT_A2DP_SINK_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_a2dp_sink_callback },
        { BT_A2DP_SOURCE_CALLBACK_START, BT_A2DP_SOURCE_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_a2dp_source_callback },
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
        { BT_AVRCP_TARGET_CALLBACK_START, BT_AVRCP_TARGET_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_avrcp_target_callback },
#endif
#ifdef CONFIG_BLUETOOTH_BLE_ADV
        { BT_ADVERTISER_CALLBACK_START, BT_ADVERTISER_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_advertiser_callback },
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
        { BT_SCAN_CALLBACK_START, BT_SCAN_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_scan_callback },
#endif
#ifdef CONFIG_BLUETOOTH_GATT
        { BT_GATT_CLIENT_CALLBACK_START, BT_GATT_CLIENT_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_gattc_callback },
        { BT_GATT_SERVER_CALLBACK_START, BT_GATT_SERVER_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_gatts_callback },
#endif
#ifdef CONFIG_BLUETOOTH_SPP
        { BT_SPP_CALLBACK_START, BT_SPP_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_spp_callback },
#endif
#ifdef CONFIG_BLUETOOTH_PAN
        { BT_PAN_CALLBACK_START, BT_PAN_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_pan_callback },
#endif
#ifdef CONFIG_BLUETOOTH_HID_DEVICE
        { BT_HID_DEVICE_CALLBACK_START, BT_HID_DEVICE_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_hid_device_callback },
#endif
#ifdef CONFIG_BLUETOOTH_L2CAP
        { BT_L2CAP_CALLBACK_START, BT_L2CAP_CALLBACK_END, (bt_socket_callback_t)bt_socket_client_l2cap_callback },
#endif
    };

    for (size_t i = 0; i < sizeof(callback_map) / sizeof(callback_map[0]); ++i) {
        if (packet->code > callback_map[i].start && packet->code < callback_map[i].end) {
            callback_map[i].callback(NULL, -1, ins, packet);
            return;
        }
    }

    BT_LOGE("%s, Unhandled message: %d", __func__, (int)packet->code);
}

static void bt_socket_client_msg_process(bt_client_msg_t* msg)
{
    bt_message_packet_t* packet = &msg->packet;

    bt_socket_client_callback_process(msg->ins, packet, false);
    free(msg);
}

static void bt_socket_client_async_close(uv_handle_t* handle)
{
    free(handle);
}

static void bt_socket_client_async_cb(uv_async_t* handle)
{
    bt_instance_t* ins = handle->data;

    for (;;) {
        uv_mutex_lock(&ins->lock);
        bt_client_msg_t* msg = (bt_client_msg_t*)list_remove_head(&ins->msg_queue);
        if (!msg) {
            uv_mutex_unlock(&ins->lock);
            return;
        }
        uv_mutex_unlock(&ins->lock);

        bt_socket_client_msg_process(msg);
    }
}

static bt_status_t callback_send_to_external(bt_instance_t* ins, bt_client_msg_t* msg)
{
    uv_mutex_lock(&ins->lock);
    if (!ins->external_async) {
        ins->external_async = malloc(sizeof(uv_async_t));
        int ret = uv_async_init(ins->external_loop, ins->external_async, bt_socket_client_async_cb);
        if (ret != 0) {
            uv_mutex_unlock(&ins->lock);
            return BT_STATUS_BUSY;
        }
        ins->external_async->data = ins;
    }

    list_add_tail(&ins->msg_queue, &msg->node);
    uv_async_send(ins->external_async);
    uv_mutex_unlock(&ins->lock);

    return BT_STATUS_SUCCESS;
}

static void bt_socket_client_work(uv_work_t* req)
{
    bt_socket_client_msg_process(req->data);
}

static void bt_socket_client_after_work(uv_work_t* req, int status)
{
    assert(req);

    free(req);
}

static bt_status_t callback_send_to_queue_work(bt_instance_t* ins, bt_client_msg_t* msg)
{
    uv_work_t* work = zalloc(sizeof(*work));
    if (work == NULL)
        return BT_STATUS_NOMEM;

    work->data = msg;
    if (uv_queue_work(ins->client_loop, work, bt_socket_client_work, bt_socket_client_after_work) != 0) {
        free(work);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static int bt_socket_client_receive(uv_poll_t* poll, int fd, void* userdata)
{
    bt_instance_t* ins = userdata;
    bt_message_packet_t* packet;
    int ret;

    packet = ins->packet;

    ret = recv(fd, (char*)packet + ins->offset, sizeof(*packet) - ins->offset, 0);
    if (ret == 0) {
        thread_loop_remove_poll(poll);
        return ret;
    } else if (ret < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            return BT_STATUS_SUCCESS;
        }
        return ret;
    }

    ins->offset += ret;

    if (ins->offset != sizeof(*packet)) {
        return BT_STATUS_SUCCESS;
    } else {
        ins->offset = 0;
    }

    if (packet->code > BT_MESSAGE_START && packet->code < BT_MESSAGE_END) {
        if (ins->cpacket == NULL)
            return BT_STATUS_SUCCESS;

        memcpy(ins->cpacket, packet, sizeof(*packet));
        uv_sem_post(&ins->message_processed);
        return BT_STATUS_SUCCESS;
    } else if (packet->code > BT_CALLBACK_START && packet->code < BT_CALLBACK_END) {
        bt_client_msg_t* msg = malloc(sizeof(*msg));
        if (!msg)
            return BT_STATUS_NOMEM;

        msg->ins = ins;
        memcpy(&msg->packet, packet, sizeof(*packet));
        if (ins->external_loop) {
            bt_status_t status = callback_send_to_external(ins, msg);
            if (status != BT_STATUS_SUCCESS) {
                free(msg);
                return status;
            }
        } else {
            if (callback_send_to_queue_work(ins, msg) != BT_STATUS_SUCCESS) {
                free(msg);
                return BT_STATUS_FAIL;
            }
        }
    } else {
        assert(0);
    }

    return BT_STATUS_SUCCESS;
}

static void bt_socket_client_handle_event(uv_poll_t* poll, int status, int events)
{
    uv_os_fd_t fd;
    int ret;
    bt_instance_t* ins = poll->data;

    ret = uv_fileno((uv_handle_t*)poll, &fd);
    if (ret) {
        thread_loop_remove_poll(poll);
        return;
    }

    if (status != 0 || events & UV_DISCONNECT) {
        thread_loop_remove_poll(poll);
        if (ins && ins->disconnected) {
            BT_LOGE("%s socket disconnect, status = %d, events = %d", __func__, status, events);
            ins->disconnected(ins, NULL, status);
        }
    } else if (events & UV_READABLE) {
        ret = bt_socket_client_receive(poll, fd, poll->data);
        if (ret != BT_STATUS_SUCCESS)
            thread_loop_remove_poll(poll);
    }
}

static int bt_socket_client_connect(int family, const char* name,
    const char* cpu, int port)
{
    union {
        struct sockaddr_in inet_addr;
        struct sockaddr_un local_addr;
#ifdef CONFIG_NET_RPMSG
        struct sockaddr_rpmsg rpmsg_addr;
#endif
    } u = { 0 };
    socklen_t addr_len = 0;
    int fd;

    if (family == PF_LOCAL) {
        u.local_addr.sun_family = AF_LOCAL;
        snprintf(u.local_addr.sun_path, UNIX_PATH_MAX,
            BLUETOOTH_SOCKADDR_NAME, name);
        addr_len = sizeof(struct sockaddr_un);
    } else if (family == AF_INET) {
        u.inet_addr.sin_family = AF_INET;
#if defined(ANDROID)
        u.inet_addr.sin_addr.s_addr = htonl(CONFIG_INADDR_LOOPBACK);
#else
        u.inet_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#endif
        u.inet_addr.sin_port = htons(port);
        addr_len = sizeof(struct sockaddr_in);
    } else {
#ifdef CONFIG_NET_RPMSG
        u.rpmsg_addr.rp_family = AF_RPMSG;
        snprintf(u.rpmsg_addr.rp_name, RPMSG_SOCKET_NAME_SIZE,
            BLUETOOTH_SOCKADDR_NAME, name);
        if (cpu != NULL)
            strlcpy(u.rpmsg_addr.rp_cpu, cpu, sizeof(u.rpmsg_addr.rp_cpu));
        addr_len = sizeof(struct sockaddr_rpmsg);
#endif
    }
    if (!addr_len) {
        return -errno;
    }

    fd = socket(family, SOCK_STREAM, 0);
    if (fd < 0)
        return -errno;

    if (connect(fd, (struct sockaddr*)&u, addr_len) < 0) {
        close(fd);
        return -errno;
    }

#ifdef CONFIG_NET_SOCKOPTS
    setSocketBuf(fd, SO_SNDBUF);
    setSocketBuf(fd, SO_RCVBUF);
#endif

    return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bt_socket_client_sendrecv(bt_instance_t* ins, bt_message_packet_t* packet,
    bt_message_type_t code)
{
    uint8_t* send_data;
    int send_size;
    int ret;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    uv_mutex_lock(&ins->mutex);

    packet->code = code;

    ins->cpacket = packet;

    send_data = (uint8_t*)packet;
    send_size = sizeof(*packet);
    while (send_size > 0) {
        ret = send(ins->peer_fd, send_data, send_size, 0);
        if (ret == 0) {
            break;
        } else if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            break;
        }
        send_data += ret;
        send_size -= ret;
    }

    if (ret <= 0) {
        uv_mutex_unlock(&ins->mutex);
        BT_LOGE("%s, bt socket send ret: %d error: %d", __func__, ret, errno);
        return BT_STATUS_FAIL;
    }

    uv_sem_wait(&ins->message_processed);

    ins->cpacket = NULL;

    uv_mutex_unlock(&ins->mutex);

    return BT_STATUS_SUCCESS;
}

int bt_socket_client_init(bt_instance_t* ins, int family,
    const char* name, const char* cpu, int port)
{
    uv_poll_t* poll;
    int retry = CLIENT_MAX_RETRY;

    ins->client_loop = zalloc(sizeof(uv_loop_t));
    if (!ins->client_loop)
        return BT_STATUS_NOMEM;

    if (thread_loop_init(ins->client_loop) != 0) {
        free(ins->client_loop);
        ins->client_loop = NULL;
        return BT_STATUS_FAIL;
    }

    ins->packet = malloc(sizeof(bt_message_packet_t));
    if (ins->packet == NULL) {
        bt_socket_client_deinit(ins);
        return BT_STATUS_NOMEM;
    }

    ins->offset = 0;
    list_initialize(&ins->msg_queue);
    uv_mutex_init(&ins->lock);

    uv_sem_init(&ins->message_processed, 0);
    uv_mutex_init(&ins->mutex);
    do {
        ins->peer_fd = bt_socket_client_connect(family, name, cpu, port);
        if (ins->peer_fd <= 0 && !retry) {
            /* connect fail, go out */
            bt_socket_client_deinit(ins);
            return BT_STATUS_PARM_INVALID;
        } else if (ins->peer_fd <= 0) {
            /* connect fail, retry after sleep 100ms */
            usleep(CLIENT_DELAY_MS(retry) * 1000);
            continue;
        } else {
            /* success, goto next step */
            break;
        }
    } while (retry--);

    poll = thread_loop_poll_fd(ins->client_loop, ins->peer_fd, UV_READABLE | UV_DISCONNECT,
        bt_socket_client_handle_event, ins);
    if (poll == NULL) {
        bt_socket_client_deinit(ins);
        return BT_STATUS_PARM_INVALID;
    }

    ins->poll = poll;

    thread_loop_run(ins->client_loop, true, "bt_client");

    return BT_STATUS_SUCCESS;
}

static void bt_socket_sync_close(void* data)
{
    bt_instance_t* ins = data;

    if (ins->poll)
        thread_loop_remove_poll((uv_poll_t*)ins->poll);
    ins->poll = NULL;

    if (ins->peer_fd > 0)
        close(ins->peer_fd);
    ins->peer_fd = -1;
}

void bt_socket_client_free_callbacks(bt_instance_t* ins, callbacks_list_t* cbsl)
{
    do_in_thread_loop(ins->client_loop, bt_callbacks_list_free, cbsl);
}

void bt_socket_client_deinit(bt_instance_t* ins)
{
    uv_sem_destroy(&ins->message_processed);
    uv_mutex_destroy(&ins->mutex);

    if (ins->packet)
        free(ins->packet);

    if (!ins->poll)
        bt_socket_sync_close(ins);
    else
        do_in_thread_loop_sync(ins->client_loop, bt_socket_sync_close, ins);

    if (ins->external_loop && ins->external_async) {
        struct list_node* node;
        struct list_node* tmp;
        uv_mutex_lock(&ins->lock);
        list_for_every_safe(&ins->msg_queue, node, tmp)
        {
            list_delete(node);
            free(node);
        }
        uv_mutex_unlock(&ins->lock);
        uv_close((uv_handle_t*)ins->external_async, bt_socket_client_async_close);
    }

    uv_mutex_destroy(&ins->lock);
    thread_loop_exit(ins->client_loop);
    free(ins->client_loop);
}

/*
 Async client
*/
typedef struct {
    bt_message_type_t code;
    // bt_message_packet_t* packet;
    bt_socket_reply_cb_t reply_cb;
    void* cb;
    void* userdata;
} bt_message_context_t;

typedef struct {
    uv_write_t req;
    bt_message_packet_t packet;
    bt_socket_async_client_t* priv;
} bt_socket_write_t;

static void bt_socket_alloc_cb(uv_handle_t* handle,
    size_t suggested_size, uv_buf_t* buf)
{
    bt_socket_async_client_t* priv = uv_handle_get_data(handle);

    if (priv->packet == NULL) {
        priv->packet = malloc(sizeof(bt_message_packet_t));
        if (priv->packet == NULL) {
            BT_LOGE("malloc failed");
            buf = NULL;
            suggested_size = 0;
            return;
        }

        priv->offset = 0;
    }

    if (priv->offset == 0) {
        buf->base = (char*)priv->packet;
        buf->len = sizeof(bt_message_packet_t);
    } else {
        buf->base = ((char*)priv->packet) + priv->offset;
        buf->len = sizeof(bt_message_packet_t) - priv->offset;
    }
}

static int bt_socket_async_client_handle_packet(bt_instance_t* ins, bt_message_packet_t* packet)
{
    bt_socket_async_client_t* priv = ins->priv;

    if (packet->code > BT_MESSAGE_START && packet->code < BT_MESSAGE_END) {
        bt_message_context_t* ctx = (bt_message_context_t*)(uintptr_t)packet->context;
        bt_message_context_t* head = bt_list_node(bt_list_head(priv->pending_queue));

        assert(head == ctx);

        if (ctx && ctx->reply_cb)
            ctx->reply_cb(ins, packet, ctx->cb, ctx->userdata);

        bt_list_remove_node(priv->pending_queue, bt_list_head(priv->pending_queue));
    } else if (packet->code > BT_CALLBACK_START && packet->code < BT_CALLBACK_END) {
        bt_socket_client_callback_process(ins, packet, true);
    } else {
        assert(0);
    }

    return BT_STATUS_SUCCESS;
}

static void bt_socket_read_cb(uv_stream_t* stream,
    ssize_t nread, const uv_buf_t* buf)
{
    bt_socket_async_client_t* priv = uv_handle_get_data((uv_handle_t*)stream);

    if (nread > 0) {
        assert(priv->offset + nread <= sizeof(bt_message_packet_t));
        priv->offset += nread;

        if (priv->offset == sizeof(bt_message_packet_t)) {
            bt_socket_async_client_handle_packet(priv->ins, priv->packet);
            priv->offset = 0;
        }
    } else if (nread < 0) {
        if (priv->disconnected)
            priv->disconnected(priv->ins, priv->user_data, nread);
    }
}

static void bt_socket_close_cb(uv_handle_t* handle)
{
    free(handle);
}

static void bt_socket_connect_cb(uv_connect_t* req, int status)
{
    bt_socket_async_client_t* priv = uv_req_get_data((uv_req_t*)req);

    if (status != 0) {
        BT_LOGE("bt async client connect failed: %s", uv_strerror(status));
        if (priv->disconnected)
            priv->disconnected(priv->ins, priv->user_data, status);
    } else {
        BT_LOGI("bt async client connect success");
        uv_read_start((uv_stream_t*)priv->pipe, bt_socket_alloc_cb, bt_socket_read_cb);
        if (priv->connected)
            priv->connected(priv->ins, priv->user_data);
    }
}

static void bt_socket_write_cb(uv_write_t* req, int status)
{
    free(req);
}

static void bt_socket_context_free(void* data)
{
    /* callback to user the request was canceled because the instance was released? */

    /* free data */
    free(data);
}

int bt_socket_client_send_with_reply(bt_instance_t* ins, bt_message_packet_t* packet,
    bt_message_type_t code, bt_socket_reply_cb_t reply, void* cb, void* userdata)
{
    uv_buf_t buf;
    bt_message_context_t* ctx;
    bt_socket_async_client_t* priv;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    priv = ins->priv;

    ctx = malloc(sizeof(bt_message_context_t));
    if (!ctx)
        return BT_STATUS_NOMEM;

    ctx->code = code;
    ctx->reply_cb = reply;
    ctx->cb = cb;
    ctx->userdata = userdata;

    packet->code = code;
    packet->context = (uintptr_t)ctx;

    bt_socket_write_t* wreq = malloc(sizeof(bt_socket_write_t));
    if (wreq == NULL) {
        free(ctx);
        return BT_STATUS_NOMEM;
    }

    wreq->priv = priv;
    memcpy(&wreq->packet, packet, sizeof(bt_message_packet_t));
    buf = uv_buf_init((char*)&wreq->packet, sizeof(bt_message_packet_t));
    if (uv_write(&wreq->req, (uv_stream_t*)priv->pipe, &buf, 1, bt_socket_write_cb) != 0) {
        free(wreq);
        free(ctx);
        return BT_STATUS_FAIL;
    }

    bt_list_add_tail(priv->pending_queue, ctx);

    return BT_STATUS_SUCCESS;
}

int bt_socket_async_client_init(bt_instance_t* ins, uv_loop_t* loop, int family,
    const char* name, const char* cpu, int port, bt_ipc_connected_cb_t connected,
    bt_ipc_disconnected_cb_t disconnected, void* user_data)
{
    int ret;
    bt_socket_async_client_t* priv;

    if (ins == NULL || loop == NULL)
        return BT_STATUS_PARM_INVALID;

    priv = calloc(1, sizeof(bt_socket_async_client_t));
    if (priv == NULL)
        return BT_STATUS_NOMEM;

    priv->user_data = user_data;
    priv->loop = loop;
    priv->ins = ins;
    priv->connected = connected;
    priv->disconnected = disconnected;
    priv->pending_queue = bt_list_new(bt_socket_context_free);

    if (family == AF_LOCAL || family == AF_RPMSG) {
        priv->pipe = malloc(sizeof(uv_pipe_t));
        if (priv->pipe == NULL)
            goto fail;

        ret = uv_pipe_init(priv->loop, priv->pipe, 0);
        if (ret != 0)
            goto fail;

        uv_handle_set_data((uv_handle_t*)priv->pipe, priv);
        uv_req_set_data((uv_req_t*)&priv->conn_req, priv);

        if (family == AF_LOCAL) {
            char path[UNIX_PATH_MAX];

            snprintf(path, UNIX_PATH_MAX, BLUETOOTH_SOCKADDR_NAME, name);
            uv_pipe_connect(&priv->conn_req, priv->pipe, path, bt_socket_connect_cb);
        }
#ifdef CONFIG_NET_RPMSG
        else if (family == AF_RPMSG) {
            char rp_name[RPMSG_SOCKET_NAME_SIZE];

            snprintf(rp_name, RPMSG_SOCKET_NAME_SIZE, BLUETOOTH_SOCKADDR_NAME, name);
            uv_pipe_rpmsg_connect(&priv->conn_req, priv->pipe, rp_name, cpu, bt_socket_connect_cb);
        }
#endif
        else {
            return BT_STATUS_NOT_SUPPORTED;
        }
    }

    ins->priv = priv;

    return BT_STATUS_SUCCESS;
fail:
    bt_socket_async_client_deinit(ins);
    return BT_STATUS_FAIL;
}

void bt_socket_async_client_deinit(bt_instance_t* ins)
{
    bt_socket_async_client_t* priv;

    if (ins == NULL)
        return;

    priv = ins->priv;
    if (priv == NULL)
        return;

    uv_read_stop((uv_stream_t*)priv->pipe);

    if (priv->pending_queue) {
        bt_list_free(priv->pending_queue);
        priv->pending_queue = NULL;
    }

    if (priv->pipe)
        uv_close((uv_handle_t*)priv->pipe, bt_socket_close_cb);

    free(priv->packet);
    free(priv);
    ins->priv = NULL;
}