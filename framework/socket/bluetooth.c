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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bluetooth.h"
#include "bt_debug.h"
#include "bt_socket.h"
#include "manager_service.h"
#include "service_loop.h"

bt_instance_t* bluetooth_create_instance(void)
{
    bt_status_t status;
    bt_instance_t* ins;

    ins = zalloc(sizeof(bt_instance_t));
    if (ins == NULL) {
        return NULL;
    }

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

    if (status != BT_STATUS_SUCCESS) {
        free(ins);
        return NULL;
    }

    status = manager_create_instance(PTR2INT(uint64_t) ins, BLUETOOTH_SYSTEM,
        "local", getpid(), 0, &ins->app_id);
    if (status != BT_STATUS_SUCCESS) {
        bt_socket_client_deinit(ins);
        free(ins);
        ins = NULL;
    }
#if 0
    packet.manager_pl._bluetooth_create_instance.pid = getpid();
    packet.manager_pl._bluetooth_create_instance.handle = (uint32_t)ins;
    packet.manager_pl._bluetooth_create_instance.type = BLUETOOTH_USER;
    snprintf(packet.manager_pl._bluetooth_create_instance.cpu_name,
             sizeof(packet.manager_pl._bluetooth_create_instance.cpu_name),
             "%s", CONFIG_RPTUN_LOCAL_CPUNAME);

    status = bt_socket_client_sendrecv(ins, &packet, BT_MANAGER_CREATE_INSTANCE);
    if (status != BT_STATUS_SUCCESS || packet.manager_r.status != BT_STATUS_SUCCESS) {
        bluetooth_delete_instance(ins);
        return NULL;
    }
    ins->app_id = packet.manager_r.v32;
#endif

    return ins;
}

bt_instance_t* bluetooth_create_async_instance(uv_loop_t* loop, bt_ipc_connected_cb_t connected, bt_ipc_disconnected_cb_t disconnected, void* user_data)
{
    bt_status_t status;
    bt_instance_t* ins;

    ins = zalloc(sizeof(bt_instance_t));
    if (ins == NULL) {
        return NULL;
    }

#if defined(CONFIG_BLUETOOTH_SERVER)
    status = bt_socket_async_client_init(ins, loop, PF_LOCAL,
        "bluetooth", NULL, CONFIG_BLUETOOTH_SOCKET_PORT, connected, disconnected, user_data);
#elif defined(CONFIG_NET_RPMSG)
    status = bt_socket_async_client_init(ins, loop, AF_RPMSG,
        "bluetooth", CONFIG_BLUETOOTH_RPMSG_CPUNAME, CONFIG_BLUETOOTH_SOCKET_PORT, connected, disconnected, user_data);
#elif defined(CONFIG_NET_IPv4)
    status = bt_socket_async_client_init(ins, loop, AF_INET,
        "bluetooth", NULL, CONFIG_BLUETOOTH_SOCKET_PORT, connected, disconnected, user_data);
#else
    status = bt_socket_async_client_init(ins, loop, PF_LOCAL,
        "bluetooth", NULL, CONFIG_BLUETOOTH_SOCKET_PORT, connected, disconnected, user_data);
#endif

    if (status != BT_STATUS_SUCCESS) {
        free(ins);
        return NULL;
    }

    status = manager_create_instance(PTR2INT(uint64_t) ins, BLUETOOTH_SYSTEM,
        "local", getpid(), 0, &ins->app_id);
    if (status != BT_STATUS_SUCCESS) {
        bt_socket_client_deinit(ins);
        free(ins);
        ins = NULL;
    }

    return ins;
}
bt_instance_t* bluetooth_find_instance(pid_t pid)
{
    bt_status_t status;
    uint64_t handle;

    status = manager_get_instance("local", pid, &handle);
    if (status != BT_STATUS_SUCCESS) {
        return NULL;
    }
    return INT2PTR(bt_instance_t*) handle;
}

bt_instance_t* bluetooth_get_instance(void)
{
    bt_instance_t* bluetooth_ins = bluetooth_find_instance(getpid());

    if (bluetooth_ins == NULL)
        return bluetooth_create_instance();
    else
        return bluetooth_ins;
}

void* bluetooth_get_proxy(bt_instance_t* ins, enum profile_id id)
{
    return NULL;
}

void bluetooth_delete_instance(bt_instance_t* ins)
{
    BT_SOCKET_INS_VALID(ins, );

    manager_delete_instance(ins->app_id);
    bt_socket_client_deinit(ins);
    free(ins);
}

void bluetooth_delete_async_instance(bt_instance_t* ins)
{
    BT_SOCKET_INS_VALID(ins, );

    manager_delete_instance(ins->app_id);
    bt_socket_async_client_deinit(ins);
    free(ins);
}

bt_status_t bluetooth_start_service(bt_instance_t* ins, enum profile_id id)
{
    bt_status_t status;
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    packet.manager_pl._bluetooth_start_service.appid = ins->app_id;
    packet.manager_pl._bluetooth_start_service.id = id;
    status = bt_socket_client_sendrecv(ins, &packet, BT_MANAGER_START_SERVICE);
    if (status != BT_STATUS_SUCCESS || packet.manager_r.status != BT_STATUS_SUCCESS) {
        return packet.manager_r.status;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bluetooth_stop_service(bt_instance_t* ins, enum profile_id id)
{
    bt_status_t status;
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    packet.manager_pl._bluetooth_stop_service.appid = ins->app_id;
    packet.manager_pl._bluetooth_stop_service.id = id;
    status = bt_socket_client_sendrecv(ins, &packet, BT_MANAGER_STOP_SERVICE);
    if (status != BT_STATUS_SUCCESS || packet.manager_r.status != BT_STATUS_SUCCESS) {
        return packet.manager_r.status;
    }

    return BT_STATUS_SUCCESS;
}

#include "uv.h"
bool bluetooth_set_external_uv(bt_instance_t* ins, uv_loop_t* ext_loop)
{
    BT_SOCKET_INS_VALID(ins, false);

    ins->external_loop = ext_loop;

    return true;
}
