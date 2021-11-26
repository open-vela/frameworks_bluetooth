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
#define LOG_TAG "gap_test"
#include "bt_tools.h"
#include "btm_gap.h"
#include "btm_manager.h"
#include "euv_pty.h"
#include "utils/log.h"
#include <debug.h>
#include <nuttx/list.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    struct list_node node;

} gap_device_t;

static int start_server_cmd(void* handle, int argc, char* argv[]);
static int stop_server_cmd(void* handle, int argc, char* argv[]);
static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int write_cmd(void* handle, int argc, char* argv[]);

static struct list_node device_list = LIST_INITIAL_VALUE(device_list);
static btm_gap_interface_t* gap_interface = NULL;
void* manager_handle = NULL;
void* gap_hanlde = NULL;
static bt_command_t g_gap_tables[] = {

    { "disconnect", disconnect_cmd, "\"disconnect peer device  param: <port>\"" },
    { "write", write_cmd, "\"write data to peer      param: <port> <data>\"" },
};

static struct option gap_options[] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\tport: serial port (1~32)\n"
           "\tuuid: uuid default 0x1101\n"
           "\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_gap_tables); i++) {
        printf("\t%-8s\t%s\n", g_gap_tables[i].cmd, g_gap_tables[i].help);
    }
}

static int reply_pair_request_cmd(void* handle, int argc, char* argv[])
{
    uint16_t port;
    uint16_t uuid;
    bt_address addr;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    port = atoi(argv[1]);

    BT_LOGD("%s, address:%s port:%d, uuid:0x%04x", __func__, argv[0], port, uuid);
    gap_interface->

        return 0;
}

void test_discovery_state_changed_callback(void* gap_handle, bt_discovery_state state)
{
    BT_LOGD("%s", __func__);
}
void test_adapter_state_changed_callback(void* gap_handle, stack_state_t state)
{
    BT_LOGD("%s", __func__);
}
void test_device_found_callback(void* gap_handle, bt_device_t* device)
{
    BT_LOGD("%s, device %02x%02x%02x%02x%02x%02x", __func__, device->addr[0], device->addr[1], device->addr[2], device->addr[3], device->addr[4], device->addr[5]);
}

const btm_gap_callbacks_t gap_callbacks = {
    .discovery_state_changed_callback_cb = test_discovery_state_changed_callback,
    .state_changed_cb = test_adapter_state_changed_callback,
    .device_found_callback_cb = test_device_found_callback,
};

int gap_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (gap_interface == NULL) {
        gap_interface = get_gap_instance();
        // /    gap_test_interface->gap_register_callbacks(manager_handle, &gap_hanlde, &gap_callbacks);
    }

    while ((opt = getopt_long(argc, argv, "h", gap_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_gap_tables); i++) {
            if (strncmp(g_gap_tables[i].cmd, argv[1], strlen(g_gap_tables[i].cmd)) == 0) {
                if (g_gap_tables[i].func) {
                    ret = g_gap_tables[i].func(handle, argc - 2, &argv[2]);
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