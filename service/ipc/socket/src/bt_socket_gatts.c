
/****************************************************************************
 * service/ipc/socket/src/bt_socket_gatts.c
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
#include "bt_gatts.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "service_loop.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CALLBACK_REMOTE(_remote, _type, _cback, ...) \
    do {                                             \
        _type *_cbs = (_type *)_remote->callback;    \
        if (_cbs && _cbs->_cback) {                  \
            _cbs->_cback(_remote, ##__VA_ARGS__);    \
        }                                            \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
#include "gatts_service.h"
#include "service_manager.h"

static void on_connected_cb(gatts_handle_t srv_handle, bt_address_t *addr)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_connected.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_CONNECTED);
}
static void on_disconnected_cb(gatts_handle_t srv_handle, bt_address_t *addr)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_disconnected.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_DISCONNECTED);
}
static void on_started_cb(gatts_handle_t srv_handle, gatt_status_t status)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    packet.gatts_cb._on_started.status = status;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_STARTED);
}
static void on_stopped_cb(gatts_handle_t srv_handle, gatt_status_t status)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    packet.gatts_cb._on_stopped.status = status;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_STOPPED);
}
static void on_notify_complete_cb(gatts_handle_t srv_handle, gatt_status_t status, uint16_t attr_handle)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    packet.gatts_cb._on_nofity_complete.status = status;
    packet.gatts_cb._on_nofity_complete.attr_handle = attr_handle;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_NOTIFY_COMPLETE);
}
static void on_mtu_changed_cb(gatts_handle_t srv_handle, bt_address_t *addr, uint32_t mtu)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_mtu_changed.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_mtu_changed.mtu = mtu;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_MTU_CHANGED);
}
static uint16_t on_read_request_cb(gatts_handle_t srv_handle, uint16_t attr_handle, uint32_t req_handle)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    packet.gatts_cb._on_read_request.attr_handle = attr_handle;
    packet.gatts_cb._on_read_request.req_handle = req_handle;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_READ_REQUEST);
    return 0;
}
static uint16_t on_write_request_cb(gatts_handle_t srv_handle, uint16_t attr_handle, const uint8_t *value, uint16_t length, uint16_t offset)
{
    bt_message_packet_t packet;
    bt_gatts_remote_t *gatts_remote = if_gatts_get_remote(srv_handle);

    if (length > sizeof(packet.gatts_cb._on_write_request.value)) {
        BT_LOGW("exceeds gatts maximum attr value size :%d", length);
        length = sizeof(packet.gatts_cb._on_write_request.value);
    }

    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    packet.gatts_cb._on_write_request.attr_handle = attr_handle;
    packet.gatts_cb._on_write_request.offset = offset;
    packet.gatts_cb._on_write_request.length = length;
    memcpy(packet.gatts_cb._on_write_request.value, value, length);
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_WRITE_REQUEST);
    return length;
}
const static gatts_callbacks_t g_gatts_socket_cbs = {
    .on_connected = on_connected_cb,
    .on_disconnected = on_disconnected_cb,
    .on_started = on_started_cb,
    .on_stopped = on_stopped_cb,
    .on_notify_complete = on_notify_complete_cb,
    .on_mtu_changed = on_mtu_changed_cb,
};
/****************************************************************************
 * Public Functions
 ****************************************************************************/
void bt_socket_server_gatts_process(service_poll_t *poll, int fd,
                                    bt_instance_t *ins, bt_message_packet_t *packet)
{
    switch (packet->code) {
    case BT_GATT_SERVER_REGISTER_SERVICE: {
        gatts_interface_t *profile = (gatts_interface_t *)service_manager_get_profile(PROFILE_GATTS);
        bt_gatts_remote_t *gatts_remote = malloc(sizeof(bt_gatts_remote_t));
        if (!gatts_remote) {
            packet->gatts_r.status = BT_STATUS_NO_RESOURCES;
            break;
        }

        gatts_remote->ins = ins;
        gatts_remote->cookie = packet->gatts_pl._bt_gatts_register.cookie;
        packet->gatts_r.status = profile->register_service(gatts_remote,
                                                           &packet->gatts_r.handle,
                                                           (gatts_callbacks_t *)&g_gatts_socket_cbs);
        if (packet->gatts_r.status != BT_STATUS_SUCCESS)
            free(gatts_remote);
        break;
    }
    case BT_GATT_SERVER_UNREGISTER_SERVICE: {
        bt_gatts_remote_t *gatts_remote = NULL;
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_unregister_service)(
                                           packet->gatts_pl._bt_gatts_unregister.handle);

        gatts_remote = if_gatts_get_remote(packet->gatts_pl._bt_gatts_unregister.handle);
        if (packet->gatts_r.status == BT_STATUS_SUCCESS)
            free(gatts_remote);
        break;
    }
    case BT_GATT_SERVER_CONNECT:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_connect)(
                                           packet->gatts_pl._bt_gatts_connect.handle,
                                           &packet->gatts_pl._bt_gatts_connect.addr,
                                           packet->gatts_pl._bt_gatts_connect.addr_type);
        break;
    case BT_GATT_SERVER_DISCONNECT:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_disconnect)(
                                           packet->gatts_pl._bt_gatts_disconnect.handle);
        break;
    case BT_GATT_SERVER_CREATE_SERVICE_TABLE: {
        gatt_srv_db_t srv_db = {
            .attr_num = packet->gatts_pl._bt_gatts_create_srv_tbl.attr_num,
            .attr_db = packet->gatts_pl._bt_gatts_create_srv_tbl.attr_db,
        };
        gatt_attr_db_t *attr_inst = srv_db.attr_db;
        for (int i = 0; i < srv_db.attr_num; i++, attr_inst++) {
            if (attr_inst->read_cb)
                attr_inst->read_cb = on_read_request_cb;
            if (attr_inst->write_cb)
                attr_inst->write_cb = on_write_request_cb;
        }

        packet->gatts_r.status = BTSYMBOLS(bt_gatts_create_service_table)(
                                           packet->gatts_pl._bt_gatts_create_srv_tbl.handle,
                                           &srv_db);
        break;
    }
    case BT_GATT_SERVER_START:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_start)(
                                           packet->gatts_pl._bt_gatts_start.handle);
        break;
    case BT_GATT_SERVER_STOP:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_stop)(
                                           packet->gatts_pl._bt_gatts_stop.handle);
        break;
    case BT_GATT_SERVER_RESPONSE:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_response)(
                                           packet->gatts_pl._bt_gatts_response.handle,
                                           packet->gatts_pl._bt_gatts_response.req_handle,
                                           packet->gatts_pl._bt_gatts_response.value,
                                           packet->gatts_pl._bt_gatts_response.length);
        break;
    case BT_GATT_SERVER_NOTIFY:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_notify)(
                                           packet->gatts_pl._bt_gatts_notify.handle,
                                           packet->gatts_pl._bt_gatts_notify.attr_handle,
                                           packet->gatts_pl._bt_gatts_notify.value,
                                           packet->gatts_pl._bt_gatts_notify.length);
        break;
    case BT_GATT_SERVER_INDICATE:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_indicate)(
                                           packet->gatts_pl._bt_gatts_notify.handle,
                                           packet->gatts_pl._bt_gatts_notify.attr_handle,
                                           packet->gatts_pl._bt_gatts_notify.value,
                                           packet->gatts_pl._bt_gatts_notify.length);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_gatts_callback(service_poll_t *poll,
                                    int fd, bt_instance_t *ins, bt_message_packet_t *packet)
{
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)packet->gatts_cb._on_callback.remote;
    switch (packet->code) {
    case BT_GATT_SERVER_ON_CONNECTED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
                        on_connected,
                        &packet->gatts_cb._on_connected.addr);
        break;
    case BT_GATT_SERVER_ON_DISCONNECTED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
                        on_disconnected,
                        &packet->gatts_cb._on_disconnected.addr);
        break;
    case BT_GATT_SERVER_ON_STARTED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
                        on_started,
                        packet->gatts_cb._on_started.status);
        break;
    case BT_GATT_SERVER_ON_STOPPED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
                        on_stopped,
                        packet->gatts_cb._on_stopped.status);
        break;
    case BT_GATT_SERVER_ON_MTU_CHANGED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
                        on_mtu_changed,
                        &packet->gatts_cb._on_mtu_changed.addr,
                        packet->gatts_cb._on_mtu_changed.mtu);
        break;
    case BT_GATT_SERVER_NOTIFY_COMPLETE:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
                        on_notify_complete,
                        packet->gatts_cb._on_nofity_complete.status,
                        packet->gatts_cb._on_nofity_complete.attr_handle);
        break;
    case BT_GATT_SERVER_ON_READ_REQUEST: {
        if (!gatts_remote->srv_db)
            break;

        gatt_attr_db_t *attr_db = gatts_remote->srv_db->attr_db;
        for (int i = 0; i < gatts_remote->srv_db->attr_num; i++, attr_db++) {
            if (attr_db->handle == packet->gatts_cb._on_read_request.attr_handle &&
                attr_db->read_cb) {
                attr_db->read_cb(gatts_remote,
                                 packet->gatts_cb._on_read_request.attr_handle,
                                 packet->gatts_cb._on_read_request.req_handle);
                break;
            }
        }
        break;
    }
    case BT_GATT_SERVER_ON_WRITE_REQUEST: {
        if (!gatts_remote->srv_db)
            break;

        gatt_attr_db_t *attr_db = gatts_remote->srv_db->attr_db;
        for (int i = 0; i < gatts_remote->srv_db->attr_num; i++, attr_db++) {
            if (attr_db->handle == packet->gatts_cb._on_write_request.attr_handle &&
                attr_db->write_cb) {
                attr_db->write_cb(gatts_remote,
                                  packet->gatts_cb._on_write_request.attr_handle,
                                  packet->gatts_cb._on_write_request.value,
                                  packet->gatts_cb._on_write_request.length,
                                  packet->gatts_cb._on_write_request.offset);
                break;
            }
        }
        break;
    }
    default:
        return BT_STATUS_PARM_INVALID;
    }
    return BT_STATUS_SUCCESS;
}
