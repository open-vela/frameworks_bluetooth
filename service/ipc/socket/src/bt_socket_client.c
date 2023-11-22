/****************************************************************************
 * frameworks/media/media_daemon.c
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

static void bt_socket_client_work(service_work_t *work, void *userdata)
{
    bt_client_msg_t *msg = userdata;
    if (msg->packet.code > BT_ADAPTER_CALLBACK_START && msg->packet.code < BT_ADAPTER_CALLBACK_END) {
        bt_socket_client_adapter_callback(NULL, -1, msg->ins, &msg->packet);
    } else if (msg->packet.code > BT_HFP_AG_CALLBACK_START && msg->packet.code < BT_HFP_AG_CALLBACK_END) {
        bt_socket_client_hfp_ag_callback(NULL, -1, msg->ins, &msg->packet);
    }
    free(msg);
}

static void bt_socket_client_after_work(service_work_t *work, void *userdata)
{
}

static int bt_socket_client_receive(service_poll_t *poll, int fd, void *userdata)
{
    bt_instance_t *ins = userdata;
    bt_message_packet_t packet;
    int ret;

    ret = recv(fd, &packet, sizeof(packet), 0);
    if (ret <= 0)
        return ret;

    if ((packet.code > BT_ADAPTER_MESSAGE_START && packet.code < BT_ADAPTER_MESSAGE_END) ||
        (packet.code > BT_DEVICE_MESSAGE_START && packet.code < BT_DEVICE_MESSAGE_END) ||
        (packet.code > BT_HFP_AG_MESSAGE_START && packet.code < BT_HFP_AG_MESSAGE_END)) {
        if (ins->packet == NULL)
            return BT_STATUS_SUCCESS;

        memcpy(ins->packet, &packet, sizeof(packet));
        uv_mutex_lock(&ins->mutex);
        uv_cond_signal(&ins->cond);
        uv_mutex_unlock(&ins->mutex);
        return BT_STATUS_SUCCESS;
    }

    if ((packet.code > BT_ADAPTER_CALLBACK_START && packet.code < BT_ADAPTER_CALLBACK_END) ||
        (packet.code > BT_HFP_AG_CALLBACK_START && packet.code < BT_HFP_AG_CALLBACK_END)) {
        bt_client_msg_t *msg = malloc(sizeof(*msg));
        if (!msg)
            return BT_STATUS_NOMEM;

        msg->ins = ins;
        memcpy(&msg->packet, &packet, sizeof(packet));
        if (!service_loop_work(msg, bt_socket_client_work, bt_socket_client_after_work)) {
            free(msg);
            return BT_STATUS_FAIL;
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

    ins->packet = packet;

    ret = send(ins->peer_fd, packet, sizeof(*packet), 0);
    if (ret <= 0) {
        uv_mutex_unlock(&ins->mutex);
        return BT_STATUS_FAIL;
    }

    uv_cond_wait(&ins->cond, &ins->mutex);

    ins->packet = NULL;

    uv_mutex_unlock(&ins->mutex);

    return BT_STATUS_SUCCESS;
}

int bt_socket_client_init(bt_instance_t *ins, int family,
                          const char *name, const char *cpu, int port)
{
    service_loop_init();

    uv_cond_init(&ins->cond);
    uv_mutex_init(&ins->mutex);

    ins->peer_fd = bt_socket_client_connect(family, name, cpu, port);
    if (ins->peer_fd <= 0 ||
        (service_loop_poll_fd(ins->peer_fd, POLL_READABLE,
                              bt_socket_client_handle_event, ins) == NULL))
        return BT_STATUS_PARM_INVALID;

    service_loop_run(true, "bt_client");

    return BT_STATUS_SUCCESS;
}
