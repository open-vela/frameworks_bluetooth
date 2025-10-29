/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#include <string.h>

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

bt_status_t bt_pbap_pce_get_contact_by_name(bt_instance_t* ins, bt_address_t* addr, char* name)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_get_contact_by_name.addr, addr, sizeof(bt_address_t));
    strlcpy(packet.pbap_pce_pl._bt_pbap_pce_get_contact_by_name.name, name, BT_PBAP_PCE_PROPERTY_MAX_LEN);
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_GET_CONTACT_BY_NAME);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bt_status_t bt_pbap_pce_get_contact_by_number(bt_instance_t* ins, bt_address_t* addr, char* number)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_get_contact_by_number.addr, addr, sizeof(bt_address_t));
    strlcpy(packet.pbap_pce_pl._bt_pbap_pce_get_contact_by_number.number, number, BT_PBAP_PCE_PROPERTY_MAX_LEN);
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_GET_CONTACT_BY_NUMBER);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bt_status_t bt_pbap_pce_add_to_blacklist(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_add_to_blacklist.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_ADD_TO_BLACKLIST);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bt_status_t bt_pbap_pce_remove_from_blacklist(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_remove_from_blacklist.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_REMOVE_FROM_BLACKLIST);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pbap_pce_r.status;
}

bool bt_pbap_pce_is_in_blacklist(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    memcpy(&packet.pbap_pce_pl._bt_pbap_pce_is_in_blacklist.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_PBAP_PCE_IS_IN_BLACKLIST);
    if (status != BT_STATUS_SUCCESS)
        return false;

    return packet.pbap_pce_r.value_bool;
}
