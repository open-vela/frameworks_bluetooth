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
#define LOG_TAG "L2CAP_api"

#include <stdint.h>

#include "bt_l2cap.h"
#include "bt_socket.h"
#include "l2cap_service.h"
#include "utils/log.h"

void* bt_l2cap_register_callbacks(bt_instance_t* ins, const l2cap_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* handle;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->l2cap_callbacks != NULL) {
        return NULL;
    }

    ins->l2cap_callbacks = bt_callbacks_list_new(1);
    if (!ins->l2cap_callbacks) {
        return NULL;
    }

    handle = bt_remote_callbacks_register(ins->l2cap_callbacks, NULL, (void*)callbacks);
    if (handle == NULL) {
        bt_callbacks_list_free(ins->l2cap_callbacks);
        ins->l2cap_callbacks = NULL;
        return NULL;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_REGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.l2cap_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->l2cap_callbacks);
        ins->l2cap_callbacks = NULL;
        return NULL;
    }

    return handle;
}

bool bt_l2cap_unregister_callbacks(bt_instance_t* ins, void* handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->l2cap_callbacks) {
        return false;
    }

    bt_remote_callbacks_unregister(ins->l2cap_callbacks, NULL, handle);

    cbsl = ins->l2cap_callbacks;
    ins->l2cap_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_UNREGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.l2cap_r.status != BT_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

bt_status_t bt_l2cap_listen(bt_instance_t* ins, void* handle, l2cap_config_option_t* option)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.l2cap_pl._bt_l2cap_listen.option, option, sizeof(packet.l2cap_pl._bt_l2cap_listen.option));
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_LISTEN);
    if (status != BT_STATUS_SUCCESS)
        return status;

    option->psm = packet.l2cap_pl._bt_l2cap_listen.option.psm;
    option->id = packet.l2cap_pl._bt_l2cap_listen.option.id;
    strlcpy(option->proxy_name, packet.l2cap_pl._bt_l2cap_listen.option.proxy_name, sizeof(option->proxy_name));

    return packet.l2cap_r.status;
}

bt_status_t bt_l2cap_connect(bt_instance_t* ins, void* handle, bt_address_t* addr, l2cap_config_option_t* option)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.l2cap_pl._bt_l2cap_connect.addr, addr, sizeof(packet.l2cap_pl._bt_l2cap_connect.addr));
    memcpy(&packet.l2cap_pl._bt_l2cap_connect.option, option, sizeof(packet.l2cap_pl._bt_l2cap_connect.option));
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    option->id = packet.l2cap_pl._bt_l2cap_connect.option.id;
    strlcpy(option->proxy_name, packet.l2cap_pl._bt_l2cap_connect.option.proxy_name, sizeof(option->proxy_name));

    return packet.l2cap_r.status;
}

bt_status_t bt_l2cap_disconnect(bt_instance_t* ins, void* handle, uint16_t id)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.l2cap_pl._bt_l2cap_disconnect.id = id;
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.l2cap_r.status;
}

bt_status_t bt_l2cap_stop_listen(bt_instance_t* ins, void* handle, uint16_t psm)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.l2cap_pl._bt_l2cap_stop_listen.transport = BT_TRANSPORT_BLE;
    packet.l2cap_pl._bt_l2cap_stop_listen.psm = psm;
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_STOP_LISTEN);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.l2cap_r.status;
}

bt_status_t bt_l2cap_stop_listen_with_transport(bt_instance_t* ins, void* handle, bt_transport_t transport, uint16_t psm)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.l2cap_pl._bt_l2cap_stop_listen.transport = transport;
    packet.l2cap_pl._bt_l2cap_stop_listen.psm = psm;
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_STOP_LISTEN);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.l2cap_r.status;
}

bt_status_t bt_l2cap_send_echo_req(bt_instance_t* ins, void* handle, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.l2cap_pl._bt_l2cap_send_echo_req.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_SEND_ECHO_REQ);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.l2cap_r.status;
}

bt_status_t bt_l2cap_send_conf_req(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t cid)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.l2cap_pl._bt_l2cap_send_conf_req.addr, addr, sizeof(bt_address_t));
    packet.l2cap_pl._bt_l2cap_send_conf_req.cid = cid;
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_SEND_CONF_REQ);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.l2cap_r.status;
}

bt_status_t bt_l2cap_send_br_data(bt_instance_t* ins, void* handle,
    bt_address_t* addr, uint16_t cid, uint8_t* data, uint16_t len)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (len > sizeof(packet.l2cap_pl._bt_l2cap_send_br_data.data))
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.l2cap_pl._bt_l2cap_send_br_data.addr, addr, sizeof(bt_address_t));
    packet.l2cap_pl._bt_l2cap_send_br_data.cid = cid;
    packet.l2cap_pl._bt_l2cap_send_br_data.len = len;
    memcpy(packet.l2cap_pl._bt_l2cap_send_br_data.data, data, len);
    status = bt_socket_client_sendrecv(ins, &packet, BT_L2CAP_SEND_BR_DATA);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.l2cap_r.status;
}