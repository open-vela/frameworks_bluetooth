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
#include "bt_internal.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "gatts_service.h"
#include "service_manager.h"
#include "utils/log.h"

#define CHECK_NULL_PTR(ptr)                \
    do {                                   \
        if (!ptr)                          \
            return BT_STATUS_PARM_INVALID; \
    } while (0)

static bt_gatts_remote_t* gatts_remote_new(bt_instance_t* ins, gatts_callbacks_t* callbacks)
{
    bt_gatts_remote_t* remote = malloc(sizeof(bt_gatts_remote_t));
    if (!remote)
        return NULL;

    remote->db_list = bt_list_new(NULL);
    if (!remote->db_list) {
        free(remote);
        return NULL;
    }

    remote->ins = ins;
    remote->callbacks = callbacks;
    remote->cookie = 0;

    return remote;
}

static void gatts_remote_destroy(bt_gatts_remote_t* remote)
{
    if (!remote)
        return;

    bt_list_free(remote->db_list);
    free(remote);
}

bt_status_t bt_gatts_register_service(bt_instance_t* ins, gatts_handle_t* phandle, gatts_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    CHECK_NULL_PTR(phandle);

    if (ins->gatts_remote_list == NULL) {
        ins->gatts_remote_list = bt_list_new((bt_list_free_cb_t)gatts_remote_destroy);
        if (ins->gatts_remote_list == NULL) {
            return BT_STATUS_NOMEM;
        }
    }

    gatts_remote = gatts_remote_new(ins, callbacks);
    if (!gatts_remote) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    packet.gatts_pl._bt_gatts_register.cookie = PTR2INT(uint64_t) gatts_remote;
    status = bt_socket_client_sendrecv(ins, &packet, BT_GATT_SERVER_REGISTER_SERVICE);
    if (status != BT_STATUS_SUCCESS) {
        goto fail;
    }
    if (packet.gatts_r.status != BT_STATUS_SUCCESS) {
        status = packet.gatts_r.status;
        goto fail;
    }

    gatts_remote->cookie = packet.gatts_r.handle;
    gatts_remote->user_phandle = phandle;
    bt_list_add_tail(ins->gatts_remote_list, gatts_remote);

    *phandle = gatts_remote;
    return BT_STATUS_SUCCESS;

fail:
    if (gatts_remote) {
        gatts_remote_destroy(gatts_remote);
    }
    if (!bt_list_length(ins->gatts_remote_list)) {
        bt_list_free(ins->gatts_remote_list);
        ins->gatts_remote_list = NULL;
    }
    return status;
}

bt_status_t bt_gatts_unregister_service(gatts_handle_t srv_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_instance_t* ins;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;
    void** user_phandle;

    CHECK_NULL_PTR(gatts_remote);

    ins = gatts_remote->ins;
    packet.gatts_pl._bt_gatts_unregister.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    status = bt_socket_client_sendrecv(ins, &packet, BT_GATT_SERVER_UNREGISTER_SERVICE);
    user_phandle = gatts_remote->user_phandle;
    bt_list_remove(ins->gatts_remote_list, gatts_remote);
    *user_phandle = NULL;

    if (!bt_list_length(ins->gatts_remote_list)) {
        bt_list_free(ins->gatts_remote_list);
        ins->gatts_remote_list = NULL;
    }

    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    if (packet.gatts_r.status != BT_STATUS_SUCCESS) {
        return packet.gatts_r.status;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_gatts_connect(gatts_handle_t srv_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    packet.gatts_pl._bt_gatts_connect.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_connect.addr_type = addr_type;
    memcpy(&packet.gatts_pl._bt_gatts_connect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_connect_bear(gatts_handle_t srv_handle, bt_address_t* addr, ble_addr_type_t addr_type, uint8_t bear_type)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    packet.gatts_pl._bt_gatts_connect_bear.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_connect_bear.addr_type = addr_type;
    packet.gatts_pl._bt_gatts_connect_bear.bear_type = bear_type;
    memcpy(&packet.gatts_pl._bt_gatts_connect_bear.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_CONNECT_BEAR);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_disconnect(gatts_handle_t srv_handle, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    packet.gatts_pl._bt_gatts_disconnect.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    memcpy(&packet.gatts_pl._bt_gatts_disconnect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_add_attr_table(gatts_handle_t srv_handle, gatt_srv_db_t* srv_db)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;
    uint8_t* raw_data = (uint8_t*)packet.gatts_pl._bt_gatts_add_attr_table.attr_db;
    uint32_t data_length = sizeof(packet.gatts_pl._bt_gatts_add_attr_table.attr_db[0]) * srv_db->attr_num;
    gatt_attr_db_t* attr_inst = srv_db->attr_db;

    CHECK_NULL_PTR(gatts_remote);
    if (data_length > sizeof(packet.gatts_pl._bt_gatts_add_attr_table.attr_db))
        return BT_STATUS_PARM_INVALID;

    raw_data += data_length;
    for (int i = 0; i < srv_db->attr_num; i++, attr_inst++) {
        memcpy(&packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].uuid, &attr_inst->uuid,
            sizeof(packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].uuid));
        packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].handle = attr_inst->handle;
        packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].type = attr_inst->type;
        packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].rsp_type = attr_inst->rsp_type;
        packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].properties = attr_inst->properties;
        packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].permissions = attr_inst->permissions;
        packet.gatts_pl._bt_gatts_add_attr_table.attr_db[i].attr_length = attr_inst->attr_length;

        if (attr_inst->rsp_type == ATTR_AUTO_RSP && attr_inst->attr_length) {
            data_length += attr_inst->attr_length;
            if (data_length > sizeof(packet.gatts_pl._bt_gatts_add_attr_table.attr_db))
                return BT_STATUS_PARM_INVALID;

            memcpy(raw_data, attr_inst->attr_value, attr_inst->attr_length);
            raw_data += attr_inst->attr_length;
        }
    }

    packet.gatts_pl._bt_gatts_add_attr_table.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_add_attr_table.attr_num = srv_db->attr_num;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_ADD_ATTR_TABLE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    if (packet.gatts_r.status == BT_STATUS_SUCCESS) {
        bt_list_add_tail(gatts_remote->db_list, srv_db);
    }

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_remove_attr_table(gatts_handle_t srv_handle, uint16_t attr_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    packet.gatts_pl._bt_gatts_remove_attr_table.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_remove_attr_table.attr_handle = attr_handle;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_REMOVE_ATTR_TABLE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    if (packet.gatts_r.status == BT_STATUS_SUCCESS) {
        bt_list_node_t* node;
        bt_list_t* list = gatts_remote->db_list;
        for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
            gatt_srv_db_t* srv_db = (gatt_srv_db_t*)bt_list_node(node);
            if (srv_db->attr_db->handle == attr_handle) {
                bt_list_remove(gatts_remote->db_list, srv_db);
                break;
            }
        }
    }

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_set_attr_value(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);
    if (length > sizeof(packet.gatts_pl._bt_gatts_set_attr_value.value))
        return BT_STATUS_PARM_INVALID;

    packet.gatts_pl._bt_gatts_set_attr_value.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_set_attr_value.attr_handle = attr_handle;
    packet.gatts_pl._bt_gatts_set_attr_value.length = length;
    memcpy(packet.gatts_pl._bt_gatts_set_attr_value.value, value, length);
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_SET_ATTR_VALUE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_get_attr_value(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t* length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    packet.gatts_pl._bt_gatts_get_attr_value.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    packet.gatts_pl._bt_gatts_get_attr_value.attr_handle = attr_handle;
    packet.gatts_pl._bt_gatts_get_attr_value.length = *length;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_GET_ATTR_VALUE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    if (packet.gatts_r.status == BT_STATUS_SUCCESS) {
        *length = packet.gatts_r.length;
        memcpy(value, packet.gatts_r.value, *length);
    }

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_response(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t req_handle, uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    if (length > sizeof(packet.gatts_pl._bt_gatts_response.value))
        return BT_STATUS_PARM_INVALID;

    packet.gatts_pl._bt_gatts_response.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    memcpy(&packet.gatts_pl._bt_gatts_response.addr, addr, sizeof(bt_address_t));
    packet.gatts_pl._bt_gatts_response.req_handle = req_handle;
    packet.gatts_pl._bt_gatts_response.length = length;
    memcpy(packet.gatts_pl._bt_gatts_response.value, value, length);
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_RESPONSE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_notify(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);
    if (length > sizeof(packet.gatts_pl._bt_gatts_notify.value))
        return BT_STATUS_PARM_INVALID;

    packet.gatts_pl._bt_gatts_notify.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    memcpy(&packet.gatts_pl._bt_gatts_notify.addr, addr, sizeof(bt_address_t));
    packet.gatts_pl._bt_gatts_notify.attr_handle = attr_handle;
    packet.gatts_pl._bt_gatts_notify.length = length;
    memcpy(packet.gatts_pl._bt_gatts_notify.value, value, length);
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_NOTIFY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_indicate(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);
    if (length > sizeof(packet.gatts_pl._bt_gatts_notify.value))
        return BT_STATUS_PARM_INVALID;

    packet.gatts_pl._bt_gatts_notify.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    memcpy(&packet.gatts_pl._bt_gatts_notify.addr, addr, sizeof(bt_address_t));
    packet.gatts_pl._bt_gatts_notify.attr_handle = attr_handle;
    packet.gatts_pl._bt_gatts_notify.length = length;
    memcpy(packet.gatts_pl._bt_gatts_notify.value, value, length);
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_INDICATE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_read_phy(gatts_handle_t srv_handle, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    packet.gatts_pl._bt_gatts_phy.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    memcpy(&packet.gatts_pl._bt_gatts_phy.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_READ_PHY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}

bt_status_t bt_gatts_update_phy(gatts_handle_t srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gatts_remote_t* gatts_remote = (bt_gatts_remote_t*)srv_handle;

    CHECK_NULL_PTR(gatts_remote);

    packet.gatts_pl._bt_gatts_phy.handle = PTR2INT(uint64_t) gatts_remote->cookie;
    memcpy(&packet.gatts_pl._bt_gatts_phy.addr, addr, sizeof(bt_address_t));
    packet.gatts_pl._bt_gatts_phy.tx_phy = tx_phy;
    packet.gatts_pl._bt_gatts_phy.rx_phy = rx_phy;
    status = bt_socket_client_sendrecv(gatts_remote->ins, &packet, BT_GATT_SERVER_UPDATE_PHY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gatts_r.status;
}
