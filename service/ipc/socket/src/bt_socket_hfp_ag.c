/****************************************************************************
 * service/ipc/socket/src/bt_socket_hfp_ag.c
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
#include "bt_hfp_ag.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "hfp_ag_service.h"
#include "service_loop.h"
#include "service_manager.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->hfp_ag_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_connection_state_changed.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_cb._on_connection_state_changed.state = state;

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_CONNECTION_STATE_CHANGED);
}

static void on_audio_state_changed_cb(void* cookie, bt_address_t* addr, hfp_audio_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_audio_state_changed.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_cb._on_audio_state_changed.state = state;

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_AUDIO_STATE_CHANGED);
}

static void on_voice_recognition_command_cb(void* cookie, bt_address_t* addr, bool started)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_voice_recognition_state_changed.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_cb._on_voice_recognition_state_changed.started = started;

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_VOICE_RECOGNITION_STATE_CHANGED);
}

static void on_hf_battery_update_cb(void* cookie, bt_address_t* addr, uint8_t value)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_battery_level_changed.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_cb._on_battery_level_changed.value = value;

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_BATTERY_LEVEL_CHANGED);
}

static void on_volume_control_cb(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_volume_control.addr, addr, sizeof(bt_address_t));
    packet.hfp_ag_cb._on_volume_control.type = type;
    packet.hfp_ag_cb._on_volume_control.volume = volume;

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_VOLUME_CONTROL);
}

static void on_answer_call_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_answer_call.addr, addr, sizeof(bt_address_t));

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_ANSWER_CALL);
}

static void on_reject_call_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_reject_call.addr, addr, sizeof(bt_address_t));

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_REJECT_CALL);
}

static void on_hangup_call_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_hangup_call.addr, addr, sizeof(bt_address_t));

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_HANGUP_CALL);
}

static void on_dial_call_cb(void* cookie, bt_address_t* addr, const char* number)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_dial_call.addr, addr, sizeof(bt_address_t));
    if (number != NULL)
        strlcpy(packet.hfp_ag_cb._on_dial_call.number, number,
            sizeof(packet.hfp_ag_cb._on_dial_call.number));

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_DIAL_CALL);
}

static void on_at_cmd_received_cb(void* cookie, bt_address_t* addr, const char* at_command)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_at_cmd_received.addr, addr, sizeof(bt_address_t));
    if (at_command != NULL)
        strlcpy(packet.hfp_ag_cb._on_at_cmd_received.cmd, at_command,
            sizeof(packet.hfp_ag_cb._on_at_cmd_received.cmd));

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_AT_COMMAND_RECEIVED);
}

static void on_vendor_specific_at_cmd_received_cb(void* cookie, bt_address_t* addr, const char* command, uint16_t company_id, const char* value)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_vend_spec_at_cmd_received.addr, addr, sizeof(bt_address_t));
    if (command != NULL)
        strlcpy(packet.hfp_ag_cb._on_vend_spec_at_cmd_received.command, command,
            sizeof(packet.hfp_ag_cb._on_vend_spec_at_cmd_received.command));

    packet.hfp_ag_cb._on_vend_spec_at_cmd_received.company_id = company_id;

    if (value != NULL)
        strlcpy(packet.hfp_ag_cb._on_vend_spec_at_cmd_received.value, value,
            sizeof(packet.hfp_ag_cb._on_vend_spec_at_cmd_received.value));

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_VENDOR_SPECIFIC_AT_COMMAND_RECEIVED);
}

static void on_clcc_cmd_received_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_ag_cb._on_clcc_cmd_received.addr, addr, sizeof(bt_address_t));

    bt_socket_server_send(ins, &packet, BT_HFP_AG_ON_CLCC_COMMAND_RECEIVED);
}

const static hfp_ag_callbacks_t g_hfp_ag_socket_cbs = {
    .connection_state_cb = on_connection_state_changed_cb,
    .audio_state_cb = on_audio_state_changed_cb,
    .vr_cmd_cb = on_voice_recognition_command_cb,
    .hf_battery_update_cb = on_hf_battery_update_cb,
    .volume_control_cb = on_volume_control_cb,
    .answer_call_cb = on_answer_call_cb,
    .reject_call_cb = on_reject_call_cb,
    .hangup_call_cb = on_hangup_call_cb,
    .dial_call_cb = on_dial_call_cb,
    .at_cmd_cb = on_at_cmd_received_cb,
    .vender_specific_at_cmd_cb = on_vendor_specific_at_cmd_received_cb,
    .clcc_cmd_cb = on_clcc_cmd_received_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_hfp_ag_process(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet)
{
    hfp_ag_interface_t* profile;

    switch (packet->code) {
    case BT_HFP_AG_REGISTER_CALLBACK:
        if (ins->hfp_ag_cookie == NULL) {
            profile = (hfp_ag_interface_t*)service_manager_get_profile(PROFILE_HFP_AG);
            if (profile) {
                ins->hfp_ag_cookie = profile->register_callbacks((void*)ins, (void*)&g_hfp_ag_socket_cbs);
                if (ins->hfp_ag_cookie)
                    packet->hfp_ag_r.status = BT_STATUS_SUCCESS;
                else
                    packet->hfp_ag_r.status = BT_STATUS_NO_RESOURCES;
            } else {
                packet->hfp_ag_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            }
        } else {
            packet->hfp_ag_r.status = BT_STATUS_BUSY;
        }
        break;
    case BT_HFP_AG_UNREGISTER_CALLBACK:
        if (ins->hfp_ag_cookie) {
            profile = (hfp_ag_interface_t*)service_manager_get_profile(PROFILE_HFP_AG);
            if (profile)
                profile->unregister_callbacks((void**)&ins, ins->hfp_ag_cookie);
            ins->hfp_ag_cookie = NULL;
            packet->hfp_ag_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->hfp_ag_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case BT_HFP_AG_IS_CONNECTED:
        packet->hfp_ag_r.value_bool = BTSYMBOLS(bt_hfp_ag_is_connected)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_is_connected.addr);
        break;

    case BT_HFP_AG_IS_AUDIO_CONNECTED:
        packet->hfp_ag_r.value_bool = BTSYMBOLS(bt_hfp_ag_is_audio_connected)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_is_audio_connected.addr);
        break;
    case BT_HFP_AG_GET_CONNECTION_STATE:
        packet->hfp_ag_r.profile_conn_state = BTSYMBOLS(bt_hfp_ag_get_connection_state)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_get_connection_state.addr);
        break;
    case BT_HFP_AG_CONNECT:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_connect)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_connect.addr);
        break;
    case BT_HFP_AG_DISCONNECT:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_disconnect)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_disconnect.addr);
        break;
    case BT_HFP_AG_CONNECT_AUDIO:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_connect_audio)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_connect_audio.addr);
        break;
    case BT_HFP_AG_DISCONNECT_AUDIO:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_disconnect_audio)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_disconnect_audio.addr);
        break;
    case BT_HFP_AG_START_VIRTUAL_CALL:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_start_virtual_call)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_start_virtual_call.addr);
        break;
    case BT_HFP_AG_STOP_VIRTUAL_CALL:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_stop_virtual_call)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_stop_virtual_call.addr);
        break;
    case BT_HFP_AG_START_VOICE_RECOGNITION:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_start_voice_recognition)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_start_voice_recognition.addr);
        break;
    case BT_HFP_AG_STOP_VOICE_RECOGNITION:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_stop_voice_recognition)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_stop_voice_recognition.addr);
        break;
    case BT_HFP_AG_PHONE_STATE_CHANGE:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_phone_state_change)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.addr,
            packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.num_active,
            packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.num_held,
            packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.call_state,
            packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.type,
            packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.number[0] ? packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.number : NULL,
            packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.name[0] ? packet->hfp_ag_pl._bt_hfp_ag_phone_state_change.name : NULL);
        break;
    case BT_HFP_AG_NOTIFY_DEVICE_STATUS:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_notify_device_status)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_notify_device_status.addr,
            packet->hfp_ag_pl._bt_hfp_ag_notify_device_status.network,
            packet->hfp_ag_pl._bt_hfp_ag_notify_device_status.roam,
            packet->hfp_ag_pl._bt_hfp_ag_notify_device_status.signal,
            packet->hfp_ag_pl._bt_hfp_ag_notify_device_status.battery);
        break;
    case BT_HFP_AG_VOLUME_CONTROL:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_volume_control)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_volume_control.addr,
            packet->hfp_ag_pl._bt_hfp_ag_volume_control.type,
            packet->hfp_ag_pl._bt_hfp_ag_volume_control.volume);
        break;
    case BT_HFP_AG_SEND_AT_COMMAND:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_send_at_command)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_send_at_cmd.addr,
            packet->hfp_ag_pl._bt_hfp_ag_send_at_cmd.cmd);
        break;
    case BT_HFP_AG_SEND_VENDOR_SPECIFIC_AT_COMMAND:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_send_vendor_specific_at_command)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.addr,
            packet->hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.cmd,
            packet->hfp_ag_pl._bt_hfp_ag_send_vendor_specific_at_cmd.value);
        break;
    case BT_HFP_AG_SEND_CLCC_RESPONSE:
        packet->hfp_ag_r.status = BTSYMBOLS(bt_hfp_ag_send_clcc_response)(ins,
            &packet->hfp_ag_pl._bt_hfp_ag_send_at_cmd.addr,
            packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.index,
            packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.dir,
            packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.state,
            packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.mode,
            packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.mpty,
            packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.type,
            packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.number[0] ? packet->hfp_ag_pl._bt_hfp_ag_send_clcc_response.number : NULL);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_hfp_ag_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_HFP_AG_ON_CONNECTION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            connection_state_cb,
            &packet->hfp_ag_cb._on_connection_state_changed.addr,
            packet->hfp_ag_cb._on_connection_state_changed.state);
        break;
    case BT_HFP_AG_ON_AUDIO_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            audio_state_cb,
            &packet->hfp_ag_cb._on_audio_state_changed.addr,
            packet->hfp_ag_cb._on_audio_state_changed.state);
        break;
    case BT_HFP_AG_ON_VOICE_RECOGNITION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            vr_cmd_cb,
            &packet->hfp_ag_cb._on_voice_recognition_state_changed.addr,
            packet->hfp_ag_cb._on_voice_recognition_state_changed.started);
        break;
    case BT_HFP_AG_ON_BATTERY_LEVEL_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            hf_battery_update_cb,
            &packet->hfp_ag_cb._on_battery_level_changed.addr,
            packet->hfp_ag_cb._on_battery_level_changed.value);
        break;
    case BT_HFP_AG_ON_VOLUME_CONTROL:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            volume_control_cb,
            &packet->hfp_ag_cb._on_volume_control.addr,
            packet->hfp_ag_cb._on_volume_control.type,
            packet->hfp_ag_cb._on_volume_control.volume);
        break;
    case BT_HFP_AG_ON_ANSWER_CALL:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            answer_call_cb,
            &packet->hfp_ag_cb._on_answer_call.addr);
        break;
    case BT_HFP_AG_ON_REJECT_CALL:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            reject_call_cb,
            &packet->hfp_ag_cb._on_reject_call.addr);
        break;
    case BT_HFP_AG_ON_HANGUP_CALL:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            hangup_call_cb,
            &packet->hfp_ag_cb._on_hangup_call.addr);
        break;
    case BT_HFP_AG_ON_DIAL_CALL:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            dial_call_cb,
            &packet->hfp_ag_cb._on_dial_call.addr,
            packet->hfp_ag_cb._on_dial_call.number[0] ? packet->hfp_ag_cb._on_dial_call.number : NULL);
        break;
    case BT_HFP_AG_ON_AT_COMMAND_RECEIVED:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            at_cmd_cb,
            &packet->hfp_ag_cb._on_at_cmd_received.addr,
            packet->hfp_ag_cb._on_at_cmd_received.cmd);
        break;
    case BT_HFP_AG_ON_VENDOR_SPECIFIC_AT_COMMAND_RECEIVED:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            vender_specific_at_cmd_cb,
            &packet->hfp_ag_cb._on_vend_spec_at_cmd_received.addr,
            packet->hfp_ag_cb._on_vend_spec_at_cmd_received.command,
            packet->hfp_ag_cb._on_vend_spec_at_cmd_received.company_id,
            packet->hfp_ag_cb._on_vend_spec_at_cmd_received.value);
        break;
    case BT_HFP_AG_ON_CLCC_COMMAND_RECEIVED:
        CALLBACK_FOREACH(CBLIST, hfp_ag_callbacks_t,
            clcc_cmd_cb,
            &packet->hfp_ag_cb._on_clcc_cmd_received.addr);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
