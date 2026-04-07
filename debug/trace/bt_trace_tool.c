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
 *   start        start trace capture
 *   stop         stop trace capture
 *   status       show trace status
 *   clock-test   test clock accuracy [delay_ms]
 *   spp          spp trace filter management
 *
 * Note: use system 'trace dump [path]' to export trace data.
 */

#include <stdio.h>
#include "bt_trace_tool.h"

static void usage(void)
{
    printf("Usage: bt_trace <command> [parameters]\n");
    printf("\n");
    printf("Commands:\n");
    printf("  start        start trace capture\n");
    printf("  stop         stop trace capture\n");
    printf("  status       show trace status\n");
    printf("  clock-test   test clock accuracy [delay_ms]\n");
    printf("  spp          spp trace filter management\n");
    printf("\n");
    printf("Note: use system 'trace dump [path]' to export trace data\n");
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
