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
#define LOG_TAG "pan_api"

#include <stdint.h>

#include "bt_pan.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "pan_service.h"
#include "service_manager.h"
#include "utils/log.h"

void* bt_pan_register_callbacks(bt_instance_t* ins, const pan_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* handle;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->panu_callbacks != NULL) {
        handle = bt_remote_callbacks_register(ins->panu_callbacks, NULL, (void*)callbacks);
        return handle;
    }

    ins->panu_callbacks = bt_callbacks_list_new(1);

    handle = bt_remote_callbacks_register(ins->panu_callbacks, NULL, (void*)callbacks);
    if (handle == NULL) {
        bt_callbacks_list_free(ins->panu_callbacks);
        ins->panu_callbacks = NULL;
        return NULL;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_PAN_REGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.pan_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->panu_callbacks);
        ins->panu_callbacks = NULL;
        return NULL;
    }

    return handle;
}

bool bt_pan_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->panu_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->panu_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->panu_callbacks) > 0) {
        return true;
    }

    cbsl = ins->panu_callbacks;
    ins->panu_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_PAN_UNREGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.pan_r.status != BT_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

bt_status_t bt_pan_connect(bt_instance_t* ins, bt_address_t* addr, uint8_t dst_role, uint8_t src_role)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.pan_pl._bt_pan_connect.addr, addr, sizeof(*addr));
    packet.pan_pl._bt_pan_connect.dst_role = dst_role;
    packet.pan_pl._bt_pan_connect.src_role = src_role;

    status = bt_socket_client_sendrecv(ins, &packet, BT_PAN_CONNECT);
    if (status != BT_STATUS_SUCCESS || packet.pan_r.status != BT_STATUS_SUCCESS) {
        return packet.pan_r.status;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_pan_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.pan_pl._bt_pan_disconnect.addr, addr, sizeof(*addr));

    status = bt_socket_client_sendrecv(ins, &packet, BT_PAN_DISCONNECT);
    if (status != BT_STATUS_SUCCESS || packet.pan_r.status != BT_STATUS_SUCCESS) {
        return packet.pan_r.status;
    }

    return BT_STATUS_SUCCESS;
}
