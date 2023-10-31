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
#include "bt_hfp_ag.h"
#include "bt_tools.h"

static int connect_cmd(void *handle, int argc, char *argv[]);
static int disconnect_cmd(void *handle, int argc, char *argv[]);
static int connect_audio_cmd(void *handle, int argc, char *argv[]);
static int disconnect_audio_cmd(void *handle, int argc, char *argv[]);
static int start_voice_recognition_cmd(void *handle, int argc, char *argv[]);
static int stop_voice_recognition_cmd(void *handle, int argc, char *argv[]);

static bt_command_t g_hfp_ag_tables[] = {
    {"connect",          connect_cmd,                 0, "\"establish hfp SLC connection , params: <address>\""},
    { "disconnect",      disconnect_cmd,              0, "\"disconnect hfp SLC connection, params: <address>\""},
    { "connectaudio",    connect_audio_cmd,           0, "\"establish hfp SCO connection , params: <address>\""},
    { "disconnectaudio", disconnect_audio_cmd,        0, "\"disconnect hfp SCO connection, params: <address>\""},
    { "startvr",         start_voice_recognition_cmd, 0, "\"start voice recognition      , params: <address>\""},
    { "stopvr",          stop_voice_recognition_cmd,  0, "\"stop voice recognition       , params: <address>\""},
};

static void *ag_callbacks = NULL;

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_hfp_ag_tables); i++) {
        printf("\t%-8s\t%s\n", g_hfp_ag_tables[i].cmd, g_hfp_ag_tables[i].help);
    }
}

static int connect_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_ag_connect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_cmd(void *handle, int argc, char *argv[])
{
    bt_address_t addr;
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_ag_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int connect_audio_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_ag_connect_audio(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_audio_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_ag_disconnect_audio(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int start_voice_recognition_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_ag_start_voice_recognition(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int stop_voice_recognition_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_ag_stop_voice_recognition(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void ag_connection_state_callback(void *context, bt_address_t *addr, profile_connection_state_t state)
{
    PRINT_ADDR("ag_connection_state_callback, addr:%s, state:%d", addr, state);
}

static void ag_audio_state_callback(void *context, bt_address_t *addr, hfp_audio_state_t state)
{
    PRINT_ADDR("ag_audio_state_callback, addr:%s, state:%d", addr, state);
}

static void ag_vr_cmd_callback(void *context, bt_address_t *addr, bool started)
{
    PRINT_ADDR("ag_vr_cmd_callback, addr:%s, started:%d", addr, started);
}

static void ag_battery_update_callback(void *context, bt_address_t *addr, uint8_t value)
{
    PRINT_ADDR("ag_battery_update_callback, addr:%s, battery:%d", addr, value);
}

static const hfp_ag_callbacks_t hfp_ag_cbs = {
    sizeof(hfp_ag_cbs),
    ag_connection_state_callback,
    ag_audio_state_callback,
    ag_vr_cmd_callback,
    ag_battery_update_callback,
};

int hfp_ag_commond_init(void *handle)
{
    ag_callbacks = bt_hfp_ag_register_callbacks(handle, &hfp_ag_cbs);

    return 0;
}

int hfp_ag_commond_uninit(void *handle)
{
    bt_hfp_ag_unregister_callbacks(handle, ag_callbacks);

    return 0;
}

int hfp_ag_command_exec(void *handle, int argc, char *argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_hfp_ag_tables, ARRAY_SIZE(g_hfp_ag_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
