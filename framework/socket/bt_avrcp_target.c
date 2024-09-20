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
#define LOG_TAG "avrcp_target_api"

#include <stdint.h>

#include "bt_avrcp_target.h"
#include "bt_socket.h"

void* bt_avrcp_target_register_callbacks(bt_instance_t* ins, const avrcp_target_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->avrcp_target_callbacks != NULL) {
        cookie = bt_remote_callbacks_register(ins->avrcp_target_callbacks, NULL, (void*)callbacks);
        return cookie;
    }

    ins->avrcp_target_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    cookie = bt_remote_callbacks_register(ins->avrcp_target_callbacks, NULL, (void*)callbacks);
    if (cookie == NULL) {
        bt_callbacks_list_free(ins->avrcp_target_callbacks);
        ins->avrcp_target_callbacks = NULL;
        return cookie;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_AVRCP_TARGET_REGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.avrcp_target_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->avrcp_target_callbacks);
        ins->avrcp_target_callbacks = NULL;
        return NULL;
    }

    return cookie;
}

bool bt_avrcp_target_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->avrcp_target_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->avrcp_target_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->avrcp_target_callbacks) > 0) {
        return true;
    }

    bt_socket_client_free_callbacks(ins, ins->avrcp_target_callbacks);
    ins->avrcp_target_callbacks = NULL;

    status = bt_socket_client_sendrecv(ins, &packet, BT_AVRCP_TARGET_UNREGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.avrcp_target_r.status != BT_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

bt_status_t bt_avrcp_target_get_play_status_response(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t play_status,
    uint32_t song_len, uint32_t song_pos)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.avrcp_target_pl._bt_avrcp_target_get_play_status_response.addr, addr, sizeof(packet.avrcp_target_pl._bt_avrcp_target_get_play_status_response.addr));
    packet.avrcp_target_pl._bt_avrcp_target_get_play_status_response.play_status = play_status;
    packet.avrcp_target_pl._bt_avrcp_target_get_play_status_response.song_len = song_len;
    packet.avrcp_target_pl._bt_avrcp_target_get_play_status_response.song_pos = song_pos;
    status = bt_socket_client_sendrecv(ins, &packet, BT_AVRCP_TARGET_GET_PLAY_STATUS_RESPONSE);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.avrcp_target_r.status;
}

bt_status_t bt_avrcp_target_play_status_notify(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t play_status)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.avrcp_target_pl._bt_avrcp_target_play_status_notify.addr, addr, sizeof(packet.avrcp_target_pl._bt_avrcp_target_play_status_notify.addr));
    packet.avrcp_target_pl._bt_avrcp_target_play_status_notify.play_status = play_status;
    status = bt_socket_client_sendrecv(ins, &packet, BT_AVRCP_TARGET_PLAY_STATUS_NOTIFY);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.avrcp_target_r.status;
}
