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

#include "a2dp_source_service.h"
#include "adapter_internel.h"
#include "bluetooth.h"
#include "bt_a2dp_source.h"
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
#define CBLIST (ins->a2dp_source_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
static a2dp_source_interface_t* get_profile_service(void)
{
    return (a2dp_source_interface_t*)service_manager_get_profile(PROFILE_A2DP);
}

static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.a2dp_source_cb._connection_state_changed.addr, addr, sizeof(bt_address_t));
    packet.a2dp_source_cb._connection_state_changed.state = state;
    bt_socket_server_send(ins, &packet, BT_A2DP_SOURCE_CONNECTION_STATE_CHANGE);
}

static void on_audio_state_changed_cb(void* cookie, bt_address_t* addr, a2dp_audio_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.a2dp_source_cb._audio_state_changed.addr, addr, sizeof(bt_address_t));
    packet.a2dp_source_cb._audio_state_changed.state = state;
    bt_socket_server_send(ins, &packet, BT_A2DP_SOURCE_AUDIO_STATE_CHANGE);
}

static void on_audio_config_changed_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.a2dp_source_cb._audio_config_state_changed.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(ins, &packet, BT_A2DP_SOURCE_CONFIG_CHANGE);
}

const static a2dp_source_callbacks_t g_a2dp_source_cbs = {
    .size = sizeof(a2dp_source_callbacks_t),
    .connection_state_cb = on_connection_state_changed_cb,
    .audio_state_cb = on_audio_state_changed_cb,
    .audio_source_config_cb = on_audio_config_changed_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_a2dp_source_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_A2DP_SOURCE_IS_CONNECTED: {
        packet->a2dp_source_r.bbool = BTSYMBOLS(bt_a2dp_source_is_connected)(ins,
            &packet->a2dp_source_pl._bt_a2dp_source_is_connected.addr);
        break;
    }
    case BT_A2DP_SOURCE_IS_PLAYING: {
        packet->a2dp_source_r.bbool = BTSYMBOLS(bt_a2dp_source_is_playing)(ins,
            &packet->a2dp_source_pl._bt_a2dp_source_is_playing.addr);
        break;
    }
    case BT_A2DP_SOURCE_GET_CONNECTION_STATE: {
        packet->a2dp_source_r.state = BTSYMBOLS(bt_a2dp_source_get_connection_state)(ins,
            &packet->a2dp_source_pl._bt_a2dp_source_get_connection_state.addr);
        break;
    }
    case BT_A2DP_SOURCE_CONNECT: {
        packet->a2dp_source_r.status = BTSYMBOLS(bt_a2dp_source_connect)(ins,
            &packet->a2dp_source_pl._bt_a2dp_source_connect.addr);
        break;
    }
    case BT_A2DP_SOURCE_DISCONNECT: {
        packet->a2dp_source_r.status = BTSYMBOLS(bt_a2dp_source_disconnect)(ins,
            &packet->a2dp_source_pl._bt_a2dp_source_disconnect.addr);
        break;
    }
    case BT_A2DP_SOURCE_SET_ACTIVE_DEVICE: {
        packet->a2dp_source_r.status = BTSYMBOLS(bt_a2dp_source_set_active_device)(ins,
            &packet->a2dp_source_pl._bt_a2dp_source_set_active_device.addr);
        break;
    }
    case BT_A2DP_SOURCE_SET_SILENCE_DEVICE: {
        packet->a2dp_source_r.status = BTSYMBOLS(bt_a2dp_source_set_active_device)(ins,
            &packet->a2dp_source_pl._bt_a2dp_source_set_silence_device.addr);
        break;
    }
    case BT_A2DP_SOURCE_REGISTER_CALLBACKS: {
        if (ins->a2dp_source_cookie == NULL) {
            a2dp_source_interface_t* profile = get_profile_service();
            ins->a2dp_source_cookie = profile->register_callbacks(ins, &g_a2dp_source_cbs);
            if (ins->a2dp_source_cookie) {
                packet->a2dp_source_r.status = BT_STATUS_SUCCESS;
            } else {
                packet->a2dp_source_r.status = BT_STATUS_FAIL;
            }
        } else {
            packet->a2dp_source_r.status = BT_STATUS_SUCCESS;
        }
        break;
    }
    case BT_A2DP_SOURCE_UNREGISTER_CALLBACKS: {
        if (ins->a2dp_source_cookie) {
            a2dp_source_interface_t* profile = get_profile_service();
            if (profile->unregister_callbacks(NULL, ins->a2dp_source_cookie)) {
                packet->a2dp_source_r.status = BT_STATUS_SUCCESS;
            } else {
                packet->a2dp_source_r.status = BT_STATUS_FAIL;
            }
            ins->a2dp_source_cookie = NULL;
        } else {
            packet->a2dp_source_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    }
    default:
        break;
    }
}

#endif

int bt_socket_client_a2dp_source_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_A2DP_SOURCE_CONNECTION_STATE_CHANGE: {
        CALLBACK_FOREACH(CBLIST, a2dp_source_callbacks_t,
            connection_state_cb,
            &packet->a2dp_source_cb._connection_state_changed.addr,
            packet->a2dp_source_cb._connection_state_changed.state);
        break;
    }
    case BT_A2DP_SOURCE_AUDIO_STATE_CHANGE: {
        CALLBACK_FOREACH(CBLIST, a2dp_source_callbacks_t,
            audio_state_cb,
            &packet->a2dp_source_cb._audio_state_changed.addr,
            packet->a2dp_source_cb._audio_state_changed.state);
        break;
    }
    case BT_A2DP_SOURCE_CONFIG_CHANGE: {
        CALLBACK_FOREACH(CBLIST, a2dp_source_callbacks_t,
            audio_source_config_cb,
            &packet->a2dp_source_cb._audio_config_state_changed.addr);
        break;
    }
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
