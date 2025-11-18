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
#define LOG_TAG "gattc"

#include <stdint.h>

#include "bt_async.h"
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

typedef struct {
    void* userdata;
    void* gattc_remote;
    gattc_handle_t* user_phandle;
} bt_gattc_create_connect_data_t;

static void gattc_status_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_status_cb_t ret_cb = (bt_status_cb_t)cb;

    if (!ret_cb)
        return;

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, userdata);
        return;
    }

    ret_cb(ins, packet->gattc_r.status, userdata);
}

static void gattc_get_attribute_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_gattc_get_attribute_cb_t ret_cb = (bt_gattc_get_attribute_cb_t)cb;

    if (!ret_cb)
        return;

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, NULL, userdata);
        return;
    }

    ret_cb(ins, packet->gattc_r.status, &packet->gattc_r.attr_desc, userdata);
}

static void gattc_create_connect_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_gattc_create_connect_data_t* data = userdata;
    bt_socket_async_client_t* priv = ins->priv;
    bt_gattc_create_connect_cb_t ret_cb = (bt_gattc_create_connect_cb_t)cb;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)data->gattc_remote;

    if (!packet || packet->gattc_r.status != BT_STATUS_SUCCESS)
        goto error;

    gattc_remote->cookie = INT2PTR(void*) packet->gattc_r.handle;
    gattc_remote->user_phandle = data->user_phandle;
    bt_list_add_tail(priv->gattc_remote_list, gattc_remote);
    *(data->user_phandle) = gattc_remote;

    ret_cb(ins, packet->gattc_r.status, data->user_phandle, data->userdata);
    free(userdata);
    return;

error:
    if (gattc_remote) {
        free(gattc_remote);
    }

    if (!bt_list_length(priv->gattc_remote_list)) {
        bt_list_free(priv->gattc_remote_list);
        priv->gattc_remote_list = NULL;
    }

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, data->user_phandle, data->userdata);
        free(userdata);
        return;
    }

    ret_cb(ins, packet->gattc_r.status, data->user_phandle, data->userdata);
    free(userdata);
}

static void gattc_delete_connect_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_gattc_delete_connect_cb_t ret_cb = (bt_gattc_delete_connect_cb_t)cb;

    if (!ret_cb)
        return;

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, userdata);
        return;
    }

    ret_cb(ins, packet->gattc_r.status, userdata);
}

static void gattc_write_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_gattc_write_cb_t ret_cb = (bt_gattc_write_cb_t)cb;

    if (!ret_cb)
        return;

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, userdata);
        return;
    }

    ret_cb(ins, packet->gattc_r.status, userdata);
}

bt_status_t bt_gattc_create_connect_async(bt_instance_t* ins, gattc_handle_t* phandle, gattc_callbacks_t* callbacks,
    bt_gattc_create_connect_cb_t cb, void* userdata)
{
    bt_gattc_create_connect_data_t* data = NULL;
    bt_socket_async_client_t* priv;
    bt_message_packet_t packet = { 0 };
    bt_status_t status;
    bt_gattc_remote_t* gattc_remote;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(cb, BT_STATUS_PARM_INVALID);

    priv = ins->priv;
    if (!priv)
        return BT_STATUS_IPC_ERROR;

    if (priv->gattc_remote_list == NULL) {
        priv->gattc_remote_list = bt_list_new(free);
        if (priv->gattc_remote_list == NULL) {
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

    data = calloc(1, sizeof(bt_gattc_create_connect_data_t));
    data->userdata = userdata;
    data->gattc_remote = (void*)gattc_remote;
    data->user_phandle = phandle;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_GATT_CLIENT_CREATE_CONNECT, gattc_create_connect_reply, (void*)cb, data);
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    return BT_STATUS_SUCCESS;

fail:
    if (gattc_remote) {
        free(gattc_remote);
    }

    if (!bt_list_length(priv->gattc_remote_list)) {
        bt_list_free(priv->gattc_remote_list);
        priv->gattc_remote_list = NULL;
    }

    if (data)
        free(data);

    return status;
}

bt_status_t bt_gattc_delete_connect_async(gattc_handle_t conn_handle, bt_gattc_delete_connect_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_socket_async_client_t* priv;
    bt_status_t status;
    bt_instance_t* ins;
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;
    void** user_phandle;

    CHECK_NULL_PTR(gattc_remote);

    priv = gattc_remote->ins->priv;
    if (!priv || !priv->gattc_remote_list)
        return BT_STATUS_IPC_ERROR;

    ins = gattc_remote->ins;
    packet.gattc_pl._bt_gattc_delete.handle = PTR2INT(uint64_t) gattc_remote->cookie;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_GATT_CLIENT_DELETE_CONNECT, gattc_delete_connect_reply, (void*)cb, userdata);

    user_phandle = gattc_remote->user_phandle;
    bt_list_remove(priv->gattc_remote_list, gattc_remote);
    *user_phandle = NULL;

    if (!bt_list_length(priv->gattc_remote_list)) {
        bt_list_free(priv->gattc_remote_list);
        priv->gattc_remote_list = NULL;
    }

    return status;
}

bt_status_t bt_gattc_connect_async(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_connect.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_connect.addr_type = addr_type;
    memcpy(&packet.gattc_pl._bt_gattc_connect.addr, addr, sizeof(bt_address_t));

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_CONNECT, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_disconnect_async(gattc_handle_t conn_handle, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_disconnect.handle = PTR2INT(uint64_t) gattc_remote->cookie;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_DISCONNECT, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_discover_service_async(gattc_handle_t conn_handle, bt_uuid_t* filter_uuid, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_discover_service.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    if (filter_uuid == NULL)
        packet.gattc_pl._bt_gattc_discover_service.filter_uuid.type = 0;
    else
        memcpy(&packet.gattc_pl._bt_gattc_discover_service.filter_uuid, filter_uuid, sizeof(bt_uuid_t));

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_DISCOVER_SERVICE, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_get_attribute_by_handle_async(gattc_handle_t conn_handle, uint16_t attr_handle, bt_gattc_get_attribute_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_get_attr_by_handle.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_get_attr_by_handle.attr_handle = attr_handle;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_GET_ATTRIBUTE_BY_HANDLE, gattc_get_attribute_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_get_attribute_by_uuid_async(gattc_handle_t conn_handle, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid,
    bt_gattc_get_attribute_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_get_attr_by_uuid.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_get_attr_by_uuid.start_handle = start_handle;
    packet.gattc_pl._bt_gattc_get_attr_by_uuid.end_handle = end_handle;
    memcpy(&packet.gattc_pl._bt_gattc_get_attr_by_uuid.attr_uuid, attr_uuid, sizeof(bt_uuid_t));

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_GET_ATTRIBUTE_BY_UUID, gattc_get_attribute_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_read_async(gattc_handle_t conn_handle, uint16_t attr_handle, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_read.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_read.attr_handle = attr_handle;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_READ, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_write_async(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);
    if (length > sizeof(packet.gattc_pl._bt_gattc_write.value))
        return BT_STATUS_PARM_INVALID;

    packet.gattc_pl._bt_gattc_write.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_write.attr_handle = attr_handle;
    packet.gattc_pl._bt_gattc_write.length = length;
    memcpy(packet.gattc_pl._bt_gattc_write.value, value, length);

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_WRITE, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_write_without_response_async(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length,
    bt_gattc_write_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);
    if (length > sizeof(packet.gattc_pl._bt_gattc_write.value))
        return BT_STATUS_PARM_INVALID;

    packet.gattc_pl._bt_gattc_write.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_write.attr_handle = attr_handle;
    packet.gattc_pl._bt_gattc_write.length = length;
    memcpy(packet.gattc_pl._bt_gattc_write.value, value, length);

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_WRITE_NR, gattc_write_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_subscribe_async(gattc_handle_t conn_handle, uint16_t attr_handle, uint16_t ccc_value, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_subscribe.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_subscribe.attr_handle = attr_handle;
    packet.gattc_pl._bt_gattc_subscribe.ccc_value = ccc_value;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_SUBSCRIBE, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_unsubscribe_async(gattc_handle_t conn_handle, uint16_t attr_handle, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_subscribe.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_subscribe.attr_handle = attr_handle;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_UNSUBSCRIBE, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_exchange_mtu_async(gattc_handle_t conn_handle, uint32_t mtu, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_exchange_mtu.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_exchange_mtu.mtu = mtu;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_EXCHANGE_MTU, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_update_connection_parameter_async(gattc_handle_t conn_handle, uint32_t min_interval, uint32_t max_interval,
    uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length,
    uint32_t max_connection_event_length, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_update_connection_param.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_update_connection_param.min_interval = min_interval;
    packet.gattc_pl._bt_gattc_update_connection_param.max_interval = max_interval;
    packet.gattc_pl._bt_gattc_update_connection_param.latency = latency;
    packet.gattc_pl._bt_gattc_update_connection_param.timeout = timeout;
    packet.gattc_pl._bt_gattc_update_connection_param.min_connection_event_length = min_connection_event_length;
    packet.gattc_pl._bt_gattc_update_connection_param.max_connection_event_length = max_connection_event_length;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_UPDATE_CONNECTION_PARAM, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_read_phy_async(gattc_handle_t conn_handle, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_phy.handle = PTR2INT(uint64_t) gattc_remote->cookie;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_READ_PHY, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_update_phy_async(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_phy.handle = PTR2INT(uint64_t) gattc_remote->cookie;
    packet.gattc_pl._bt_gattc_phy.tx_phy = tx_phy;
    packet.gattc_pl._bt_gattc_phy.rx_phy = rx_phy;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_UPDATE_PHY, gattc_status_reply, (void*)cb, userdata);
}

bt_status_t bt_gattc_read_rssi_async(gattc_handle_t conn_handle, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_gattc_remote_t* gattc_remote = (bt_gattc_remote_t*)conn_handle;

    CHECK_NULL_PTR(gattc_remote);

    packet.gattc_pl._bt_gattc_rssi.handle = PTR2INT(uint64_t) gattc_remote->cookie;

    return bt_socket_client_send_with_reply(gattc_remote->ins, &packet, BT_GATT_CLIENT_READ_RSSI, gattc_status_reply, (void*)cb, userdata);
}
