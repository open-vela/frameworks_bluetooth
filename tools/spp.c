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
#define LOG_TAG "spp_tool"
#include "bt_tools.h"
#include "btm_manager.h"
#include "btm_spp.h"
#include "bts_service.h"
#include "bts_spp.h"
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

typedef struct {
    uint8_t port;
    enum {
        TRANS_NONE = 0,
        TRANS_WRITING,
        TRANS_SENDING,
        TRANS_RECVING,
    } state;
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
static int send_cmd(void* handle, int argc, char* argv[]);
static int speed_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);
extern uint64_t get_os_timestamp_us(void);

static const char* TRANS_START = "START:";
static const char* TRANS_START_ACK = "START_ACK";
static const char* TRANS_EOF = "EOF";

static struct list_node device_list = LIST_INITIAL_VALUE(device_list);
static spp_interface_t* spp_interface = NULL;
static sem_t spp_send_sem;
static transmit_context_t trans_ctx = { 0 };
static void* g_spp_handle = NULL;
static bt_command_t g_spp_tables[] = {
    BT_CMD("start", start_server_cmd, "\"start spp server        param: <port> <uuid>\", note:server port must be odd number, range in (1~28) 1,2,3,...,28"),
    BT_CMD("stop", stop_server_cmd, "\"stop  spp server        param: <port>\""),
    BT_CMD("connect", connect_cmd, "\"connect spp device      param: <address> <port> <uuid>\""),
    BT_CMD("disconnect", disconnect_cmd, "\"disconnect peer device  param: <address> <port>\""),
    BT_CMD("write", write_cmd, "\"write data to peer      param: <port> <data>\""),
    BT_CMD("send", send_cmd, "\"transmit bulk data      param: <port> <length> <iterations>\""),
    BT_CMD("speed", speed_cmd, "\"performance test        param: <port> <iteration>\" note:iteration * 990 shoule less than free memory"),
    BT_CMD("dump", dump_cmd, "\"dump spp current state\""),
};

static void usage(void)
{
#ifndef CONFIG_BLUETOOTH_DISABLE_HELP
    printf("Usage:\n");
    printf("\tport: serial port (1~32)\n"
           "\tuuid: uuid default 0x1101\n"
           "\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_spp_tables); i++) {
        printf("\t%-8s\t%s\n", g_spp_tables[i].cmd, g_spp_tables[i].help);
    }
#endif
}

static spp_device_t* find_pty_by_port(int port)
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

    BT_LOGW("Device not found for port:%d", port);
    return NULL;
}

static int spp_sem_post(sem_t* sem)
{
    return sem_post(sem);
}

static int spp_sem_timedwait(sem_t* sem, uint16_t timeout)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout;

    return sem_timedwait(sem, &ts);
}

static void spp_trans_reset(void)
{
    memset(&trans_ctx, 0, sizeof(trans_ctx));
}

static void show_result(uint64_t start, uint64_t end, uint32_t bytes)
{
    float use = (float)(end - start) / 1000;
    float spd = (float)(bytes / 1024) / use;

    BT_LOGD("transmit done, total: %" PRIu32 " bytes, use: %f seconds, speed: %f KB/s", bytes, use, spd);
}

static void spp_data_received(euv_pty_t* handle, const uint8_t* buf, ssize_t size)
{
    transmit_context_t* ctx = &trans_ctx;

    switch (ctx->state) {
    case TRANS_NONE:
        if (strncmp((const char*)buf, TRANS_START, strlen(TRANS_START)) == 0) {
            spp_trans_reset();
            ctx->state = TRANS_RECVING;
            sscanf((const char*)buf, "START:%" PRIu32 ";", &ctx->trans_total_size);
            BT_LOGD("receive start, waiting for %" PRIu32 " bytes transmit done", ctx->trans_total_size);
            euv_pty_write(handle, (uint8_t*)TRANS_START_ACK, strlen(TRANS_START_ACK), NULL);
            ctx->start_timestamp = get_os_timestamp_us() / 1000;
        } else
            lib_dumpbuffer("spp read", buf, size);
        break;
    case TRANS_SENDING:
        if (strncmp((const char*)buf, TRANS_EOF, strlen(TRANS_EOF)) == 0) {
            ctx->end_timestamp = get_os_timestamp_us() / 1000;
            show_result(ctx->start_timestamp, ctx->end_timestamp, ctx->trans_total_size);
            spp_trans_reset();
        } else if (strncmp((const char*)buf, TRANS_START_ACK, strlen(TRANS_START_ACK)) == 0) {
            spp_sem_post(&spp_send_sem);
        }
        break;
    case TRANS_RECVING:
        ctx->received_size += size;
        if (ctx->received_size >= ctx->trans_total_size) {
            ctx->end_timestamp = get_os_timestamp_us() / 1000;
            show_result(ctx->start_timestamp, ctx->end_timestamp, ctx->trans_total_size);
            euv_pty_write(handle, (uint8_t*)TRANS_EOF, 4, NULL);
            spp_trans_reset();
        }
        break;
    default:
        break;
    }
}

static void pty_read_cb(euv_pty_t* handle, const uint8_t* buf, ssize_t size)
{
    if (size > 0)
        spp_data_received(handle, buf, size);
    else if (size < 0) {
        BT_LOGE("%s read failed, status:%d", __func__, size);
        euv_pty_read_stop(handle);
        euv_pty_close(handle);
    }
}

static void check_resource_release(uint16_t port)
{
    spp_device_t* device;

    device = find_pty_by_port(port);
    if (device == NULL)
        return;

    euv_pty_close(device->pty);
    device->pty = NULL;
    list_delete(&device->node);
    free(device);
}

static int connection_state_callback(const bt_address addr, uint16_t scn, uint16_t port, spp_connection_state_t state)
{
    BT_LOGD("%s scn: %d, port: %d, state:%d", __func__, scn, port, state);

    if (state == SPP_CONNECTION_STATE_DISCONNECTED) {
        check_resource_release(port);
        spp_trans_reset();
    }

    return 0;
}

static void pty_open_callback(const bt_address addr, uint16_t port, char* name)
{
    int fd = open(name, O_RDWR | O_NOCTTY);
    if (fd < 0)
        return;

    BT_LOGD("%s port: %d, name:%s, slave fd:%d", __func__, port, name, fd);
    spp_device_t* device = malloc(sizeof(spp_device_t));
    device->fd = fd;
    device->port = port;
    device->pty = euv_pty_init(get_service_loop(), fd, UV_TTY_MODE_IO);
    if (device->pty == NULL) {
        free(device);
        BT_LOGE("%s pty init error", __func__);
        return;
    }

    list_add_tail(&device_list, &device->node);
    euv_pty_read_start(device->pty, 2048, pty_read_cb);
}

static int start_server_cmd(void* handle, int argc, char* argv[])
{
    uint16_t uuid;

    if (argc < 1)
        return -1;

    uint16_t scn = atoi(argv[0]);
    if (argc == 2)
        uuid = strtol(argv[1], NULL, 16);
    else
        uuid = BT_UUID_SERVCLASS_SERIAL_PORT;

    BT_LOGD("%s, scn:%d, uuid: 0x%04x\n", __func__, scn, uuid);
    bt_result_code ret = spp_interface->server_start(handle, scn, uuid);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("failed, server_start ret: %d", ret);
    }

    return 0;
}

static int stop_server_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return -1;

    uint16_t scn = atoi(argv[0]);
    BT_LOGD("%s, scn:%d", __func__, scn);
    bt_result_code ret = spp_interface->server_stop(handle, scn);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, server_stop ret: %d", ret);
    }

    return 0;
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    int16_t scn;
    uint16_t uuid;
    uint16_t port;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    scn = atoi(argv[1]);

    if (argc == 3)
        uuid = strtol(argv[2], NULL, 16);
    else
        uuid = BT_UUID_SERVCLASS_SERIAL_PORT;

    BT_LOGD("%s, address:%s scn:%d, uuid:0x%04x", __func__, argv[0], scn, uuid);
    bt_result_code ret = spp_interface->client_connect(handle, addr, scn, uuid, &port);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("failed, client_connect ret: %d", ret);
    }
    BT_LOGD("succed, generate connection port: %d", port);

    return 0;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    spp_device_t* device;
    bt_address addr;
    uint16_t port;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    port = atoi(argv[1]);
    device = find_pty_by_port(port);
    if (device == NULL) {
        return -1;
    }

    BT_LOGD("%s, address:%s port:%d", __func__, argv[0], port);
    bt_result_code ret = spp_interface->disconnect(handle, addr, port);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("failed, disconnect ret: %d", ret);
    }

    return 0;
}

static void write_complete(euv_pty_t* handle, uint8_t* buf, int status)
{
    transmit_context_t* ctx = &trans_ctx;

    ctx->bulk_count--;
    if (ctx->bulk_count) {
        euv_pty_write(handle, buf, ctx->bulk_length, write_complete);
    } else {
        free(buf);
        if (ctx->state == TRANS_WRITING)
            spp_trans_reset();
    }
}

static int write_cmd(void* handle, int argc, char* argv[])
{
    spp_device_t* device;
    uint16_t port;
    uint8_t* buf;

    if (argc < 2)
        return -1;

    port = atoi(argv[0]);
    device = find_pty_by_port(port);
    if (device == NULL)
        return -1;

    if (trans_ctx.state != TRANS_NONE) {
        BT_LOGD("spp tool is transmitting");
        return -1;
    }

    trans_ctx.state = TRANS_WRITING;
    trans_ctx.bulk_length = strlen(argv[1]);
    trans_ctx.bulk_count = 1;

    buf = (uint8_t*)strdup(argv[1]);
    euv_pty_write(device->pty, buf, trans_ctx.bulk_length, write_complete);

    return 0;
}

static int send_cmd(void* handle, int argc, char* argv[])
{
    spp_device_t* device;
    uint16_t port;
    uint8_t* buf;

    if (argc < 3)
        return -1;

    port = atoi(argv[0]);
    device = find_pty_by_port(port);
    if (device == NULL)
        return -1;

    if (trans_ctx.state != TRANS_NONE) {
        BT_LOGD("spp tool is transmitting");
        return -1;
    }

    trans_ctx.state = TRANS_WRITING;
    trans_ctx.bulk_length = atoi(argv[1]);
    trans_ctx.bulk_count = atoi(argv[2]);

    buf = malloc(trans_ctx.bulk_length);
    memset(buf, 0xA5, trans_ctx.bulk_length);

    euv_pty_write(device->pty, buf, trans_ctx.bulk_length, write_complete);

    return 0;
}

static int speed_cmd(void* handle, int argc, char* argv[])
{
    static uint8_t start_buf[100];
    spp_device_t* device;
    uint16_t port, times;
    uint8_t* buf;

    if (argc < 2)
        return -1;

    port = atoi(argv[0]);
    times = atoi(argv[1]);

    device = find_pty_by_port(port);
    if (device == NULL)
        return -1;

    if (trans_ctx.state != TRANS_NONE) {
        BT_LOGD("spp tool is transmitting");
        return -1;
    }

    trans_ctx.state = TRANS_SENDING;
    trans_ctx.bulk_length = 990;
    trans_ctx.bulk_count = times;
    trans_ctx.trans_total_size = trans_ctx.bulk_length * trans_ctx.bulk_count;

    memset(start_buf, 0, sizeof(start_buf));
    sprintf((char*)start_buf, "START:%" PRIu32 ";", trans_ctx.trans_total_size);
    BT_LOGD("transmit start, waiting for %" PRIu32 " bytes transmit done", trans_ctx.trans_total_size);
    euv_pty_write(device->pty, start_buf, strlen((const char*)start_buf), NULL);
    /* wait start ack */
    if (spp_sem_timedwait(&spp_send_sem, 2) < 0) {
        spp_trans_reset();
        return -1;
    }

    buf = malloc(trans_ctx.bulk_length);
    memset(buf, 0xA5, trans_ctx.bulk_length);

    trans_ctx.start_timestamp = get_os_timestamp_us() / 1000;
    euv_pty_write(device->pty, buf, trans_ctx.bulk_length, write_complete);

    return 0;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    bts_spp_state_dump();

    return 0;
}

static spp_callbacks_t spp_test_cbs = {
    sizeof(spp_callbacks_t),
    pty_open_callback,
    connection_state_callback,
};

int spp_command_init(void)
{
    if (spp_interface == NULL) {
        sem_init(&spp_send_sem, 0, 0);
        spp_interface = get_spp_interface();
        g_spp_handle = NULL;
        spp_interface->set_callbacks(&g_spp_handle, &spp_test_cbs);
    }

    return 0;
}

void spp_command_uninit(void)
{
    sem_destroy(&spp_send_sem);
    if (spp_interface)
        spp_interface->reset_callbacks(&g_spp_handle);
    spp_interface = NULL;
}

int spp_command(void* handle, int argc, char* argv[])
{
    int ret = -1;

    if (spp_interface == NULL) {
        printf("can't exec spp command before bt enabled\n");
        return 0;
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_spp_tables); i++) {
            if (strcmp(g_spp_tables[i].cmd, argv[1]) == 0) {
                if (g_spp_tables[i].func) {
                    ret = g_spp_tables[i].func(g_spp_handle, argc - 2, &argv[2]);
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
