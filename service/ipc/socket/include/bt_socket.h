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
#ifndef _BT_SOCKET_H__
#define _BT_SOCKET_H__

#include "bluetooth.h"
#include "bt_message.h"

/* Macros for number of items.
 * (aka. ARRAY_SIZE, ArraySize, Size of an Array)
 */

#ifndef nitems
#  define nitems(_a)    (sizeof(_a) / sizeof(0[(_a)]))
#endif /* nitems */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLUETOOTH_SOCKADDR_NAME "bt:%s"
#define BLUETOOTH_SERVER_MAXCONN 10

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Client */

int bt_socket_client_init(bt_instance_t *ins, int family,
                          const char *name, const char *cpu,
                          int port);

int bt_socket_client_sendrecv(bt_instance_t *ins,
                              bt_message_packet_t *packet,
                              bt_message_type_t code);

/* Server */

int bt_socket_server_init(const char *name, int port);

int bt_socket_server_send(bt_instance_t *ins, bt_message_packet_t *packet,
                          bt_message_type_t code);

/* Manager */
void bt_socket_server_manager_process(service_poll_t *poll,
    int fd, bt_instance_t *ins, bt_message_packet_t *packet);

int bt_socket_client_manager_callback(service_poll_t *poll,
    int fd, bt_instance_t *ins, bt_message_packet_t *packet);

/* Adapter */

void bt_socket_server_adapter_process(service_poll_t *poll,
    int fd, bt_instance_t *ins, bt_message_packet_t *packet);

int bt_socket_client_adapter_callback(service_poll_t *poll,
    int fd, bt_instance_t *ins, bt_message_packet_t *packet);

/* Device */

void bt_socket_server_device_process(service_poll_t *poll,
    int fd, bt_instance_t *ins, bt_message_packet_t *packet);

#ifdef __cplusplus
}
#endif

#endif /* _BT_SOCKET_H__ */
