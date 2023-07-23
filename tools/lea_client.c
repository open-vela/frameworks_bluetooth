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

static int connect_device(void *handle, int argc, char *argv[]);
static int connect_audio(void *handle, int argc, char *argv[]);
static int disconnect_device(void *handle, int argc, char *argv[]);
static int disconnect_audio(void *handle, int argc, char *argv[]);
static int get_connection_state(void *handle, int argc, char *argv[]);

static bt_command_t g_lea_client_tables[] = {
    {"connect",          connect_device,       0, "\"connect device, params: <address>\""          },
    { "connectaudio",    connect_audio,        0, "\"connect audio, params: <address> <context>\"" },
    { "disconnect",      disconnect_device,    0, "\"disconnect device, params: <address>\""       },
    { "disconnectaudio", disconnect_audio,     0, "\"disconnect audio, params: <address>\""        },
    { "constate",        get_connection_state, 0, "\"get lea connection state, params: <address>\""},
};

static struct option lea_client_options[] = {
    {"help", 0, 0, 'h'},
    { 0,     0, 0, 0  }
};

static void *lea_client_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_client_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_client_tables[i].cmd, g_lea_client_tables[i].help);
    }
}

static int connect_device(void *handle, int argc, char *argv[])
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

static int connect_audio(void *handle, int argc, char *argv[])
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

static int disconnect_device(void *handle, int argc, char *argv[])
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

static int disconnect_audio(void *handle, int argc, char *argv[])
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

static int get_connection_state(void *handle, int argc, char *argv[])
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

static void client_stack_state_callback(void *cookie, lea_client_stack_state_t enabled)
{
    PRINT("client_stack_state_callback enable:%d", enabled);
}

static void client_connection_state_callback(void *cookie,
                                             profile_connection_state_t state, bt_address_t *bd_addr)
{
    PRINT_ADDR("lea_connection_state_callback, addr:%s, state:%d", bd_addr, state);
}

static const lea_client_callbacks_t lea_client_cbs = {
    sizeof(lea_client_cbs),
    .client_stack_state_cb = client_stack_state_callback,
    .client_connection_state_cb = client_connection_state_callback,
};

int leac_command_init(void *handle)
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

void leac_command_uninit(void *handle)
{
    bt_status_t ret;

    bt_lea_client_unregister_callbacks(handle, lea_client_callbacks);
    ret = bluetooth_stop_service(handle, PROFILE_LEAUDIO_CLIENT);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
    }
}

int leac_command_exec(void *handle, int argc, char *argv[])
{
    int opt, ret = CMD_USAGE_FAULT;

    while ((opt = getopt_long(argc, argv, "h", lea_client_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return CMD_OK;
        default:
            break;
        }
    }

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_client_tables, ARRAY_SIZE(g_lea_client_tables), argc, argv);

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[0]);
        usage();
    }

    return ret;
}
