/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#include "bt_trace.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "utils/btsnoop_log.h"

void bluetooth_enable_btsnoop_log(bt_instance_t* ins)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );

    (void)bt_socket_client_sendrecv(ins, &packet, BT_LOG_ENABLE);
}

void bluetooth_disable_btsnoop_log(bt_instance_t* ins)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );

    (void)bt_socket_client_sendrecv(ins, &packet, BT_LOG_DISABLE);
}

void bluetooth_set_btsnoop_filter(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );

    packet.log_pl._bt_log_set_flag.filter_flag = filter_flag;
    (void)bt_socket_client_sendrecv(ins, &packet, BT_LOG_SET_FILTER);
}

void bluetooth_remove_btsnoop_filter(bt_instance_t* ins, btsnoop_filter_flag_t filter_flag)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );

    packet.log_pl._bt_log_remove_flag.filter_flag = filter_flag;
    (void)bt_socket_client_sendrecv(ins, &packet, BT_LOG_REMOVE_FILTER);
}