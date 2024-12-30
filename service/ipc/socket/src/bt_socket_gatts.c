
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
#include "gatts_service.h"
#include "service_manager.h"

static void on_connected_cb(gatts_handle_t srv_handle, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_connected.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_CONNECTED);
}
static void on_disconnected_cb(gatts_handle_t srv_handle, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_disconnected.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_DISCONNECTED);
}
static void on_attr_table_added_cb(gatts_handle_t srv_handle, gatt_status_t status, uint16_t attr_handle)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    packet.gatts_cb._on_attr_table_added.status = status;
    packet.gatts_cb._on_attr_table_added.attr_handle = attr_handle;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_ATTR_TABLE_ADDED);
}
static void on_attr_table_removed_cb(gatts_handle_t srv_handle, gatt_status_t status, uint16_t attr_handle)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    packet.gatts_cb._on_attr_table_removed.status = status;
    packet.gatts_cb._on_attr_table_removed.attr_handle = attr_handle;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_ATTR_TABLE_REMOVED);
}
static void on_notify_complete_cb(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_nofity_complete.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_nofity_complete.status = status;
    packet.gatts_cb._on_nofity_complete.attr_handle = attr_handle;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_NOTIFY_COMPLETE);
}
static void on_mtu_changed_cb(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t mtu)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_mtu_changed.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_mtu_changed.mtu = mtu;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_MTU_CHANGED);
}
static void on_phy_read_cb(gatts_handle_t srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_phy_updated.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_phy_updated.tx_phy = tx_phy;
    packet.gatts_cb._on_phy_updated.rx_phy = rx_phy;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_PHY_READ);
}
static void on_phy_updated_cb(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_phy_updated.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_phy_updated.status = status;
    packet.gatts_cb._on_phy_updated.tx_phy = tx_phy;
    packet.gatts_cb._on_phy_updated.rx_phy = rx_phy;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_PHY_UPDATED);
}
static uint16_t on_read_request_cb(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_read_request.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_read_request.attr_handle = attr_handle;
    packet.gatts_cb._on_read_request.req_handle = req_handle;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_READ_REQUEST);
    return 0;
}
static uint16_t on_write_request_cb(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, const uint8_t* value, uint16_t length, uint16_t offset)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);

    if (length > sizeof(packet.gatts_cb._on_write_request.value)) {
        BT_LOGW("exceeds gatts maximum attr value size :%d", length);
        length = sizeof(packet.gatts_cb._on_write_request.value);
    }

    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_write_request.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_write_request.attr_handle = attr_handle;
    packet.gatts_cb._on_write_request.offset = offset;
    packet.gatts_cb._on_write_request.length = length;
    memcpy(packet.gatts_cb._on_write_request.value, value, length);
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_WRITE_REQUEST);
    return length;
}
static void on_conn_param_changed_cb(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    bt_message_packet_t packet = { 0 };
    bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(srv_handle);
    packet.gatts_cb._on_callback.remote = gatts_remote->cookie;
    memcpy(&packet.gatts_cb._on_conn_param_changed.addr, addr, sizeof(bt_address_t));
    packet.gatts_cb._on_conn_param_changed.interval = connection_interval;
    packet.gatts_cb._on_conn_param_changed.latency = peripheral_latency;
    packet.gatts_cb._on_conn_param_changed.timeout = supervision_timeout;
    bt_socket_server_send(gatts_remote->ins, &packet, BT_GATT_SERVER_ON_CONN_PARAM_CHANGED);
}
const static gatts_callbacks_t g_gatts_socket_cbs = {
    .on_connected = on_connected_cb,
    .on_disconnected = on_disconnected_cb,
    .on_attr_table_added = on_attr_table_added_cb,
    .on_attr_table_removed = on_attr_table_removed_cb,
    .on_notify_complete = on_notify_complete_cb,
    .on_mtu_changed = on_mtu_changed_cb,
    .on_phy_read = on_phy_read_cb,
    .on_phy_updated = on_phy_updated_cb,
    .on_conn_param_changed = on_conn_param_changed_cb,
};
/****************************************************************************
 * Public Functions
 ****************************************************************************/
void bt_socket_server_gatts_process(service_poll_t* poll, int fd,
    bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_GATT_SERVER_REGISTER_SERVICE: {
        gatts_interface_t* profile = (gatts_interface_t*)service_manager_get_profile(PROFILE_GATTS);
        bt_gatts_remote_t* gatts_remote = malloc(sizeof(bt_gatts_remote_t));
        if (!gatts_remote) {
            packet->gatts_r.status = BT_STATUS_NO_RESOURCES;
            break;
        }

        gatts_remote->ins = ins;
        gatts_remote->cookie = packet->gatts_pl._bt_gatts_register.cookie;
        packet->gatts_r.status = profile->register_service(gatts_remote,
            (void**)&packet->gatts_r.handle,
            (gatts_callbacks_t*)&g_gatts_socket_cbs);
        if (packet->gatts_r.status != BT_STATUS_SUCCESS)
            free(gatts_remote);
        break;
    }
    case BT_GATT_SERVER_UNREGISTER_SERVICE: {
        bt_gatts_remote_t* gatts_remote = if_gatts_get_remote(INT2PTR(void*) packet->gatts_pl._bt_gatts_unregister.handle);
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_unregister_service)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_unregister.handle);

        if (packet->gatts_r.status == BT_STATUS_SUCCESS)
            free(gatts_remote);
        break;
    }
    case BT_GATT_SERVER_CONNECT:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_connect)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_connect.handle,
            &packet->gatts_pl._bt_gatts_connect.addr,
            packet->gatts_pl._bt_gatts_connect.addr_type);
        break;
    case BT_GATT_SERVER_CONNECT_BEAR:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_connect_bear)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_connect_bear.handle,
            &packet->gatts_pl._bt_gatts_connect_bear.addr,
            packet->gatts_pl._bt_gatts_connect_bear.addr_type,
            packet->gatts_pl._bt_gatts_connect_bear.bear_type);
        break;
    case BT_GATT_SERVER_DISCONNECT:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_disconnect)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_disconnect.handle,
            &packet->gatts_pl._bt_gatts_disconnect.addr);
        break;
    case BT_GATT_SERVER_ADD_ATTR_TABLE: {
        uint8_t* raw_data = (uint8_t*)packet->gatts_pl._bt_gatts_add_attr_table.attr_db;
        gatt_srv_db_t srv_db;
        gatt_attr_db_t* attr_inst;

        srv_db.attr_num = packet->gatts_pl._bt_gatts_add_attr_table.attr_num;
        srv_db.attr_db = zalloc(sizeof(gatt_attr_db_t) * packet->gatts_pl._bt_gatts_add_attr_table.attr_num);
        if (!srv_db.attr_db) {
            packet->gatts_r.status = BT_STATUS_NO_RESOURCES;
            break;
        }

        attr_inst = srv_db.attr_db;
        raw_data += sizeof(packet->gatts_pl._bt_gatts_add_attr_table.attr_db[0]) * srv_db.attr_num;
        for (int i = 0; i < srv_db.attr_num; i++, attr_inst++) {
            memcpy(&attr_inst->uuid, &packet->gatts_pl._bt_gatts_add_attr_table.attr_db[i].uuid,
                sizeof(attr_inst->uuid));
            attr_inst->handle = packet->gatts_pl._bt_gatts_add_attr_table.attr_db[i].handle;
            attr_inst->type = packet->gatts_pl._bt_gatts_add_attr_table.attr_db[i].type;
            attr_inst->rsp_type = packet->gatts_pl._bt_gatts_add_attr_table.attr_db[i].rsp_type;
            attr_inst->properties = packet->gatts_pl._bt_gatts_add_attr_table.attr_db[i].properties;
            attr_inst->permissions = packet->gatts_pl._bt_gatts_add_attr_table.attr_db[i].permissions;
            attr_inst->attr_length = packet->gatts_pl._bt_gatts_add_attr_table.attr_db[i].attr_length;

            if (attr_inst->rsp_type == ATTR_RSP_BY_APP) {
                attr_inst->read_cb = on_read_request_cb;
                attr_inst->write_cb = on_write_request_cb;
            } else if (attr_inst->attr_length) {
                attr_inst->attr_value = raw_data;
                raw_data += attr_inst->attr_length;
            }
        }

        packet->gatts_r.status = BTSYMBOLS(bt_gatts_add_attr_table)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_add_attr_table.handle,
            &srv_db);
        free(srv_db.attr_db);

        break;
    }
    case BT_GATT_SERVER_REMOVE_ATTR_TABLE:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_remove_attr_table)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_remove_attr_table.handle,
            packet->gatts_pl._bt_gatts_remove_attr_table.attr_handle);
        break;
    case BT_GATT_SERVER_SET_ATTR_VALUE:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_set_attr_value)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_set_attr_value.handle,
            packet->gatts_pl._bt_gatts_set_attr_value.attr_handle,
            packet->gatts_pl._bt_gatts_set_attr_value.value,
            packet->gatts_pl._bt_gatts_set_attr_value.length);
        break;
    case BT_GATT_SERVER_GET_ATTR_VALUE:
        packet->gatts_r.length = packet->gatts_pl._bt_gatts_get_attr_value.length;
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_get_attr_value)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_get_attr_value.handle,
            packet->gatts_pl._bt_gatts_get_attr_value.attr_handle,
            packet->gatts_r.value,
            &packet->gatts_r.length);
        break;
    case BT_GATT_SERVER_RESPONSE:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_response)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_response.handle,
            &packet->gatts_pl._bt_gatts_response.addr,
            packet->gatts_pl._bt_gatts_response.req_handle,
            packet->gatts_pl._bt_gatts_response.value,
            packet->gatts_pl._bt_gatts_response.length);
        break;
    case BT_GATT_SERVER_NOTIFY:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_notify)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_notify.handle,
            &packet->gatts_pl._bt_gatts_notify.addr,
            packet->gatts_pl._bt_gatts_notify.attr_handle,
            packet->gatts_pl._bt_gatts_notify.value,
            packet->gatts_pl._bt_gatts_notify.length);
        break;
    case BT_GATT_SERVER_INDICATE:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_indicate)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_notify.handle,
            &packet->gatts_pl._bt_gatts_notify.addr,
            packet->gatts_pl._bt_gatts_notify.attr_handle,
            packet->gatts_pl._bt_gatts_notify.value,
            packet->gatts_pl._bt_gatts_notify.length);
        break;
    case BT_GATT_SERVER_READ_PHY:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_read_phy)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_phy.handle,
            &packet->gatts_pl._bt_gatts_phy.addr);
        break;
    case BT_GATT_SERVER_UPDATE_PHY:
        packet->gatts_r.status = BTSYMBOLS(bt_gatts_update_phy)(
            INT2PTR(gatts_handle_t) packet->gatts_pl._bt_gatts_phy.handle,
            &packet->gatts_pl._bt_gatts_phy.addr,
            packet->gatts_pl._bt_gatts_phy.tx_phy,
            packet->gatts_pl._bt_gatts_phy.rx_phy);
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_gatts_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    bt_gatts_remote_t* gatts_remote = INT2PTR(bt_gatts_remote_t*) packet->gatts_cb._on_callback.remote;
    CHECK_REMOTE_VALID(ins->gatts_remote_list, gatts_remote);

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
    case BT_GATT_SERVER_ON_ATTR_TABLE_ADDED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
            on_attr_table_added,
            packet->gatts_cb._on_attr_table_added.status,
            packet->gatts_cb._on_attr_table_added.attr_handle);
        break;
    case BT_GATT_SERVER_ON_ATTR_TABLE_REMOVED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
            on_attr_table_removed,
            packet->gatts_cb._on_attr_table_removed.status,
            packet->gatts_cb._on_attr_table_removed.attr_handle);
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
            &packet->gatts_cb._on_nofity_complete.addr,
            packet->gatts_cb._on_nofity_complete.status,
            packet->gatts_cb._on_nofity_complete.attr_handle);
        break;
    case BT_GATT_SERVER_ON_PHY_READ:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
            on_phy_read,
            &packet->gatts_cb._on_phy_updated.addr,
            packet->gatts_cb._on_phy_updated.tx_phy,
            packet->gatts_cb._on_phy_updated.rx_phy);
        break;
    case BT_GATT_SERVER_ON_PHY_UPDATED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
            on_phy_updated,
            &packet->gatts_cb._on_phy_updated.addr,
            packet->gatts_cb._on_phy_updated.status,
            packet->gatts_cb._on_phy_updated.tx_phy,
            packet->gatts_cb._on_phy_updated.rx_phy);
        break;
    case BT_GATT_SERVER_ON_CONN_PARAM_CHANGED:
        CALLBACK_REMOTE(gatts_remote, gatts_callbacks_t,
            on_conn_param_changed,
            &packet->gatts_cb._on_conn_param_changed.addr,
            packet->gatts_cb._on_conn_param_changed.interval,
            packet->gatts_cb._on_conn_param_changed.latency,
            packet->gatts_cb._on_conn_param_changed.timeout);
        break;
    case BT_GATT_SERVER_ON_READ_REQUEST: {
        bt_list_node_t* node;
        bt_list_t* list = gatts_remote->db_list;
        for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
            gatt_srv_db_t* srv_db = (gatt_srv_db_t*)bt_list_node(node);
            gatt_attr_db_t* attr_db = srv_db->attr_db;
            for (int i = 0; i < srv_db->attr_num; i++, attr_db++) {
                if (attr_db->handle == packet->gatts_cb._on_read_request.attr_handle && attr_db->read_cb) {
                    attr_db->read_cb(gatts_remote,
                        &packet->gatts_cb._on_read_request.addr,
                        packet->gatts_cb._on_read_request.attr_handle,
                        packet->gatts_cb._on_read_request.req_handle);
                    return BT_STATUS_SUCCESS;
                }
            }
        }
        break;
    }
    case BT_GATT_SERVER_ON_WRITE_REQUEST: {
        bt_list_node_t* node;
        bt_list_t* list = gatts_remote->db_list;
        for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
            gatt_srv_db_t* srv_db = (gatt_srv_db_t*)bt_list_node(node);
            gatt_attr_db_t* attr_db = srv_db->attr_db;
            for (int i = 0; i < srv_db->attr_num; i++, attr_db++) {
                if (attr_db->handle == packet->gatts_cb._on_write_request.attr_handle && attr_db->write_cb) {
                    attr_db->write_cb(gatts_remote,
                        &packet->gatts_cb._on_write_request.addr,
                        packet->gatts_cb._on_write_request.attr_handle,
                        packet->gatts_cb._on_write_request.value,
                        packet->gatts_cb._on_write_request.length,
                        packet->gatts_cb._on_write_request.offset);
                    return BT_STATUS_SUCCESS;
                }
            }
        }
        break;
    }
    default:
        return BT_STATUS_PARM_INVALID;
    }
    return BT_STATUS_SUCCESS;
}
