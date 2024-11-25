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
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->panu_callbacks)
/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
#include "pan_service.h"
#include "service_manager.h"

static void pan_netif_state_cb(void* cookie, pan_netif_state_t state,
    int local_role, const char* ifname)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    packet.pan_cb._netif_state_cb.state = state;
    packet.pan_cb._netif_state_cb.local_role = local_role;
    if (ifname && strlen(ifname))
        strncpy(packet.pan_cb._netif_state_cb.ifname, ifname, sizeof(packet.pan_cb._netif_state_cb.ifname) - 1);

    bt_socket_server_send(ins, &packet, BT_PAN_NETIF_STATE_CB);
}

static void pan_connection_state_cb(void* cookie, profile_connection_state_t state,
    bt_address_t* bd_addr, uint8_t local_role,
    uint8_t remote_role)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    packet.pan_cb._connection_state_cb.state = state;
    memcpy(&packet.pan_cb._connection_state_cb.bd_addr, bd_addr, sizeof(*bd_addr));
    packet.pan_cb._connection_state_cb.local_role = local_role;
    packet.pan_cb._connection_state_cb.remote_role = remote_role;

    bt_socket_server_send(ins, &packet, BT_PAN_CONNECTION_STATE_CB);
}

static pan_callbacks_t g_pan_socket_cbs = {
    sizeof(g_pan_socket_cbs),
    pan_netif_state_cb,
    pan_connection_state_cb,
};
/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_pan_process(service_poll_t* poll, int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_PAN_REGISTER_CALLBACKS: {
        if (ins->panu_cookie == NULL) {
            pan_interface_t* pan = (pan_interface_t*)service_manager_get_profile(PROFILE_PANU);
            ins->panu_cookie = pan->register_callbacks(ins, (void*)&g_pan_socket_cbs);
            if (ins->panu_cookie) {
                packet->pan_r.status = BT_STATUS_SUCCESS;
            }
        }
        break;
    }
    case BT_PAN_UNREGISTER_CALLBACKS: {
        if (ins->panu_cookie) {
            pan_interface_t* pan = (pan_interface_t*)service_manager_get_profile(PROFILE_PANU);
            pan->unregister_callbacks((void**)&ins, ins->panu_cookie);
            ins->panu_cookie = NULL;
        }
        break;
    }
    case BT_PAN_CONNECT: {
        packet->pan_r.status = BTSYMBOLS(bt_pan_connect)(ins, &packet->pan_pl._bt_pan_connect.addr,
            packet->pan_pl._bt_pan_connect.dst_role,
            packet->pan_pl._bt_pan_connect.src_role);
        break;
    }
    case BT_PAN_DISCONNECT: {
        packet->pan_r.status = BTSYMBOLS(bt_pan_disconnect)(ins, &packet->pan_pl._bt_pan_connect.addr);
        break;
    }
    default:
        break;
    }
}
#endif

int bt_socket_client_pan_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_PAN_NETIF_STATE_CB: {
        {
            CALLBACK_FOREACH(CBLIST, pan_callbacks_t,
                netif_state_cb,
                packet->pan_cb._netif_state_cb.state,
                packet->pan_cb._netif_state_cb.local_role,
                packet->pan_cb._netif_state_cb.ifname);
            break;
        }
        break;
    }
    case BT_PAN_CONNECTION_STATE_CB: {
        CALLBACK_FOREACH(CBLIST, pan_callbacks_t,
            connection_state_cb,
            packet->pan_cb._connection_state_cb.state,
            &packet->pan_cb._connection_state_cb.bd_addr,
            packet->pan_cb._connection_state_cb.local_role,
            packet->pan_cb._connection_state_cb.remote_role);
        break;
    }
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
