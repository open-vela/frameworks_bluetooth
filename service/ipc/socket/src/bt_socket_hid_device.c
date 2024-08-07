
/****************************************************************************
 * service/ipc/socket/src/bt_socket_hid_device.c
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
#include "bt_hid_device.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "service_loop.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->hidd_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
#include "hid_device_service.h"
#include "service_manager.h"

static void on_app_state_changed_cb(void* cookie, hid_app_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    packet.hidd_cb._app_state.state = state;
    bt_socket_server_send(ins, &packet, BT_HID_DEVICE_APP_STATE);
}
static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, bool le_hid,
    profile_connection_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.hidd_cb._connection_state.addr, addr, sizeof(bt_address_t));
    packet.hidd_cb._connection_state.le_hid = le_hid;
    packet.hidd_cb._connection_state.state = state;
    bt_socket_server_send(ins, &packet, BT_HID_DEVICE_CONNECTION_STATE);
}
static void on_get_report_cb(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint8_t rpt_id, uint16_t buffer_size)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.hidd_cb._on_get_report.addr, addr, sizeof(bt_address_t));
    packet.hidd_cb._on_get_report.rpt_type = rpt_type;
    packet.hidd_cb._on_get_report.rpt_id = rpt_id;
    packet.hidd_cb._on_get_report.buffer_size = buffer_size;
    bt_socket_server_send(ins, &packet, BT_HID_DEVICE_ON_GET_REPORT);
}
static void on_set_report_cb(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    if (rpt_size > sizeof(packet.hidd_cb._on_set_report.rpt_data)) {
        BT_LOGW("exceeds hidd maximum report size :%d", rpt_size);
        rpt_size = sizeof(packet.hidd_cb._on_set_report.rpt_data);
    }

    memcpy(&packet.hidd_cb._on_set_report.addr, addr, sizeof(bt_address_t));
    packet.hidd_cb._on_set_report.rpt_type = rpt_type;
    packet.hidd_cb._on_set_report.rpt_size = rpt_size;
    memcpy(packet.hidd_cb._on_set_report.rpt_data, rpt_data, rpt_size);
    bt_socket_server_send(ins, &packet, BT_HID_DEVICE_ON_SET_REPORT);
}
static void on_receive_report_cb(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    if (rpt_size > sizeof(packet.hidd_cb._on_receive_report.rpt_data)) {
        BT_LOGW("exceeds hidd maximum report size :%d", rpt_size);
        rpt_size = sizeof(packet.hidd_cb._on_receive_report.rpt_data);
    }

    memcpy(&packet.hidd_cb._on_receive_report.addr, addr, sizeof(bt_address_t));
    packet.hidd_cb._on_receive_report.rpt_type = rpt_type;
    packet.hidd_cb._on_receive_report.rpt_size = rpt_size;
    memcpy(packet.hidd_cb._on_receive_report.rpt_data, rpt_data, rpt_size);
    bt_socket_server_send(ins, &packet, BT_HID_DEVICE_ON_RECEIVE_REPORT);
}
static void on_virtual_unplug_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    memcpy(&packet.hidd_cb._on_virtual_unplug.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(ins, &packet, BT_HID_DEVICE_ON_VIRTUAL_UNPLUG);
}
const static hid_device_callbacks_t g_hid_device_socket_cbs = {
    .app_state_cb = on_app_state_changed_cb,
    .connection_state_cb = on_connection_state_changed_cb,
    .get_report_cb = on_get_report_cb,
    .set_report_cb = on_set_report_cb,
    .receive_report_cb = on_receive_report_cb,
    .virtual_unplug_cb = on_virtual_unplug_cb,
};

static void parse_and_copy_sdp(char* sdp_data, hid_device_sdp_settings_t* sdp_setting)
{
    uint32_t data_offset = 0;
    uint32_t info_len = offsetof(hid_info_t, dsc_list);

    sdp_setting->name = sdp_data;
    data_offset = (strlen(sdp_setting->name) + 1);
    sdp_data += data_offset;

    sdp_setting->description = sdp_data;
    data_offset = (strlen(sdp_setting->description) + 1);
    sdp_data += data_offset;

    sdp_setting->provider = sdp_data;
    data_offset = (strlen(sdp_setting->provider) + 1);
    sdp_data += data_offset;

    memcpy(&sdp_setting->hids_info, sdp_data, info_len);
    sdp_data += info_len;

    sdp_setting->hids_info.dsc_list = (uint8_t*)sdp_data;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void bt_socket_server_hid_device_process(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet)
{
    hid_device_interface_t* profile;
    hid_device_sdp_settings_t temp_sdp_setting;

    switch (packet->code) {
    case BT_HID_DEVICE_REGISTER_CALLBACK:
        if (ins->hidd_cookie == NULL) {
            profile = (hid_device_interface_t*)service_manager_get_profile(PROFILE_HID_DEV);
            ins->hidd_cookie = profile->register_callbacks((void*)ins, (void*)&g_hid_device_socket_cbs);
            if (ins->hidd_cookie)
                packet->hidd_r.status = BT_STATUS_SUCCESS;
            else
                packet->hidd_r.status = BT_STATUS_NO_RESOURCES;
        } else {
            packet->hidd_r.status = BT_STATUS_BUSY;
        }
        break;
    case BT_HID_DEVICE_UNREGISTER_CALLBACK:
        if (ins->hidd_cookie) {
            profile = (hid_device_interface_t*)service_manager_get_profile(PROFILE_HID_DEV);
            profile->unregister_callbacks((void**)&ins, ins->hidd_cookie);
            ins->hidd_cookie = NULL;
            packet->hidd_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->hidd_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case BT_HID_DEVICE_REGISTER_APP:
        parse_and_copy_sdp((char*)packet->hidd_pl._bt_hid_device_register_app.sdp, &temp_sdp_setting);
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_register_app)(ins,
            &temp_sdp_setting,
            packet->hidd_pl._bt_hid_device_register_app.le_hid);
        break;
    case BT_HID_DEVICE_UNREGISTER_APP:
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_unregister_app)(ins);
        break;
    case BT_HID_DEVICE_CONNECT:
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_connect)(ins,
            &packet->hidd_pl._bt_hid_device_connect.addr);
        break;
    case BT_HID_DEVICE_DISCONNECT:
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_disconnect)(ins,
            &packet->hidd_pl._bt_hid_device_disconnect.addr);
        break;
    case BT_HID_DEVICE_SEND_REPORT:
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_send_report)(ins,
            &packet->hidd_pl._bt_hid_device_send_report.addr,
            packet->hidd_pl._bt_hid_device_send_report.rpt_id,
            packet->hidd_pl._bt_hid_device_send_report.rpt_data,
            packet->hidd_pl._bt_hid_device_send_report.rpt_size);
        break;
    case BT_HID_DEVICE_RESPONSE_REPORT:
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_response_report)(ins,
            &packet->hidd_pl._bt_hid_device_response_report.addr,
            packet->hidd_pl._bt_hid_device_response_report.rpt_type,
            packet->hidd_pl._bt_hid_device_response_report.rpt_data,
            packet->hidd_pl._bt_hid_device_response_report.rpt_size);
        break;
    case BT_HID_DEVICE_REPORT_ERROR:
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_report_error)(ins,
            &packet->hidd_pl._bt_hid_device_report_error.addr,
            packet->hidd_pl._bt_hid_device_report_error.error);
        break;
    case BT_HID_DEVICE_VIRTUAL_UNPLUG:
        packet->hidd_r.status = BTSYMBOLS(bt_hid_device_virtual_unplug)(ins,
            &packet->hidd_pl._bt_hid_device_virtual_unplug.addr);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_hid_device_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_HID_DEVICE_APP_STATE:
        CALLBACK_FOREACH(CBLIST, hid_device_callbacks_t,
            app_state_cb,
            packet->hidd_cb._app_state.state);
        break;
    case BT_HID_DEVICE_CONNECTION_STATE:
        CALLBACK_FOREACH(CBLIST, hid_device_callbacks_t,
            connection_state_cb,
            &packet->hidd_cb._connection_state.addr,
            packet->hidd_cb._connection_state.le_hid,
            packet->hidd_cb._connection_state.state);
        break;
    case BT_HID_DEVICE_ON_GET_REPORT:
        CALLBACK_FOREACH(CBLIST, hid_device_callbacks_t,
            get_report_cb,
            &packet->hidd_cb._on_get_report.addr,
            packet->hidd_cb._on_get_report.rpt_type,
            packet->hidd_cb._on_get_report.rpt_id,
            packet->hidd_cb._on_get_report.buffer_size);
        break;
    case BT_HID_DEVICE_ON_SET_REPORT:
        CALLBACK_FOREACH(CBLIST, hid_device_callbacks_t,
            set_report_cb,
            &packet->hidd_cb._on_set_report.addr,
            packet->hidd_cb._on_set_report.rpt_type,
            packet->hidd_cb._on_set_report.rpt_size,
            packet->hidd_cb._on_set_report.rpt_data);
        break;
    case BT_HID_DEVICE_ON_RECEIVE_REPORT:
        CALLBACK_FOREACH(CBLIST, hid_device_callbacks_t,
            receive_report_cb,
            &packet->hidd_cb._on_receive_report.addr,
            packet->hidd_cb._on_receive_report.rpt_type,
            packet->hidd_cb._on_receive_report.rpt_size,
            packet->hidd_cb._on_receive_report.rpt_data);
        break;
    case BT_HID_DEVICE_ON_VIRTUAL_UNPLUG:
        CALLBACK_FOREACH(CBLIST, hid_device_callbacks_t,
            virtual_unplug_cb,
            &packet->hidd_cb._on_virtual_unplug.addr);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }
    return BT_STATUS_SUCCESS;
}
