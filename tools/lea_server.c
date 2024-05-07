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
#include "bt_lea_server.h"
#include "bt_tools.h"

static int start_announce_cmd(void* handle, int argc, char* argv[]);
static int stop_announce_cmd(void* handle, int argc, char* argv[]);
static int disconnect_device(void* handle, int argc, char* argv[]);
static int get_connection_state(void* handle, int argc, char* argv[]);
static int disconnect_audio(void* handle, int argc, char* argv[]);

static bt_command_t g_lea_server_tables[] = {
    { "startance", start_announce_cmd, 0, "\"start lea announce, params: <adv_id> <announce_type>\"" },
    { "stopance", stop_announce_cmd, 0, "\"stop lea announce, params: <adv_id>\"" },
    { "disconnect", disconnect_device, 0, "\"disconnect lea connection, params: <address>\"" },
    { "constate", get_connection_state, 0, "\"get lea connection state, params: <address>\"" },
    { "disconnectaudio", disconnect_audio, 0, "\"disconnect lea audio, params: <address>\"" },
};

static void* lea_server_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_server_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_server_tables[i].cmd, g_lea_server_tables[i].help);
    }
}

static int start_announce_cmd(void* handle, int argc, char* argv[])
{
    uint16_t adv_size;
    uint16_t md_size;
    uint8_t adv_id;
    uint8_t announce_type;
    uint8_t adv_data[] = { 0x02, 0x01, 0x06, 0x09, 0x09, 0x42, 0x52, 0x54, 0x2D,
        0x49, 0x44, 0x4D, 0x30, 0x03, 0x02, 0x00, 0xFF };
    uint8_t md_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    adv_size = sizeof(adv_data) / sizeof(adv_data[0]);
    md_size = sizeof(md_data) / sizeof(md_data[0]);
    adv_id = atoi(argv[0]);
    announce_type = atoi(argv[1]);

    if (bt_lea_server_start_announce(handle, adv_id, announce_type, adv_data, adv_size, md_data, md_size) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int stop_announce_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_lea_server_stop_announce(handle, atoi(argv[0])) != BT_STATUS_SUCCESS)
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

    if (bt_lea_server_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
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

    if (bt_lea_server_disconnect_audio(handle, &addr) != BT_STATUS_SUCCESS)
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

    state = bt_lea_server_get_connection_state(handle, &addr);

    PRINT_ADDR("get_connection_state, addr:%s, state:%d", &addr, state);

    return CMD_OK;
}

static void server_stack_state_callback(void* cookie,
    lea_server_stack_state_t enabled)
{
    PRINT("server_stack_state_callback enable:%d", enabled);
}

static void server_connection_state_callback(void* cookie,
    profile_connection_state_t state, bt_address_t* bd_addr)
{
    PRINT_ADDR("lea_connection_state_callback, addr:%s, state:%d", bd_addr, state);
}

static const lea_server_callbacks_t lea_server_cbs = {
    sizeof(lea_server_cbs),
    server_stack_state_callback,
    server_connection_state_callback,
};

int leas_command_init(void* handle)
{
    bt_status_t ret;

    ret = bluetooth_start_service(handle, PROFILE_LEAUDIO_SERVER);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
        return ret;
    }

    lea_server_callbacks = bt_lea_server_register_callbacks(handle, &lea_server_cbs);
    return 0;
}

void leas_command_uninit(void* handle)
{
    bt_status_t ret;

    bt_lea_server_unregister_callbacks(handle, lea_server_callbacks);
    ret = bluetooth_stop_service(handle, PROFILE_LEAUDIO_SERVER);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
    }
}

int leas_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_server_tables, ARRAY_SIZE(g_lea_server_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
