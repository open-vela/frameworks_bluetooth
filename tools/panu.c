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

#include <stdlib.h>
#include <string.h>

#include "bt_pan.h"
#include "bt_tools.h"

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_pan_tables[] = {
    { "connect", connect_cmd, 0, "\"connect PAN     param: <address> <dstrole> <srcrole> \"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect PAN  param: <address>\"" },
    { "dump", dump_cmd, 0, "\"dump PAN current state\"" },
};

static void* pan_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_pan_tables); i++) {
        printf("\t%-8s\t%s\n", g_pan_tables[i].cmd, g_pan_tables[i].help);
    }
}

static void pan_connection_state_cb(void* cookie, profile_connection_state_t state,
    bt_address_t* addr, uint8_t local_role,
    uint8_t remote_role)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PRINT("%s, addr:%s, state:%d, local_role:%d, remote_role:%d", __func__,
        addr_str, state, local_role, remote_role);
}

static void pan_netif_state_cb(void* cookie, pan_netif_state_t state,
    int local_role, const char* ifname)
{
    PRINT("%s ifname:%s, state:%d, local_role:%d", __func__, ifname, state, local_role);
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    uint32_t src_role, dst_role;
    bt_address_t addr;

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    dst_role = atoi(argv[1]);
    src_role = atoi(argv[2]);
    if (bt_pan_connect(handle, &addr, dst_role, src_role) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("%s, address:%s", __func__, argv[0]);

    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    bt_pan_disconnect(handle, &addr);

    return CMD_OK;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static const pan_callbacks_t pan_test_cbs = {
    sizeof(pan_callbacks_t),
    pan_netif_state_cb,
    pan_connection_state_cb,
};

int pan_command_init(void* handle)
{
    pan_callbacks = bt_pan_register_callbacks(handle, &pan_test_cbs);

    return 0;
}

void pan_command_uninit(void* handle)
{
    bt_pan_unregister_callbacks(handle, pan_callbacks);
}

int pan_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_pan_tables, ARRAY_SIZE(g_pan_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}