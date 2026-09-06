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
#define LOG_TAG "hidh_api"

#include <stdint.h>
#include <string.h>

#include "bt_hid_host.h"
#include "bt_message_hid_host.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "utils/log.h"

void* bt_hid_host_register_callbacks(bt_instance_t* ins, const hid_host_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->hidh_callbacks == NULL)
        ins->hidh_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    cookie = bt_remote_callbacks_register(ins->hidh_callbacks, ins, (void*)callbacks);
    if (cookie == NULL)
        return NULL;

    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_REGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hidh_r.status != BT_STATUS_SUCCESS) {
        bt_remote_callbacks_unregister(ins->hidh_callbacks, NULL, cookie);
        return NULL;
    }

    return cookie;
}

bool bt_hid_host_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);
    if (!ins->hidh_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->hidh_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->hidh_callbacks) > 0)
        return true;

    cbsl = ins->hidh_callbacks;
    ins->hidh_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_UNREGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hidh_r.status != BT_STATUS_SUCCESS)
        return false;

    return true;
}

bt_status_t bt_hid_host_connect(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.hidh_pl._bt_hid_host_connect.addr, addr, sizeof(bt_address_t));
    packet.hidh_pl._bt_hid_host_connect.transport = transport;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_CONNECT);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}

bt_status_t bt_hid_host_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.hidh_pl._bt_hid_host_disconnect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_DISCONNECT);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}

bt_status_t bt_hid_host_get_report(bt_instance_t* ins, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.hidh_pl._bt_hid_host_get_report.addr, addr, sizeof(bt_address_t));
    packet.hidh_pl._bt_hid_host_get_report.report_id = report_id;
    packet.hidh_pl._bt_hid_host_get_report.report_type = report_type;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_GET_REPORT);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}

bt_status_t bt_hid_host_set_report(bt_instance_t* ins, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type,
    const uint8_t* data, uint16_t len)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    if (len > sizeof(packet.hidh_pl._bt_hid_host_set_report.data))
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.hidh_pl._bt_hid_host_set_report.addr, addr, sizeof(bt_address_t));
    packet.hidh_pl._bt_hid_host_set_report.report_id = report_id;
    packet.hidh_pl._bt_hid_host_set_report.report_type = report_type;
    packet.hidh_pl._bt_hid_host_set_report.len = len;
    memcpy(packet.hidh_pl._bt_hid_host_set_report.data, data, len);
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_SET_REPORT);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}

bt_status_t bt_hid_host_set_protocol(bt_instance_t* ins, bt_address_t* addr,
    uint8_t protocol_mode)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.hidh_pl._bt_hid_host_set_protocol.addr, addr, sizeof(bt_address_t));
    packet.hidh_pl._bt_hid_host_set_protocol.protocol_mode = protocol_mode;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_SET_PROTOCOL);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}

bt_status_t bt_hid_host_suspend(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.hidh_pl._bt_hid_host_suspend.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_SUSPEND);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}

bt_status_t bt_hid_host_exit_suspend(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.hidh_pl._bt_hid_host_exit_suspend.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_EXIT_SUSPEND);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}

bt_status_t bt_hid_host_set_mode(bt_instance_t* ins, bt_address_t* addr,
    uint8_t mode, uint8_t level)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.hidh_pl._bt_hid_host_set_mode.addr, addr, sizeof(bt_address_t));
    packet.hidh_pl._bt_hid_host_set_mode.mode = mode;
    packet.hidh_pl._bt_hid_host_set_mode.level = level;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_HOST_SET_MODE);
    return status != BT_STATUS_SUCCESS ? status : packet.hidh_r.status;
}
