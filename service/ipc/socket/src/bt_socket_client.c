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

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/socket.h>
#ifndef __NuttX__
#include <linux/un.h>
#else
#include <sys/un.h>
#endif
#ifdef CONFIG_NET_RPMSG
#include <netpacket/rpmsg.h>
#endif

#include "adapter_internel.h"
#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_internal.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct _work_msg {
    bt_instance_t *ins;
    bt_message_packet_t packet;
} bt_client_msg_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void bt_socket_client_msg_process(bt_client_msg_t *msg)
{
    bt_message_packet_t *packet = &msg->packet;

    if (packet->code > BT_ADAPTER_CALLBACK_START && packet->code < BT_ADAPTER_CALLBACK_END) {
        bt_socket_client_adapter_callback(NULL, -1, msg->ins, packet);
    } else if (packet->code > BT_HFP_AG_CALLBACK_START && packet->code < BT_HFP_AG_CALLBACK_END) {
        bt_socket_client_hfp_ag_callback(NULL, -1, msg->ins, &msg->packet);
    } else if (packet->code > BT_HFP_HF_CALLBACK_START && packet->code < BT_HFP_HF_CALLBACK_END) {
        bt_socket_client_hfp_hf_callback(NULL, -1, msg->ins, &msg->packet);
    } else if (msg->packet.code > BT_A2DP_SINK_CALLBACK_START && msg->packet.code < BT_A2DP_SINK_CALLBACK_END) {
        bt_socket_client_a2dp_sink_callback(NULL, -1, msg->ins, &msg->packet);
    } else if (msg->packet.code > BT_A2DP_SOURCE_CALLBACK_START && msg->packet.code < BT_A2DP_SOURCE_CALLBACK_END) {
        bt_socket_client_a2dp_source_callback(NULL, -1, msg->ins, &msg->packet);
    } else if (packet->code > BT_ADVERTISER_CALLBACK_START && packet->code < BT_ADVERTISER_CALLBACK_END) {
        bt_socket_client_advertiser_callback(NULL, -1, msg->ins, packet);
    } else if (packet->code > BT_SCAN_CALLBACK_START && packet->code < BT_SCAN_CALLBACK_END) {
        bt_socket_client_scan_callback(NULL, -1, msg->ins, packet);
    } else if (packet->code > BT_GATT_CLIENT_CALLBACK_START && packet->code < BT_GATT_CLIENT_CALLBACK_END) {
        bt_socket_client_gattc_callback(NULL, -1, msg->ins, packet);
    } else if (packet->code > BT_GATT_SERVER_CALLBACK_START && packet->code < BT_GATT_SERVER_CALLBACK_END) {
        bt_socket_client_gatts_callback(NULL, -1, msg->ins, packet);
    } else if (packet->code > BT_SPP_CALLBACK_START && packet->code < BT_SPP_CALLBACK_END) {
        bt_socket_client_spp_callback(NULL, -1, msg->ins, packet);
    } else if (packet->code > BT_PAN_CALLBACK_START && packet->code < BT_PAN_CALLBACK_END) {
        bt_socket_client_pan_callback(NULL, -1, msg->ins, packet);
    } else if (packet->code > BT_HID_DEVICE_CALLBACK_START && packet->code < BT_HID_DEVICE_CALLBACK_END) {
        bt_socket_client_hid_device_callback(NULL, -1, msg->ins, packet);
    } else {
    }

    free(msg);
}

static void bt_socket_client_work(service_work_t *work, void *userdata)
{
    bt_socket_client_msg_process(userdata);
}

static void bt_socket_client_async_close(uv_handle_t *handle)
{
    free(handle);
}

static void bt_socket_client_async_cb(uv_async_t *handle)
{
    bt_socket_client_msg_process(handle->data);
    uv_close((uv_handle_t *)handle, bt_socket_client_async_close);
}

static bt_status_t bt_socket_client_async_to_external(bt_instance_t *ins, bt_client_msg_t *msg)
{
    uv_async_t *async = malloc(sizeof(*async));

    if (!async)
        return BT_STATUS_NOMEM;

    int ret = uv_async_init(ins->external_loop, async, bt_socket_client_async_cb);
    if (ret != 0) {
        free(async);
        return BT_STATUS_BUSY;
    }

    async->data = msg;
    uv_async_send(async);

    return BT_STATUS_SUCCESS;
}

static int bt_socket_client_receive(service_poll_t *poll, int fd, void *userdata)
{
    bt_instance_t *ins = userdata;
    bt_message_packet_t *packet;
    int ret;

    packet = ins->packet;

    ret = recv(fd, (char *)packet + ins->offset, sizeof(*packet) - ins->offset, 0);
    if (ret == 0) {
        service_loop_remove_poll(poll);
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
        uv_mutex_lock(&ins->mutex);
        uv_cond_signal(&ins->cond);
        uv_mutex_unlock(&ins->mutex);
        return BT_STATUS_SUCCESS;
    }

    if (packet->code > BT_CALLBACK_START && packet->code < BT_CALLBACK_END) {
        bt_client_msg_t *msg = malloc(sizeof(*msg));
        if (!msg)
            return BT_STATUS_NOMEM;

        msg->ins = ins;
        memcpy(&msg->packet, packet, sizeof(*packet));
        if (ins->external_loop) {
            bt_status_t status = bt_socket_client_async_to_external(ins, msg);
            if (status != BT_STATUS_SUCCESS) {
                free(msg);
                return status;
            }
        } else {
            if (!service_loop_work(msg, bt_socket_client_work, NULL)) {
                free(msg);
                return BT_STATUS_FAIL;
            }
        }
    }

    return BT_STATUS_SUCCESS;
}

static void bt_socket_client_handle_event(service_poll_t *poll,
                                          int revent, void *userdata)
{
    uv_os_fd_t fd;
    int ret;

    ret = uv_fileno((uv_handle_t *)&poll->handle, &fd);
    if (ret) {
        service_loop_remove_poll(poll);
        return;
    }

    if (revent & POLL_ERROR || revent & POLL_DISCONNECT) {
        service_loop_remove_poll(poll);
    } else if (revent & POLL_READABLE) {
        ret = bt_socket_client_receive(poll, fd, userdata);
        if (ret != BT_STATUS_SUCCESS)
            service_loop_remove_poll(poll);
    }
}

static int bt_socket_client_connect(int family, const char *name,
                                    const char *cpu, int port)
{
    union {
        struct sockaddr_in inet_addr;
        struct sockaddr_un local_addr;
#ifdef CONFIG_NET_RPMSG
        struct sockaddr_rpmsg rpmsg_addr;
#endif
    } u;
    socklen_t addr_len;
    int fd;

    if (family == PF_LOCAL) {
        u.local_addr.sun_family = AF_LOCAL;
        snprintf(u.local_addr.sun_path, UNIX_PATH_MAX,
                 BLUETOOTH_SOCKADDR_NAME, name);
        addr_len = sizeof(struct sockaddr_un);
    } else if (family == AF_INET) {
        u.inet_addr.sin_family = AF_INET;
        u.inet_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        u.inet_addr.sin_port = htons(port);
        addr_len = sizeof(struct sockaddr_in);
    } else {
#ifdef CONFIG_NET_RPMSG
        u.rpmsg_addr.rp_family = AF_RPMSG;
        snprintf(u.rpmsg_addr.rp_name, RPMSG_SOCKET_NAME_SIZE,
                 BLUETOOTH_SOCKADDR_NAME, name);
        if (cpu != NULL)
            strcpy(u.rpmsg_addr.rp_cpu, cpu);
        addr_len = sizeof(struct sockaddr_rpmsg);
#endif
    }

    fd = socket(family, SOCK_STREAM, 0);
    if (fd <= 0)
        return -errno;

    if (connect(fd, (struct sockaddr *)&u, addr_len) < 0) {
        close(fd);
        return -errno;
    }

    return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bt_socket_client_sendrecv(bt_instance_t *ins, bt_message_packet_t *packet,
                              bt_message_type_t code)
{
    int ret;

    uv_mutex_lock(&ins->mutex);

    packet->code = code;

    ins->cpacket = packet;

    ret = send(ins->peer_fd, packet, sizeof(*packet), 0);

    if (ret <= 0) {
        uv_mutex_unlock(&ins->mutex);
        return BT_STATUS_FAIL;
    }

    uv_cond_wait(&ins->cond, &ins->mutex);

    ins->cpacket = NULL;

    uv_mutex_unlock(&ins->mutex);

    return BT_STATUS_SUCCESS;
}

int bt_socket_client_init(bt_instance_t *ins, int family,
                          const char *name, const char *cpu, int port)
{
    service_poll_t *poll;
    service_loop_init();

    ins->packet = malloc(sizeof(bt_message_packet_t));
    if (ins->packet == NULL)
        return BT_STATUS_NOMEM;

    ins->offset = 0;

    uv_cond_init(&ins->cond);
    uv_mutex_init(&ins->mutex);

    ins->peer_fd = bt_socket_client_connect(family, name, cpu, port);
    if (ins->peer_fd <= 0) {
        bt_socket_client_deinit(ins);
        return BT_STATUS_PARM_INVALID;
    }

    poll = service_loop_poll_fd(ins->peer_fd, POLL_READABLE,
                                bt_socket_client_handle_event, ins);
    if (poll == NULL) {
        bt_socket_client_deinit(ins);
        return BT_STATUS_PARM_INVALID;
    }

    ins->poll = poll;

    service_loop_run(true, "bt_client");

    return BT_STATUS_SUCCESS;
}

void bt_socket_client_deinit(bt_instance_t *ins)
{
    uv_cond_destroy(&ins->cond);
    uv_mutex_destroy(&ins->mutex);

    if (ins->packet)
        free(ins->packet);

    if (ins->poll)
        service_loop_remove_poll(ins->poll);

    if (ins->peer_fd > 0)
        close(ins->peer_fd);

    service_loop_exit();
}
