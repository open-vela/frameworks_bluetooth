/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#include <string.h>

#include "bluetooth.h"
#include "bt_hid_host.h"
#include "bt_internal.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "hid_host_service.h"
#include "service_manager.h"
#include "utils/log.h"

#define CBLIST (__async ? __async->hidh_callbacks : ins->hidh_callbacks)

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)

static void on_connection_state_cb(void* cookie, bt_address_t* addr,
    bt_transport_t transport, profile_connection_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.hidh_cb._connection_state.addr, addr, sizeof(bt_address_t));
    packet.hidh_cb._connection_state.transport = transport;
    packet.hidh_cb._connection_state.state = state;
    bt_socket_server_send(ins, &packet, BT_HID_HOST_CONNECTION_STATE);
}

static void on_report_map_cb(void* cookie, bt_address_t* addr,
    uint8_t service_index, const uint8_t* data, uint16_t len)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    uint16_t copy_len = len < sizeof(packet.hidh_cb._report_map.data) ? len : sizeof(packet.hidh_cb._report_map.data);

    memcpy(&packet.hidh_cb._report_map.addr, addr, sizeof(bt_address_t));
    packet.hidh_cb._report_map.service_index = service_index;
    packet.hidh_cb._report_map.len = copy_len;
    memcpy(packet.hidh_cb._report_map.data, data, copy_len);
    bt_socket_server_send(ins, &packet, BT_HID_HOST_REPORT_MAP);
}

static void on_input_report_cb(void* cookie, bt_address_t* addr,
    uint8_t service_index, uint8_t report_id,
    const uint8_t* data, uint16_t len)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    uint16_t copy_len = len < sizeof(packet.hidh_cb._input_report.data) ? len : sizeof(packet.hidh_cb._input_report.data);

    memcpy(&packet.hidh_cb._input_report.addr, addr, sizeof(bt_address_t));
    packet.hidh_cb._input_report.service_index = service_index;
    packet.hidh_cb._input_report.report_id = report_id;
    packet.hidh_cb._input_report.len = copy_len;
    memcpy(packet.hidh_cb._input_report.data, data, copy_len);
    bt_socket_server_send(ins, &packet, BT_HID_HOST_INPUT_REPORT);
}

static void on_get_report_cb(void* cookie, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type,
    const uint8_t* data, uint16_t len)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    uint16_t copy_len = len < sizeof(packet.hidh_cb._get_report_result.data) ? len : sizeof(packet.hidh_cb._get_report_result.data);

    memcpy(&packet.hidh_cb._get_report_result.addr, addr, sizeof(bt_address_t));
    packet.hidh_cb._get_report_result.report_id = report_id;
    packet.hidh_cb._get_report_result.report_type = report_type;
    packet.hidh_cb._get_report_result.len = copy_len;
    memcpy(packet.hidh_cb._get_report_result.data, data, copy_len);
    bt_socket_server_send(ins, &packet, BT_HID_HOST_GET_REPORT_RESULT);
}

static void on_pnp_id_cb(void* cookie, bt_address_t* addr,
    uint8_t vid_src, uint16_t vid, uint16_t pid, uint16_t version)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.hidh_cb._pnp_id.addr, addr, sizeof(bt_address_t));
    packet.hidh_cb._pnp_id.vid_src = vid_src;
    packet.hidh_cb._pnp_id.vid = vid;
    packet.hidh_cb._pnp_id.pid = pid;
    packet.hidh_cb._pnp_id.version = version;
    bt_socket_server_send(ins, &packet, BT_HID_HOST_PNP_ID);
}

static void on_battery_level_cb(void* cookie, bt_address_t* addr,
    uint8_t bat_index, uint8_t level)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.hidh_cb._battery_level.addr, addr, sizeof(bt_address_t));
    packet.hidh_cb._battery_level.bat_index = bat_index;
    packet.hidh_cb._battery_level.level = level;
    bt_socket_server_send(ins, &packet, BT_HID_HOST_BATTERY_LEVEL);
}

static void on_mode_changed_cb(void* cookie, bt_address_t* addr,
    uint8_t mode, int status)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.hidh_cb._mode_changed.addr, addr, sizeof(bt_address_t));
    packet.hidh_cb._mode_changed.mode = mode;
    packet.hidh_cb._mode_changed.status = status;
    bt_socket_server_send(ins, &packet, BT_HID_HOST_MODE_CHANGED);
}

static const hid_host_callbacks_t g_hid_host_socket_cbs = {
    .size = sizeof(hid_host_callbacks_t),
    .connection_state_cb = on_connection_state_cb,
    .report_map_cb = on_report_map_cb,
    .input_report_cb = on_input_report_cb,
    .get_report_cb = on_get_report_cb,
    .pnp_id_cb = on_pnp_id_cb,
    .battery_level_cb = on_battery_level_cb,
    .mode_changed_cb = on_mode_changed_cb,
};

#endif /* CONFIG_BLUETOOTH_SERVER && __NuttX__ */

void bt_socket_server_hid_host_process(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet)
{
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
    hid_host_interface_t* profile;

    switch (packet->code) {
    case BT_HID_HOST_REGISTER_CALLBACK:
        if (ins->hidh_cookie == NULL) {
            profile = (hid_host_interface_t*)service_manager_get_profile(PROFILE_HID_HOST);
            ins->hidh_cookie = profile->register_callbacks((void*)ins, (void*)&g_hid_host_socket_cbs);
        }

        packet->hidh_r.status = ins->hidh_cookie ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
        break;
    case BT_HID_HOST_UNREGISTER_CALLBACK:
        if (ins->hidh_cookie) {
            profile = (hid_host_interface_t*)service_manager_get_profile(PROFILE_HID_HOST);
            profile->unregister_callbacks((void**)&ins, ins->hidh_cookie);
            ins->hidh_cookie = NULL;
        }

        packet->hidh_r.status = BT_STATUS_SUCCESS;
        break;
    case BT_HID_HOST_CONNECT:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_connect)(ins,
            &packet->hidh_pl._bt_hid_host_connect.addr,
            packet->hidh_pl._bt_hid_host_connect.transport);
        break;
    case BT_HID_HOST_DISCONNECT:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_disconnect)(ins,
            &packet->hidh_pl._bt_hid_host_disconnect.addr);
        break;
    case BT_HID_HOST_GET_REPORT:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_get_report)(ins,
            &packet->hidh_pl._bt_hid_host_get_report.addr,
            packet->hidh_pl._bt_hid_host_get_report.report_id,
            packet->hidh_pl._bt_hid_host_get_report.report_type);
        break;
    case BT_HID_HOST_SET_REPORT:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_set_report)(ins,
            &packet->hidh_pl._bt_hid_host_set_report.addr,
            packet->hidh_pl._bt_hid_host_set_report.report_id,
            packet->hidh_pl._bt_hid_host_set_report.report_type,
            packet->hidh_pl._bt_hid_host_set_report.data,
            packet->hidh_pl._bt_hid_host_set_report.len);
        break;
    case BT_HID_HOST_SET_PROTOCOL:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_set_protocol)(ins,
            &packet->hidh_pl._bt_hid_host_set_protocol.addr,
            packet->hidh_pl._bt_hid_host_set_protocol.protocol_mode);
        break;
    case BT_HID_HOST_SUSPEND:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_suspend)(ins,
            &packet->hidh_pl._bt_hid_host_suspend.addr);
        break;
    case BT_HID_HOST_EXIT_SUSPEND:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_exit_suspend)(ins,
            &packet->hidh_pl._bt_hid_host_exit_suspend.addr);
        break;
    case BT_HID_HOST_SET_MODE:
        packet->hidh_r.status = BTSYMBOLS(bt_hid_host_set_mode)(ins,
            &packet->hidh_pl._bt_hid_host_set_mode.addr,
            packet->hidh_pl._bt_hid_host_set_mode.mode,
            packet->hidh_pl._bt_hid_host_set_mode.level);
        break;
    default:
        break;
    }
#endif
}

int bt_socket_client_hid_host_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    bt_socket_async_client_t* __async = NULL;

    if (is_async)
        __async = ins->priv;

    switch (packet->code) {
    case BT_HID_HOST_CONNECTION_STATE:
        CALLBACK_FOREACH(CBLIST, hid_host_callbacks_t,
            connection_state_cb,
            &packet->hidh_cb._connection_state.addr,
            packet->hidh_cb._connection_state.transport,
            packet->hidh_cb._connection_state.state);
        break;
    case BT_HID_HOST_REPORT_MAP:
        CALLBACK_FOREACH(CBLIST, hid_host_callbacks_t,
            report_map_cb,
            &packet->hidh_cb._report_map.addr,
            packet->hidh_cb._report_map.service_index,
            packet->hidh_cb._report_map.data,
            packet->hidh_cb._report_map.len);
        break;
    case BT_HID_HOST_INPUT_REPORT:
        CALLBACK_FOREACH(CBLIST, hid_host_callbacks_t,
            input_report_cb,
            &packet->hidh_cb._input_report.addr,
            packet->hidh_cb._input_report.service_index,
            packet->hidh_cb._input_report.report_id,
            packet->hidh_cb._input_report.data,
            packet->hidh_cb._input_report.len);
        break;
    case BT_HID_HOST_GET_REPORT_RESULT:
        CALLBACK_FOREACH(CBLIST, hid_host_callbacks_t,
            get_report_cb,
            &packet->hidh_cb._get_report_result.addr,
            packet->hidh_cb._get_report_result.report_id,
            packet->hidh_cb._get_report_result.report_type,
            packet->hidh_cb._get_report_result.data,
            packet->hidh_cb._get_report_result.len);
        break;
    case BT_HID_HOST_PNP_ID:
        CALLBACK_FOREACH(CBLIST, hid_host_callbacks_t,
            pnp_id_cb,
            &packet->hidh_cb._pnp_id.addr,
            packet->hidh_cb._pnp_id.vid_src,
            packet->hidh_cb._pnp_id.vid,
            packet->hidh_cb._pnp_id.pid,
            packet->hidh_cb._pnp_id.version);
        break;
    case BT_HID_HOST_BATTERY_LEVEL:
        CALLBACK_FOREACH(CBLIST, hid_host_callbacks_t,
            battery_level_cb,
            &packet->hidh_cb._battery_level.addr,
            packet->hidh_cb._battery_level.bat_index,
            packet->hidh_cb._battery_level.level);
        break;
    case BT_HID_HOST_MODE_CHANGED:
        CALLBACK_FOREACH(CBLIST, hid_host_callbacks_t,
            mode_changed_cb,
            &packet->hidh_cb._mode_changed.addr,
            packet->hidh_cb._mode_changed.mode,
            packet->hidh_cb._mode_changed.status);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }
    return BT_STATUS_SUCCESS;
}

#undef CBLIST
