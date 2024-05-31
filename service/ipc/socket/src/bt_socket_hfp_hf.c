/****************************************************************************
 * service/ipc/socket/src/bt_socket_hfp_hf.c
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
#include "bt_hfp_hf.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "hfp_hf_service.h"
#include "service_loop.h"
#include "service_manager.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->hfp_hf_callbacks)

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

    memcpy(&packet.hfp_hf_cb._on_connection_state_changed.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_connection_state_changed.state = state;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_CONNECTION_STATE_CHANGED);
}

static void on_audio_state_changed_cb(void* cookie, bt_address_t* addr, hfp_audio_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_audio_state_changed.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_audio_state_changed.state = state;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_AUDIO_STATE_CHANGED);
}

static void on_voice_recognition_command_cb(void* cookie, bt_address_t* addr, bool started)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_voice_recognition_state_changed.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_voice_recognition_state_changed.started = started;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_VOICE_RECOGNITION_STATE_CHANGED);
}

static void on_call_state_changed_cb(void* cookie, bt_address_t* addr, hfp_current_call_t* call)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_call_state_changed_cb.addr, addr, sizeof(bt_address_t));
    memcpy(&packet.hfp_hf_cb._on_call_state_changed_cb.call, call, sizeof(hfp_current_call_t));

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_CALL_STATE_CHANGED);
}

static void on_at_cmd_complete_cb(void* cookie, bt_address_t* addr, const char* resp)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_at_cmd_complete_cb.addr, addr, sizeof(bt_address_t));
    if (resp != NULL)
        strncpy(packet.hfp_hf_cb._on_at_cmd_complete_cb.resp, resp, HFP_AT_LEN_MAX);

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_AT_CMD_COMPLETE);
}

static void on_ring_indication_cb(void* cookie, bt_address_t* addr, bool inband_ring_tone)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_ring_indication_cb.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_ring_indication_cb.inband_ring_tone = inband_ring_tone;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_RING_INDICATION);
}

static void on_volume_changed_cb(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_volume_changed_cb.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_volume_changed_cb.type = type;
    packet.hfp_hf_cb._on_volume_changed_cb.volume = volume;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_VOLUME_CHANGED);
}

static void on_call_cb(void* cookie, bt_address_t* addr, hfp_call_t call)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_call_cb.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_call_cb.value = call;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_CALL_IND_RECEIVED);
}

static void on_callsetup_cb(void* cookie, bt_address_t* addr, hfp_callsetup_t callsetup)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_callsetup_cb.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_callsetup_cb.value = callsetup;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_CALLSETUP_IND_RECEIVED);
}

static void on_callheld_cb(void* cookie, bt_address_t* addr, hfp_callheld_t callheld)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.hfp_hf_cb._on_callheld_cb.addr, addr, sizeof(bt_address_t));
    packet.hfp_hf_cb._on_callheld_cb.value = callheld;

    bt_socket_server_send(ins, &packet, BT_HFP_HF_ON_CALLHELD_IND_RECEIVED);
}

const static hfp_hf_callbacks_t g_hfp_hf_socket_cbs = {
    .connection_state_cb = on_connection_state_changed_cb,
    .audio_state_cb = on_audio_state_changed_cb,
    .vr_cmd_cb = on_voice_recognition_command_cb,
    .call_state_changed_cb = on_call_state_changed_cb,
    .cmd_complete_cb = on_at_cmd_complete_cb,
    .ring_indication_cb = on_ring_indication_cb,
    .volume_changed_cb = on_volume_changed_cb,
    .call_cb = on_call_cb,
    .callsetup_cb = on_callsetup_cb,
    .callheld_cb = on_callheld_cb,
};

static bool bt_socket_allocator(void** data, uint32_t size)
{
    *data = zalloc(size);
    if (!(*data))
        return false;

    return true;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_hfp_hf_process(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet)
{
    hfp_hf_interface_t* profile;

    switch (packet->code) {
    case BT_HFP_HF_REGISTER_CALLBACK:
        if (ins->hfp_hf_cookie == NULL) {
            profile = (hfp_hf_interface_t*)service_manager_get_profile(PROFILE_HFP_HF);
            if (profile) {
                ins->hfp_hf_cookie = profile->register_callbacks((void*)ins, (void*)&g_hfp_hf_socket_cbs);
                if (ins->hfp_hf_cookie)
                    packet->hfp_hf_r.status = BT_STATUS_SUCCESS;
                else
                    packet->hfp_hf_r.status = BT_STATUS_NO_RESOURCES;
            } else {
                packet->hfp_hf_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            }
        } else {
            packet->hfp_hf_r.status = BT_STATUS_BUSY;
        }
        break;
    case BT_HFP_HF_UNREGISTER_CALLBACK:
        if (ins->hfp_hf_cookie) {
            profile = (hfp_hf_interface_t*)service_manager_get_profile(PROFILE_HFP_HF);
            if (profile)
                profile->unregister_callbacks((void**)&ins, ins->hfp_hf_cookie);
            ins->hfp_hf_cookie = NULL;
            packet->hfp_hf_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->hfp_hf_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case BT_HFP_HF_IS_CONNECTED:
        packet->hfp_hf_r.value_bool = BTSYMBOLS(bt_hfp_hf_is_connected)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_is_connected.addr);
        break;
    case BT_HFP_HF_IS_AUDIO_CONNECTED:
        packet->hfp_hf_r.value_bool = BTSYMBOLS(bt_hfp_hf_is_audio_connected)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_is_audio_connected.addr);
        break;
    case BT_HFP_HF_GET_CONNECTION_STATE:
        packet->hfp_hf_r.profile_conn_state = BTSYMBOLS(bt_hfp_hf_get_connection_state)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_get_connection_state.addr);
        break;
    case BT_HFP_HF_CONNECT:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_connect)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_connect.addr);
        break;
    case BT_HFP_HF_DISCONNECT:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_disconnect)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_disconnect.addr);
        break;
    case BT_HFP_HF_SET_CONNECTION_POLICY:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_set_connection_policy)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_set_connection_policy.addr, packet->hfp_hf_pl._bt_hfp_hf_set_connection_policy.policy);
        break;
    case BT_HFP_HF_CONNECT_AUDIO:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_connect_audio)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_connect_audio.addr);
        break;
    case BT_HFP_HF_DISCONNECT_AUDIO:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_disconnect_audio)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_disconnect_audio.addr);
        break;
    case BT_HFP_HF_START_VOICE_RECOGNITION:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_start_voice_recognition)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_start_voice_recognition.addr);
        break;
    case BT_HFP_HF_STOP_VOICE_RECOGNITION:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_stop_voice_recognition)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_stop_voice_recognition.addr);
        break;
    case BT_HFP_HF_DIAL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_dial)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_dial.addr,
            (const char*)&packet->hfp_hf_pl._bt_hfp_hf_dial.number);
        break;
    case BT_HFP_HF_DIAL_MEMORY:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_dial_memory)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_dial_memory.addr,
            packet->hfp_hf_pl._bt_hfp_hf_dial_memory.memory);
        break;
    case BT_HFP_HF_REDIAL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_redial)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_redial.addr);
        break;
    case BT_HFP_HF_ACCEPT_CALL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_accept_call)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_accept_call.addr,
            packet->hfp_hf_pl._bt_hfp_hf_accept_call.flag);
        break;
    case BT_HFP_HF_REJECT_CALL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_reject_call)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_reject_call.addr);
        break;
    case BT_HFP_HF_HOLD_CALL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_hold_call)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_hold_call.addr);
        break;
    case BT_HFP_HF_TERMINATE_CALL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_terminate_call)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_terminate_call.addr);
        break;
    case BT_HFP_HF_CONTROL_CALL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_control_call)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_control_call.addr,
            packet->hfp_hf_pl._bt_hfp_hf_control_call.chld,
            packet->hfp_hf_pl._bt_hfp_hf_control_call.index);
        break;
    case BT_HFP_HF_QUERY_CURRENT_CALLS: {
        hfp_current_call_t* calls = NULL;
        int num = 0;
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_query_current_calls)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_query_current_calls.addr,
            &calls, &num, (bt_allocator_t)bt_socket_allocator);
        packet->hfp_hf_pl._bt_hfp_hf_query_current_calls.num = num;
        memcpy(packet->hfp_hf_pl._bt_hfp_hf_query_current_calls.calls, calls,
            sizeof(hfp_current_call_t) * MIN(num, HFP_CALL_LIST_MAX));
        if (calls)
            free(calls);

        break;
    }
    case BT_HFP_HF_SEND_AT_CMD:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_send_at_cmd)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_send_at_cmd.addr,
            (const char*)&packet->hfp_hf_pl._bt_hfp_hf_send_at_cmd.cmd);
        break;
    case BT_HFP_HF_UPDATE_BATTERY_LEVEL:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_update_battery_level)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_update_battery_level.addr,
            packet->hfp_hf_pl._bt_hfp_hf_update_battery_level.level);
        break;
    case BT_HFP_HF_SEND_DTMF:
        packet->hfp_hf_r.status = BTSYMBOLS(bt_hfp_hf_send_dtmf)(ins,
            &packet->hfp_hf_pl._bt_hfp_hf_send_dtmf.addr,
            packet->hfp_hf_pl._bt_hfp_hf_send_dtmf.dtmf);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_hfp_hf_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_HFP_HF_ON_CONNECTION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            connection_state_cb,
            &packet->hfp_hf_cb._on_connection_state_changed.addr,
            packet->hfp_hf_cb._on_connection_state_changed.state);
        break;
    case BT_HFP_HF_ON_AUDIO_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            audio_state_cb,
            &packet->hfp_hf_cb._on_audio_state_changed.addr,
            packet->hfp_hf_cb._on_audio_state_changed.state);
        break;
    case BT_HFP_HF_ON_VOICE_RECOGNITION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            vr_cmd_cb,
            &packet->hfp_hf_cb._on_voice_recognition_state_changed.addr,
            packet->hfp_hf_cb._on_voice_recognition_state_changed.started);
        break;
    case BT_HFP_HF_ON_CALL_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            call_state_changed_cb,
            &packet->hfp_hf_cb._on_call_state_changed_cb.addr,
            &packet->hfp_hf_cb._on_call_state_changed_cb.call);
        break;
    case BT_HFP_HF_ON_AT_CMD_COMPLETE:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            cmd_complete_cb,
            &packet->hfp_hf_cb._on_at_cmd_complete_cb.addr,
            packet->hfp_hf_cb._on_at_cmd_complete_cb.resp);
        break;
    case BT_HFP_HF_ON_RING_INDICATION:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            ring_indication_cb,
            &packet->hfp_hf_cb._on_ring_indication_cb.addr,
            packet->hfp_hf_cb._on_ring_indication_cb.inband_ring_tone);
        break;
    case BT_HFP_HF_ON_VOLUME_CHANGED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            volume_changed_cb,
            &packet->hfp_hf_cb._on_volume_changed_cb.addr,
            packet->hfp_hf_cb._on_volume_changed_cb.type,
            packet->hfp_hf_cb._on_volume_changed_cb.volume);
        break;
    case BT_HFP_HF_ON_CALL_IND_RECEIVED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            call_cb,
            &packet->hfp_hf_cb._on_call_cb.addr,
            packet->hfp_hf_cb._on_call_cb.value);
        break;
    case BT_HFP_HF_ON_CALLSETUP_IND_RECEIVED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            callsetup_cb,
            &packet->hfp_hf_cb._on_callsetup_cb.addr,
            packet->hfp_hf_cb._on_callsetup_cb.value);
        break;
    case BT_HFP_HF_ON_CALLHELD_IND_RECEIVED:
        CALLBACK_FOREACH(CBLIST, hfp_hf_callbacks_t,
            callheld_cb,
            &packet->hfp_hf_cb._on_callheld_cb.addr,
            packet->hfp_hf_cb._on_callheld_cb.value);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
