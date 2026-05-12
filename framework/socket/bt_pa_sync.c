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

#include "bt_pa_sync.h"
#include "bt_socket.h"

bt_status_t bt_pa_sync_create(bt_instance_t* ins, const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_create_param_t* params, const bt_pa_sync_callbacks_t* cbs, const void* context)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(addr, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(cbs, BT_STATUS_PARM_INVALID);

    memcpy(&packet.pa_sync_pl._bt_pa_sync_create.addr, addr, sizeof(bt_le_address_t));
    packet.pa_sync_pl._bt_pa_sync_create.sid = sid;
    packet.pa_sync_pl._bt_pa_sync_create.cbs = PTR2INT(uint64_t) cbs;
    if (params) {
        packet.pa_sync_pl._bt_pa_sync_create.have_params = true;
        memcpy(&packet.pa_sync_pl._bt_pa_sync_create.params, params,
            sizeof(bt_pa_sync_create_param_t));
    }

    if (context)
        packet.pa_sync_pl._bt_pa_sync_create.context = PTR2INT(uint64_t) context;

    status = bt_socket_client_sendrecv(ins, &packet, BT_PA_SYNC_CREATE_SYNC);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pa_sync_r.status;
}

bt_status_t bt_pa_sync_terminate(bt_instance_t* ins, const bt_le_address_t* addr, uint8_t sid)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(addr, BT_STATUS_PARM_INVALID);

    memcpy(&packet.pa_sync_pl._bt_pa_sync_terminate.addr, addr, sizeof(bt_le_address_t));
    packet.pa_sync_pl._bt_pa_sync_terminate.sid = sid;

    status = bt_socket_client_sendrecv(ins, &packet, BT_PA_SYNC_TERMINATE_SYNC);
    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.pa_sync_r.status;
}
