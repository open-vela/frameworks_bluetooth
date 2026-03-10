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
#include "bt_tools.h"

typedef struct {
    bt_scanner_t* scanner;
} bttool_auracast_sink_t;

static int scan_cmd(void* handle, int argc, char* argv[]);

static bttool_auracast_sink_t* g_auracast_sink = NULL;
static bt_command_t g_auracast_sink_tables[] = {
    { "scan", scan_cmd, 0, "\"Search for nearby Auracast sources\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_auracast_sink_tables); i++) {
        printf("\t%-8s\t%s\n", g_auracast_sink_tables[i].cmd, g_auracast_sink_tables[i].help);
    }
}

int scan_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

int auracast_sink_command_init(void* handle)
{
    return CMD_OK;
}

void auracast_sink_command_uninit(void* handle)
{
}

int auracast_sink_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_auracast_sink_tables,
            ARRAY_SIZE(g_auracast_sink_tables), argc, argv, 0);

    if (ret < 0)
        usage();

    return ret;
}
