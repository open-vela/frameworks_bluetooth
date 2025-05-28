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

#include "bt_async.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "bt_trace.h"
#include "utils/btsnoop_log.h"

bt_status_t bluetooth_enable_btsnoop_log_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_LOG_ENABLE, NULL, (void*)cb, userdata);
}

bt_status_t bluetooth_disable_btsnoop_log_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_LOG_DISABLE, NULL, (void*)cb, userdata);
}

bt_status_t bluetooth_set_btsnoop_filter_async(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag,
    bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.log_pl._bt_log_set_flag.filter_flag = filter_flag;

    return bt_socket_client_send_with_reply(ins, &packet, BT_LOG_SET_FILTER, NULL, (void*)cb, userdata);
}

bt_status_t bluetooth_remove_btsnoop_filter_async(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag,
    bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.log_pl._bt_log_remove_flag.filter_flag = filter_flag;

    return bt_socket_client_send_with_reply(ins, &packet, BT_LOG_REMOVE_FILTER, NULL, (void*)cb, userdata);
}