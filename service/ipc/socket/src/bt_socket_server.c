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

#define LOG_TAG "bt_socket_server"

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
#include "service_manager.h"

#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
static bt_list_t* g_instances_list = NULL;
/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
    struct list_node node;
    int offset;
    bt_message_packet_t packet;
} bt_packet_cache_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool ins_compare(void* data, void* context)
{
    return data == context;
}

static bool bt_socket_server_is_ins_detached(bt_instance_t* ins)
{
    return (bt_list_find(g_instances_list, ins_compare, ins) == NULL);
}

static int bt_socket_server_send_internal(bt_instance_t* ins,
    void* packet, int size, int offset)
{
    int ret;

    ret = send(ins->peer_fd, (char*)packet + offset, size, 0);
    if (ret == 0) {
        return -1;
    } else if (ret < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            ret = 0;
        } else {
            return -1;
        }
    }

    return ret;
}

static int bt_socket_server_trysend(bt_instance_t* ins)
{
    bt_packet_cache_t* cache;
    struct list_node* node;
    struct list_node* tmp;
    bool reset = false;
    int size;
    int ret;

    list_for_every_safe(&ins->msg_queue, node, tmp)
    {
        reset = true;
        cache = (bt_packet_cache_t*)node;
        size = sizeof(cache->packet) - cache->offset;
        ret = bt_socket_server_send_internal(ins, &cache->packet,
            size, cache->offset);
        if (ret < 0) {
            service_loop_remove_poll(ins->poll);
            ins->poll = NULL;
            list_delete(node);
            free(node);
            break;
        } else if (ret != size) {
            cache->offset += ret;
            break;
        } else {
            list_delete(node);
            free(node);
        }
    }

    if (list_length(&ins->msg_queue) > 0) {
        if (ins->poll) {
            service_loop_reset_poll(ins->poll, POLL_READABLE | POLL_WRITABLE);
        }
        return -1;
    } else if (reset && ins->poll) {
        service_loop_reset_poll(ins->poll, POLL_READABLE);
    }

    return 0;
}

static int bt_socket_server_receive(service_poll_t* poll, int fd, void* userdata)
{
    bt_instance_t* ins = userdata;
    bt_message_packet_t* packet = (bt_message_packet_t*)ins->packet;
    int ret;

    ret = recv(fd, (uint8_t*)packet + ins->offset, sizeof(*packet) - ins->offset, 0);
    if (ret == 0) {
        BT_LOGE("%s, bt socket disconnected", __func__);
        return -1;
    } else if (ret < 0) {
        if (errno == EINTR || errno == EAGAIN)
            return 0;
        BT_LOGE("%s, bt socket recv ret: %d error: %d", __func__, ret, errno);
        return -1;
    }

    ins->offset += ret;
    if (ins->offset < sizeof(*packet))
        return 0;
    else
        ins->offset = 0;

    if (packet->code > BT_MANAGER_MESSAGE_START && packet->code < BT_MANAGER_MESSAGE_END) {
        bt_socket_server_manager_process(poll, fd, ins, packet);
    } else if (packet->code > BT_ADAPTER_MESSAGE_START && packet->code < BT_ADAPTER_MESSAGE_END) {
        bt_socket_server_adapter_process(poll, fd, ins, packet);
    } else if (packet->code > BT_DEVICE_MESSAGE_START && packet->code < BT_DEVICE_MESSAGE_END) {
        bt_socket_server_device_process(poll, fd, ins, packet);
    } else if (packet->code > BT_A2DP_SOURCE_MESSAGE_START && packet->code < BT_A2DP_SOURCE_MESSAGE_END) {
        bt_socket_server_a2dp_source_process(poll, fd, ins, packet);
    } else if (packet->code > BT_A2DP_SINK_MESSAGE_START && packet->code < BT_A2DP_SINK_MESSAGE_END) {
        bt_socket_server_a2dp_sink_process(poll, fd, ins, packet);
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    } else if (packet->code > BT_AVRCP_TARGET_MESSAGE_START && packet->code < BT_AVRCP_TARGET_MESSAGE_END) {
        bt_socket_server_avrcp_target_process(poll, fd, ins, packet);
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    } else if (packet->code > BT_AVRCP_CONTROL_MESSAGE_START && packet->code < BT_AVRCP_CONTROL_MESSAGE_END) {
        bt_socket_server_avrcp_control_process(poll, fd, ins, packet);
#endif
    } else if (packet->code > BT_HFP_AG_MESSAGE_START && packet->code < BT_HFP_AG_MESSAGE_END) {
        bt_socket_server_hfp_ag_process(poll, fd, ins, packet);
    } else if (packet->code > BT_HFP_HF_MESSAGE_START && packet->code < BT_HFP_HF_MESSAGE_END) {
        bt_socket_server_hfp_hf_process(poll, fd, ins, packet);
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    } else if (packet->code > BT_ADVERTISER_MESSAGE_START && packet->code < BT_ADVERTISER_MESSAGE_END) {
        bt_socket_server_advertiser_process(poll, fd, ins, packet);
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    } else if (packet->code > BT_SCAN_MESSAGE_START && packet->code < BT_SCAN_MESSAGE_END) {
        bt_socket_server_scan_process(poll, fd, ins, packet);
#endif
#if defined(CONFIG_BLUETOOTH_GATT)
    } else if (packet->code > BT_GATT_CLIENT_MESSAGE_START && packet->code < BT_GATT_CLIENT_MESSAGE_END) {
        bt_socket_server_gattc_process(poll, fd, ins, packet);
    } else if (packet->code > BT_GATT_SERVER_MESSAGE_START && packet->code < BT_GATT_SERVER_MESSAGE_END) {
        bt_socket_server_gatts_process(poll, fd, ins, packet);
#endif
    } else if (packet->code > BT_SPP_MESSAGE_START && packet->code < BT_SPP_MESSAGE_END) {
        bt_socket_server_spp_process(poll, fd, ins, packet);
    } else if (packet->code > BT_PAN_MESSAGE_START && packet->code < BT_PAN_MESSAGE_END) {
        bt_socket_server_pan_process(poll, fd, ins, packet);
    } else if (packet->code > BT_HID_DEVICE_MESSAGE_START && packet->code < BT_HID_DEVICE_MESSAGE_END) {
        bt_socket_server_hid_device_process(poll, fd, ins, packet);
#ifdef CONFIG_BLUETOOTH_L2CAP
    } else if (packet->code > BT_L2CAP_MESSAGE_START && packet->code < BT_L2CAP_MESSAGE_END) {
        bt_socket_server_l2cap_process(poll, fd, ins, packet);
#endif
    } else if (packet->code > BT_LOG_MESSAGE_START && packet->code < BT_LOG_MESSAGE_END) {
        bt_socket_server_log_process(poll, fd, ins, packet);
    } else {
        BT_LOGE("%s, Unhandled message:%" PRIu32, __func__, packet->code);
        assert(0);
        return BT_STATUS_PARM_INVALID;
    }

    return bt_socket_server_send(ins, packet, packet->code);
}

static void bt_unregister_callbacks(bt_instance_t* ins)
{
    bt_message_packet_t packet;
    profile_msg_t msg;

    // unreigster adapter callback
    packet.code = BT_ADAPTER_UNREGISTER_CALLBACK;
    bt_socket_server_adapter_process(ins->poll, ins->peer_fd, ins, &packet);

    // unregsiter profile callback
    msg.event = PROFILE_EVT_REMOTE_DETACH;
    msg.data.data = ins;
    service_manager_processmsg(&msg);

    // TODO: unregister other Profile callback(GATT, LE ADV, LE SCAN)
}

static void bt_socket_server_ins_release(bt_instance_t* ins)
{
    struct list_node* node;
    struct list_node* tmp;

    bt_unregister_callbacks(ins);

    if (ins->poll)
        service_loop_remove_poll(ins->poll);

    list_for_every_safe(&ins->msg_queue, node, tmp)
    {
        list_delete(node);
        free(node);
    }

    if (ins->peer_fd)
        close(ins->peer_fd);

    if (ins->packet)
        free(ins->packet);

    bt_list_remove(g_instances_list, ins);
    free(ins);
}

static void bt_socket_server_handle_event(service_poll_t* poll,
    int revent, void* userdata)
{
    uv_os_fd_t fd;
    int ret;
    bt_instance_t* ins = userdata;

    ret = uv_fileno((uv_handle_t*)&poll->handle, &fd);
    if (ret) {
        bt_socket_server_ins_release(ins);
        return;
    }

    if (revent & POLL_ERROR || revent & POLL_DISCONNECT) {
        BT_LOGE("%s, revent = %d", __func__, revent);
        bt_socket_server_ins_release(ins);
    } else if (revent & POLL_READABLE) {
        ret = bt_socket_server_receive(poll, fd, userdata);
        if (ret)
            bt_socket_server_ins_release(ins);
    } else if (revent & POLL_WRITABLE) {
        bt_socket_server_trysend(ins);
    }
}

static void bt_socket_server_callback(service_poll_t* poll,
    int revent, void* userdata)
{
    bt_instance_t* remote_ins;
    uv_os_fd_t fd;
    int ret;

    ret = uv_fileno((uv_handle_t*)&poll->handle, &fd);
    if (ret) {
        service_loop_remove_poll(poll);
        return;
    }

    fd = accept(fd, NULL, NULL);
    if (fd < 0)
        return;

    if (revent & POLL_ERROR || revent & POLL_DISCONNECT) {
        service_loop_remove_poll(poll);
        close(fd);
        return;
    }

#ifdef CONFIG_NET_SOCKOPTS
    setSocketBuf(fd, SO_RCVBUF);
    setSocketBuf(fd, SO_SNDBUF);
#endif

    remote_ins = zalloc(sizeof(bt_instance_t));
    if (!remote_ins)
        goto error;

    remote_ins->packet = zalloc(sizeof(bt_message_packet_t));
    if (!remote_ins->packet)
        goto error;

    list_initialize(&remote_ins->msg_queue);
    remote_ins->peer_fd = fd;
    remote_ins->poll = service_loop_poll_fd(fd, POLL_READABLE | POLL_DISCONNECT,
        bt_socket_server_handle_event, remote_ins);
    if (!remote_ins->poll)
        goto error;

    bt_list_add_tail(g_instances_list, remote_ins);
    return;

error:
    if (fd >= 0)
        close(fd);
    if (remote_ins) {
        if (remote_ins->packet)
            free(remote_ins->packet);
        free(remote_ins);
    }
}

static int bt_socket_server_listen(int family, const char* name, int port)
{
    union {
        struct sockaddr_in inet_addr;
        struct sockaddr_un local_addr;
#ifdef CONFIG_NET_RPMSG
        struct sockaddr_rpmsg rpmsg_addr;
#endif
    } u = { 0 };
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
#ifdef CONFIG_NET_RPMSG
    } else if (family == AF_RPMSG) {
        u.rpmsg_addr.rp_family = AF_RPMSG;
        snprintf(u.rpmsg_addr.rp_name, RPMSG_SOCKET_NAME_SIZE,
            BLUETOOTH_SOCKADDR_NAME, name);
        strcpy(u.rpmsg_addr.rp_cpu, "");
        addr_len = sizeof(struct sockaddr_rpmsg);
#endif
    } else {
        close(fd);
        return -EPFNOSUPPORT;
    }

    ret = bind(fd, (struct sockaddr*)&u, addr_len);
    if (ret >= 0)
        ret = listen(fd, BLUETOOTH_SERVER_MAXCONN);

    if (ret < 0) {
        close(fd);
        return -errno;
    }

    return fd;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int bt_socket_server_send(bt_instance_t* ins, bt_message_packet_t* packet,
    bt_message_type_t code)
{
    bt_packet_cache_t* cache;
    int ret;

    if (bt_socket_server_is_ins_detached(ins))
        return -1;

    packet->code = code;

    ret = bt_socket_server_trysend(ins);
    if (ret == 0) {
        ret = bt_socket_server_send_internal(ins, packet, sizeof(*packet), 0);
        if (ret < 0) {
            service_loop_remove_poll(ins->poll);
            ins->poll = NULL;
            return ret;
        }
    } else {
        ret = 0;
    }

    if (ret != sizeof(*packet) && ins->poll) {
        cache = malloc(sizeof(*cache));
        if (cache == NULL)
            return BT_STATUS_NOMEM;

        list_add_tail(&ins->msg_queue, &cache->node);
        memcpy(&cache->packet, packet, sizeof(*packet));
        cache->offset = ret;

        service_loop_reset_poll(ins->poll, POLL_READABLE | POLL_WRITABLE);
    }

    return 0;
}

int bt_socket_server_init(const char* name, int port)
{
    service_poll_t* lpoll = NULL;
    int local;
#ifdef CONFIG_BLUETOOTH_NET_IPv4
    service_poll_t* ipoll = NULL;
    int inet = -1;
#endif
#ifdef CONFIG_NET_RPMSG
    service_poll_t* rpoll = NULL;
    int rpmsg = -1;
#endif

    g_instances_list = bt_list_new(NULL);
    local = bt_socket_server_listen(PF_LOCAL, name, port);
    if (local <= 0)
        goto fail;

    lpoll = service_loop_poll_fd(local, POLL_READABLE,
        bt_socket_server_callback, NULL);
    if (lpoll == NULL)
        goto fail;

#ifdef CONFIG_BLUETOOTH_NET_IPv4
    inet = bt_socket_server_listen(AF_INET, name, port);
    if (inet <= 0)
        goto fail;

    ipoll = service_loop_poll_fd(inet, POLL_READABLE,
        bt_socket_server_callback, NULL);
    if (ipoll == NULL)
        goto fail;
#endif
#ifdef CONFIG_NET_RPMSG
    rpmsg = bt_socket_server_listen(AF_RPMSG, name, port);
    if (rpmsg <= 0)
        goto fail;

    rpoll = service_loop_poll_fd(rpmsg, POLL_READABLE,
        bt_socket_server_callback, NULL);
    if (rpoll == NULL)
        goto fail;
#endif

    return OK;

fail:
    if (g_instances_list)
        bt_list_free(g_instances_list);
    if (lpoll != NULL)
        service_loop_remove_poll(lpoll);
    if (local > 0)
        close(local);

#ifdef CONFIG_BLUETOOTH_NET_IPv4
    if (ipoll != NULL)
        service_loop_remove_poll(ipoll);
    if (inet > 0)
        close(inet);
#endif

#ifdef CONFIG_NET_RPMSG
    /* rpoll must be NULL at this position */
    if (rpmsg > 0)
        close(rpmsg);
#endif

    return -EINVAL;
}
