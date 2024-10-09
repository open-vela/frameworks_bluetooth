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

#include <stdint.h>

#include "bt_a2dp_sink.h"
#include "bt_socket.h"

void* bt_a2dp_sink_register_callbacks(bt_instance_t* ins, const a2dp_sink_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* handle;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->a2dp_sink_callbacks != NULL) {
        handle = bt_remote_callbacks_register(ins->a2dp_sink_callbacks, NULL, (void*)callbacks);
        return handle;
    }

    ins->a2dp_sink_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

#ifdef CONFIG_BLUETOOTH_FEATURE
    handle = bt_remote_callbacks_register(ins->a2dp_sink_callbacks, ins, (void*)callbacks);
#else
    handle = bt_remote_callbacks_register(ins->a2dp_sink_callbacks, NULL, (void*)callbacks);
#endif

    if (handle == NULL) {
        bt_callbacks_list_free(ins->a2dp_sink_callbacks);
        ins->a2dp_sink_callbacks = NULL;
        return handle;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_REGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.a2dp_sink_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->a2dp_sink_callbacks);
        ins->a2dp_sink_callbacks = NULL;
        return NULL;
    }

    return handle;
}

bool bt_a2dp_sink_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);
    if (!ins->a2dp_sink_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->a2dp_sink_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->a2dp_sink_callbacks) > 0) {
        return true;
    }

    cbsl = ins->a2dp_sink_callbacks;
    ins->a2dp_sink_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_UNREGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.a2dp_sink_r.status != BT_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

bool bt_a2dp_sink_is_connected(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    memcpy(&packet.a2dp_sink_pl._bt_a2dp_sink_is_connected.addr, addr, sizeof(bt_address_t));

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_IS_CONNECTED);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    return packet.a2dp_sink_r.bbool;
}

bool bt_a2dp_sink_is_playing(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    memcpy(&packet.a2dp_sink_pl._bt_a2dp_sink_is_playing.addr, addr, sizeof(bt_address_t));

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_IS_PLAYING);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    return packet.a2dp_sink_r.bbool;
}

profile_connection_state_t bt_a2dp_sink_get_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, PROFILE_STATE_DISCONNECTED);
    memcpy(&packet.a2dp_sink_pl._bt_a2dp_sink_get_connection_state.addr, addr, sizeof(bt_address_t));

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_GET_CONNECTION_STATE);
    if (status != BT_STATUS_SUCCESS) {
        return PROFILE_STATE_DISCONNECTED;
    }

    return packet.a2dp_sink_r.state;
}

bt_status_t bt_a2dp_sink_connect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.a2dp_sink_pl._bt_a2dp_sink_connect.addr, addr, sizeof(bt_address_t));

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_CONNECT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.a2dp_sink_r.status;
}

bt_status_t bt_a2dp_sink_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.a2dp_sink_pl._bt_a2dp_sink_disconnect.addr, addr, sizeof(bt_address_t));

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_DISCONNECT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.a2dp_sink_r.status;
}

bt_status_t bt_a2dp_sink_set_active_device(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.a2dp_sink_pl._bt_a2dp_sink_set_active_device.addr, addr, sizeof(bt_address_t));

    status = bt_socket_client_sendrecv(ins, &packet, BT_A2DP_SINK_SET_ACTIVE_DEVICE);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.a2dp_sink_r.status;
}
