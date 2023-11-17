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

static void on_dir_changed_cb(void* cookie, bt_address_t* addr, uint16_t status)
{
    bt_message_packet_t packet;
    bt_instance_t* ins = cookie;

    memcpy(&packet.pbap_pce_cb._on_dir_changed.addr, addr, sizeof(bt_address_t));
    packet.pbap_pce_cb._on_dir_changed.status = status;

    bt_socket_server_send(ins, &packet, BT_PBAP_PCE_ON_DIR_CHANGED);
}

static void on_vcard_listing_data_cb(void* cookie, bt_address_t* addr, uint16_t len, char* obj)
{
    bt_message_packet_t packet;
    bt_instance_t* ins = cookie;
    int segment, offset;

    offset = 0;
    memcpy(&packet.pbap_pce_cb._on_vcard_listing_data_received.addr, addr, sizeof(bt_address_t));
    while (len) { /* Split vCard Listing objects into segments under PBAP_PKT_LEN_MAX bytes */
        segment = MIN(len, PBAP_PKT_LEN_MAX);
        memset(packet.pbap_pce_cb._on_vcard_listing_data_received.object, 0,
            sizeof(packet.pbap_pce_cb._on_vcard_listing_data_received.object));
        memcpy(packet.pbap_pce_cb._on_vcard_listing_data_received.object, obj + offset, segment);
        packet.pbap_pce_cb._on_vcard_listing_data_received.len = segment;
        bt_socket_server_send(ins, &packet, BT_PBAP_PCE_ON_VCARD_LISTING_DATA_RECEIVED);

        offset += segment;
        len -= segment;
    }
}

static void on_vcard_listing_end_cb(void* cookie, bt_address_t* addr, uint16_t status)
{
    bt_message_packet_t packet;
    bt_instance_t* ins = cookie;

    memcpy(&packet.pbap_pce_cb._on_vcard_listing_end.addr, addr, sizeof(bt_address_t));
    packet.pbap_pce_cb._on_vcard_listing_end.status = status;

    bt_socket_server_send(ins, &packet, BT_PBAP_PCE_ON_VCARD_LISTING_END);
}

static void on_vcard_data_cb(void* cookie, bt_address_t* addr, uint16_t len, char* obj)
{
    bt_message_packet_t packet;
    bt_instance_t* ins = cookie;
    int segment, offset;

    offset = 0;
    memcpy(&packet.pbap_pce_cb._on_vcard_data_received.addr, addr, sizeof(bt_address_t));
    while (len) { /* Split vCard objects into segments under PBAP_PKT_LEN_MAX bytes */
        segment = MIN(len, PBAP_PKT_LEN_MAX);
        memset(packet.pbap_pce_cb._on_vcard_data_received.object, 0,
            sizeof(packet.pbap_pce_cb._on_vcard_data_received.object));
        memcpy(packet.pbap_pce_cb._on_vcard_data_received.object, obj + offset, segment);
        packet.pbap_pce_cb._on_vcard_data_received.len = segment;
        bt_socket_server_send(ins, &packet, BT_PBAP_PCE_ON_VCARD_DATA_RECEIVED);

        offset += segment;
        len -= segment;
    }
}

static void on_vcard_end_cb(void* cookie, bt_address_t* addr, uint16_t status)
{
    bt_message_packet_t packet;
    bt_instance_t* ins = cookie;

    memcpy(&packet.pbap_pce_cb._on_vcard_end.addr, addr, sizeof(bt_address_t));
    packet.pbap_pce_cb._on_vcard_end.status = status;

    bt_socket_server_send(ins, &packet, BT_PBAP_PCE_ON_VCARD_END);
}

const static pbap_pce_callbacks_t g_pbap_pce_socket_cbs = {
    .connection_state_cb = on_connection_state_changed_cb,
    .dir_changed_cb = on_dir_changed_cb,
    .vcard_listing_data_cb = on_vcard_listing_data_cb,
    .vcard_listing_end_cb = on_vcard_listing_end_cb,
    .vcard_data_cb = on_vcard_data_cb,
    .vcard_end_cb = on_vcard_end_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_pbap_pce_process(service_poll_t* poll, int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    pbap_pce_interface_t* profile;

    switch (packet->code) {
    case BT_PBAP_PCE_REGISTER_CALLBACK:
        if (ins->pbap_pce_cookie == NULL) {
            profile = (pbap_pce_interface_t*)service_manager_get_profile(PROFILE_PBAP_PCE);
            if (profile) {
                ins->pbap_pce_cookie = profile->register_callbacks((void*)ins, (void*)&g_pbap_pce_socket_cbs);
                if (ins->pbap_pce_cookie)
                    packet->pbap_pce_r.status = BT_STATUS_SUCCESS;
                else
                    packet->pbap_pce_r.status = BT_STATUS_NO_RESOURCES;
            } else {
                packet->pbap_pce_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            }
        } else {
            packet->pbap_pce_r.status = BT_STATUS_BUSY;
        }
        break;
    case BT_PBAP_PCE_UNREGISTER_CALLBACK:
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
    case BT_PBAP_PCE_CONNECT:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_connect)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_connect.addr);
        break;
    case BT_PBAP_PCE_DISCONNECT:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_disconnect)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_disconnect.addr);
        break;
    case BT_PBAP_PCE_CHANGE_DIRECTORY:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_change_directory)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_change_dir.addr,
            packet->pbap_pce_pl._bt_pbap_pce_change_dir.dir);
        break;
    case BT_PBAP_PCE_PULL_VCARD_LISTING:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_pull_vcard_listing)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.addr,
            packet->pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.property,
            packet->pbap_pce_pl._bt_pbap_pce_pull_vcard_listing.value);
        break;
    case BT_PBAP_PCE_PULL_VCARD:
        packet->pbap_pce_r.status = BTSYMBOLS(bt_pbap_pce_pull_vcard)(ins,
            &packet->pbap_pce_pl._bt_pbap_pce_pull_vcard.addr,
            packet->pbap_pce_pl._bt_pbap_pce_pull_vcard.object,
            packet->pbap_pce_pl._bt_pbap_pce_pull_vcard.filter);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_pbap_pce_callback(service_poll_t* poll, int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_PBAP_PCE_ON_CONNECTION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            connection_state_cb,
            &packet->pbap_pce_cb._on_connection_state_changed.addr,
            packet->pbap_pce_cb._on_connection_state_changed.state);
        break;
    case BT_PBAP_PCE_ON_DIR_CHANGED:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            dir_changed_cb,
            &packet->pbap_pce_cb._on_dir_changed.addr,
            packet->pbap_pce_cb._on_dir_changed.status);
        break;
    case BT_PBAP_PCE_ON_VCARD_LISTING_DATA_RECEIVED:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            vcard_listing_data_cb,
            &packet->pbap_pce_cb._on_vcard_listing_data_received.addr,
            packet->pbap_pce_cb._on_vcard_listing_data_received.len,
            packet->pbap_pce_cb._on_vcard_listing_data_received.object);
        break;
    case BT_PBAP_PCE_ON_VCARD_LISTING_END:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            vcard_listing_end_cb,
            &packet->pbap_pce_cb._on_vcard_listing_end.addr,
            packet->pbap_pce_cb._on_vcard_listing_end.status);
        break;
    case BT_PBAP_PCE_ON_VCARD_DATA_RECEIVED:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            vcard_data_cb,
            &packet->pbap_pce_cb._on_vcard_data_received.addr,
            packet->pbap_pce_cb._on_vcard_data_received.len,
            packet->pbap_pce_cb._on_vcard_data_received.object);
        break;
    case BT_PBAP_PCE_ON_VCARD_END:
        CALLBACK_FOREACH(CBLIST, pbap_pce_callbacks_t,
            vcard_end_cb,
            &packet->pbap_pce_cb._on_vcard_end.addr,
            packet->pbap_pce_cb._on_vcard_end.status);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
