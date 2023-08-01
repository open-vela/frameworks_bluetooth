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
#include "bt_lea_vmicpc.h"
#include "bt_tools.h"

// vcs client interface
static int vcc_vol_state_get(void *handle, int argc, char **argv);
static int vcc_vol_flags_get(void *handle, int argc, char **argv);
static int vcc_vol_change(void *handle, int argc, char **argv);
static int vcc_vol_unmute_change(void *handle, int argc, char **argv);
static int vcc_abs_vol_set(void *handle, int argc, char **argv);
static int vcc_mute_state_set(void *handle, int argc, char **argv);
// mics client interface
static int micc_mute_state_get(void *handle, int argc, char **argv);
static int micc_mute_state_set(void *handle, int argc, char **argv);

static bt_command_t g_lea_vmicpc_tables[] = {
    {"volget",           vcc_vol_state_get,     0, "get volume state                param: <addr>"                          },
    { "flagsget",        vcc_vol_flags_get,     0, "get volume flags                param: <addr>"                          },
    { "volchange",       vcc_vol_change,        0, "up or down volume               param1: <addr> param2: up(1)/down(0) "  },
    { "volunmutechange", vcc_vol_unmute_change, 0, "up or down volume and unmute    param: <addr> param2: up(1)/down(0) "   },
    { "absvolset",       vcc_abs_vol_set,       0, "set absolute volume             param1: <addr> param2:volume(0~255)"    },
    { "volmuteset",      vcc_mute_state_set,    0, "set volume mute                 param1: <addr> param2:mute(1)/unmute(0)"},
    { "micmuteget",      micc_mute_state_get,   0, "get mic mute state              param: <addr>"                          },
    { "micmuteset",      micc_mute_state_set,   0, "set mic mute state              param1: <addr> param2:mute(1)/unmute(0)"},
};

static struct option lea_vmicpc_options[] = {
    {"help", 0, 0, 'h'},
    { 0,     0, 0, 0  }
};

static void *vmicpc_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_lea_vmicpc_tables); i++) {
        printf("\t%-8s\t%s\n", g_lea_vmicpc_tables[i].cmd, g_lea_vmicpc_tables[i].help);
    }
}

/* interface */
static int vcc_vol_state_get(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_vmicpc_get_volume_state(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int vcc_vol_flags_get(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_vmicpc_get_volume_flags(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int vcc_vol_change(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int dir = atoi(argv[1]);

    if (bt_lea_vmicpc_change_volume(handle, &addr, dir) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int vcc_vol_unmute_change(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int dir = atoi(argv[1]);

    if (bt_lea_vmicpc_change_unmute_volume(handle, &addr, dir) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int vcc_abs_vol_set(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int volume = atoi(argv[1]);

    if (bt_lea_vmicpc_set_volume(handle, &addr, volume) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int vcc_mute_state_set(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int mute = atoi(argv[1]);

    if (bt_lea_vmicpc_set_volume_mute(handle, &addr, mute) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int micc_mute_state_get(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_lea_vmicpc_get_mic_state(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int micc_mute_state_set(void *handle, int argc, char **argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int mute = atoi(argv[1]);

    if (bt_lea_vmicpc_set_mic_mute(handle, &addr, mute) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void vmicpc_volume_state_callback(void *context, bt_address_t *addr, int volume, int mute)
{
    PRINT_ADDR("vmicpc_volume_state_callback, addr:%s", addr);
    PRINT("vmicpc_volume_state_callback volume:%d, mute:%d", volume, mute);
}

static void vmicpc_volume_flags_callback(void *context, bt_address_t *addr, int flags)
{
    PRINT_ADDR("vmicpc_volume_flags_callback, addr:%s", addr);
    PRINT("vmicpc_volume_flags_callback flags:%d", flags);
}

static void vmicpc_mic_state_callback(void *context, bt_address_t *addr, int mute)
{
    PRINT_ADDR("vmicpc_mic_state_callback, addr:%s", addr);
    PRINT("vmicpc_mic_state_callback mic:%d", mute);
}

static const lea_vmicpc_callbacks_t lea_vmicpc_cbs = {
    sizeof(lea_vmicpc_cbs),
    vmicpc_volume_state_callback,
    vmicpc_volume_flags_callback,
    vmicpc_mic_state_callback,
};

int lea_vmicpc_command_init(void *handle)
{
    vmicpc_callbacks = bt_lea_vmicpc_register_callbacks(handle, &lea_vmicpc_cbs);

    return CMD_OK;
}

void lea_vmicpc_command_uninit(void *handle)
{
    bt_lea_vmicpc_unregister_callbacks(handle, vmicpc_callbacks);
}

int vmicpc_command_exec(void *handle, int argc, char *argv[])
{
    int opt, ret = CMD_USAGE_FAULT;

    while ((opt = getopt_long(argc, argv, "h", lea_vmicpc_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return CMD_OK;
        default:
            break;
        }
    }

    if (argc > 0)
        ret = execute_command_in_table(handle, g_lea_vmicpc_tables, ARRAY_SIZE(g_lea_vmicpc_tables), argc, argv);

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[0]);
        usage();
    }

    return ret;
}
