/****************************************************************************
 * service/ipc/socket/src/bt_socket_server.c
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
#ifdef CONFIG_NET_RPMSG
#include <netpacket/rpmsg.h>
#endif
#ifndef __NuttX__
#include <linux/un.h>
#else
#include <sys/un.h>
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

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int bt_socket_server_receive(service_poll_t *poll, int fd, void *userdata)
{
    bt_instance_t *ins = userdata;
    bt_message_packet_t packet;
    int ret;

    ret = recv(fd, &packet, sizeof(packet), 0);
    if (ret <= 0)
        return ret;

    if (packet.code > BT_MANAGER_MESSAGE_START &&
        packet.code < BT_MANAGER_MESSAGE_END) {
        bt_socket_server_manager_process(poll, fd, ins, &packet);
    } else if (packet.code > BT_ADAPTER_MESSAGE_START &&
        packet.code < BT_ADAPTER_MESSAGE_END) {
        bt_socket_server_adapter_process(poll, fd, ins, &packet);
    } else if (packet.code > BT_DEVICE_MESSAGE_START &&
               packet.code < BT_DEVICE_MESSAGE_END) {
        bt_socket_server_device_process(poll, fd, ins, &packet);
    } else if (packet.code > BT_HFP_AG_MESSAGE_START &&
               packet.code < BT_HFP_AG_MESSAGE_END) {
        bt_socket_server_hfp_ag_process(poll, fd, ins, &packet);
    } else if (packet.code > BT_HFP_HF_MESSAGE_START &&
               packet.code < BT_HFP_HF_MESSAGE_END) {
        bt_socket_server_hfp_hf_process(poll, fd, ins, &packet);
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    ret = send(fd, &packet, sizeof(packet), 0);
    if (ret <= 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

static void bt_socket_server_handle_event(service_poll_t *poll,
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
        ret = bt_socket_server_receive(poll, fd, userdata);
        if (ret)
            service_loop_remove_poll(poll);
    }
}

static void bt_socket_server_callback(service_poll_t *poll,
                                      int revent, void *userdata)
{
    bt_instance_t *remote_ins;
    uv_os_fd_t fd;
    int ret;

    ret = uv_fileno((uv_handle_t *)&poll->handle, &fd);
    if (ret) {
        service_loop_remove_poll(poll);
        return;
    }

    fd = accept(fd, NULL, NULL);
    if (fd < 0)
        return;

    if (revent & POLL_ERROR || revent & POLL_DISCONNECT) {
        service_loop_remove_poll(poll);
    } else if (revent & POLL_READABLE) {
        remote_ins = zalloc(sizeof(bt_instance_t));
        if (service_loop_poll_fd(fd, POLL_READABLE,
                                 bt_socket_server_handle_event, remote_ins) == NULL) {
            free(remote_ins);
            close(fd);
        }

        remote_ins->peer_fd = fd;
    }
}

static int bt_socket_server_listen(int family, const char *name, int port)
{
    union {
        struct sockaddr_in inet_addr;
        struct sockaddr_un local_addr;
#ifdef CONFIG_NET_RPMSG
        struct sockaddr_rpmsg rpmsg_addr;
#endif
    } u;
    int addr_len;
    int ret;
    int fd;

    fd = socket(family, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0)
        return -errno;

    if (family == PF_LOCAL) {
        u.local_addr.sun_family = AF_LOCAL;
        snprintf(u.local_addr.sun_path, UNIX_PATH_MAX,
                 BLUETOOTH_SOCKADDR_NAME, name);
        addr_len = sizeof(struct sockaddr_un);
    } else if (family == AF_INET) {
        u.inet_addr.sin_family = AF_INET;
        u.inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        u.inet_addr.sin_port = htons(port);
        addr_len = sizeof(struct sockaddr_in);
    } else {
#ifdef CONFIG_NET_RPMSG
        u.rpmsg_addr.rp_family = AF_RPMSG;
        snprintf(u.rpmsg_addr.rp_name, RPMSG_SOCKET_NAME_SIZE,
                 BLUETOOTH_SOCKADDR_NAME, name);
        strcpy(u.rpmsg_addr.rp_cpu, "");
        addr_len = sizeof(struct sockaddr_rpmsg);
#endif
    }

    ret = bind(fd, (struct sockaddr *)&u, addr_len);
    if (ret >= 0)
        ret = listen(fd, BLUETOOTH_SERVER_MAXCONN);

    if (ret < 0)
        close(fd);

    return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bt_socket_server_send(bt_instance_t *ins, bt_message_packet_t *packet,
                          bt_message_type_t code)
{
    int ret;

    packet->code = code;

    ret = send(ins->peer_fd, packet, sizeof(*packet), 0);
    if (ret <= 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

int bt_socket_server_init(const char *name, int port)
{
    service_poll_t *lpoll;
    int local;
#ifdef CONFIG_NET_IPv4
    service_poll_t *ipoll;
    int inet;
#endif
#ifdef CONFIG_NET_RPMSG
    service_poll_t *rpoll;
    int rpmsg;
#endif

    local = bt_socket_server_listen(PF_LOCAL, name, port);
    if (local > 0) {
        lpoll = service_loop_poll_fd(local, POLL_READABLE,
                                     bt_socket_server_callback, NULL);
    }

    if (local <= 0 || lpoll == NULL)
        goto fail;

#ifdef CONFIG_NET_IPv4
    inet = bt_socket_server_listen(AF_INET, name, port);
    if (inet > 0) {
        ipoll = service_loop_poll_fd(inet, POLL_READABLE,
                                     bt_socket_server_callback, NULL);
    }

    if (inet <= 0 || ipoll == NULL)
        goto fail;
#endif
#ifdef CONFIG_NET_RPMSG
    rpmsg = bt_socket_server_listen(AF_RPMSG, name, port);
    if (rpmsg > 0) {
        rpoll = service_loop_poll_fd(rpmsg, POLL_READABLE,
                                     bt_socket_server_callback, NULL);
    }
    if (rpmsg <= 0 || rpoll == NULL)
        goto fail;
#endif

    return OK;

fail:
    if (lpoll != NULL)
        service_loop_remove_poll(lpoll);
    if (local > 0)
        close(local);

#ifdef CONFIG_NET_IPv4
    if (ipoll != NULL)
        service_loop_remove_poll(ipoll);
    if (inet > 0)
        close(inet);
#endif

#ifdef CONFIG_NET_RPMSG
    if (rpoll != NULL)
        service_loop_remove_poll(rpoll);
    if (rpmsg > 0)
        close(rpmsg);
#endif

    return -EINVAL;
}
