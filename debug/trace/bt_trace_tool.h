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
 * Lightweight CLI framework shim for the standalone bt_trace tool.
 * Provides the same macros/types as bt_tools.h but without pulling in
 * libuv, framework, or bttool dependencies.
 */
#ifndef __BT_TRACE_TOOL_H__
#define __BT_TRACE_TOOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define CMD_OK              (0)
#define CMD_INVALID_PARAM   (-1)
#define CMD_INVALID_OPT     (-4)
#define CMD_INVALID_ADDR    (-5)
#define CMD_PARAM_NOT_ENOUGH (-6)
#define CMD_UNKNOWN         (-7)
#define CMD_USAGE_FAULT     (-8)
#define CMD_ERROR           (-9)

#define PRINT(fmt, args...) printf("[bt_trace] " fmt "\n", ##args)

typedef struct {
    char* cmd;
    int (*func)(void* handle, int argc, char** argv);
    int opt;
    char* help;
} bt_command_t;

static inline int execute_command_in_table(void* handle,
    bt_command_t* table, uint32_t table_size, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_USAGE_FAULT;

    for (uint32_t i = 0; i < table_size; i++) {
        if (strcmp(table[i].cmd, argv[0]) == 0)
            return table[i].func(handle, argc - 1, argv + 1);
    }

    return CMD_USAGE_FAULT;
}

#ifdef CONFIG_BLUETOOTH_EVENT_TRACE
int trace_command_exec(void* handle, int argc, char* argv[]);
#endif

#endif /* __BT_TRACE_TOOL_H__ */
