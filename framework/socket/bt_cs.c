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
#include "bt_cs.h"
#include "bt_message.h"
#include "bt_socket.h"
#include <stdint.h>

void* bt_cs_register_callbacks(bt_instance_t* ins, const cs_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* cookie;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->cs_callbacks != NULL) {
        cookie = bt_remote_callbacks_register(ins->cs_callbacks, NULL, (void*)callbacks);
        return cookie;
    }

    ins->cs_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    cookie = bt_remote_callbacks_register(ins->cs_callbacks, NULL, (void*)callbacks);
    if (cookie == NULL) {
        bt_callbacks_list_free(ins->cs_callbacks);
        ins->cs_callbacks = NULL;
        return cookie;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_CS_REGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.cs_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->cs_callbacks);
        ins->cs_callbacks = NULL;
        return NULL;
    }

    return cookie;
}

bool bt_cs_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->cs_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->cs_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->cs_callbacks) > 0) {
        return true;
    }

    bt_callbacks_list_free(ins->cs_callbacks);
    ins->cs_callbacks = NULL;

    status = bt_socket_client_sendrecv(ins, &packet, BT_CS_UNREGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.cs_r.status != BT_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

bt_status_t bt_cs_start_distance_measurement(bt_instance_t* ins, const bt_distance_measurement_params_t* params)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.cs_pl._bt_cs_start_distance_measurement.params, params, sizeof(bt_distance_measurement_params_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_CS_START_DISTANCE_MEASUREMENT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    return packet.cs_r.status;
}

bt_status_t bt_cs_stop_distance_measurement(bt_instance_t* ins, bt_address_t* addr, uint8_t method, bool timeout)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.cs_pl._bt_cs_stop_distance_measurement.addr, addr, sizeof(packet.cs_pl._bt_cs_stop_distance_measurement.addr));
    packet.cs_pl._bt_cs_stop_distance_measurement.method = method;
    packet.cs_pl._bt_cs_stop_distance_measurement.timeout_bool = timeout;
    status = bt_socket_client_sendrecv(ins, &packet, BT_CS_STOP_DISTANCE_MEASUREMENT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    return packet.cs_r.status;
}

bt_status_t bt_cs_set_config(bt_instance_t* ins, bt_address_t* addr, const bt_cs_set_params_t* params)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (addr)
        memcpy(&packet.cs_pl._bt_cs_set_config.addr, addr, sizeof(bt_address_t));

    memcpy(&packet.cs_pl._bt_cs_set_config.params, params, sizeof(bt_cs_set_params_t));
    status = bt_socket_client_sendrecv(ins, &packet, BT_CS_SET_CONFIG);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    return packet.cs_r.status;
}

#ifdef CONFIG_BT_CS_RAS_TEST
bt_status_t bt_cs_test(bt_instance_t* ins, const void* data, uint16_t len)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    if (data) {
        memcpy(&packet.cs_pl._bt_cs_test.data, data, len);
    }

    packet.cs_pl._bt_cs_test.len = len;

    status = bt_socket_client_sendrecv(ins, &packet, BT_CS_TEST);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }
    return packet.cs_r.status;
}
#endif /* CONFIG_BT_CS_RAS_TEST */