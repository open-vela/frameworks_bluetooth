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
#define LOG_TAG "hid_device_api"

#include <stdint.h>

#include "bt_hid_device.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "hid_device_service.h"
#include "service_manager.h"
#include "utils/log.h"

static bt_status_t safety_assemble_sdp_array(uint8_t** sdp_ptr, size_t* remaining_space, const char* src)
{
    uint32_t src_len = strlen(src);
    if (src_len + 1 > *remaining_space) {
        return BT_STATUS_NO_RESOURCES;
    }

    memcpy(*sdp_ptr, src, src_len);
    (*sdp_ptr)[src_len] = '\0';
    *sdp_ptr += (src_len + 1);
    *remaining_space -= (src_len + 1);

    return BT_STATUS_SUCCESS;
}

static bt_status_t safety_assemble_hid_info(uint8_t** sdp_ptr, size_t* remaining_space, const hid_info_t* hid_info)
{
    uint32_t info_len = offsetof(hid_info_t, dsc_list);

    if (info_len + hid_info->dsc_list_length > *remaining_space) {
        return BT_STATUS_NO_RESOURCES;
    }

    memcpy(*sdp_ptr, hid_info, info_len);
    *sdp_ptr += info_len;
    *remaining_space -= info_len;

    memcpy(*sdp_ptr, hid_info->dsc_list, hid_info->dsc_list_length);
    *sdp_ptr += hid_info->dsc_list_length;
    *remaining_space -= hid_info->dsc_list_length;

    return BT_STATUS_SUCCESS;
}

void* bt_hid_device_register_callbacks(bt_instance_t* ins, const hid_device_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->hidd_callbacks != NULL) {
        return NULL;
    }

    ins->hidd_callbacks = bt_callbacks_list_new(1);

    cookie = bt_remote_callbacks_register(ins->hidd_callbacks, NULL, (void*)callbacks);
    if (cookie == NULL) {
        bt_callbacks_list_free(ins->hidd_callbacks);
        ins->hidd_callbacks = NULL;
        return NULL;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_REGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hidd_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->hidd_callbacks);
        ins->hidd_callbacks = NULL;
        return NULL;
    }

    return cookie;
}

bool bt_hid_device_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->hidd_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->hidd_callbacks, NULL, cookie);

    cbsl = ins->hidd_callbacks;
    ins->hidd_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_UNREGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.hidd_r.status != BT_STATUS_SUCCESS)
        return false;

    return true;
}

bt_status_t bt_hid_device_register_app(bt_instance_t* ins, hid_device_sdp_settings_t* sdp, bool le_hid)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.hidd_pl._bt_hid_device_register_app.le_hid = le_hid;
    uint8_t* sdp_ptr = packet.hidd_pl._bt_hid_device_register_app.sdp;
    size_t remaining_space = sizeof(packet.hidd_pl._bt_hid_device_register_app.sdp);

    if (BT_STATUS_SUCCESS != safety_assemble_sdp_array(&sdp_ptr, &remaining_space, sdp->name)) {
        return BT_STATUS_NO_RESOURCES;
    }

    if (BT_STATUS_SUCCESS != safety_assemble_sdp_array(&sdp_ptr, &remaining_space, sdp->description)) {
        return BT_STATUS_NO_RESOURCES;
    }

    if (BT_STATUS_SUCCESS != safety_assemble_sdp_array(&sdp_ptr, &remaining_space, sdp->provider)) {
        return BT_STATUS_NO_RESOURCES;
    }

    if (BT_STATUS_SUCCESS != safety_assemble_hid_info(&sdp_ptr, &remaining_space, &sdp->hids_info)) {
        return BT_STATUS_NO_RESOURCES;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_REGISTER_APP);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}

bt_status_t bt_hid_device_unregister_app(bt_instance_t* ins)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_UNREGISTER_APP);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}

bt_status_t bt_hid_device_connect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hidd_pl._bt_hid_device_connect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}

bt_status_t bt_hid_device_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hidd_pl._bt_hid_device_disconnect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}

bt_status_t bt_hid_device_send_report(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (rpt_size > sizeof(packet.hidd_pl._bt_hid_device_send_report.rpt_data))
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.hidd_pl._bt_hid_device_send_report.addr, addr, sizeof(bt_address_t));
    packet.hidd_pl._bt_hid_device_send_report.rpt_id = rpt_id;
    packet.hidd_pl._bt_hid_device_send_report.rpt_size = rpt_size;
    memcpy(packet.hidd_pl._bt_hid_device_send_report.rpt_data, rpt_data, rpt_size);
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_SEND_REPORT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}

bt_status_t bt_hid_device_response_report(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (rpt_size > sizeof(packet.hidd_pl._bt_hid_device_response_report.rpt_data))
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.hidd_pl._bt_hid_device_response_report.addr, addr, sizeof(bt_address_t));
    packet.hidd_pl._bt_hid_device_response_report.rpt_type = rpt_type;
    packet.hidd_pl._bt_hid_device_response_report.rpt_size = rpt_size;
    memcpy(packet.hidd_pl._bt_hid_device_response_report.rpt_data, rpt_data, rpt_size);
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_RESPONSE_REPORT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}

bt_status_t bt_hid_device_report_error(bt_instance_t* ins, bt_address_t* addr, hid_status_error_t error)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hidd_pl._bt_hid_device_report_error.addr, addr, sizeof(bt_address_t));
    packet.hidd_pl._bt_hid_device_report_error.error = error;
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_REPORT_ERROR);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}

bt_status_t bt_hid_device_virtual_unplug(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.hidd_pl._bt_hid_device_virtual_unplug.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_HID_DEVICE_VIRTUAL_UNPLUG);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.hidd_r.status;
}
