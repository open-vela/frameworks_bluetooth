/****************************************************************************
 * service/ipc/socket/src/bt_socket_auracast_sink.c
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

#include "bt_internal.h"

#include "auracast_sink_service.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "service_manager.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (__async ? __async->auracast_sink_callbacks : ins->auracast_sink_callbacks)

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static const inline auracast_sink_interface_t* get_profile_service(void)
{
    return (auracast_sink_interface_t*)service_manager_get_profile(PROFILE_AURACAST_SINK);
}

static void on_sync_established_cb(void* cookie, const bt_le_address_t* addr, uint8_t sid)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.auracast_sink_cb._on_sync_established.addr, addr, sizeof(bt_le_address_t));
    packet.auracast_sink_cb._on_sync_established.sid = sid;

    bt_socket_server_send(ins, &packet, BT_AURACAST_SINK_ON_SYNC_ESTABLISHED);
}

static void on_sync_terminated_cb(void* cookie, const bt_le_address_t* addr, uint8_t sid)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.auracast_sink_cb._on_sync_terminated.addr, addr, sizeof(bt_le_address_t));
    packet.auracast_sink_cb._on_sync_terminated.sid = sid;

    bt_socket_server_send(ins, &packet, BT_AURACAST_SINK_ON_SYNC_TERMINATED);
}

static const bt_auracast_sink_callbacks_t g_auracast_sink_socket_cb = {
    .on_sync_established = on_sync_established_cb,
    .on_sync_terminated = on_sync_terminated_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void bt_socket_server_auracast_sink_process(service_poll_t* poll, int fd, bt_instance_t* ins,
    bt_message_packet_t* packet)
{
    const auracast_sink_interface_t* profile;

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case AURACAST_SINK_SUBCODE_REGISTER_CALLBACKS:
        if (ins->auracast_sink_cookie) {
            packet->auracast_sink_r.status = BT_STATUS_BUSY;
            break;
        }

        profile = get_profile_service();
        if (!profile) {
            packet->auracast_sink_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            break;
        }

        ins->auracast_sink_cookie = profile->register_callbacks(ins, &g_auracast_sink_socket_cb);
        if (!ins->auracast_sink_cookie) {
            packet->auracast_sink_r.status = BT_STATUS_NO_RESOURCES;
            break;
        }

        packet->auracast_sink_r.status = BT_STATUS_SUCCESS;
        break;
    case AURACAST_SINK_SUBCODE_UNREGISTER_CALLBACKS:
        if (!ins->auracast_sink_cookie) {
            packet->auracast_sink_r.status = BT_STATUS_NOT_FOUND;
            break;
        }

        profile = get_profile_service();
        if (profile && profile->unregister_callbacks) {
            packet->auracast_sink_r.status = profile->unregister_callbacks(NULL,
                                                 ins->auracast_sink_cookie)
                ? BT_STATUS_SUCCESS
                : BT_STATUS_FAIL;
        }

        ins->auracast_sink_cookie = NULL;
        break;
    case AURACAST_SINK_SUBCODE_CREATE_SYNC:
        packet->auracast_sink_r.status = BTSYMBOLS(bt_auracast_sink_create_sync)(ins,
            &packet->auracast_sink_pl._bt_auracast_sink_create_sync.addr,
            packet->auracast_sink_pl._bt_auracast_sink_create_sync.sid,
            packet->auracast_sink_pl._bt_auracast_sink_create_sync.bitfield,
            packet->auracast_sink_pl._bt_auracast_sink_create_sync.encrypted
                ? packet->auracast_sink_pl._bt_auracast_sink_create_sync.broadcast_code
                : NULL);
        break;
    case AURACAST_SINK_SUBCODE_TERMINATE_SYNC:
        packet->auracast_sink_r.status = BTSYMBOLS(bt_auracast_sink_terminate_sync)(ins,
            &packet->auracast_sink_pl._bt_auracast_sink_terminate_sync.addr,
            packet->auracast_sink_pl._bt_auracast_sink_terminate_sync.sid);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_auracast_sink_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    bt_socket_async_client_t* __async = NULL;

    if (is_async)
        __async = ins->priv;

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case AURACAST_SINK_SUBCODE_SYNC_ESTABLISHED_CALLBACK:
        CALLBACK_FOREACH(CBLIST, bt_auracast_sink_callbacks_t,
            on_sync_established,
            &packet->auracast_sink_cb._on_sync_established.addr,
            packet->auracast_sink_cb._on_sync_established.sid);
        break;
    case AURACAST_SINK_SUBCODE_SYNC_TERMINATED_CALLBACK:
        CALLBACK_FOREACH(CBLIST, bt_auracast_sink_callbacks_t,
            on_sync_terminated,
            &packet->auracast_sink_cb._on_sync_terminated.addr,
            packet->auracast_sink_cb._on_sync_terminated.sid);
        break;
    default:
        break;
    }

    return BT_STATUS_SUCCESS;
}
