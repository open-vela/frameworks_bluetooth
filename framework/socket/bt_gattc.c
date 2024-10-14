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
#define LOG_TAG "gattc"

#include <stdint.h>

#include "bt_gattc.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "bt_socket.h"
#include "gattc_service.h"
#include "service_manager.h"
#include "utils/log.h"

#define CHECK_NULL_PTR(ptr)                \
    do {                                   \
        if (!ptr)                          \
            return BT_STATUS_PARM_INVALID; \
    } while (0)

bt_status_t bt_gattc_create_connect(bt_instance_t* ins, gattc_handle_t* phandle, gattc_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    CHECK_NULL_PTR(phandle);

    if (ins->gattc_remote_list == NULL) {
        ins->gattc_remote_list = bt_list_new(free);
        if (ins->gattc_remote_list == NULL) {
            return BT_STATUS_NOMEM;
        }
    }

    gattc_remote = (bt_gattc_remote_t*)malloc(sizeof(bt_gattc_remote_t));
    if (!gattc_remote) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    gattc_remote->ins = ins;
    gattc_remote->callbacks = callbacks;

    packet.gattc_pl._bt_gattc_create.cookie = PTR2INT(uint64_t) gattc_remote;
    status = bt_socket_client_sendrecv(ins, &packet, BT_GATT_CLIENT_CREATE_CONNECT);
    if (status != BT_STATUS_SUCCESS) {
        goto fail;
    }
    if (packet.gattc_r.status != BT_STATUS_SUCCESS) {
        status = packet.gattc_r.status;
        goto fail;
    }

    gattc_remote->cookie = packet.gattc_r.handle;
    gattc_remote->user_phandle = phandle;
    bt_list_add_tail(ins->gattc_remote_list, gattc_remote);

    *phandle = gattc_remote;
    return BT_STATUS_SUCCESS;

fail:
    if (gattc_remote) {
        free(gattc_remote);
    }
    if (!bt_list_length(ins->gattc_remote_list)) {
        bt_list_free(ins->gattc_remote_list);
        ins->gattc_remote_list = NULL;
    }
    return status;
}

bt_status_t bt_gattc_delete_connect(gattc_handle_t conn_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_instance_t* ins;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    void** user_phandle;

    CHECK_NULL_PTR(gattc_remote);

    ins = gattc_remote->ins;
    packet.gattc_pl._bt_gattc_delete.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    status = bt_socket_client_sendrecv(ins, &packet, BT_GATT_CLIENT_DELETE_CONNECT);
    user_phandle = gattc_remote->user_phandle;
    bt_list_remove(ins->gattc_remote_list, gattc_remote);
    *user_phandle = NULL;

    if (!bt_list_length(ins->gattc_remote_list)) {
        bt_list_free(ins->gattc_remote_list);
        ins->gattc_remote_list = NULL;
    }

    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    if (packet.gattc_r.status != BT_STATUS_SUCCESS) {
        return packet.gattc_r.status;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_connect(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_connect.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_connect.addr_type = addr_type;
    memcpy(&packet.gattc_pl._bt_gattc_connect.addr, addr, sizeof(bt_address_t));
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_CONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_disconnect(gattc_handle_t conn_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_disconnect.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_DISCONNECT);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_discover_service(gattc_handle_t conn_handle, bt_uuid_t* filter_uuid)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_discover_service.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    if (filter_uuid == NULL)
        packet.gattc_pl._bt_gattc_discover_service.filter_uuid.type = 0;
    else
        memcpy(&packet.gattc_pl._bt_gattc_discover_service.filter_uuid, filter_uuid, sizeof(bt_uuid_t));
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_DISCOVER_SERVICE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_get_attribute_by_handle(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_get_attr_by_handle.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_get_attr_by_handle.attr_handle = attr_handle;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_GET_ATTRIBUTE_BY_HANDLE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    memcpy(attr_desc, &packet.gattc_r.attr_desc, sizeof(gatt_attr_desc_t));
    return packet.gattc_r.status;
}

bt_status_t bt_gattc_get_attribute_by_uuid(gattc_handle_t conn_handle, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_get_attr_by_uuid.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_get_attr_by_uuid.start_handle = start_handle;
    packet.gattc_pl._bt_gattc_get_attr_by_uuid.end_handle = end_handle;
    memcpy(&packet.gattc_pl._bt_gattc_get_attr_by_uuid.attr_uuid, attr_uuid, sizeof(bt_uuid_t));
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_GET_ATTRIBUTE_BY_UUID);
    if (status != BT_STATUS_SUCCESS)
        return status;

    memcpy(attr_desc, &packet.gattc_r.attr_desc, sizeof(gatt_attr_desc_t));
    return packet.gattc_r.status;
}

bt_status_t bt_gattc_read(gattc_handle_t conn_handle, uint16_t attr_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_read.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_read.attr_handle = attr_handle;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_READ);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_write(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);
    if (length > sizeof(packet.gattc_pl._bt_gattc_write.value))
        return BT_STATUS_PARM_INVALID;

    packet.gattc_pl._bt_gattc_write.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_write.attr_handle = attr_handle;
    packet.gattc_pl._bt_gattc_write.length = length;
    memcpy(packet.gattc_pl._bt_gattc_write.value, value, length);
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_WRITE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_write_without_response(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);
    if (length > sizeof(packet.gattc_pl._bt_gattc_write.value))
        return BT_STATUS_PARM_INVALID;

    packet.gattc_pl._bt_gattc_write.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_write.attr_handle = attr_handle;
    packet.gattc_pl._bt_gattc_write.length = length;
    memcpy(packet.gattc_pl._bt_gattc_write.value, value, length);
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_WRITE_NR);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_subscribe(gattc_handle_t conn_handle, uint16_t attr_handle, uint16_t ccc_value)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_subscribe.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_subscribe.attr_handle = attr_handle;
    packet.gattc_pl._bt_gattc_subscribe.ccc_value = ccc_value;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_SUBSCRIBE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_unsubscribe(gattc_handle_t conn_handle, uint16_t attr_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_subscribe.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_subscribe.attr_handle = attr_handle;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_UNSUBSCRIBE);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_exchange_mtu(gattc_handle_t conn_handle, uint32_t mtu)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_exchange_mtu.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_exchange_mtu.mtu = mtu;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_EXCHANGE_MTU);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_update_connection_parameter(gattc_handle_t conn_handle, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_update_connection_param.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_update_connection_param.min_interval = min_interval;
    packet.gattc_pl._bt_gattc_update_connection_param.max_interval = max_interval;
    packet.gattc_pl._bt_gattc_update_connection_param.latency = latency;
    packet.gattc_pl._bt_gattc_update_connection_param.timeout = timeout;
    packet.gattc_pl._bt_gattc_update_connection_param.min_connection_event_length = min_connection_event_length;
    packet.gattc_pl._bt_gattc_update_connection_param.max_connection_event_length = max_connection_event_length;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_UPDATE_CONNECTION_PARAM);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_read_phy(gattc_handle_t conn_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_phy.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_READ_PHY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_update_phy(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_phy.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_phy.tx_phy = tx_phy;
    packet.gattc_pl._bt_gattc_phy.rx_phy = rx_phy;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_UPDATE_PHY);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}

bt_status_t bt_gattc_read_rssi(gattc_handle_t conn_handle)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_rssi.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    status = bt_socket_client_sendrecv(gattc_remote->ins, &packet, BT_GATT_CLIENT_READ_RSSI);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.gattc_r.status;
}
