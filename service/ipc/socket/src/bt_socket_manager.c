/****************************************************************************
 * service/ipc/socket/src/bt_socket_manager.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "bt_internal.h"

#include "bluetooth.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
void bt_socket_server_manager_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_MANAGER_CREATE_INSTANCE: {
        packet->manager_r.status = manager_create_instance(packet->manager_pl._bluetooth_create_instance.handle,
            packet->manager_pl._bluetooth_create_instance.type,
            packet->manager_pl._bluetooth_create_instance.cpu_name,
            packet->manager_pl._bluetooth_create_instance.pid, 0,
            &packet->manager_r.v32);
        break;
    }
    case BT_MANAGER_DELETE_INSTANCE: {
        packet->manager_r.status = manager_delete_instance(packet->manager_pl._bluetooth_delete_instance.v32);
        break;
    }
    case BT_MANAGER_GET_INSTANCE: {
        packet->manager_r.status = manager_get_instance(packet->manager_pl._bluetooth_get_instance.cpu_name,
            packet->manager_pl._bluetooth_get_instance.pid,
            &packet->manager_r.v64);
        break;
    }
    case BT_MANAGER_START_SERVICE: {
        packet->manager_r.status = manager_start_service(packet->manager_pl._bluetooth_start_service.appid,
            packet->manager_pl._bluetooth_start_service.id);
        break;
    }
    case BT_MANAGER_STOP_SERVICE: {
        packet->manager_r.status = manager_stop_service(packet->manager_pl._bluetooth_stop_service.appid,
            packet->manager_pl._bluetooth_stop_service.id);
        break;
    }
    default:
        break;
    }
}
#endif
int bt_socket_client_manager_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
