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
#include "bt_lea_mcp.h"
#include "bt_tools.h"

static int mcp_read_remote_info(void* handle, int argc, char* argv[]);
static int mcp_media_control_request(void* handle, int argc, char* argv[]);
static int mcp_search_control_request(void* handle, int argc, char* argv[]);

static bt_command_t g_lea_mcp_tables[] = {
    { "readinfo", mcp_read_remote_info, 0, "read remote media info   param: <addr><opcode>" },
    { "mediacontrolrequest", mcp_media_control_request, 0, "media control request    param: <addr><opcode:Play><offset:Nth segment>" },
    { "searchrequest", mcp_search_control_request, 0, "mcp search request       param: <addr><number><type><parameter>" },
};

static void* mcp_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_mcp_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_mcp_tables[i].cmd, g_lea_mcp_tables[i].help);
    }
}

/* interface */
static int mcp_read_remote_info(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    uint8_t opcode = atoi(argv[1]);
    if (bt_lea_mcp_read_info(handle, &addr, opcode) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcp_media_control_request(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    uint32_t opcode = atoi(argv[1]);
    int32_t n = atoi(argv[2]);

    if (bt_lea_mcp_media_control_request(handle, &addr, opcode, n) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int mcp_search_control_request(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    uint8_t number = atoi(argv[1]);
    uint32_t type = atoi(argv[2]);
    uint8_t* parameter = (uint8_t*)strdup(argv[3]);

    if (bt_lea_mcp_search_control_request(handle, &addr, number, type, parameter) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void mcp_test_callback(void* context, bt_address_t* addr, uint8_t event)
{
    PRINT_ADDR("mcp_test_callback, addr:%s, event:%d ", addr, event);
}

static const lea_mcp_callbacks_t lea_mcp_cbs = {
    sizeof(lea_mcp_cbs),
    mcp_test_callback,
};

int lea_mcp_commond_init(void* handle)
{
    mcp_callbacks = bt_lea_mcp_register_callbacks(handle, &lea_mcp_cbs);
    return CMD_OK;
}

void lea_mcp_commond_uninit(void* handle)
{
    bt_status_t ret;

    bt_lea_mcp_unregister_callbacks(handle, mcp_callbacks);
    ret = bluetooth_stop_service(handle, PROFILE_LEAUDIO_MCP);
    if (ret != BT_STATUS_SUCCESS) {
        PRINT("%s, failed ret:%d", __func__, ret);
    }
}

int lea_mcp_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_mcp_tables, ARRAY_SIZE(g_lea_mcp_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
