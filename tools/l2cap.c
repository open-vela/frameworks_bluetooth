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
#include <stdlib.h>
#include <string.h>

#include "bt_l2cap.h"
#include "bt_list.h"
#include "bt_tools.h"
#include "euv_pipe.h"
#include "uv_thread_loop.h"

typedef struct {
    struct list_node node;
    euv_pipe_t* pipe;
    uint16_t psm;
    uint16_t cid;
    uint16_t id;
} l2cap_channel_t;

static uv_loop_t g_l2cap_thread;
static void* g_l2cap_handle;
static struct list_node channel_list = LIST_INITIAL_VALUE(channel_list);

static void on_connected(void* handle, l2cap_connect_params_t* params)
{
}

static void on_disconnected(void* handle, bt_address_t* addr, uint16_t id, uint32_t reason)
{
}

static l2cap_callbacks_t l2cap_callback = {
    .size = sizeof(l2cap_callbacks_t),
    .on_connected = on_connected,
    .on_disconnected = on_disconnected,
};

static int connect_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static int listen_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static int write_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static bt_command_t g_l2cap_commands[] = {
    { "connect", connect_cmd, 0, "\"connect l2cap channel      param: <address> <psm>\"" },
    { "listen", listen_cmd, 0, "\"listen l2cap channel        param: <psm>\"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect l2cap channel  param: <id>\"" },
    { "write", write_cmd, 0, "\"write data to peer   param: <id> <data>\"" },
};

int l2cap_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0) {
        ret = execute_command_in_table(handle, g_l2cap_commands, ARRAY_SIZE(g_l2cap_commands), argc, argv);
    }

    return ret;
}

int l2cap_command_init(void* handle)
{
    thread_loop_init(&g_l2cap_thread);
    thread_loop_run(&g_l2cap_thread, true, "bttool-l2cap");
    g_l2cap_handle = bt_l2cap_register_callbacks(handle, &l2cap_callback);

    return 0;
}

void l2cap_command_uninit(void* handle)
{
    bt_l2cap_unregister_callbacks(handle, g_l2cap_handle);
    thread_loop_exit(&g_l2cap_thread);
    memset(&g_l2cap_thread, 0, sizeof(g_l2cap_thread));
}
