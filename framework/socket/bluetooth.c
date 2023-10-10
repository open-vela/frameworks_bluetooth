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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "manager_service.h"

#include "bluetooth.h"
#include "service_loop.h"
#include "bt_socket.h"

bt_instance_t *bluetooth_create_instance(void)
{
    bt_status_t status;
    bt_instance_t *ins;
    uint32_t app_id;

    ins = malloc(sizeof(bt_instance_t));
    if (ins == NULL) {
        return NULL;
    }

    status = manager_create_instance((uint32_t)ins, BLUETOOTH_SYSTEM,
                                     "local", getpid(), 0, &app_id);
    if (status == BT_STATUS_SUCCESS) {
#if defined(CONFIG_BLUETOOTH_SERVER)
      status = bt_socket_client_init(ins, PF_LOCAL,
          "bluetooth", NULL, CONFIG_BLUETOOTH_SOCKET_PORT);
#elif defined(CONFIG_NET_RPMSG)
      status = bt_socket_client_init(ins, AF_RPMSG,
          "bluetooth", CONFIG_BLUETOOTH_RPMSG_CPUNAME, CONFIG_BLUETOOTH_SOCKET_PORT);
#elif defined(CONFIG_NET_IPv4)
      status = bt_socket_client_init(ins, AF_INET,
          "bluetooth", NULL, CONFIG_BLUETOOTH_SOCKET_PORT);
#else
      status = bt_socket_client_init(ins, PF_LOCAL,
          "bluetooth", NULL, CONFIG_BLUETOOTH_SOCKET_PORT);
#endif
    }

    if (status != BT_STATUS_SUCCESS) {
      bluetooth_delete_instance(ins);
      ins = NULL;
    }

    return ins;
}

bt_instance_t *bluetooth_get_instance(void)
{
    bt_status_t status;
    uint32_t handle;

    status = manager_get_instance("local", getpid(), &handle);
    if (status)
        return (bt_instance_t *)handle;
    else
        return bluetooth_create_instance();
}

void *bluetooth_get_proxy(bt_instance_t *ins, enum profile_id id)
{
    return NULL;
}

void bluetooth_delete_instance(bt_instance_t *ins)
{
    manager_delete_instance(ins->app_id);
    free(ins);
}

bt_status_t bluetooth_start_service(bt_instance_t *ins, enum profile_id id)
{
    return BT_STATUS_FAIL;
}

bt_status_t bluetooth_stop_service(bt_instance_t *ins, enum profile_id id)
{
    return BT_STATUS_FAIL;
}
