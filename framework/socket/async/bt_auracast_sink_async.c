/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#define LOG_TAG "auracast_sink_api"

#include <stdint.h>

#include "bt_auracast_sink.h"
#include "bt_message.h"
#include "bt_socket.h"

#include "utils/log.h"

static void auracast_sink_status_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb,
    void* userdata)
{
    bt_status_cb_t ret_cb = (bt_status_cb_t)cb;

    HANDLE_BT_ASYNC_CALLBACK(ret_cb, ins, packet, auracast_sink_r, userdata);
}

static void auracast_sink_register_callback_reply(bt_instance_t* ins, bt_message_packet_t* packet,
    void* cb, void* userdata)
{
    bt_register_callback_cb_t ret_cb = (bt_register_callback_cb_t)cb;
    bt_register_callback_data_t* data = userdata;
    bt_socket_async_client_t* priv = ins->priv;
    bt_status_t status;

    if (!packet) {
        status = BT_STATUS_UNHANDLED;
        goto error;
    }

    if (packet->auracast_sink_r.status != BT_STATUS_SUCCESS) {
        status = packet->auracast_sink_r.status;
        goto error;
    }

    ret_cb(ins, packet->auracast_sink_r.status, data->cookie, data->userdata);

    free(data);
    return;

error:
    bt_callbacks_list_free(priv->auracast_sink_callbacks);
    priv->auracast_sink_callbacks = NULL;
    ret_cb(ins, status, data->cookie, data->userdata);
    free(data);
}

bt_status_t bt_auracast_sink_register_callbacks_async(bt_instance_t* ins,
    const bt_auracast_sink_callbacks_t* cbs, bt_register_callback_cb_t cb, void* userdata)
{
    bt_register_callback_data_t* data = NULL;
    bt_message_packet_t packet = { 0 };
    bt_socket_async_client_t* priv;
    bt_status_t status;
    void* cookie = NULL;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(cbs, BT_STATUS_PARM_INVALID);

    BT_LOGD("%s, cbs = %p", __func__, cbs);

    priv = ins->priv;
    if (!priv)
        return BT_STATUS_IPC_ERROR;

    data = zalloc(sizeof(bt_register_callback_data_t));
    if (!data)
        return BT_STATUS_NOMEM;

    if (priv->auracast_sink_callbacks == NULL) {
        priv->auracast_sink_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
        if (priv->auracast_sink_callbacks == NULL) {
            status = BT_STATUS_NOMEM;
            goto error;
        }
    }

    cookie = bt_remote_callbacks_register(priv->auracast_sink_callbacks, NULL, (void*)cbs);
    if (cookie == NULL) {
        status = BT_STATUS_NO_RESOURCES;
        goto error;
    }

    data->userdata = userdata;
    data->cookie = cookie;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_AURACAST_SINK_REGISTER_CALLBACKS,
        auracast_sink_register_callback_reply, cb, data);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    BT_LOGD("cookie = %p", cookie);
    return BT_STATUS_SUCCESS;

error:
    free(data);
    if (cookie)
        bt_remote_callbacks_unregister(priv->auracast_sink_callbacks, NULL, cookie);

    if (bt_callbacks_list_count(priv->auracast_sink_callbacks) == 0) {
        bt_callbacks_list_free(priv->auracast_sink_callbacks);
        priv->auracast_sink_callbacks = NULL;
    }

    return status;
}

bt_status_t bt_auracast_sink_unregister_callbacks_async(bt_instance_t* ins, void* cookie,
    bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };
    bt_socket_async_client_t* priv;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    BT_LOGD("%s, cookie = %p", __func__, cookie);

    priv = ins->priv;
    if (!priv || !priv->auracast_sink_callbacks)
        return BT_STATUS_IPC_ERROR;

    bt_remote_callbacks_unregister(priv->auracast_sink_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(priv->auracast_sink_callbacks) > 0) {
        return BT_STATUS_SUCCESS;
    }

    bt_callbacks_list_free(priv->auracast_sink_callbacks);
    priv->auracast_sink_callbacks = NULL;

    return bt_socket_client_send_with_reply(ins, &packet, BT_AURACAST_SINK_UNREGISTER_CALLBACKS,
        auracast_sink_status_reply, cb, userdata);
}

bt_status_t bt_auracast_sink_create_sync_async(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, uint32_t bitfield, const uint8_t* broadcast_code, bt_status_cb_t cb,
    void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    BT_LOGD("%s", __func__);

    memcpy(&packet.auracast_sink_pl._bt_auracast_sink_create_sync.addr, addr,
        sizeof(bt_le_address_t));
    packet.auracast_sink_pl._bt_auracast_sink_create_sync.sid = sid;
    packet.auracast_sink_pl._bt_auracast_sink_create_sync.bitfield = bitfield;
    if (broadcast_code) {
        packet.auracast_sink_pl._bt_auracast_sink_create_sync.encrypted = true;
        memcpy(&packet.auracast_sink_pl._bt_auracast_sink_create_sync.broadcast_code,
            broadcast_code, BT_AURACAST_BROADCAST_CODE_LEN);
    } else {
        packet.auracast_sink_pl._bt_auracast_sink_create_sync.encrypted = false;
    }

    return bt_socket_client_send_with_reply(ins, &packet, BT_AURACAST_SINK_CREATE_SYNC,
        auracast_sink_status_reply, cb, userdata);
}

bt_status_t bt_auracast_sink_terminate_sync_async(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    BT_LOGD("%s", __func__);

    memcpy(&packet.auracast_sink_pl._bt_auracast_sink_terminate_sync.addr, addr,
        sizeof(bt_le_address_t));
    packet.auracast_sink_pl._bt_auracast_sink_terminate_sync.sid = sid;
    return bt_socket_client_send_with_reply(ins, &packet, BT_AURACAST_SINK_TERMINATE_SYNC,
        auracast_sink_status_reply, cb, userdata);
}
