/****************************************************************************
 * service/ipc/socket/src/bt_socket_pbap_pce.c
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
#include "bt_message.h"
#include "bt_pbap_pce.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "pbap_pce_service.h"
#include "service_loop.h"
#include "service_manager.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->pbap_pce_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    bt_message_packet_t packet;
    bt_instance_t* ins = cookie;

    memcpy(&packet.pbap_pce_cb._on_connection_state_changed.addr, addr, sizeof(bt_address_t));
    packet.pbap_pce_cb._on_connection_state_changed.state = state;

    bt_socket_server_send(ins, &packet, BT_PBAP_PCE_ON_CONNECTION_STATE_CHANGED);
}

static void get_contact_end_cb(void* cookie, bt_status_t status, bt_pce_get_contact_req_type_t req_type, char* req_data, bt_pce_contact_t* contact)
{
    bt_message_packet_t packet;
    bt_instance_t* ins = cookie;

    packet.pbap_pce_cb._on_get_contact_end.status = status;
    packet.pbap_pce_cb._on_get_contact_end.req_type = req_type;

    strlcpy(packet.pbap_pce_cb._on_get_contact_end.req_data, req_data, sizeof(packet.pbap_pce_cb._on_get_contact_end.req_data));
    memcpy(&packet.pbap_pce_cb._on_get_contact_end.contact, contact, sizeof(bt_pce_contact_t));

    bt_socket_server_send(ins, &packet, BT_PBAP_PCE_ON_GET_CONTACT_END);
}

const static pbap_pce_callbacks_t g_pbap_pce_socket_cbs = {
    .connection_state_cb = on_connection_state_changed_cb,
    .get_contact_end_cb = get_contact_end_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_pbap_pce_process(service_poll_t* poll, int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    pbap_pce_interface_t* profile;

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case PBAP_PCE_SUBCODE_REGISTER_CALLBACK:
        if (ins->pbap_pce_cookie != NULL) {
            packet->pbap_pce_r.status = BT_STATUS_BUSY;
            break;
        }

        profile = (pbap_pce_interface_t*)service_manager_get_profile(PROFILE_PBAP_PCE);
        if (profile == NULL) {
            packet->pbap_pce_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            break;
        }

        ins->pbap_pce_cookie = profile->register_callbacks((void*)ins, (void*)&g_pbap_pce_socket_cbs);
        if (ins->pbap_pce_cookie)
            packet->pbap_pce_r.status = BT_STATUS_SUCCESS;
        else
            packet->pbap_pce_r.status = BT_STATUS_NO_RESOURCES;

        break;
    case PBAP_PCE_SUBCODE_UNREGISTER_CALLBACK:
        if (ins->pbap_pce_cookie) {
            profile = (pbap_pce_interface_t*)service_manager_get_profile(PROFILE_PBAP_PCE);
            if (profile)
                profile->unregister_callbacks((void**)&ins, ins->pbap_pce_cookie);
            ins->pbap_pce_cookie = NULL;
            packet->pbap_pce_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->pbap_pce_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case PBAP_PCE_SUBCODE_CONNECT:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_connect)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_connect.addr);
        break;
    case PBAP_PCE_SUBCODE_DISCONNECT:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_disconnect)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_disconnect.addr);
        break;
    case PBAP_PCE_SUBCODE_GET_CONTACT_BY_NAME:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_get_contact_by_name)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_get_contact_by_name.addr,
            packet->pbap_pce_pl._bt_pbap_pce_get_contact_by_name.name);
        break;
    case PBAP_PCE_SUBCODE_GET_CONTACT_BY_NUMBER:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_get_contact_by_number)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_get_contact_by_number.addr,
            packet->pbap_pce_pl._bt_pbap_pce_get_contact_by_number.number);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_pbap_pce_callback(service_poll_t* poll, int fd, bt_instance_t* ins,
    bt_message_packet_t* packet, bool is_async)
{
    bt_socket_async_client_t* __async = NULL;

    if (is_async)
        __async = ins->priv;

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case PBAP_PCE_SUBCODE_ON_CONNECTION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            connection_state_cb,
            &packet->pbap_pce_cb._on_connection_state_changed.addr,
            packet->pbap_pce_cb._on_connection_state_changed.state);
        break;
    case PBAP_PCE_SUBCODE_ON_GET_CONTACT_END:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            get_contact_end_cb,
            packet->pbap_pce_cb._on_get_contact_end.status,
            packet->pbap_pce_cb._on_get_contact_end.req_type,
            packet->pbap_pce_cb._on_get_contact_end.req_data,
            &packet->pbap_pce_cb._on_get_contact_end.contact);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
