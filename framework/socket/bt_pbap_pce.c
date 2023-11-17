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
#define LOG_TAG "pbap_pce_api"

#include <stdint.h>

#include "bt_pbap_pce.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "pbap_pce_service.h"
#include "service_manager.h"
#include "utils/log.h"

void* bt_pbap_pce_register_callbacks(bt_instance_t* ins, const pbap_pce_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    if (ins->pbap_pce_callbacks != NULL)
        return NULL;

    ins->pbap_pce_callbacks = bt_callbacks_list_new(2);

    cookie = bt_remote_callbacks_register(ins->pbap_pce_callbacks, NULL, (void*)callbacks);
    if (cookie == NULL)
        return NULL;

    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_REGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.pbap_pce_r.status != BT_STATUS_SUCCESS)
        return NULL;

    return cookie;
}

bool bt_pbap_pce_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;

    bt_remote_callbacks_unregister(ins->pbap_pce_callbacks, NULL, cookie);
    ins->pbap_pce_callbacks = NULL;

    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_UNREGISTER_CALLBACK);
    if (status != BT_STATUS_SUCCESS || packet.pbap_pce_r.status != BT_STATUS_SUCCESS)
        return false;

    return true;
}

bt_status_t bt_pbap_pce_connect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_connect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bt_status_t bt_pbap_pce_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_disconnect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bt_status_t bt_pbap_pce_change_directory(bt_instance_t* ins, bt_address_t* addr, const char* dir)
{
    bt_message_packet_t packet;
    bt_status_t status;

    if (strlen(dir) > PBAP_PKT_LEN_MAX)
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_change_dir.addr, addr, sizeof(bt_address_t));
    strncpy(packet.pbap_pce_pl._bt_pbap_pce_change_dir.dir, dir,
        sizeof(packet.pbap_pce_pl._bt_pbap_pce_change_dir.dir));
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_CHANGE_DIRECTORY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bt_status_t bt_pbap_pce_pull_vcard_listing(bt_instance_t* ins, bt_address_t* addr,
    pbap_search_property_t property, const char* value)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.addr, addr, sizeof(bt_address_t));
    if (value) {
        if (strlen(value) > PBAP_PKT_LEN_MAX)
            return BT_STATUS_PARM_INVALID;

        packet.pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.property = property;
        strncpy(packet.pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.value, value,
            sizeof(packet.pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.value));
    } else {
        packet.pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.property = PCE_SEARCH_PROPERTY_NONE;
        memset(packet.pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.value, 0,
            sizeof(packet.pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.value));
    }
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_PULL_VCARD_LISTING);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bt_status_t bt_pbap_pce_pull_vcard(bt_instance_t* ins, bt_address_t* addr, const char* object, uint64_t filter)
{
    bt_message_packet_t packet;
    bt_status_t status;

    if (!object)
        return BT_STATUS_PARM_INVALID;

    if (strlen(object) > PBAP_PKT_LEN_MAX)
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_pull_vcard.addr, addr, sizeof(bt_address_t));
    strncpy(packet.pbap_pce_pl._bt_pbap_pce_pull_vcard.object, object,
        sizeof(packet.pbap_pce_pl._bt_pbap_pce_pull_vcard.object));
    packet.pbap_pce_pl._bt_pbap_pce_pull_vcard.filter = filter;
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_PULL_VCARD);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}
