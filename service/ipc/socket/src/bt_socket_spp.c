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
#include <sys/un.h>

#include "bt_internal.h"

#include "bluetooth.h"
#include "bt_config.h"
#include "bt_debug.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "bt_spp.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->spp_callbacks)

#ifdef CONFIG_RPMSG_UART
#define SPP_UART_DEV "/dev/ttyDROID"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
#include "service_manager.h"
#include "spp_service.h"

static void spp_proxy_state_cb(void* handle, bt_address_t* addr, spp_proxy_state_t state, uint16_t scn, uint16_t port, char* name)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = handle;

    memcpy(&packet.spp_cb._proxy_state_cb.addr, addr, sizeof(*addr));
    packet.spp_cb._proxy_state_cb.state = state;
    packet.spp_cb._proxy_state_cb.scn = scn;
    packet.spp_cb._proxy_state_cb.port = port;
    if (name && strlen(name))
        strncpy(packet.spp_cb._proxy_state_cb.name, name, sizeof(packet.spp_cb._proxy_state_cb.name) - 1);

    bt_socket_server_send(ins, &packet, BT_SPP_PROXY_STATE_CB);
}

static void spp_connection_state_cb(void* handle, bt_address_t* addr,
    uint16_t scn, uint16_t port,
    profile_connection_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = handle;

    memcpy(&packet.spp_cb._connection_state_cb.addr, addr, sizeof(*addr));
    packet.spp_cb._connection_state_cb.scn = scn;
    packet.spp_cb._connection_state_cb.port = port;
    packet.spp_cb._connection_state_cb.state = state;

    bt_socket_server_send(ins, &packet, BT_SPP_CONNECTION_STATE_CB);
}
static spp_callbacks_t g_spp_socket_cb = {
    .size = sizeof(g_spp_socket_cb),
    .connection_state_cb = spp_connection_state_cb,
    .proxy_state_cb = spp_proxy_state_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_spp_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    spp_interface_t* profile = (spp_interface_t*)service_manager_get_profile(PROFILE_SPP);

    switch (packet->code) {
    case BT_SPP_REGISTER_APP: {
        if (ins->spp_cookie == NULL) {
            ins->spp_cookie = profile->register_app(ins, packet->spp_pl._bt_spp_register_app.name_len ? packet->spp_pl._bt_spp_register_app.name : NULL, &g_spp_socket_cb);
            packet->spp_r.handle = PTR2INT(uint64_t) ins->spp_cookie;
        } else {
            packet->spp_r.handle = 0;
        }
        break;
    }
    case BT_SPP_UNREGISTER_APP: {
        if (ins->spp_cookie) {
            void* handle = NULL;
            packet->spp_r.status = profile->unregister_app(&handle, ins->spp_cookie);
            ins->spp_cookie = NULL;
        }
        break;
    }
    case BT_SPP_SERVER_START: {
        packet->spp_r.status = profile->server_start(ins->spp_cookie,
            packet->spp_pl._bt_spp_server_start.scn,
            &packet->spp_pl._bt_spp_server_start.uuid,
            packet->spp_pl._bt_spp_server_start.max_connection);
        break;
    }
    case BT_SPP_SERVER_STOP: {
        packet->spp_r.status = profile->server_stop(ins->spp_cookie,
            packet->spp_pl._bt_spp_server_stop.scn);
        break;
    }
    case BT_SPP_CONNECT: {
        packet->spp_r.status = profile->connect(ins->spp_cookie,
            &packet->spp_pl._bt_spp_connect.addr,
            packet->spp_pl._bt_spp_connect.scn,
            &packet->spp_pl._bt_spp_connect.uuid,
            &packet->spp_pl._bt_spp_connect.port);
        break;
    }
    case BT_SPP_DISCONNECT: {
        packet->spp_r.status = profile->disconnect(ins->spp_cookie,
            &packet->spp_pl._bt_spp_disconnect.addr,
            packet->spp_pl._bt_spp_disconnect.port);
        break;
    }
    default:
        break;
    }
}
#endif

#if !defined(CONFIG_BLUETOOTH_SERVER) && defined(CONFIG_BLUETOOTH_RPMSG_CPUNAME)
static bool rpmsg_tty_mount_path(const char* src, char* dest, int len, const char* mount_cpu)
{
    char* path = strstr(src, "/dev/");

    if (!path || path != src) {
        return false;
    }

#if defined(CONFIG_RPMSG_UART)
    strlcpy(dest, SPP_UART_DEV, len);
#else
    if (snprintf(dest, len, "/dev/%s/%s", mount_cpu, src + 5) < 0)
        return false;
#endif

    return true;
}
#endif

int bt_socket_client_spp_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_SPP_PROXY_STATE_CB: {
        char* name = packet->spp_cb._proxy_state_cb.name;
#if !defined(CONFIG_BLUETOOTH_SERVER) && defined(CONFIG_BLUETOOTH_RPMSG_CPUNAME)
        char rename[64];
        if (rpmsg_tty_mount_path(name, rename, 64, CONFIG_BLUETOOTH_RPMSG_CPUNAME))
            name = rename;
#endif
        CALLBACK_FOREACH(CBLIST, spp_callbacks_t,
            proxy_state_cb,
            &packet->spp_cb._proxy_state_cb.addr,
            packet->spp_cb._proxy_state_cb.state,
            packet->spp_cb._proxy_state_cb.scn,
            packet->spp_cb._proxy_state_cb.port,
            name);
        break;
    }
    case BT_SPP_CONNECTION_STATE_CB: {
        CALLBACK_FOREACH(CBLIST, spp_callbacks_t,
            connection_state_cb,
            &packet->spp_cb._connection_state_cb.addr,
            packet->spp_cb._connection_state_cb.scn,
            packet->spp_cb._connection_state_cb.port,
            packet->spp_cb._connection_state_cb.state);
        break;
    }

    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
