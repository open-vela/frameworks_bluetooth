/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "spp_api"

#include <stdint.h>

#include "bt_profile.h"
#include "bt_socket.h"
#include "bt_spp.h"
#include "service_manager.h"
#include "spp_service.h"
#include "utils/log.h"

void* bt_spp_register_app_with_name(bt_instance_t* ins, const char* name, const spp_callbacks_t* callbacks)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;
    void* handle;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->spp_callbacks != NULL) {
        return NULL;
    }

    ins->spp_callbacks = bt_callbacks_list_new(1);

    handle = bt_remote_callbacks_register(ins->spp_callbacks, NULL, (void*)callbacks);
    if (handle == NULL) {
        bt_callbacks_list_free(ins->spp_callbacks);
        ins->spp_callbacks = NULL;
        return NULL;
    }

    if (name) {
        packet.spp_pl._bt_spp_register_app.name_len = strlen(name);
        strlcpy(packet.spp_pl._bt_spp_register_app.name, name, sizeof(packet.spp_pl._bt_spp_register_app.name));
    } else
        packet.spp_pl._bt_spp_register_app.name_len = 0;

    status = bt_socket_client_sendrecv(ins, &packet, BT_SPP_REGISTER_APP);
    if (status != BT_STATUS_SUCCESS || !packet.spp_r.handle) {
        bt_callbacks_list_free(ins->spp_callbacks);
        ins->spp_callbacks = NULL;
        return NULL;
    }

    return handle;
}

void* bt_spp_register_app(bt_instance_t* ins, const spp_callbacks_t* callbacks)
{
    return bt_spp_register_app_with_name(ins, NULL, callbacks);
}

void* bt_spp_register_app_ext(bt_instance_t* ins, const char* name, int port_type, const spp_callbacks_t* callbacks)
{
    return bt_spp_register_app_with_name(ins, NULL, callbacks);
}

bt_status_t bt_spp_unregister_app(bt_instance_t* ins, void* handle)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (!ins->spp_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->spp_callbacks, NULL, handle);
    bt_socket_client_free_callbacks(ins, ins->spp_callbacks);
    ins->spp_callbacks = NULL;

    status = bt_socket_client_sendrecv(ins, &packet, BT_SPP_UNREGISTER_APP);
    if (status != BT_STATUS_SUCCESS || packet.spp_r.status != BT_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

bt_status_t bt_spp_server_start(bt_instance_t* ins, void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t max_connection)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.spp_pl._bt_spp_server_start.scn = scn;
    memcpy(&packet.spp_pl._bt_spp_server_start.uuid, uuid, sizeof(*uuid));
    packet.spp_pl._bt_spp_server_start.max_connection = max_connection;

    status = bt_socket_client_sendrecv(ins, &packet, BT_SPP_SERVER_START);
    if (status != BT_STATUS_SUCCESS || packet.spp_r.status != BT_STATUS_SUCCESS) {
        return packet.spp_r.status;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_spp_server_stop(bt_instance_t* ins, void* handle, uint16_t scn)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.spp_pl._bt_spp_server_stop.scn = scn;

    status = bt_socket_client_sendrecv(ins, &packet, BT_SPP_SERVER_STOP);
    if (status != BT_STATUS_SUCCESS || packet.spp_r.status != BT_STATUS_SUCCESS) {
        return packet.spp_r.status;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_spp_connect(bt_instance_t* ins, void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.spp_pl._bt_spp_connect.addr, addr, sizeof(*addr));
    packet.spp_pl._bt_spp_connect.scn = scn;
    memcpy(&packet.spp_pl._bt_spp_connect.uuid, uuid, sizeof(*uuid));

    status = bt_socket_client_sendrecv(ins, &packet, BT_SPP_CONNECT);
    if (status != BT_STATUS_SUCCESS || packet.spp_r.status != BT_STATUS_SUCCESS) {
        return packet.spp_r.status;
    }

    *port = packet.spp_pl._bt_spp_connect.port;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_spp_disconnect(bt_instance_t* ins, void* handle, bt_address_t* addr, uint16_t port)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.spp_pl._bt_spp_disconnect.addr, addr, sizeof(*addr));
    packet.spp_pl._bt_spp_disconnect.port = port;

    status = bt_socket_client_sendrecv(ins, &packet, BT_SPP_DISCONNECT);
    if (status != BT_STATUS_SUCCESS || packet.spp_r.status != BT_STATUS_SUCCESS) {
        return packet.spp_r.status;
    }

    return BT_STATUS_SUCCESS;
}
