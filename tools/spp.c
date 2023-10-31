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
#include <debug.h>
#include <nuttx/list.h>
#include <stdlib.h>
#include <string.h>

#include "bt_spp.h"
#include "bt_tools.h"
#include "bt_uuid.h"
#include "euv_pty.h"
#include "uv_thread_loop.h"

typedef struct {
    struct list_node node;
    euv_pty_t *pty;
    int fd;
    int port;
} spp_device_t;

typedef struct {
    void *handle;
    bt_address_t addr;
    uint16_t port;
    uint16_t scn;
    uint8_t *buf;
    uint16_t len;
    uint32_t state;
    char *name;
} spp_cmd_t;

static int start_server_cmd(void *handle, int argc, char *argv[]);
static int stop_server_cmd(void *handle, int argc, char *argv[]);
static int connect_cmd(void *handle, int argc, char *argv[]);
static int disconnect_cmd(void *handle, int argc, char *argv[]);
static int write_cmd(void *handle, int argc, char *argv[]);
static int dump_cmd(void *handle, int argc, char *argv[]);

static struct list_node device_list = LIST_INITIAL_VALUE(device_list);
static sem_t spp_sem;
static void *spp_app_handle = NULL;
static uv_loop_t spp_thread_loop;

static bt_command_t g_spp_tables[] = {
    {"start",       start_server_cmd, 0, "\"start spp server        param: <scn>(range in [1,28]) <uuid>\""},
    { "stop",       stop_server_cmd,  0, "\"stop  spp server        param: <scn>(range in [1,28])\""       },
    { "connect",    connect_cmd,      0, "\"connect spp device      param: <address> <port> <uuid>\""      },
    { "disconnect", disconnect_cmd,   0, "\"disconnect peer device  param: <address> <port>\""             },
    { "write",      write_cmd,        0, "\"write data to peer      param: <port> <data>\""                },
    { "dump",       dump_cmd,         0, "\"dump spp current state\""                                      },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\tport: serial port (1~28)\n"
           "\tuuid: uuid default 0x1101\n"
           "\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_spp_tables); i++) {
        printf("\t%-8s\t%s\n", g_spp_tables[i].cmd, g_spp_tables[i].help);
    }
}

static spp_device_t *find_pty_by_port(int port)
{
    struct list_node *list = &device_list;
    struct list_node *node;
    spp_device_t *device;

    list_for_every(list, node)
    {
        device = (spp_device_t *)node;
        if (device->port == port) {
            return device;
        }
    }

    PRINT("Device not found for port:%d", port);
    return NULL;
}

static int spp_sem_post(sem_t *sem)
{
    return sem_post(sem);
}

static void spp_data_received(euv_pty_t *handle, const uint8_t *buf, ssize_t size)
{
    lib_dumpbuffer("spp read", buf, size);
}

static void pty_read_cb(euv_pty_t *handle, const uint8_t *buf, ssize_t size)
{
    if (size > 0)
        spp_data_received(handle, buf, size);
    else if (size < 0) {
        PRINT("%s read failed, status:%d", __func__, size);
        euv_pty_read_stop(handle);
        euv_pty_close(handle);
    }
}

static void check_resource_release(uint16_t port)
{
    spp_device_t *device;

    device = find_pty_by_port(port);
    if (device == NULL)
        return;

    euv_pty_close(device->pty);
    device->pty = NULL;
    list_delete(&device->node);
    free(device);
}

static void connection_state_process(void *data)
{
    spp_cmd_t *msg = data;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(&msg->addr, addr_str);
    PRINT("%s addr:%s, scn: %d, port: %d, state:%d", __func__, addr_str, msg->scn, msg->port, msg->state);

    if (msg->state == PROFILE_STATE_DISCONNECTED)
        check_resource_release(msg->port);
}

static void connection_state_callback(void *handle, bt_address_t *addr, uint16_t scn,
                                      uint16_t port, profile_connection_state_t state)
{
    spp_cmd_t *msg = malloc(sizeof(spp_cmd_t));
    if (!msg)
        return;

    msg->handle = handle;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));
    msg->scn = scn;
    msg->port = port;
    msg->state = state;

    do_in_thread_loop(&spp_thread_loop, connection_state_process, (void *)msg);
}

static void pty_open_process(void *data)
{
    spp_cmd_t *msg = data;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    int fd = open(msg->name, O_RDWR | O_NOCTTY);
    if (fd < 0)
        return;

    bt_addr_ba2str(&msg->addr, addr_str);

    PRINT("%s addr:%s, scn:%d, port: %d, name:%s, slave fd:%d", __func__, addr_str, msg->scn, msg->port, msg->name, fd);
    spp_device_t *device = malloc(sizeof(spp_device_t));
    device->fd = fd;
    device->port = msg->port;
    device->pty = euv_pty_init(&spp_thread_loop, fd, UV_TTY_MODE_IO);
    if (device->pty == NULL) {
        free(device);
        PRINT("%s pty init error", __func__);
        return;
    }

    list_add_tail(&device_list, &device->node);
    euv_pty_read_start(device->pty, 2048, pty_read_cb);
}

static void pty_open_callback(void *handle, bt_address_t *addr, uint16_t scn, uint16_t port, char *name)
{
    spp_cmd_t *msg = malloc(sizeof(spp_cmd_t));
    if (!msg)
        return;

    msg->handle = handle;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));
    msg->scn = scn;
    msg->port = port;
    msg->name = strdup(name);

    do_in_thread_loop(&spp_thread_loop, pty_open_process, (void *)msg);
}

static int start_server_cmd(void *handle, int argc, char *argv[])
{
    uint16_t uuid;
    bt_uuid_t uuid16;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint16_t scn = atoi(argv[0]);
    if (argc == 2)
        uuid = strtol(argv[1], NULL, 16);
    else
        uuid = BT_UUID_SERVCLASS_SERIAL_PORT;

    bt_uuid16_create(&uuid16, uuid);
    if (bt_spp_server_start(handle, spp_app_handle, scn, &uuid16, 1) != BT_STATUS_SUCCESS) {
        PRINT("server_start failed, scn:%d, uuid: 0x%04x\n", scn, uuid);
        return CMD_ERROR;
    }

    return CMD_OK;
}

static int stop_server_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint16_t scn = atoi(argv[0]);
    bt_spp_server_stop(handle, spp_app_handle, scn);

    return CMD_OK;
}

static int connect_cmd(void *handle, int argc, char *argv[])
{
    int16_t scn;
    uint16_t uuid;
    uint16_t port;
    bt_uuid_t uuid16;
    bt_address_t addr;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    scn = atoi(argv[1]);

    if (argc == 3)
        uuid = strtol(argv[2], NULL, 16);
    else
        uuid = BT_UUID_SERVCLASS_SERIAL_PORT;

    bt_uuid16_create(&uuid16, uuid);
    if (bt_spp_connect(handle, spp_app_handle, &addr, scn, &uuid16, &port) != BT_STATUS_SUCCESS) {
        PRINT("connect scn:%d, failed\n", scn);
        return CMD_ERROR;
    }

    PRINT("%s, address:%s scn:%d, port:%d, uuid:0x%04x", __func__, argv[0], scn, port, uuid);
    return CMD_OK;
}

static void spp_disconnect(void *data)
{
    spp_device_t *device;
    spp_cmd_t *msg = data;
    bt_address_t addr;

    device = find_pty_by_port(msg->port);
    if (device == NULL) {
        free(data);
        return;
    }

    bt_addr_set_empty(&addr);
    bt_spp_disconnect(msg->handle, spp_app_handle, &addr, msg->port);
    PRINT("%s, port:%d disconnecting", __func__, msg->port);
    free(data);
}

static int disconnect_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    spp_cmd_t *msg = malloc(sizeof(spp_cmd_t));
    if (!msg)
        return CMD_ERROR;

    msg->handle = handle;
    msg->port = atoi(argv[1]);

    PRINT("%s, address:%s port:%d", __func__, argv[0], msg->port);
    do_in_thread_loop(&spp_thread_loop, spp_disconnect, msg);

    return CMD_OK;
}

static void write_complete(euv_pty_t *handle, uint8_t *buf, int status)
{
    spp_sem_post(&spp_sem);
    free(buf);
}

static void spp_write(void *data)
{
    spp_device_t *device;
    spp_cmd_t *msg = data;

    device = find_pty_by_port(msg->port);
    if (!device) {
        free(msg->buf);
        free(msg);
        return;
    }

    euv_pty_write(device->pty, msg->buf, msg->len, write_complete);
    free(msg);
}

static int write_cmd(void *handle, int argc, char *argv[])
{
    uint16_t port;
    uint8_t *buf;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    port = atoi(argv[0]);
    buf = (uint8_t *)strdup(argv[1]);

    spp_cmd_t *msg = malloc(sizeof(spp_cmd_t));
    if (!msg) {
        free(buf);
        return CMD_ERROR;
    }

    sem_wait(&spp_sem);
    msg->port = port;
    msg->buf = buf;
    msg->len = strlen(argv[1]);

    do_in_thread_loop(&spp_thread_loop, spp_write, msg);
    return CMD_OK;
}

static int dump_cmd(void *handle, int argc, char *argv[])
{
    return CMD_OK;
}

static spp_callbacks_t spp_cbs = {
    sizeof(spp_callbacks_t),
    pty_open_callback,
    connection_state_callback,
};

int spp_command_init(void *handle)
{
    sem_init(&spp_sem, 0, 10);
    thread_loop_init(&spp_thread_loop);
    thread_loop_run(&spp_thread_loop, true, "spp_client");
    spp_app_handle = bt_spp_register_app(handle, &spp_cbs);

    return 0;
}

void spp_command_uninit(void *handle)
{
    bt_spp_unregister_app(handle, spp_app_handle);
    sem_destroy(&spp_sem);
    thread_loop_exit(&spp_thread_loop);
}

int spp_command_exec(void *handle, int argc, char *argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_spp_tables, ARRAY_SIZE(g_spp_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
