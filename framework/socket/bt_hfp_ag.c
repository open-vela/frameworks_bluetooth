/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#define LOG_TAG "hfp_ag_api"

#include <stdint.h>

#include "bt_hfp_ag.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "hfp_ag_service.h"
#include "service_manager.h"
#include "utils/log.h"

void* bt_hfp_ag_register_callbacks(bt_instance_t* ins, const hfp_ag_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->hfp_ag_callbacks != NULL) {
        cookie = bt_remote_callbacks_register(ins->hfp_ag_callbacks, NULL, (void*)callbacks);
        return cookie;
    }

    ins->hfp_ag_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    cookie = bt_remote_callbacks_register(ins->hfp_ag_callbacks, NULL, (void*)callbacks);
    if (cookie == NULL) {
        bt_callbacks_list_free(ins->hfp_ag_callbacks);
        ins->hfp_ag_callbacks = NULL;
        return NULL;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_REGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hfp_ag_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->hfp_ag_callbacks);
        ins->hfp_ag_callbacks = NULL;
        return NULL;
    }

    return cookie;
}

bool bt_hfp_ag_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->hfp_ag_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->hfp_ag_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->hfp_ag_callbacks) > 0) {
        return true;
    }

    cbsl = ins->hfp_ag_callbacks;
    ins->hfp_ag_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_UNREGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hfp_ag_r.status != BT_STATUS_SUCCESS)
        return false;

    return true;
}

bool bt_hfp_ag_is_connected(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_is_connected.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_IS_CONNECTED);
    if (status != BT_STATUS_SUCCESS)
        return false;

    return packet.hfp_ag_r.value_bool;
}

bool bt_hfp_ag_is_audio_connected(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_is_audio_connected.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_IS_AUDIO_CONNECTED);
    if (status != BT_STATUS_SUCCESS)
        return false;

    return packet.hfp_ag_r.value_bool;
}

profile_connection_state_t bt_hfp_ag_get_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, PROFILE_STATE_DISCONNECTED);
    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_get_connection_state.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_GET_CONNECTION_STATE);
    if (status != BT_STATUS_SUCCESS)
        return PROFILE_STATE_DISCONNECTED;

    return packet.hfp_ag_r.profile_conn_state;
}

bt_status_t bt_hfp_ag_connect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_connect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_disconnect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_connect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_connect_audio.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_CONNECT_AUDIO);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_disconnect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_disconnect_audio.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_DISCONNECT_AUDIO);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t BTSYMBOLS(bt_hfp_ag_start_virtual_call)(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_start_virtual_call.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_START_VIRTUAL_CALL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t BTSYMBOLS(bt_hfp_ag_stop_virtual_call)(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_stop_virtual_call.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_STOP_VIRTUAL_CALL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_start_voice_recognition(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_start_voice_recognition.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_START_VOICE_RECOGNITION);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_stop_voice_recognition(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_stop_voice_recognition.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_STOP_VOICE_RECOGNITION);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_phone_state_change(bt_instance_t* ins, bt_address_t* addr,
    uint8_t num_active, uint8_t num_held,
    hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.num_active = num_active;
    packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.num_held = num_held;
    packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.call_state = call_state;
    packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.type = type;
    if (number)
        strlcpy(packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.number, number, sizeof(packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.number));
    if (name)
        strlcpy(packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.name, name, sizeof(packet.hfp_ag_pl._bt_hfp_ag_phone_state_change.name));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_PHONE_STATE_CHANGE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_notify_device_status(bt_instance_t* ins, bt_address_t* addr,
    hfp_network_state_t network, hfp_roaming_state_t roam,
    uint8_t signal, uint8_t battery)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_notify_device_status.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_pl._bt_hfp_ag_notify_device_status.network = network;
    packet.hfp_ag_pl._bt_hfp_ag_notify_device_status.roam = roam;
    packet.hfp_ag_pl._bt_hfp_ag_notify_device_status.signal = signal;
    packet.hfp_ag_pl._bt_hfp_ag_notify_device_status.battery = battery;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_NOTIFY_DEVICE_STATUS);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_volume_control(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_volume_control.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_pl._bt_hfp_ag_volume_control.type = type;
    packet.hfp_ag_pl._bt_hfp_ag_volume_control.volume = volume;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_VOLUME_CONTROL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_send_at_command(bt_instance_t* ins, bt_address_t* addr, const char* at_command)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_send_at_cmd.addr, addr, sizeof(bt_address_t));
    strncpy(packet.hfp_ag_pl._bt_hfp_ag_send_at_cmd.cmd, at_command, HFP_AT_LEN_MAX);
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_SEND_AT_COMMAND);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_send_vendor_specific_at_command(bt_instance_t* ins, bt_address_t* addr, const char* command, const char* value)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.addr, addr, sizeof(bt_address_t));
    strlcpy(packet.hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.cmd, command, sizeof(packet.hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.cmd));
    strlcpy(packet.hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.value, value, sizeof(packet.hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.value));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_SEND_VENDOR_SPECIFIC_AT_COMMAND);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}

bt_status_t bt_hfp_ag_send_clcc_response(bt_instance_t* ins, bt_address_t* addr, uint32_t index,
    hfp_call_direction_t dir, hfp_ag_call_state_t state, hfp_call_mode_t mode,
    hfp_call_mpty_type_t mpty, hfp_call_addrtype_t type, const char* number)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.index = index;
    packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.dir = dir;
    packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.state = state;
    packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.mode = mode;
    packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.mpty = mpty;
    packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.type = type;

    if (number) {
        strlcpy(packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.number, number, sizeof(packet.hfp_ag_pl._bt_hfp_ag_send_clcc_response.number));
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_AG_SEND_CLCC_RESPONSE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_ag_r.status;
}