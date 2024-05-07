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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_lea_client.h"
#include "bt_tools.h"

static int connect_device(void* handle, int argc, char* argv[]);
static int connect_audio(void* handle, int argc, char* argv[]);
static int disconnect_device(void* handle, int argc, char* argv[]);
static int disconnect_audio(void* handle, int argc, char* argv[]);
static int get_connection_state(void* handle, int argc, char* argv[]);
static int get_group_id(void* handle, int argc, char* argv[]);
static int discovery_member_start(void* handle, int argc, char* argv[]);
static int discovery_member_stop(void* handle, int argc, char* argv[]);
static int group_add_member(void* handle, int argc, char* argv[]);
static int group_remove_member(void* handle, int argc, char* argv[]);
static int group_connect_audio(void* handle, int argc, char* argv[]);
static int group_disconnect_audio(void* handle, int argc, char* argv[]);
static int group_lock(void* handle, int argc, char* argv[]);
static int group_unlock(void* handle, int argc, char* argv[]);

static bt_command_t g_lea_client_tables[] = {
    { "connect", connect_device, 0, "\"connect device, params: <address>\"" },
    { "connectaudio", connect_audio, 0, "\"connect audio, params: <address> <context>\"" },
    { "disconnect", disconnect_device, 0, "\"disconnect device, params: <address>\"" },
    { "disconnectaudio", disconnect_audio, 0, "\"disconnect audio, params: <address>\"" },
    { "constate", get_connection_state, 0, "\"get lea connection state, params: <address>\"" },
    { "groupid", get_group_id, 0, "\"get group id, params: <address>\"" },
    { "discoverystart", discovery_member_start, 0, "\"discovery member start, params: <group id>\"" },
    { "discoverystop", discovery_member_stop, 0, "\"discovery member stop, params: <group id>\"" },
    { "addmember", group_add_member, 0, "\"group add member, params: <group id>  <address>\"" },
    { "removemember", group_remove_member, 0, "\"group remove member, params: <group id>  <address>\"" },
    { "groupconnectaudio", group_connect_audio, 0, "\"group connect audio, params: <group id> <conetxt>\"" },
    { "groupdisconnectaudio", group_disconnect_audio, 0, "\"group disconnect audio, params: <group id>\"" },
    { "grouplock", group_lock, 0, "\"group lock, params: <group id>\"" },
    { "groupunlock", group_unlock, 0, "\"group unlock, params: <group id>\"" },
};

static void* lea_client_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_client_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_client_tables[i].cmd, g_lea_client_tables[i].help);
    }
}

static int connect_device(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_client_connect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int connect_audio(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_client_connect_audio(handle, &addr, atoi(argv[1])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_device(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_client_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_audio(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_client_disconnect_audio(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int get_connection_state(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    profile_connection_state_t state;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    state = bt_lea_client_get_connection_state(handle, &addr);

    PRINT_ADDR("get_connection_state, addr:%s, state:%d", &addr, state);

    return CMD_OK;
}

static int get_group_id(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint32_t group_id;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_client_get_group_id(handle, &addr, &group_id) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT_ADDR("get_group_id, addr:%s, group_id:%d", &addr, group_id);

    return CMD_OK;
}

static int discovery_member_start(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_lea_client_discovery_member_start(handle, atoi(argv[0])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int discovery_member_stop(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_lea_client_discovery_member_stop(handle, atoi(argv[0])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int group_add_member(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[1], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_client_group_add_member(handle, atoi(argv[0]), &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int group_remove_member(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[1], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_client_group_remove_member(handle, atoi(argv[0]), &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int group_connect_audio(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_lea_client_group_connect_audio(handle, atoi(argv[0]), atoi(argv[1])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int group_disconnect_audio(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_lea_client_group_disconnect_audio(handle, atoi(argv[0])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int group_lock(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_lea_client_group_lock(handle, atoi(argv[0])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int group_unlock(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_lea_client_group_unlock(handle, atoi(argv[0])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void client_stack_state_callback(void* cookie, lea_client_stack_state_t enabled)
{
    PRINT("client_stack_state_callback enable:%d", enabled);
}

static void client_connection_state_callback(void* cookie,
    profile_connection_state_t state, bt_address_t* bd_addr)
{
    PRINT_ADDR("lea_connection_state_callback, addr:%s, state:%d", bd_addr, state);
}

void audio_state_callback(void* cookie, lea_audio_state_t state, bt_address_t* bd_addr)
{
    PRINT_ADDR("audio_state_callback, addr:%s, state:%d", bd_addr, state);
}

void group_member_discovered_callback(void* cookie, uint32_t group_id, bt_address_t* bd_addr)
{
    PRINT_ADDR("member_discovered_callback, addr:%s, group_id:%u", bd_addr, group_id);
}

void group_member_added_callback(void* cookie, uint32_t group_id, bt_address_t* bd_addr)
{
    PRINT_ADDR("member_added_callback, addr:%s, group_id:%u", bd_addr, group_id);
}

void group_member_removed_callback(void* cookie, uint32_t group_id, bt_address_t* bd_addr)
{
    PRINT_ADDR("member_removed_callback, addr:%s, group_id:%u", bd_addr, group_id);
}

void group_discovery_start_callback(void* cookie, uint32_t group_id)
{
    PRINT("discovery_start_callback, group_id:%u", group_id);
}

void group_discovery_stop_callback(void* cookie, uint32_t group_id)
{
    PRINT("discovery_stop_callback, group_id:%u", group_id);
}

void group_lock_callback(void* cookie, uint32_t group_id, lea_csip_lock_status result)
{
    PRINT("group_lock_callback, group_id:%u, result:%d", group_id, result);
}

void group_unlock_callback(void* cookie, uint32_t group_id, lea_csip_lock_status result)
{
    PRINT("group_unlock_callback, group_id:%u, result:%d", group_id, result);
}

static const lea_client_callbacks_t lea_client_cbs = {
    sizeof(lea_client_cbs),
    .client_stack_state_cb = client_stack_state_callback,
    .client_connection_state_cb = client_connection_state_callback,
    .client_audio_state_cb = audio_state_callback,
    .client_group_member_discovered_cb = group_member_discovered_callback,
    .client_group_member_added_cb = group_member_added_callback,
    .client_group_member_removed_cb = group_member_removed_callback,
    .client_group_discovery_start_cb = group_discovery_start_callback,
    .client_group_discovery_stop_cb = group_discovery_stop_callback,
    .client_group_lock_cb = group_lock_callback,
    .client_group_unlock_cb = group_unlock_callback,
};

int leac_command_init(void* handle)
{
    bt_status_t ret;

    ret = bluetooth_start_service(handle, PROFILE_LEAUDIO_CLIENT);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
        return ret;
    }

    lea_client_callbacks = bt_lea_client_register_callbacks(handle, &lea_client_cbs);
    return 0;
}

void leac_command_uninit(void* handle)
{
    bt_status_t ret;

    bt_lea_client_unregister_callbacks(handle, lea_client_callbacks);
    ret = bluetooth_stop_service(handle, PROFILE_LEAUDIO_CLIENT);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
    }
}

int leac_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_client_tables, ARRAY_SIZE(g_lea_client_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
