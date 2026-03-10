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
#include "bt_socket.h"

#include "utils/log.h"

void* BTSYMBOLS(bt_auracast_sink_register_callbacks)(bt_instance_t* ins,
    const bt_auracast_sink_callbacks_t* cbs)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    BT_SOCKET_INS_VALID(ins, NULL);
    BT_SOCKET_PTR_VALID(cbs, NULL);

    BT_LOGD("%s, cbs = %p", __func__, cbs);

    if (ins->auracast_sink_callbacks != NULL) {
        cookie = bt_remote_callbacks_register(ins->auracast_sink_callbacks, NULL, (void*)cbs);
        BT_LOGD("cookie = %p", cookie);
        return cookie;
    }

    ins->auracast_sink_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

#ifdef CONFIG_BLUETOOTH_FEATURE
    cookie = bt_remote_callbacks_register(ins->auracast_sink_callbacks, ins, (void*)cbs);
#else
    cookie = bt_remote_callbacks_register(ins->auracast_sink_callbacks, NULL, (void*)cbs);
#endif
    if (cookie == NULL) {
        bt_callbacks_list_free(ins->auracast_sink_callbacks);
        ins->auracast_sink_callbacks = NULL;
        return NULL;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_AURACAST_SINK_REGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.auracast_sink_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->auracast_sink_callbacks);
        ins->auracast_sink_callbacks = NULL;
        return NULL;
    }

    BT_LOGD("cookie = %p", cookie);
    return cookie;
}

bool BTSYMBOLS(bt_auracast_sink_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    BT_LOGD("%s, cookie = %p", __func__, cookie);

    if (!ins->auracast_sink_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->auracast_sink_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->auracast_sink_callbacks) > 0) {
        return true;
    }

    bt_callbacks_list_free(ins->auracast_sink_callbacks);
    ins->auracast_sink_callbacks = NULL;

    status = bt_socket_client_sendrecv(ins, &packet, BT_AURACAST_SINK_UNREGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.auracast_sink_r.status != BT_STATUS_SUCCESS)
        return false;

    BT_LOGD("service callback unregistered");

    return true;
}

bt_status_t BTSYMBOLS(bt_auracast_sink_create_sync)(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, uint32_t bitfield, const uint8_t* broadcast_code)
{
    bt_message_packet_t packet;
    bt_status_t status;

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

    status = bt_socket_client_sendrecv(ins, &packet, BT_AURACAST_SINK_CREATE_SYNC);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.auracast_sink_r.status;
}

bt_status_t BTSYMBOLS(bt_auracast_sink_terminate_sync)(bt_instance_t* ins,
    const bt_le_address_t* addr, uint8_t sid)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    BT_LOGD("%s", __func__);

    memcpy(&packet.auracast_sink_pl._bt_auracast_sink_terminate_sync.addr, addr,
        sizeof(bt_le_address_t));
    packet.auracast_sink_pl._bt_auracast_sink_terminate_sync.sid = sid;
    status = bt_socket_client_sendrecv(ins, &packet, BT_AURACAST_SINK_TERMINATE_SYNC);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.auracast_sink_r.status;
}
