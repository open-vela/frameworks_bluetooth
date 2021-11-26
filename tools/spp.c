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
#define LOG_TAG "spp_test"
#include "bt_tools.h"
#include "btm_manager.h"
#include "btm_spp.h"
#include "bts_service.h"
#include "euv_pty.h"
#include "utils/log.h"
#include <debug.h>
#include <nuttx/list.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    struct list_node node;
    euv_pty_t* pty;
    int fd;
    int port;
} spp_device_t;

static int start_server_cmd(void* handle, int argc, char* argv[]);
static int stop_server_cmd(void* handle, int argc, char* argv[]);
static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int write_cmd(void* handle, int argc, char* argv[]);

static struct list_node device_list = LIST_INITIAL_VALUE(device_list);
static spp_interface_t* spp_interface = NULL;
static bt_command_t g_spp_tables[] = {
    { "start", start_server_cmd, "\"start spp server        param: <port> <uuid>\"" },
    { "stop", stop_server_cmd, "\"stop  spp server        param: <port>\"" },
    { "connect", connect_cmd, "\"connect spp device      param: <address> <port> <uuid>\"" },
    { "disconnect", disconnect_cmd, "\"disconnect peer device  param: <port>\"" },
    { "write", write_cmd, "\"write data to peer      param: <port> <data>\"" },
};

static struct option spp_options[] = {
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
    for (int i = 0; i < ARRAY_SIZE(g_spp_tables); i++) {
        printf("\t%-8s\t%s\n", g_spp_tables[i].cmd, g_spp_tables[i].help);
    }
}

static spp_device_t* find_pty_by_port(int port)
{
    struct list_node* list = &device_list;
    spp_device_t* device;
    struct list_node* node;

    list_for_every(list, node)
    {
        device = (spp_device_t*)node;
        if (device->port == port) {
            return device;
        }
    }

    BT_LOGW("Device not found for port:%d", port);
    return NULL;
}

static void pty_read_cb(euv_pty_t* handle,
    const uint8_t* buf, ssize_t size)
{
    if (size > 0)
        lib_dumpbuffer("spp read", buf, size);
    else
        BT_LOGE("%s read failed, status:%d", __func__, size);
}

static void connection_state_callback(const bt_address addr, uint16_t port, spp_connection_state_t state)
{
    BT_LOGD("%s port: %d, state:%d", __func__, port, state);
}

static void pty_open_callback(const bt_address addr, uint16_t port, char* name, int fd)
{
    BT_LOGD("%s port: %d, name:%s, fd:%d", __func__, port, name, fd);
    spp_device_t* device = malloc(sizeof(spp_device_t));
    device->fd = fd;
    device->port = port;
    device->pty = euv_pty_init(get_service_loop(), fd, UV_TTY_MODE_IO);
    if (device->pty == NULL) {
        BT_LOGE("%s pty init error", __func__);
    }
    list_add_tail(&device_list, &device->node);
    euv_pty_read_start(device->pty, pty_read_cb);
}

static int start_server_cmd(void* handle, int argc, char* argv[])
{
    uint16_t uuid;

    if (argc < 1)
        return -1;

    uint16_t port = atoi(argv[0]);
    if (argc == 2) {
        printf("argv[1]:%s", argv[1]);
        uuid = strtol(argv[1], NULL, 16);
    } else
        uuid = BT_UUID_SERVCLASS_SERIAL_PORT;

    BT_LOGD("%s, port:%d, uuid:0x%04x", __func__, port, uuid);
    spp_interface->server_start(NULL, 5, uuid);

    return 0;
}

static int stop_server_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return -1;

    uint16_t port = atoi(argv[0]);
    BT_LOGD("%s, port:%d", __func__, port);
    spp_interface->server_stop(NULL, port);

    return 0;
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    uint16_t port;
    uint16_t uuid;
    bt_address addr;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    port = atoi(argv[1]);

    if (argc == 3) {
        printf("argv[3]:%s", argv[2]);
        uuid = strtol(argv[2], NULL, 16);
    } else
        uuid = BT_UUID_SERVCLASS_SERIAL_PORT;
    BT_LOGD("%s, address:%s port:%d, uuid:0x%04x", __func__, argv[0], port, uuid);
    spp_interface->client_connect(NULL, addr, port, uuid);

    return 0;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    uint16_t port;
    spp_device_t* device;
    bt_address addr;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    port = atoi(argv[1]);
    BT_LOGD("%s, address:%s port:%d", __func__, argv[0], port);
    device = find_pty_by_port(port);
    if (device == NULL)
        return -1;
    euv_pty_read_stop(device->pty);
    euv_pty_close(device->pty);
    list_delete(&device->node);
    spp_interface->disconnect(NULL, addr, port);

    return 0;
}

static int write_cmd(void* handle, int argc, char* argv[])
{
    spp_device_t* device;
    uint16_t port;
    if (argc < 2)
        return -1;

    port = atoi(argv[0]);
    device = find_pty_by_port(port);
    if (device == NULL)
        return -1;
    euv_pty_write(device->pty, (uint8_t*)argv[1], strlen(argv[1]), NULL);

    return 0;
}

static spp_callbacks_t spp_test_cbs = {
    sizeof(spp_callbacks_t),
    pty_open_callback,
    connection_state_callback,
};

int spp_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (spp_interface == NULL) {
        spp_interface = get_spp_interface();
        spp_interface->set_callbacks(NULL, &spp_test_cbs);
    }

    while ((opt = getopt_long(argc, argv, "h", spp_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_spp_tables); i++) {
            if (strncmp(g_spp_tables[i].cmd, argv[1], strlen(g_spp_tables[i].cmd)) == 0) {
                if (g_spp_tables[i].func) {
                    ret = g_spp_tables[i].func(handle, argc - 2, &argv[2]);
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