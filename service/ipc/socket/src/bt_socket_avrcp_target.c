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

#include "avrcp_target_service.h"
#include "bluetooth.h"
#include "bt_avrcp_target.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "service_loop.h"
#include "service_manager.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->avrcp_target_callbacks)

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

    memcpy(&packet.avrcp_target_cb._on_connection_state_changed.addr, addr, sizeof(bt_address_t));
    packet.avrcp_target_cb._on_connection_state_changed.state = state;
    bt_socket_server_send(ins, &packet, BT_AVRCP_TARGET_ON_CONNECTION_STATE_CHANGED);
}

static void on_get_play_status_request_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.avrcp_target_cb._on_get_play_status_request.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(ins, &packet, BT_AVRCP_TARGET_ON_GET_PLAY_STATUS_REQUEST);
}

static void on_register_notification_request_cb(void* cookie, bt_address_t* addr, avrcp_notification_event_t event, uint32_t interval)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.avrcp_target_cb._on_register_notification_request.addr, addr, sizeof(bt_address_t));
    packet.avrcp_target_cb._on_register_notification_request.event = event;
    packet.avrcp_target_cb._on_register_notification_request.interval = interval;
    bt_socket_server_send(ins, &packet, BT_AVRCP_TARGET_ON_REGISTER_NOTIFICATION_REQUEST);
}

static void on_received_panel_operation_cb(void* cookie, bt_address_t* addr, uint8_t op, uint8_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.avrcp_target_cb._on_received_panel_operator_cb.addr, addr, sizeof(bt_address_t));
    packet.avrcp_target_cb._on_received_panel_operator_cb.op = op;
    packet.avrcp_target_cb._on_received_panel_operator_cb.state = state;
    bt_socket_server_send(ins, &packet, BT_AVRCP_TARGET_ON_PANEL_OPERATION_RECEIVED);
}

const static avrcp_target_callbacks_t g_avrcp_target_cbs = {
    .size = sizeof(avrcp_target_callbacks_t),
    .connection_state_cb = on_connection_state_changed_cb,
    .received_get_play_status_request_cb = on_get_play_status_request_cb,
    .received_register_notification_request_cb = on_register_notification_request_cb,
    .received_panel_operation_cb = on_received_panel_operation_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_avrcp_target_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    avrcp_target_interface_t* profile;

    switch (packet->code) {
    case BT_AVRCP_TARGET_REGISTER_CALLBACKS:
        if (ins->avrcp_target_cookie == NULL) {
            profile = (avrcp_target_interface_t*)service_manager_get_profile(PROFILE_AVRCP_TG);
            if (profile) {
                ins->avrcp_target_cookie = profile->register_callbacks(ins, &g_avrcp_target_cbs);
                if (ins->avrcp_target_cookie) {
                    packet->avrcp_target_r.status = BT_STATUS_SUCCESS;
                } else {
                    packet->avrcp_target_r.status = BT_STATUS_NO_RESOURCES;
                }
            } else {
                packet->avrcp_target_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            }
        } else {
            packet->avrcp_target_r.status = BT_STATUS_BUSY;
        }
        break;
    case BT_AVRCP_TARGET_UNREGISTER_CALLBACKS:
        if (ins->avrcp_target_cookie) {
            profile = (avrcp_target_interface_t*)service_manager_get_profile(PROFILE_AVRCP_TG);
            if (profile)
                profile->unregister_callbacks((void**)&ins, ins->avrcp_target_cookie);
            ins->avrcp_target_cookie = NULL;
            packet->avrcp_target_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->avrcp_target_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case BT_AVRCP_TARGET_GET_PLAY_STATUS_RESPONSE:
        packet->avrcp_target_r.value_bool = BTSYMBOLS(bt_avrcp_target_get_play_status_response)(ins,
            &packet->avrcp_target_pl._bt_avrcp_target_get_play_status_response.addr,
            packet->avrcp_target_pl._bt_avrcp_target_get_play_status_response.play_status,
            packet->avrcp_target_pl._bt_avrcp_target_get_play_status_response.song_len,
            packet->avrcp_target_pl._bt_avrcp_target_get_play_status_response.song_pos);
        break;
    case BT_AVRCP_TARGET_PLAY_STATUS_NOTIFY:
        packet->avrcp_target_r.value_bool = BTSYMBOLS(bt_avrcp_target_play_status_notify)(ins,
            &packet->avrcp_target_pl._bt_avrcp_target_play_status_notify.addr,
            packet->avrcp_target_pl._bt_avrcp_target_play_status_notify.play_status);
        break;
    default:
        break;
    }
}

#endif

int bt_socket_client_avrcp_target_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_AVRCP_TARGET_ON_CONNECTION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, avrcp_target_callbacks_t,
            connection_state_cb,
            &packet->avrcp_target_cb._on_connection_state_changed.addr,
            packet->avrcp_target_cb._on_connection_state_changed.state);
        break;
    case BT_AVRCP_TARGET_ON_GET_PLAY_STATUS_REQUEST:
        CALLBACK_FOREACH(CBLIST, avrcp_target_callbacks_t,
            received_get_play_status_request_cb,
            &packet->avrcp_target_cb._on_get_play_status_request.addr);
        break;
    case BT_AVRCP_TARGET_ON_REGISTER_NOTIFICATION_REQUEST:
        CALLBACK_FOREACH(CBLIST, avrcp_target_callbacks_t,
            received_register_notification_request_cb,
            &packet->avrcp_target_cb._on_register_notification_request.addr,
            packet->avrcp_target_cb._on_register_notification_request.event,
            packet->avrcp_target_cb._on_register_notification_request.interval);
        break;
    case BT_AVRCP_TARGET_ON_PANEL_OPERATION_RECEIVED:
        CALLBACK_FOREACH(CBLIST, avrcp_target_callbacks_t,
            received_panel_operation_cb,
            &packet->avrcp_target_cb._on_received_panel_operator_cb.addr,
            packet->avrcp_target_cb._on_received_panel_operator_cb.op,
            packet->avrcp_target_cb._on_received_panel_operator_cb.state);
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
