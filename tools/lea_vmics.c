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
#include "bt_adapter.h"
#include "bt_lea_vmics.h"
#include "bt_tools.h"

// vcs server interface
static int vcs_volume_set(void* handle, int argc, char** argv);
static int vcs_mute_set(void* handle, int argc, char** argv);
static int vcs_vol_flags_set(void* handle, int argc, char** argv);

// mics server interface
static int mics_mute_set(void* handle, int argc, char** argv);

static bt_command_t g_lea_vmics_tables[] = {
    // vcs server interface
    { "vcsvolume", vcs_volume_set, 0, "\"leaudio server set volume param: volume(0~255)\"" },
    { "vcsmute", vcs_mute_set, 0, "\"leaudio server set mute state param: mute(0:unmute,1:mute)\"" },
    { "vcsvolflags", vcs_vol_flags_set, 0, "\"leaudio server set volume stater param: flags(0~1)\"" },
    { "micsmute", mics_mute_set, 0, "\"leaudio server set mute state param: mute(0:unmute,1:mute, 2:disable)\"" },
};

static void* vmics_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_vmics_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_vmics_tables[i].cmd, g_lea_vmics_tables[i].help);
    }
}

static int vcs_volume_set(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        return CMD_PARAM_NOT_ENOUGH;
    }

    int vol = atoi(argv[0]);
    if (bt_lea_vcs_volume_set(handle, vol) != BT_STATUS_SUCCESS) {
        return CMD_ERROR;
    }
    return CMD_OK;
}

static int vcs_mute_set(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        return CMD_PARAM_NOT_ENOUGH;
    }

    int mute = atoi(argv[0]);
    if (bt_lea_vcs_mute_set(handle, mute) != BT_STATUS_SUCCESS) {
        return CMD_ERROR;
    }
    return CMD_OK;
}

static int vcs_vol_flags_set(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        return CMD_PARAM_NOT_ENOUGH;
    }

    int flags = atoi(argv[0]);
    if (bt_lea_vcs_volume_flags_set(handle, flags) != BT_STATUS_SUCCESS) {
        return CMD_ERROR;
    }
    return CMD_OK;
}

static int mics_mute_set(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        return CMD_PARAM_NOT_ENOUGH;
    }

    int mute = atoi(argv[0]);
    if (bt_lea_mics_mute_set(handle, mute) != BT_STATUS_SUCCESS) {
        return CMD_ERROR;
    }
    return CMD_OK;
}

static void vmics_test_callback(void* context, int unused)
{
    PRINT("vmics_test_callback unused:%d", unused);
}

static const lea_vmics_callbacks_t lea_vmics_cbs = {
    sizeof(lea_vmics_cbs),
    vmics_test_callback,
};

int lea_vmics_command_init(void* handle)
{
    vmics_callbacks = bt_lea_vmics_register_callbacks(handle, &lea_vmics_cbs);
    return CMD_OK;
}

void lea_vmics_command_uninit(void* handle)
{
    bt_lea_vmics_unregister_callbacks(handle, vmics_callbacks);
}

int vmics_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_vmics_tables, ARRAY_SIZE(g_lea_vmics_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
