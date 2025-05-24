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

#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "bt_tools.h"

// #include "common.h"
// #include "distance_estimation.h"

extern int vela_le_cs_enable(void);

static int start_cs_reflector_cmd(void* handle, int argc, char* argv[]);
// static int stop_cs_reflector_cmd(void* handle, int argc, char* argv[]);

 static bt_command_t g_cs_reflector_tables[] = {
    { "start", start_cs_reflector_cmd, 1, "start channel sounding reflector\n"},
    // { "stop", stop_cs_reflector_cmd, 1, "stop  channel sounding reflector  \n"},
};

static int start_cs_reflector_cmd(void* handle, int argc, char* argv[])
{
    int ret = vela_le_cs_enable();
    PRINT("cs start is %s, ret = %d\n", ret == 0 ? "success" : "fail", ret);
    return 0;
}

int cs_reflector_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_cs_reflector_tables, ARRAY_SIZE(g_cs_reflector_tables), argc, argv, 0);

    return ret;
}