/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#include "bt_a2dp_sink.h"
#include "bt_adapter.h"
#include "bt_tools.h"

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int get_state_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_a2dp_sink_tables[] = {
    { "connect", connect_cmd, 0, "\"establish a2dp sink signal and stream connection, params: <address>\"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect a2dp sink signal and stream connection, params: <address>\"" },
    { "state", get_state_cmd, 0, "\"get a2dp sink connection or audio state , params: <address>\"" },
};

static void* sink_cbks_cookie = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_a2dp_sink_tables); i++) {
        printf("\t%-8s\t%s\n", g_a2dp_sink_tables[i].cmd, g_a2dp_sink_tables[i].help);
    }
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_a2dp_sink_connect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_a2dp_sink_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int get_state_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int state = bt_a2dp_sink_get_connection_state(handle, &addr);
    PRINT("A2DP sink connection state: %d", state);
    return CMD_OK;
}

static void a2dp_sink_connection_state_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    PRINT_ADDR("a2dp_sink_connection_state_cb, addr:%s, state:%d", addr, state);
}

static void a2dp_sink_audio_state_cb(void* cookie, bt_address_t* addr, a2dp_audio_state_t state)
{
    PRINT_ADDR("a2dp_src_audio_state_cb, addr:%s, state:%d", addr, state);
}

static void a2dp_sink_audio_config_cb(void* cookie, bt_address_t* addr)
{
    PRINT_ADDR("a2dp_sink_audio_config_cb, addr:%s", addr);
}

static const a2dp_sink_callbacks_t a2dp_sink_cbs = {
    sizeof(a2dp_sink_cbs),
    a2dp_sink_connection_state_cb,
    a2dp_sink_audio_state_cb,
    a2dp_sink_audio_config_cb,
};

int a2dp_sink_commond_init(void* handle)
{
    sink_cbks_cookie = bt_a2dp_sink_register_callbacks(handle, &a2dp_sink_cbs);

    return 0;
}

int a2dp_sink_commond_uninit(void* handle)
{
    bt_a2dp_sink_unregister_callbacks(handle, sink_cbks_cookie);

    return 0;
}

int a2dp_sink_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_a2dp_sink_tables, ARRAY_SIZE(g_a2dp_sink_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
