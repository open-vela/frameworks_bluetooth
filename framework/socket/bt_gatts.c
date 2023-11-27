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
#define LOG_TAG "gatts"

#include <stdint.h>

#include "bt_gatts.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "gatts_service.h"
#include "service_manager.h"
#include "utils/log.h"

bt_status_t bt_gatts_register_service(bt_instance_t *ins, gatts_handle_t *phandle, gatts_callbacks_t *callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote;

    gatts_remote = (bt_gatts_remote_t *)malloc(sizeof(bt_gatts_remote_t));
    if (!gatts_remote)
        return BT_STATUS_NOMEM;

    gatts_remote->ins = ins;
    gatts_remote->callback = callbacks;

    packet.gatts_pl._bt_gatts_register.cookie = gatts_remote;
    status = bt_socket_client_sendrecv(ins, &packet, BT_GATT_SERVER_REGISTER_SERVICE);
    if (status != BT_STATUS_SUCCESS) {
        free(gatts_remote);
        return status;
    }
    if (packet.gatts_r.status != BT_STATUS_SUCCESS) {
        free(gatts_remote);
        return packet.gatts_r.status;
    }

    gatts_remote->cookie = packet.gatts_r.handle;
    *phandle = gatts_remote;
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_gatts_unregister_service(gatts_handle_t srv_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    packet.gatts_pl._bt_gatts_unregister.handle = gatts_remote->cookie;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_UNREGISTER_SERVICE);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    if (packet.gatts_r.status != BT_STATUS_SUCCESS) {
        return packet.gatts_r.status;
    }

    free(gatts_remote);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_gatts_connect(gatts_handle_t srv_handle, bt_address_t *addr, ble_addr_type_t addr_type)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    packet.gatts_pl._bt_gatts_connect.handle = gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_connect.addr_type = addr_type;
    memcpy(&packet.gatts_pl._bt_gatts_connect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_disconnect(gatts_handle_t srv_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    packet.gatts_pl._bt_gatts_disconnect.handle = gatts_remote->cookie;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_create_service_table(gatts_handle_t srv_handle, gatt_srv_db_t *srv_db)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    packet.gatts_pl._bt_gatts_create_srv_tbl.handle = gatts_remote->cookie;
    memcpy(&packet.gatts_pl._bt_gatts_create_srv_tbl.srv_db, srv_db, sizeof(gatt_srv_db_t));
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_CREATE_SERVICE_TABLE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_start(gatts_handle_t srv_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    packet.gatts_pl._bt_gatts_start.handle = gatts_remote->cookie;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_START);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_stop(gatts_handle_t srv_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    packet.gatts_pl._bt_gatts_stop.handle = gatts_remote->cookie;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_STOP);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_response(gatts_handle_t srv_handle, uint32_t req_handle, uint8_t *value, uint16_t length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    if (length > sizeof(packet.gatts_pl._bt_gatts_response.value))
        return BT_STATUS_PARM_INVALID;

    packet.gatts_pl._bt_gatts_response.handle = gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_response.req_handle = req_handle;
    packet.gatts_pl._bt_gatts_response.length = length;
    memcpy(&packet.gatts_pl._bt_gatts_response.value, value, length);
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_RESPONSE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_notify(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t *value, uint16_t length, gatts_complete_cb_t cmpl_cb)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    if (length > sizeof(packet.gatts_pl._bt_gatts_notify.value))
        return BT_STATUS_PARM_INVALID;

    packet.gatts_pl._bt_gatts_notify.handle = gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_notify.attr_handle = attr_handle;
    packet.gatts_pl._bt_gatts_notify.length = length;
    packet.gatts_pl._bt_gatts_notify.cmpl_cb = cmpl_cb;
    memcpy(&packet.gatts_pl._bt_gatts_notify.value, value, length);
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_NOTIFY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_indicate(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t *value, uint16_t length, gatts_complete_cb_t cmpl_cb)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t *gatts_remote = (bt_gatts_remote_t *)srv_handle;

    if (length > sizeof(packet.gatts_pl._bt_gatts_notify.value))
        return BT_STATUS_PARM_INVALID;

    packet.gatts_pl._bt_gatts_notify.handle = gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_notify.attr_handle = attr_handle;
    packet.gatts_pl._bt_gatts_notify.length = length;
    packet.gatts_pl._bt_gatts_notify.cmpl_cb = cmpl_cb;
    memcpy(&packet.gatts_pl._bt_gatts_notify.value, value, length);
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_INDICATE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}
