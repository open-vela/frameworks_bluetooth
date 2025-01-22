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
#include <stdlib.h>
#include <string.h>

#include "bt_config.h"
#include "bt_list.h"
#include "bt_spp.h"
#include "bt_tools.h"
#include "bt_uuid.h"
#include "euv_pipe.h"
#include "uv_thread_loop.h"

typedef struct {
    struct list_node node;
    euv_pipe_t* pipe;
    int fd;
    int port;
} spp_device_t;

typedef struct {
    void* handle;
    bt_address_t addr;
    uint16_t port;
    uint16_t scn;
    uint8_t* buf;
    uint16_t len;
    uint32_t state;
    char* name;
} spp_cmd_t;

typedef struct {
    void* handle;
    uint8_t port;
    enum {
        TRANS_IDLE = 0,
        TRANS_WRITING,
        TRANS_SENDING,
        TRANS_RECVING,
    } state;
    uint8_t* bulk_buf;
    int32_t bulk_count;
    uint32_t bulk_length;
    uint32_t trans_total_size;
    uint32_t received_size;
    uint64_t start_timestamp;
    uint64_t end_timestamp;
} transmit_context_t;

static int start_server_cmd(void* handle, int argc, char* argv[]);
static int stop_server_cmd(void* handle, int argc, char* argv[]);
static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int write_cmd(void* handle, int argc, char* argv[]);
static int speed_test_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);

static const char* TRANS_START = "START:";
static const char* TRANS_START_ACK = "START_ACK";
static const char* TRANS_EOF = "EOF";

static struct list_node device_list = LIST_INITIAL_VALUE(device_list);
static sem_t spp_send_sem;
static void* spp_app_handle = NULL;
static uv_loop_t spp_thread_loop = { 0 };
static transmit_context_t trans_ctx = { 0 };

static bt_command_t g_spp_tables[] = {
    { "start", start_server_cmd, 0, "\"start spp server        param: <scn>(range in [1,28]) <uuid>\"" },
    { "stop", stop_server_cmd, 0, "\"stop  spp server        param: <scn>(range in [1,28])\"" },
    { "connect", connect_cmd, 0, "\"connect spp device      param: <address> <port> <uuid>\"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect peer device  param: <address> <port>\"" },
    { "write", write_cmd, 0, "\"write data to peer      param: <port> <data>\"" },
    { "speed", speed_test_cmd, 0, "\"performance test        param: <port> <iteration>\" note:iteration * 990 shoule less than free memory" },
    { "dump", dump_cmd, 0, "\"dump spp current state\"" },
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

static spp_device_t* find_device_by_port(int port)
{
    struct list_node* list = &device_list;
    struct list_node* node;
    spp_device_t* device;

    list_for_every(list, node)
    {
        device = (spp_device_t*)node;
        if (device->port == port) {
            return device;
        }
    }

    PRINT("Device not found for port:%d", port);
    return NULL;
}

static spp_device_t* find_device_by_handle(void* handle)
{
    struct list_node* list = &device_list;
    struct list_node* node;
    spp_device_t* device;

    list_for_every(list, node)
    {
        device = (spp_device_t*)node;
        if (device->pipe == handle) {
            return device;
        }
    }

    PRINT("Device not found for handle:%p", handle);
    return NULL;
}

static void bulk_trans_complete(euv_pipe_t* handle, uint8_t* buf, int status)
{
    transmit_context_t* ctx = &trans_ctx;

    ctx->bulk_count--;
    if (ctx->bulk_count)
        euv_pipe_write(handle, buf, ctx->bulk_length, bulk_trans_complete);
    else
        free(buf);
}

static void spp_trans_reset(void)
{
    memset(&trans_ctx, 0, sizeof(trans_ctx));
}

static void show_result(uint64_t start, uint64_t end, uint32_t bytes)
{
    float use = (float)(end - start) / 1000;
    float spd = (float)(bytes / 1024) / use;

    PRINT("transmit done, total: %" PRIu32 " bytes, use: %f seconds, speed: %f KB/s", bytes, use, spd);
}

static void speed_test_start(void* cmd)
{
    spp_cmd_t* msg = cmd;
    spp_device_t* device;
    transmit_context_t* ctx = &trans_ctx;
    static uint8_t start[100];
    uint16_t port = msg->port;
    uint16_t times = msg->len;

    free(msg);

    device = find_device_by_port(port);
    if (!device)
        return;

    if (ctx->state != TRANS_IDLE) {
        PRINT("spp is testing");
        return;
    }
    ctx->handle = device->pipe;
    ctx->state = TRANS_SENDING;
    ctx->bulk_length = 990;
    ctx->bulk_count = times;
    ctx->trans_total_size = ctx->bulk_length * ctx->bulk_count;

    memset(start, 0, sizeof(start));
    sprintf((char*)start, "START:%" PRIu32 ";", ctx->trans_total_size);
    euv_pipe_write(device->pipe, start, strlen((const char*)start), NULL);
    PRINT("transmit start, waiting for %" PRIu32 " bytes transmit done", ctx->trans_total_size);
}

static void spp_data_received(euv_pipe_t* handle, const uint8_t* buf, ssize_t size)
{
    transmit_context_t* ctx = &trans_ctx;

    if (ctx->state != TRANS_IDLE && handle != ctx->handle) {
        PRINT("spp is testing ,ignore it");
        return;
    }

    switch (ctx->state) {
    case TRANS_IDLE:
        if (strncmp((const char*)buf, TRANS_START, strlen(TRANS_START)) == 0) {
            spp_trans_reset();
            ctx->handle = handle;
            ctx->state = TRANS_RECVING;
            sscanf((const char*)buf, "START:%" PRIu32 ";", &ctx->trans_total_size);
            PRINT("receive start, waiting for %" PRIu32 " bytes transmit done", ctx->trans_total_size);
            euv_pipe_write(handle, (uint8_t*)TRANS_START_ACK, strlen(TRANS_START_ACK), NULL);
            ctx->start_timestamp = get_timestamp_msec();
        } else
            lib_dumpbuffer("spp read", buf, size);
        break;
    case TRANS_SENDING:
        if (strncmp((const char*)buf, TRANS_EOF, strlen(TRANS_EOF)) == 0) {
            ctx->end_timestamp = get_timestamp_msec();
            show_result(ctx->start_timestamp, ctx->end_timestamp, ctx->trans_total_size);
            spp_trans_reset();
        } else if (strncmp((const char*)buf, TRANS_START_ACK, strlen(TRANS_START_ACK)) == 0) {
            sem_post(&spp_send_sem);
            ctx->bulk_buf = malloc(ctx->bulk_length);
            memset(ctx->bulk_buf, 0xA5, ctx->bulk_length);
            ctx->start_timestamp = get_timestamp_msec();
            euv_pipe_write(handle, ctx->bulk_buf, ctx->bulk_length, bulk_trans_complete);
        }
        break;
    case TRANS_RECVING:
        ctx->received_size += size;
        if (ctx->received_size >= ctx->trans_total_size) {
            ctx->end_timestamp = get_timestamp_msec();
            show_result(ctx->start_timestamp, ctx->end_timestamp, ctx->trans_total_size);
            euv_pipe_write(handle, (uint8_t*)TRANS_EOF, 4, NULL);
            spp_trans_reset();
        }
        break;
    default:
        break;
    }
}

static void spp_read_cb(euv_pipe_t* handle, const uint8_t* buf, ssize_t size)
{
    if (size > 0)
        spp_data_received(handle, buf, size);
    else if (size < 0) {
        PRINT("%s read failed, status:%d", __func__, (int)size);
        euv_pipe_read_stop(handle);
        spp_device_t* device = find_device_by_handle(handle);
        if (device == NULL)
            return;

        euv_pipe_disconnect(device->pipe);
        device->pipe = NULL;
        list_delete(&device->node);
        free(device);
    }
}

static void check_resource_release(uint16_t port)
{
    spp_device_t* device;

    device = find_device_by_port(port);
    if (device == NULL)
        return;

    euv_pipe_disconnect(device->pipe);
    device->pipe = NULL;
    list_delete(&device->node);
    free(device);
}

static void connection_state_process(void* data)
{
    spp_cmd_t* msg = data;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(&msg->addr, addr_str);
    PRINT("%s addr:%s, scn: %" PRIx16 ", port: %" PRIx16 ", state:%" PRIu32, __func__, addr_str, msg->scn, msg->port, msg->state);

    if (msg->state == PROFILE_STATE_DISCONNECTED) {
        check_resource_release(msg->port);
        /**
         * Reset the SPP transation ctx when SPP disconnct.
         */
        spp_trans_reset();
    }

    free(msg);
}

static void connection_state_callback(void* handle, bt_address_t* addr, uint16_t scn,
    uint16_t port, profile_connection_state_t state)
{
    spp_cmd_t* msg = malloc(sizeof(spp_cmd_t));
    if (!msg)
        return;

    msg->handle = handle;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));
    msg->scn = scn;
    msg->port = port;
    msg->state = state;

    do_in_thread_loop(&spp_thread_loop, connection_state_process, (void*)msg);
}

static void proxy_connect_callback(euv_pipe_t* handle, int status, void* user_data)
{
    spp_device_t* device;

    PRINT("%s", __func__);

    device = user_data;
    list_add_tail(&device_list, &device->node);

    euv_pipe_read_start(handle, 2048, spp_read_cb, NULL);
}

static void spp_open_process(void* data)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    spp_cmd_t* msg = data;
    spp_device_t* device;

    bt_addr_ba2str(&msg->addr, addr_str);
    PRINT("%s addr:%s, scn:%d, port: %d, name: %s", __func__, addr_str, msg->scn, msg->port, msg->name);

    device = zalloc(sizeof(spp_device_t));
    if (!device) {
        PRINT("%s, device not exist", __func__);
        return;
    }

    device->port = msg->port;

#ifdef CONFIG_BLUETOOTH_SPP_RPMSG_NET
    device->pipe = euv_rpmsg_pipe_connect(&spp_thread_loop, msg->name, CONFIG_BLUETOOTH_RPMSG_CPUNAME, proxy_connect_callback, device);
#else
    device->pipe = euv_pipe_connect(&spp_thread_loop, msg->name, proxy_connect_callback, device);
#endif
    if (!device->pipe) {
        PRINT("%s, pipe connect failed", __func__);
        free(msg);
        free(device);
        return;
    }

    free(msg);
}

static void proxy_state_callback(void* handle, bt_address_t* addr, spp_proxy_state_t state, uint16_t scn, uint16_t port, char* name)
{
    spp_cmd_t* msg = malloc(sizeof(spp_cmd_t));
    if (!msg)
        return;

    msg->handle = handle;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));
    msg->scn = scn;
    msg->port = port;
    msg->name = strdup(name);

    if (state == SPP_PROXY_STATE_CONNECTED) {
        do_in_thread_loop(&spp_thread_loop, spp_open_process, (void*)msg);
    } else if (state == SPP_PROXY_STATE_DISCONNECTED) {
        PRINT("%s, spp proxy disconnected", __func__);
    }
}

static int start_server_cmd(void* handle, int argc, char* argv[])
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
    if (bt_spp_server_start(handle, spp_app_handle, scn, &uuid16,
            CONFIG_BLUETOOTH_SPP_SERVER_MAX_CONNECTIONS)
        != BT_STATUS_SUCCESS) {
        PRINT("server_start failed, scn:%d, uuid: 0x%04x\n", scn, uuid);
        return CMD_ERROR;
    }

    return CMD_OK;
}

static int stop_server_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint16_t scn = atoi(argv[0]);
    bt_spp_server_stop(handle, spp_app_handle, scn);

    return CMD_OK;
}

static int connect_cmd(void* handle, int argc, char* argv[])
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

static void spp_disconnect(void* data)
{
    spp_device_t* device;
    spp_cmd_t* msg = data;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    device = find_device_by_port(msg->port);
    if (device == NULL) {
        free(data);
        return;
    }

    bt_spp_disconnect(msg->handle, spp_app_handle, &msg->addr, msg->port);
    bt_addr_ba2str(&msg->addr, addr_str);
    PRINT("%s, address:%s port:%d disconnecting", __func__, addr_str, msg->port);
    free(data);
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    spp_cmd_t* msg = malloc(sizeof(spp_cmd_t));
    if (!msg)
        return CMD_ERROR;

    msg->handle = handle;
    msg->port = atoi(argv[1]);
    bt_addr_str2ba(argv[0], &msg->addr);

    PRINT("%s, address:%s port:%d", __func__, argv[0], msg->port);
    do_in_thread_loop(&spp_thread_loop, spp_disconnect, msg);

    return CMD_OK;
}

static void write_complete(euv_pipe_t* handle, uint8_t* buf, int status)
{
    free(buf);
}

static void spp_write(void* data)
{
    spp_device_t* device;
    spp_cmd_t* msg = data;

    device = find_device_by_port(msg->port);
    if (!device)
        goto error;

    if (trans_ctx.handle == device->pipe) {
        PRINT("spp is testing");
        goto error;
    }

    euv_pipe_write(device->pipe, msg->buf, msg->len, write_complete);
    free(msg);
    return;

error:
    free(msg->buf);
    free(msg);
}

static int write_cmd(void* handle, int argc, char* argv[])
{
    uint16_t port;
    uint8_t* buf;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    port = atoi(argv[0]);
    buf = (uint8_t*)strdup(argv[1]);

    spp_cmd_t* msg = malloc(sizeof(spp_cmd_t));
    if (!msg) {
        free(buf);
        return CMD_ERROR;
    }

    msg->port = port;
    msg->buf = buf;
    msg->len = strlen(argv[1]);

    do_in_thread_loop(&spp_thread_loop, spp_write, msg);
    return CMD_OK;
}

static int speed_test_cmd(void* handle, int argc, char* argv[])
{
    int port, times;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    port = atoi(argv[0]);
    times = atoi(argv[1]);
    if (port < 0 || times < 0)
        return CMD_INVALID_PARAM;

    spp_cmd_t* msg = malloc(sizeof(spp_cmd_t));
    if (!msg)
        return CMD_ERROR;

    msg->port = port;
    msg->len = times;
    do_in_thread_loop(&spp_thread_loop, speed_test_start, msg);
    /* wait start ack */
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;
    if (sem_timedwait(&spp_send_sem, &ts) < 0) {
        spp_trans_reset();
        return CMD_ERROR;
    }

    return CMD_OK;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static spp_callbacks_t spp_cbs = {
    .size = sizeof(spp_callbacks_t),
    .proxy_state_cb = proxy_state_callback,
    .connection_state_cb = connection_state_callback,
};

int spp_command_init(void* handle)
{
    sem_init(&spp_send_sem, 0, 0);
    thread_loop_init(&spp_thread_loop);
    thread_loop_run(&spp_thread_loop, true, "spp_client");
    spp_app_handle = bt_spp_register_app_with_name(handle, "btool", &spp_cbs);

    return 0;
}

void spp_command_uninit(void* handle)
{
    bt_spp_unregister_app(handle, spp_app_handle);
    sem_destroy(&spp_send_sem);
    thread_loop_exit(&spp_thread_loop);
    memset(&spp_thread_loop, 0, sizeof(spp_thread_loop));
}

int spp_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_spp_tables, ARRAY_SIZE(g_spp_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
