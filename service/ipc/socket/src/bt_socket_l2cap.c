
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
#include "bt_l2cap.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "l2cap_service.h"
#include "manager_service.h"
#include "service_loop.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CBLIST (__async ? __async->l2cap_callbacks : ins->l2cap_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
#include "l2cap_service.h"
#include "service_manager.h"

static void on_connected_cb(void* cookie, l2cap_connect_params_t* param)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.l2cap_cb._connected_cb.addr, &param->addr, sizeof(packet.l2cap_cb._connected_cb.addr));
    packet.l2cap_cb._connected_cb.transport = param->transport;
    packet.l2cap_cb._connected_cb.cid = param->cid;
    packet.l2cap_cb._connected_cb.psm = param->psm;
    packet.l2cap_cb._connected_cb.incoming_mtu = param->incoming_mtu;
    packet.l2cap_cb._connected_cb.outgoing_mtu = param->outgoing_mtu;
    packet.l2cap_cb._connected_cb.id = param->id;
    packet.l2cap_cb._connected_cb.listen_id = param->listen_id;
    strlcpy(packet.l2cap_cb._connected_cb.proxy_name, param->proxy_name, sizeof(packet.l2cap_cb._connected_cb.proxy_name));
    bt_socket_server_send(ins, &packet, BT_L2CAP_CONNECTED_CB);
}

static void on_disconnected_cb(void* cookie, bt_address_t* addr, uint16_t id, uint32_t reason)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.l2cap_cb._disconnected_cb.addr, addr, sizeof(packet.l2cap_cb._disconnected_cb.addr));
    packet.l2cap_cb._disconnected_cb.id = id;
    packet.l2cap_cb._disconnected_cb.reason = reason;
    bt_socket_server_send(ins, &packet, BT_L2CAP_DISCONNECTED_CB);
}

const static l2cap_callbacks_t g_l2cap_socket_cbs = {
    .on_connected = on_connected_cb,
    .on_disconnected = on_disconnected_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void bt_socket_server_l2cap_process(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_L2CAP_REGISTER_CALLBACKS:
        if (ins->l2cap_cookie == NULL) {
            ins->l2cap_cookie = l2cap_register_callbacks((void*)ins, (void*)&g_l2cap_socket_cbs);
            if (ins->l2cap_cookie)
                packet->l2cap_r.status = BT_STATUS_SUCCESS;
            else
                packet->l2cap_r.status = BT_STATUS_NO_RESOURCES;
        } else {
            packet->l2cap_r.status = BT_STATUS_BUSY;
        }
        break;
    case BT_L2CAP_UNREGISTER_CALLBACKS:
        if (ins->l2cap_cookie) {
            l2cap_unregister_callbacks((void**)&ins, ins->l2cap_cookie);
            ins->l2cap_cookie = NULL;
            packet->l2cap_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->l2cap_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case BT_L2CAP_LISTEN:
        packet->l2cap_r.status = BTSYMBOLS(bt_l2cap_listen)(ins, ins->l2cap_cookie,
            &packet->l2cap_pl._bt_l2cap_listen.option);
        break;
    case BT_L2CAP_CONNECT:
        packet->l2cap_r.status = BTSYMBOLS(bt_l2cap_connect)(ins, ins->l2cap_cookie,
            &packet->l2cap_pl._bt_l2cap_connect.addr,
            &packet->l2cap_pl._bt_l2cap_connect.option);
        break;
    case BT_L2CAP_DISCONNECT:
        packet->l2cap_r.status = BTSYMBOLS(bt_l2cap_disconnect)(ins, ins->l2cap_cookie,
            packet->l2cap_pl._bt_l2cap_disconnect.id);
        break;
    default:
        switch (BT_IPC_GET_SUBCODE(packet->code)) {
        case L2CAP_SUBCODE_STOP_LISTEN:
            packet->l2cap_r.status = BTSYMBOLS(bt_l2cap_stop_listen_with_transport)(ins,
                ins->l2cap_cookie,
                packet->l2cap_pl._bt_l2cap_stop_listen.transport,
                packet->l2cap_pl._bt_l2cap_stop_listen.psm);
            break;
        default:
            break;
        }
    }
}
#endif

int bt_socket_client_l2cap_callback(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    bt_socket_async_client_t* __async = NULL;

    if (is_async)
        __async = ins->priv;

    switch (packet->code) {
    case BT_L2CAP_CONNECTED_CB: {
        l2cap_connect_params_t conn_parm = {
            .transport = packet->l2cap_cb._connected_cb.transport,
            .cid = packet->l2cap_cb._connected_cb.cid,
            .psm = packet->l2cap_cb._connected_cb.psm,
            .incoming_mtu = packet->l2cap_cb._connected_cb.incoming_mtu,
            .outgoing_mtu = packet->l2cap_cb._connected_cb.outgoing_mtu,
            .id = packet->l2cap_cb._connected_cb.id,
            .listen_id = packet->l2cap_cb._connected_cb.listen_id,
        };
        strlcpy(conn_parm.proxy_name, packet->l2cap_cb._connected_cb.proxy_name, sizeof(conn_parm.proxy_name));
        memcpy(&conn_parm.addr, &packet->l2cap_cb._connected_cb.addr, sizeof(conn_parm.addr));
        CALLBACK_FOREACH(CBLIST, l2cap_callbacks_t,
            on_connected,
            &conn_parm);
        break;
    }
    case BT_L2CAP_DISCONNECTED_CB:
        CALLBACK_FOREACH(CBLIST, l2cap_callbacks_t,
            on_disconnected,
            &packet->l2cap_cb._disconnected_cb.addr,
            packet->l2cap_cb._disconnected_cb.id,
            packet->l2cap_cb._disconnected_cb.reason);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }
    return BT_STATUS_SUCCESS;
}
