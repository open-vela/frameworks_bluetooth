/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#define LOG_TAG "a2dp_snk_tool"

#include <stdlib.h>
#include <string.h>
#include <debug.h>

#include "bt_tools.h"
#include "btm_manager.h"
#include "bts_service.h"
#include "btm_a2dp_sink.h"
#include "bts_a2dp_sink.h"
#include "utils/log.h"

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);

static const a2dp_sink_interface_t* a2dp_sink_interface = NULL;
static bt_command_t g_a2dp_sink_tables[] = {
    { "connect", connect_cmd, "\"connect a2dp source device      param: <address> \"" },
    { "disconnect", disconnect_cmd, "\"disconnect peer a2dp source device  param: <address>\"" },
    { "dump", dump_cmd, "\"dump a2dp device state\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_a2dp_sink_tables); i++) {
        printf("\t%-8s\t%s\n", g_a2dp_sink_tables[i].cmd, g_a2dp_sink_tables[i].help);
    }
}

static void connection_state_callback(bt_address addr, a2dp_connection_state_t state)
{
    BT_LOGD("%s addr: %s, state:%d", __func__, addr_str(addr), state);
}

static void audio_state_callback(bt_address addr, a2dp_audio_state_t state)
{
    BT_LOGD("%s addr: %s, state:%d", __func__, addr_str(addr), state);
}

static void audio_sink_config_callback(bt_address addr)
{
    BT_LOGD("%s addr: %s", __func__, addr_str(addr));
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 1)
        return -1;

    str2ba(argv[0], addr);
    a2dp_sink_interface->connect(NULL, addr);

    return 0;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 1)
        return -1;

    str2ba(argv[0], addr);
    a2dp_sink_interface->disconnect(NULL, addr);

    return 0;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    return 0;
}

static a2dp_sink_callbacks_t a2dp_sink_cbs = {
    sizeof(a2dp_sink_callbacks_t),
    connection_state_callback,
    audio_state_callback,
    audio_sink_config_callback
};

int a2dp_sink_command(void* handle, int argc, char* argv[])
{
    int ret = -1;

    if (a2dp_sink_interface == NULL) {
        a2dp_sink_interface = get_a2dp_sink_interface();
        a2dp_sink_interface->set_callbacks(NULL, &a2dp_sink_cbs);
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_a2dp_sink_tables); i++) {
            if (strcmp(g_a2dp_sink_tables[i].cmd, argv[1]) == 0) {
                if (g_a2dp_sink_tables[i].func) {
                    ret = g_a2dp_sink_tables[i].func(handle, argc - 2, &argv[2]);
                }
            }
        }
    }

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[1]);
        usage();
    }

    return 0;
}