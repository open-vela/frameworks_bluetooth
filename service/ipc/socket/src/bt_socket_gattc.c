
/****************************************************************************
 * service/ipc/socket/src/bt_socket_gattc.c
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
#include "bt_gattc.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "service_loop.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define CHECK_REMOTE_VALID(_list, _remote)                                                     \
    do {                                                                                       \
        bt_list_node_t* _node;                                                                 \
        if (!_list)                                                                            \
            return BT_STATUS_SERVICE_NOT_FOUND;                                                \
        for (_node = bt_list_head(_list); _node != NULL; _node = bt_list_next(_list, _node)) { \
            if (bt_list_node(_node) == _remote)                                                \
                break;                                                                         \
        }                                                                                      \
        if (!_node)                                                                            \
            return BT_STATUS_SERVICE_NOT_FOUND;                                                \
    } while (0)

#define CALLBACK_REMOTE(_remote, _type, _cback, ...) \
    do {                                             \
        _type* _cbs = (_type*)_remote->callbacks;    \
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
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(CONFIG_BLUETOOTH_GATT) && defined(__NuttX__)
#include "gattc_service.h"
#include "service_manager.h"

static void on_connected_cb(gattc_handle_t conn_handle, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    memcpy(&packet.gattc_cb._on_connected.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_CONNECTED);
}
static void on_disconnected_cb(gattc_handle_t conn_handle, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    memcpy(&packet.gattc_cb._on_disconnected.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_DISCONNECTED);
}
static void on_discovered_cb(gattc_handle_t conn_handle, gatt_status_t status, bt_uuid_t* uuid,
    uint16_t start_handle, uint16_t end_handle)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_discovered.status = status;
    packet.gattc_cb._on_discovered.start_handle = start_handle;
    packet.gattc_cb._on_discovered.end_handle = end_handle;
    if (uuid == NULL)
        packet.gattc_cb._on_discovered.uuid.type = 0;
    else
        memcpy(&packet.gattc_cb._on_discovered.uuid, uuid, sizeof(bt_uuid_t));
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_DISCOVERED);
}
static void on_read_cb(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle,
    uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);

    if (length > sizeof(packet.gattc_cb._on_read.value)) {
        BT_LOGW("exceeds gattc maximum attr value size :%d", length);
        length = sizeof(packet.gattc_cb._on_read.value);
    }

    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_read.status = status;
    packet.gattc_cb._on_read.attr_handle = attr_handle;
    packet.gattc_cb._on_read.length = length;
    memcpy(packet.gattc_cb._on_read.value, value, length);
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_READ);
}
static void on_written_cb(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_written.status = status;
    packet.gattc_cb._on_written.attr_handle = attr_handle;
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_WRITTEN);
}
static void on_subscribed_cb(gattc_handle_t conn_handle, gatt_status_t status, uint16_t attr_handle, bool enable)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_subscribed.status = status;
    packet.gattc_cb._on_subscribed.attr_handle = attr_handle;
    packet.gattc_cb._on_subscribed.enable = enable;
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_SUBSCRIBED);
}
static void on_notified_cb(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);

    if (length > sizeof(packet.gattc_cb._on_notified.value)) {
        BT_LOGW("exceeds gattc maximum attr value size :%d", length);
        length = sizeof(packet.gattc_cb._on_notified.value);
    }

    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_notified.attr_handle = attr_handle;
    packet.gattc_cb._on_notified.length = length;
    memcpy(packet.gattc_cb._on_notified.value, value, length);
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_NOTIFIED);
}
static void on_mtu_updated_cb(gattc_handle_t conn_handle, gatt_status_t status, uint32_t mtu)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_mtu_updated.status = status;
    packet.gattc_cb._on_mtu_updated.mtu = mtu;
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_MTU_UPDATED);
}
static void on_phy_read_cb(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_phy_updated.tx_phy = tx_phy;
    packet.gattc_cb._on_phy_updated.rx_phy = rx_phy;
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_PHY_READ);
}
static void on_phy_updated_cb(gattc_handle_t conn_handle, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_phy_updated.status = status;
    packet.gattc_cb._on_phy_updated.tx_phy = tx_phy;
    packet.gattc_cb._on_phy_updated.rx_phy = rx_phy;
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_PHY_UPDATED);
}
static void on_rssi_read_cb(gattc_handle_t conn_handle, gatt_status_t status, int32_t rssi)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_rssi_read.status = status;
    packet.gattc_cb._on_rssi_read.rssi = rssi;
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_RSSI_READ);
}
static void on_conn_param_updated_cb(gattc_handle_t conn_handle, bt_status_t status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(conn_handle);
    packet.gattc_cb._on_callback.remote = gattc_remote->cookie;
    packet.gattc_cb._on_conn_param_updated.status = status;
    packet.gattc_cb._on_conn_param_updated.interval = connection_interval;
    packet.gattc_cb._on_conn_param_updated.latency = peripheral_latency;
    packet.gattc_cb._on_conn_param_updated.timeout = supervision_timeout;
    bt_socket_server_send(gattc_remote->ins, &packet, BT_GATT_CLIENT_ON_CONN_PARAM_UPDATED);
}
const static gattc_callbacks_t g_gattc_socket_cbs = {
    .on_connected = on_connected_cb,
    .on_disconnected = on_disconnected_cb,
    .on_discovered = on_discovered_cb,
    .on_read = on_read_cb,
    .on_written = on_written_cb,
    .on_subscribed = on_subscribed_cb,
    .on_notified = on_notified_cb,
    .on_mtu_updated = on_mtu_updated_cb,
    .on_phy_read = on_phy_read_cb,
    .on_phy_updated = on_phy_updated_cb,
    .on_rssi_read = on_rssi_read_cb,
    .on_conn_param_updated = on_conn_param_updated_cb,
};
/****************************************************************************
 * Public Functions
 ****************************************************************************/
void bt_socket_server_gattc_process(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_GATT_CLIENT_CREATE_CONNECT: {
        gattc_interface_t* profile = (gattc_interface_t*)service_manager_get_profile(PROFILE_GATTC);
        bt_gattc_remote_t* gattc_remote = malloc(sizeof(bt_gattc_remote_t));
        if (!gattc_remote) {
            packet->gattc_r.status = BT_STATUS_NO_RESOURCES;
            break;
        }

        gattc_remote->ins = ins;
        gattc_remote->cookie = packet->gattc_pl._bt_gattc_create.cookie;
        packet->gattc_r.status = profile->create_connect(gattc_remote,
            INT2PTR(void**) & packet->gattc_r.handle,
            (gattc_callbacks_t*)&g_gattc_socket_cbs);
        if (packet->gattc_r.status != BT_STATUS_SUCCESS)
            free(gattc_remote);
        break;
    }
    case BT_GATT_CLIENT_DELETE_CONNECT: {
        bt_gattc_remote_t* gattc_remote = if_gattc_get_remote(INT2PTR(void*) packet->gattc_pl._bt_gattc_delete.handle);
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_delete_connect)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_delete.handle);

        if (packet->gattc_r.status == BT_STATUS_SUCCESS)
            free(gattc_remote);
        break;
    }
    case BT_GATT_CLIENT_CONNECT:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_connect)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_connect.handle,
            &packet->gattc_pl._bt_gattc_connect.addr,
            packet->gattc_pl._bt_gattc_connect.addr_type);
        break;
    case BT_GATT_CLIENT_DISCONNECT:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_disconnect)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_disconnect.handle);
        break;
    case BT_GATT_CLIENT_DISCOVER_SERVICE:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_discover_service)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_discover_service.handle,
            &packet->gattc_pl._bt_gattc_discover_service.filter_uuid);
        break;
    case BT_GATT_CLIENT_GET_ATTRIBUTE_BY_HANDLE:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_get_attribute_by_handle)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_get_attr_by_handle.handle,
            packet->gattc_pl._bt_gattc_get_attr_by_handle.attr_handle,
            &packet->gattc_r.attr_desc);
        break;
    case BT_GATT_CLIENT_GET_ATTRIBUTE_BY_UUID:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_get_attribute_by_uuid)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_get_attr_by_uuid.handle,
            packet->gattc_pl._bt_gattc_get_attr_by_uuid.start_handle,
            packet->gattc_pl._bt_gattc_get_attr_by_uuid.end_handle,
            &packet->gattc_pl._bt_gattc_get_attr_by_uuid.attr_uuid,
            &packet->gattc_r.attr_desc);
        break;
    case BT_GATT_CLIENT_READ:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_read)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_read.handle,
            packet->gattc_pl._bt_gattc_read.attr_handle);
        break;
    case BT_GATT_CLIENT_WRITE:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_write)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_write.handle,
            packet->gattc_pl._bt_gattc_write.attr_handle,
            packet->gattc_pl._bt_gattc_write.value,
            packet->gattc_pl._bt_gattc_write.length);
        break;
    case BT_GATT_CLIENT_WRITE_NR:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_write_without_response)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_write.handle,
            packet->gattc_pl._bt_gattc_write.attr_handle,
            packet->gattc_pl._bt_gattc_write.value,
            packet->gattc_pl._bt_gattc_write.length);
        break;
    case BT_GATT_CLIENT_SUBSCRIBE:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_subscribe)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_subscribe.handle,
            packet->gattc_pl._bt_gattc_subscribe.attr_handle,
            packet->gattc_pl._bt_gattc_subscribe.ccc_value);
        break;
    case BT_GATT_CLIENT_UNSUBSCRIBE:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_unsubscribe)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_subscribe.handle,
            packet->gattc_pl._bt_gattc_subscribe.attr_handle);
        break;
    case BT_GATT_CLIENT_EXCHANGE_MTU:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_exchange_mtu)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_exchange_mtu.handle,
            packet->gattc_pl._bt_gattc_exchange_mtu.mtu);
        break;
    case BT_GATT_CLIENT_UPDATE_CONNECTION_PARAM:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_update_connection_parameter)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_update_connection_param.handle,
            packet->gattc_pl._bt_gattc_update_connection_param.min_interval,
            packet->gattc_pl._bt_gattc_update_connection_param.max_interval,
            packet->gattc_pl._bt_gattc_update_connection_param.latency,
            packet->gattc_pl._bt_gattc_update_connection_param.timeout,
            packet->gattc_pl._bt_gattc_update_connection_param.min_connection_event_length,
            packet->gattc_pl._bt_gattc_update_connection_param.max_connection_event_length);
        break;
    case BT_GATT_CLIENT_READ_PHY:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_read_phy)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_phy.handle);
        break;
    case BT_GATT_CLIENT_UPDATE_PHY:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_update_phy)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_phy.handle,
            packet->gattc_pl._bt_gattc_phy.tx_phy,
            packet->gattc_pl._bt_gattc_phy.rx_phy);
        break;
    case BT_GATT_CLIENT_READ_RSSI:
        packet->gattc_r.status = BTSYMBOLS(bt_gattc_read_rssi)(
            INT2PTR(gattc_handle_t) packet->gattc_pl._bt_gattc_rssi.handle);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_gattc_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    bt_gattc_remote_t* gattc_remote = INT2PTR(bt_gattc_remote_t*) packet->gattc_cb._on_callback.remote;
    CHECK_REMOTE_VALID(ins->gattc_remote_list, gattc_remote);

    switch (packet->code) {
    case BT_GATT_CLIENT_ON_CONNECTED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_connected,
            &packet->gattc_cb._on_connected.addr);
        break;
    case BT_GATT_CLIENT_ON_DISCONNECTED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_disconnected,
            &packet->gattc_cb._on_disconnected.addr);
        break;
    case BT_GATT_CLIENT_ON_DISCOVERED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_discovered,
            packet->gattc_cb._on_discovered.status,
            &packet->gattc_cb._on_discovered.uuid,
            packet->gattc_cb._on_discovered.start_handle,
            packet->gattc_cb._on_discovered.end_handle);
        break;
    case BT_GATT_CLIENT_ON_MTU_UPDATED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_mtu_updated,
            packet->gattc_cb._on_mtu_updated.status,
            packet->gattc_cb._on_mtu_updated.mtu);
        break;
    case BT_GATT_CLIENT_ON_READ:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_read,
            packet->gattc_cb._on_read.status,
            packet->gattc_cb._on_read.attr_handle,
            packet->gattc_cb._on_read.value,
            packet->gattc_cb._on_read.length);
        break;
    case BT_GATT_CLIENT_ON_WRITTEN:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_written,
            packet->gattc_cb._on_written.status,
            packet->gattc_cb._on_written.attr_handle);
        break;
    case BT_GATT_CLIENT_ON_SUBSCRIBED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_subscribed,
            packet->gattc_cb._on_subscribed.status,
            packet->gattc_cb._on_subscribed.attr_handle,
            packet->gattc_cb._on_subscribed.enable);
        break;
    case BT_GATT_CLIENT_ON_NOTIFIED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_notified,
            packet->gattc_cb._on_notified.attr_handle,
            packet->gattc_cb._on_notified.value,
            packet->gattc_cb._on_notified.length);
        break;
    case BT_GATT_CLIENT_ON_PHY_READ:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_phy_read,
            packet->gattc_cb._on_phy_updated.tx_phy,
            packet->gattc_cb._on_phy_updated.rx_phy);
        break;
    case BT_GATT_CLIENT_ON_PHY_UPDATED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_phy_updated,
            packet->gattc_cb._on_phy_updated.status,
            packet->gattc_cb._on_phy_updated.tx_phy,
            packet->gattc_cb._on_phy_updated.rx_phy);
        break;
    case BT_GATT_CLIENT_ON_RSSI_READ:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_rssi_read,
            packet->gattc_cb._on_rssi_read.status,
            packet->gattc_cb._on_rssi_read.rssi);
        break;
    case BT_GATT_CLIENT_ON_CONN_PARAM_UPDATED:
        CALLBACK_REMOTE(gattc_remote, gattc_callbacks_t,
            on_conn_param_updated,
            packet->gattc_cb._on_conn_param_updated.status,
            packet->gattc_cb._on_conn_param_updated.interval,
            packet->gattc_cb._on_conn_param_updated.latency,
            packet->gattc_cb._on_conn_param_updated.timeout);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }
    return BT_STATUS_SUCCESS;
}
