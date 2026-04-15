/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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

/*
 * Standalone bt_trace CLI tool entry point.
 *
 * Usage:
 *   bt_trace <command> [parameters]
 *
 * Commands:
 *   clock-test   test clock accuracy [delay_ms]
 *   spp          spp trace filter management
 *
 * Capture control is handled by system trace commands:
 *   trace start/stop/dump
 */

#include "bt_trace_tool.h"
#include <stdio.h>

static void usage(void)
{
    printf("Usage: bt_trace <command> [parameters]\n");
    printf("\n");
    printf("Commands:\n");
    printf("  clock-test   test clock accuracy [delay_ms]\n");
    printf("  spp          spp trace filter management\n");
    printf("\n");
    printf("Capture control: use system 'trace start/stop' commands\n");
    printf("Data export:     use system 'trace dump [path]'\n");
}

int main(int argc, char* argv[])
{
    int ret;

    if (argc < 2) {
        usage();
        return 0;
    }

    ret = trace_command_exec(NULL, argc - 1, argv + 1);
    if (ret == CMD_USAGE_FAULT)
        usage();

    return (ret == CMD_OK) ? 0 : 1;
}
