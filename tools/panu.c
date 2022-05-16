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
#define LOG_TAG "PAN_tool"
#include <stdlib.h>
#include <string.h>
#include "btm_manager.h"
#include "bts_service.h"
#include "btm_pan.h"
#include "bts_panu.h"
#include "bt_tools.h"

#include "utils/log.h"

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);

static const pan_interface_t *pan_interface = NULL;
static void *g_pan_handle = NULL;
static bt_command_t g_pan_tables[] = {
    { "connect", connect_cmd,       "\"connect PAN     param: <address> <dstrole> <srcrole> \"" },
    { "disconnect", disconnect_cmd, "\"disconnect PAN  param: <address>\"" },
    { "dump", dump_cmd,             "\"dump PAN current state\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_pan_tables); i++) {
        printf("\t%-8s\t%s\n", g_pan_tables[i].cmd, g_pan_tables[i].help);
    }
}

static void pan_connection_state_cb(pan_connection_state_t state,
                                   bt_address bd_addr,
                                   uint8_t local_role,
                                   uint8_t remote_role)
{
    BT_LOGD("%s, addr:%s, state:%d, local_role:%d, remote_role:%d", __func__,
           addr_str(bd_addr), state, local_role, remote_role);
}

static void pan_netif_state_cb(pan_netif_state_t state,
                               int local_role,
                               const char* ifname)
{
    BT_LOGD("%s ifname:%s, state:%d, local_role:%d", __func__, ifname, state, local_role);
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    uint32_t src_role, dst_role;
    bt_address addr;

    if (argc < 3)
        return -1;

    str2ba(argv[0], addr);
    dst_role = atoi(argv[1]);
    src_role = atoi(argv[2]);
    pan_interface->connect(NULL, addr, dst_role, src_role);

    BT_LOGD("%s, address:%s", __func__, argv[0]);
    return 0;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 1)
        return -1;

    BT_LOGD("%s, address:%s", __func__, argv[0]);
    str2ba(argv[0], addr);
    pan_interface->disconnect(NULL, addr);

    return 0;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    return 0;
}

static pan_callbacks_t pan_test_cbs = {
    sizeof(pan_callbacks_t),
    pan_netif_state_cb,
    pan_connection_state_cb,
};

int pan_command_init(void)
{
    if (pan_interface == NULL) {
        pan_interface = get_pan_interface();
        g_pan_handle = NULL;
        pan_interface->set_callbacks(&g_pan_handle, &pan_test_cbs);
    }

    return 0;
}

void pan_command_uninit(void)
{
    if (pan_interface)
        pan_interface->reset_callbacks(&g_pan_handle);
    pan_interface = NULL;
}

int pan_command(void* handle, int argc, char* argv[])
{
    int ret = -1;

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_pan_tables); i++) {
            if (strcmp(g_pan_tables[i].cmd, argv[1]) == 0) {
                if (g_pan_tables[i].func) {
                    ret = g_pan_tables[i].func(g_pan_handle, argc - 2, &argv[2]);
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