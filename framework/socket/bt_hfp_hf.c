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
#define LOG_TAG "hfp_hf_api"

#include <stdint.h>

#include "bt_hfp_hf.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "hfp_hf_service.h"
#include "service_manager.h"
#include "utils/log.h"

void* bt_hfp_hf_register_callbacks(bt_instance_t* ins, const hfp_hf_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->hfp_hf_callbacks != NULL) {
        cookie = bt_remote_callbacks_register(ins->hfp_hf_callbacks, NULL, (void*)callbacks);
        return cookie;
    }

    ins->hfp_hf_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    cookie = bt_remote_callbacks_register(ins->hfp_hf_callbacks, NULL, (void*)callbacks);
    if (cookie == NULL) {
        bt_callbacks_list_free(ins->hfp_hf_callbacks);
        ins->hfp_hf_callbacks = NULL;
        return NULL;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_REGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hfp_hf_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->hfp_hf_callbacks);
        ins->hfp_hf_callbacks = NULL;
        return NULL;
    }

    return cookie;
}

bool bt_hfp_hf_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->hfp_hf_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->hfp_hf_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->hfp_hf_callbacks) > 0) {
        return true;
    }

    cbsl = ins->hfp_hf_callbacks;
    ins->hfp_hf_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_UNREGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hfp_hf_r.status != BT_STATUS_SUCCESS)
        return false;

    return true;
}

bool bt_hfp_hf_is_connected(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_is_connected.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_IS_CONNECTED);
    if (status != BT_STATUS_SUCCESS)
        return false;

    return packet.hfp_hf_r.value_bool;
}

bool bt_hfp_hf_is_audio_connected(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_is_audio_connected.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_IS_AUDIO_CONNECTED);
    if (status != BT_STATUS_SUCCESS)
        return false;

    return packet.hfp_hf_r.value_bool;
}

profile_connection_state_t bt_hfp_hf_get_connection_state(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, PROFILE_STATE_DISCONNECTED);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_get_connection_state.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_GET_CONNECTION_STATE);
    if (status != BT_STATUS_SUCCESS)
        return PROFILE_STATE_DISCONNECTED;

    return packet.hfp_hf_r.profile_conn_state;
}

bt_status_t bt_hfp_hf_connect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_connect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_disconnect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_set_connection_policy(bt_instance_t* ins, bt_address_t* addr, connection_policy_t policy)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_set_connection_policy.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_pl._bt_hfp_hf_set_connection_policy.policy = (uint8_t)policy;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_SET_CONNECTION_POLICY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_connect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_connect_audio.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_CONNECT_AUDIO);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_disconnect_audio(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_disconnect_audio.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_DISCONNECT_AUDIO);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_start_voice_recognition(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_start_voice_recognition.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_START_VOICE_RECOGNITION);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_stop_voice_recognition(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_stop_voice_recognition.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_STOP_VOICE_RECOGNITION);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_dial(bt_instance_t* ins, bt_address_t* addr, const char* number)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (strlen(number) > HFP_PHONENUM_DIGITS_MAX)
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_dial.addr, addr, sizeof(bt_address_t));
    strncpy(packet.hfp_hf_pl._bt_hfp_hf_dial.number, number, HFP_PHONENUM_DIGITS_MAX);
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_DIAL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_dial_memory(bt_instance_t* ins, bt_address_t* addr, uint32_t memory)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_dial_memory.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_pl._bt_hfp_hf_dial_memory.memory = memory;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_DIAL_MEMORY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_redial(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_redial.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_REDIAL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_accept_call(bt_instance_t* ins, bt_address_t* addr, hfp_call_accept_t flag)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_accept_call.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_pl._bt_hfp_hf_accept_call.flag = flag;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_ACCEPT_CALL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_reject_call(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_reject_call.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_REJECT_CALL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_hold_call(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_hold_call.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_HOLD_CALL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_terminate_call(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_terminate_call.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_TERMINATE_CALL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_control_call(bt_instance_t* ins, bt_address_t* addr, hfp_call_control_t chld, uint8_t index)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_control_call.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_pl._bt_hfp_hf_control_call.chld = chld;
    packet.hfp_hf_pl._bt_hfp_hf_control_call.index = index;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_CONTROL_CALL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_query_current_calls(bt_instance_t* ins, bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator)
{
    bt_message_packet_t packet;
    bt_status_t status;
    int size;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_query_current_calls.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_QUERY_CURRENT_CALLS);
    if (status != BT_STATUS_SUCCESS)
        return status;

    *num = packet.hfp_hf_pl._bt_hfp_hf_query_current_calls.num;
    size = packet.hfp_hf_pl._bt_hfp_hf_query_current_calls.num * sizeof(hfp_current_call_t);
    if (size) {
        if (!allocator((void**)calls, size))
            return BT_STATUS_NOMEM;

        memcpy(*calls, &packet.hfp_hf_pl._bt_hfp_hf_query_current_calls.calls, size);
    }

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_send_at_cmd(bt_instance_t* ins, bt_address_t* addr, const char* cmd)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (strlen(cmd) > HFP_AT_LEN_MAX)
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_send_at_cmd.addr, addr, sizeof(bt_address_t));
    strncpy(packet.hfp_hf_pl._bt_hfp_hf_send_at_cmd.cmd, cmd, HFP_AT_LEN_MAX);
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_SEND_AT_CMD);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_update_battery_level(bt_instance_t* ins, bt_address_t* addr, uint8_t level)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_update_battery_level.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_pl._bt_hfp_hf_update_battery_level.level = level;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_UPDATE_BATTERY_LEVEL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_volume_control(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_volume_control.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_pl._bt_hfp_hf_volume_control.type = type;
    packet.hfp_hf_pl._bt_hfp_hf_volume_control.volume = volume;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_VOLUME_CONTROL);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}

bt_status_t bt_hfp_hf_send_dtmf(bt_instance_t* ins, bt_address_t* addr, char dtmf)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hfp_hf_pl._bt_hfp_hf_send_dtmf.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_pl._bt_hfp_hf_send_dtmf.dtmf = dtmf;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HFP_HF_SEND_DTMF);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hfp_hf_r.status;
}
