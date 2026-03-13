/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#include "bt_l2cap.h"
#include "bt_list.h"
#include "bt_tools.h"
#include "euv_pipe.h"
#include "uv_thread_loop.h"

#define L2CAP_TRANS_MTU_CFG 1000
#define L2CAP_TRANS_MPS_CFG 251
#define L2CAP_TRANS_CREDIT_CFG 30

#define L2CAP_LE_MTU_MIN 23
#define L2CAP_LE_MPS_MIN 23
#define L2CAP_LE_MPS_MAX 65533
#define L2CAP_LE_CREDIT_MIN 1

typedef struct {
    struct list_node node;
    euv_pipe_t* pipe;
    uint16_t psm;
    uint16_t cid;
    uint16_t id;
    bool is_listening;
} l2cap_chnl_t;

typedef struct {
    bt_address_t addr;
    uint16_t id;
    uint16_t psm;
    uint16_t cid;
    uint16_t listen_id; // for server listen
    bool is_listening;
    char* proxy_name;
    uint8_t* buf;
    uint16_t len;
} l2cap_msg_t;

typedef struct {
    void* handle;
    enum {
        TRANS_IDLE = 0,
        TRANS_WRITING,
        TRANS_SENDING,
        TRANS_RECVING,
        TRANS_ECHO,
    } state;
    uint8_t* bulk_buf;
    int32_t bulk_count;
    uint32_t bulk_length;
    uint32_t trans_total_size;
    uint32_t received_size;
    uint64_t start_timestamp;
    uint64_t end_timestamp;
} l2cap_trans_ctx_t;

static const char* TRANS_START = "START:";
static const char* TRANS_START_ACK = "START_ACK";
static const char* TRANS_EOF = "EOF";

static uv_loop_t g_l2cap_thread;
static void* g_l2cap_handle;
static struct list_node channel_list = LIST_INITIAL_VALUE(channel_list);
static l2cap_trans_ctx_t g_trans_ctx = { 0 };
static sem_t speed_tx_sem;

static void cleanup_l2cap_channel(void* data)
{
    struct list_node* node;
    struct list_node* tmp;
    struct list_node* list = &channel_list;

    list_for_every_safe(list, node, tmp)
    {
        list_delete(node);
        if (((l2cap_chnl_t*)node)->pipe) {
            euv_pipe_disconnect(((l2cap_chnl_t*)node)->pipe);
        }

        free(node);
    }
}

static l2cap_chnl_t* find_channel_by_id(uint16_t id)
{
    struct list_node* node;
    struct list_node* list = &channel_list;
    l2cap_chnl_t* channel;

    list_for_every(list, node)
    {
        channel = (l2cap_chnl_t*)node;
        if (channel->id == id) {
            return channel;
        }
    }

    return NULL;
}

static void l2cap_trans_reset(void)
{
    memset(&g_trans_ctx, 0, sizeof(g_trans_ctx));
}

static void write_complete_cb(euv_pipe_t* handle, uint8_t* buf, int status)
{
    free(buf);
}

static void bulk_trans_complete(euv_pipe_t* handle, uint8_t* buf, int status)
{
    l2cap_trans_ctx_t* ctx = &g_trans_ctx;

    ctx->bulk_count--;
    if (ctx->bulk_count)
        euv_pipe_write(handle, buf, ctx->bulk_length, bulk_trans_complete);
    else
        free(buf);
}

static bool bulk_buf_gen(uint8_t** buf, uint32_t length)
{
    uint32_t data_len;
    struct bulk_buf_t {
        char delimiter[4];
        uint32_t length;
        uint8_t filled_data[0];
    } * bulk_buf;

    if (length < sizeof(struct bulk_buf_t)) {
        PRINT("bulk length is too short");
        return false;
    }

    bulk_buf = malloc(length);
    if (!bulk_buf) {
        PRINT("allocate bulk buffer failed");
        return false;
    }

    memcpy(bulk_buf->delimiter, "Vela", 4);
    bulk_buf->length = length;
    data_len = length - sizeof(struct bulk_buf_t);
    for (uint32_t i = 0; i < data_len; i++) {
        bulk_buf->filled_data[i] = (uint8_t)(i % 256);
    }

    *buf = (uint8_t*)bulk_buf;
    return true;
}

static void show_result(uint64_t start, uint64_t end, uint32_t bytes)
{
    float use = (float)(end - start) / 1000;
    float spd = (float)(bytes / 1024) / use;

    PRINT("transmit done, total: %" PRIu32 " bytes, use: %f seconds, speed: %f KB/s", bytes, use, spd);
}

static void handle_l2cap_data_recv(euv_pipe_t* handle, const uint8_t* buf, ssize_t size)
{
    l2cap_trans_ctx_t* ctx = &g_trans_ctx;
    if (ctx->state != TRANS_IDLE && handle != ctx->handle) {
        PRINT("l2cap is testing ,ignore it");
        return;
    }

    switch (ctx->state) {
    case TRANS_IDLE:
        if (strncmp((const char*)buf, TRANS_START, strlen(TRANS_START)) == 0) {
            l2cap_trans_reset();
            ctx->handle = handle;
            ctx->state = TRANS_RECVING;
            sscanf((const char*)buf, "START:%" PRIu32 ";", &ctx->trans_total_size);
            PRINT("receive start, waiting for %" PRIu32 " bytes transmit done", ctx->trans_total_size);
            euv_pipe_write(handle, (uint8_t*)TRANS_START_ACK, strlen(TRANS_START_ACK), NULL);
            ctx->start_timestamp = get_timestamp_msec();
        } else
            lib_dumpbuffer("read data:", buf, size); // no need to free
        break;
    case TRANS_SENDING:
        if (strncmp((const char*)buf, TRANS_EOF, strlen(TRANS_EOF)) == 0) {
            ctx->end_timestamp = get_timestamp_msec();
            show_result(ctx->start_timestamp, ctx->end_timestamp, ctx->trans_total_size);
            l2cap_trans_reset();
        } else if (strncmp((const char*)buf, TRANS_START_ACK, strlen(TRANS_START_ACK)) == 0) {
            sem_post(&speed_tx_sem);
            if (!bulk_buf_gen(&ctx->bulk_buf, ctx->bulk_length)) {
                l2cap_trans_reset();
                PRINT("generate bulk buffer failed");
                // TBD: send error to peer to end test
                return;
            }

            ctx->start_timestamp = get_timestamp_msec();
            euv_pipe_write(handle, ctx->bulk_buf, ctx->bulk_length, bulk_trans_complete);
        }
        break;
    case TRANS_RECVING:
        ctx->received_size += size;
        if (ctx->received_size >= ctx->trans_total_size) {
            ctx->end_timestamp = get_timestamp_msec();
            show_result(ctx->start_timestamp, ctx->end_timestamp, ctx->trans_total_size);
            euv_pipe_write(handle, (uint8_t*)TRANS_EOF, strlen(TRANS_EOF), NULL);
            l2cap_trans_reset();
        }
        break;
    default:
        break;
    }
}

static void read_complete_cb(euv_pipe_t* pipe, const uint8_t* buf, ssize_t nread)
{
    if (nread > 0) {
        handle_l2cap_data_recv(pipe, buf, nread);

    } else if (nread < 0) {
        PRINT("read failed:%s, wait for connection disconnect", uv_strerror(nread));
    }
}

static void data_path_connected_cb(euv_pipe_t* pipe, int status, void* data)
{
    l2cap_chnl_t* channel = (l2cap_chnl_t*)data;

    PRINT("l2cap channel(id:%" PRIu16 ") data path establish status:%d", channel->id, status);
}

static void add_l2cap_channel(void* data)
{
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    l2cap_chnl_t* channel;

    channel = (l2cap_chnl_t*)zalloc(sizeof(l2cap_chnl_t));
    if (!channel) {
        PRINT("allocate channel failed");
        goto free_msg;
        // TBD: cancel l2cap channel listen or connect immediately?
    }

    PRINT("L2cap channel(id:%" PRIu16 ") alloc success", msg->id);
    channel->id = msg->id;
    channel->psm = msg->psm;
    channel->is_listening = msg->is_listening;
    channel->pipe = euv_pipe_connect(&g_l2cap_thread, msg->proxy_name, data_path_connected_cb, channel);
    if (!channel->pipe) {
        PRINT("connect pipe failed");
        free(channel);
        goto free_msg;
    }

    list_add_tail(&channel_list, &channel->node);

free_msg:
    free(msg->proxy_name);
    free(msg);
}

static void l2cap_channel_connected_process(void* data)
{
    int ret;
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    l2cap_chnl_t* channel;

    channel = find_channel_by_id(msg->id);
    if (channel == NULL) {
        PRINT("%s, channel not found", __func__);
        goto free_msg;
    }

    channel->cid = msg->cid;
    PRINT("L2cap channel(id:%" PRIu16 "/cid:0x%" PRIx16 ") connected", msg->id, msg->cid);
    ret = euv_pipe_read_start(channel->pipe, 2048, read_complete_cb, NULL);
    if (ret) {
        PRINT("start read pipe failed");
        // disconnect data path, l2cap service will disconnect l2cap channel
        euv_pipe_disconnect(channel->pipe);
        list_delete(&channel->node);
        free(channel);
        goto free_msg;
    }

    if (msg->listen_id != INVALID_L2CAP_LISTEN_ID) {
        channel->is_listening = false; /* listen channel transfer to connected(accept) channel */
        PRINT("prepare a new listen channel(id: %" PRIu16 ") for PSM:0x%" PRIx16, msg->listen_id, msg->psm);
        /* Create a new channel for listening */
        msg->id = msg->listen_id;
        msg->is_listening = true;
        add_l2cap_channel((void*)msg);
        return;
    }

free_msg:
    if (msg->proxy_name)
        free(msg->proxy_name);

    free(msg);
}

static void l2cap_channel_disconnected_process(void* data)
{
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    l2cap_chnl_t* channel;

    channel = find_channel_by_id(msg->id);
    if (channel == NULL) {
        PRINT("%s, channel not found", __func__);
        free(msg);
        return;
    }

    if (g_trans_ctx.handle == channel->pipe) {
        l2cap_trans_reset();
    }

    PRINT("free channel(id:%" PRIu16 ")", msg->id);
    if (channel->pipe) {
        euv_pipe_disconnect(channel->pipe);
        channel->pipe = NULL;
    }

    list_delete(&channel->node);
    free(channel);
    free(msg);
}

static void do_l2cap_write(void* data)
{
    l2cap_msg_t* msg;
    l2cap_chnl_t* channel;

    if (!data) {
        PRINT("invalid arg\n");
        return;
    }

    msg = (l2cap_msg_t*)data;
    channel = find_channel_by_id(msg->id);
    if (channel == NULL || channel->pipe == NULL) {
        PRINT("channel not found or pipe disconnected\n");
        free(msg->buf);
        free(msg);
        return;
    }

    PRINT("L2cap channel(id:%" PRIu16 ") write %d bytes\n", msg->id, msg->len);
    lib_dumpbuffer("write data:", msg->buf, msg->len);
    euv_pipe_write(channel->pipe, msg->buf, msg->len, write_complete_cb);
    free(msg);
}

static void do_l2cap_stop_listen(void* data)
{
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    struct list_node* node;
    struct list_node* tmp;
    struct list_node* list = &channel_list;
    l2cap_chnl_t* channel;

    list_for_every_safe(list, node, tmp)
    {
        channel = (l2cap_chnl_t*)node;
        if (channel->is_listening && channel->psm == msg->psm) {
            PRINT("free listen channel(id:%" PRIu16 ") for psm:0x%" PRIx16, channel->id, msg->psm);
            if (channel->pipe) {
                euv_pipe_disconnect(channel->pipe);
                channel->pipe = NULL;
            }

            list_delete(&channel->node);
            free(channel);
            channel = NULL;
            break;
        }
    }

    free(msg);
}

static void do_l2cap_speed_test(void* data)
{
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    l2cap_chnl_t* channel;
    l2cap_trans_ctx_t* trans_ctx = &g_trans_ctx;
    uint16_t times;
    uint16_t id;
    static uint8_t start[100];

    id = msg->id;
    times = msg->len;
    free(msg);

    channel = find_channel_by_id(id);
    if (channel == NULL || channel->pipe == NULL) {
        PRINT("channel not found or pipe disconnected");
        return;
    }

    if (trans_ctx->state != TRANS_IDLE) {
        PRINT("l2cap is testing");
        return;
    }

    trans_ctx->handle = channel->pipe;
    trans_ctx->state = TRANS_SENDING;
    trans_ctx->bulk_length = L2CAP_TRANS_MTU_CFG;
    trans_ctx->bulk_count = times;
    trans_ctx->trans_total_size = L2CAP_TRANS_MTU_CFG * times;

    PRINT("L2cap channel(id:%" PRIu16 ") speed test", id);
    memset(start, 0, sizeof(start));
    sprintf((char*)start, "START:%" PRIu32 ";", trans_ctx->trans_total_size);
    euv_pipe_write(channel->pipe, start, strlen((const char*)start), NULL);
    PRINT("transmit start, waiting for %" PRIu32 " bytes transmit done", trans_ctx->trans_total_size);
}

static void do_l2cap_flow_control(void* data)
{
    l2cap_msg_t* msg = (l2cap_msg_t*)data;
    l2cap_chnl_t* channel;
    int err;
    uint16_t id = msg->id;
    bool start_recv = msg->is_listening;
    const char* action;

    free(msg);
    channel = find_channel_by_id(id);
    if (channel == NULL || channel->pipe == NULL) {
        PRINT("L2CAP channel(id: %" PRIu16 ") not found or pipe disconnected", id);
        return;
    }

    action = start_recv ? "start" : "stop";
    err = start_recv ? euv_pipe_read_start(channel->pipe, 2048, read_complete_cb, NULL)
                     : euv_pipe_read_stop(channel->pipe);
    PRINT("%s L2CAP channel(id: %" PRIu16 ") receive %s", action, id, err ? "failed" : "success");
}

static void on_connected(void* handle, l2cap_connect_params_t* params)
{
    l2cap_msg_t* msg;

    if (!params) {
        PRINT("invalid arg\n");
        return;
    }

    msg = (l2cap_msg_t*)zalloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed\n");
        return;
    }

    msg->id = params->id;
    msg->psm = params->psm;
    msg->cid = params->cid;
    msg->listen_id = params->listen_id;
    if (params->listen_id != INVALID_L2CAP_LISTEN_ID) {
        PRINT("new listen(id: %" PRIu16 "/ proxy_name: %s) for listen psm: 0x%" PRIx16,
            params->listen_id, params->proxy_name, params->psm);
        msg->proxy_name = strdup(params->proxy_name);
        if (!msg->proxy_name) {
            PRINT("%s, allocate proxy name failed", __func__);
            free(msg);
            return;
        }
    }

    memcpy(&msg->addr, &params->addr, sizeof(bt_address_t));
    do_in_thread_loop(&g_l2cap_thread, l2cap_channel_connected_process, msg);
}

static void on_disconnected(void* handle, bt_address_t* addr, uint16_t id, uint32_t reason)
{
    l2cap_msg_t* msg;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (!addr) {
        PRINT("invalid arg\n");
        return;
    }

    bt_addr_ba2str(addr, addr_str);
    PRINT("l2cap channel(id:%" PRIu16 ") disconnected, reason:%" PRIu32 ", addr:%s\n", id, reason, addr_str);

    msg = (l2cap_msg_t*)malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed\n");
        return;
    }

    msg->id = id;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));
    do_in_thread_loop(&g_l2cap_thread, l2cap_channel_disconnected_process, msg);
}

static l2cap_callbacks_t l2cap_callback = {
    .size = sizeof(l2cap_callbacks_t),
    .on_connected = on_connected,
    .on_disconnected = on_disconnected,
};

static int validate_l2cap_params(uint16_t mtu, uint16_t mps, uint16_t credits)
{
    if (mtu < L2CAP_LE_MTU_MIN) {
        PRINT("invalid mtu:%" PRIu16 ", min:%d", mtu, L2CAP_LE_MTU_MIN);
        return -1;
    }

    if (mps < L2CAP_LE_MPS_MIN || mps > L2CAP_LE_MPS_MAX) {
        PRINT("invalid mps:%" PRIu16 ", range:%d~%d", mps, L2CAP_LE_MPS_MIN, L2CAP_LE_MPS_MAX);
        return -1;
    }

    if (credits < L2CAP_LE_CREDIT_MIN) {
        PRINT("invalid credits:%" PRIu16 ", min:%d", credits, L2CAP_LE_CREDIT_MIN);
        return -1;
    }

    if (mps > mtu) {
        PRINT("invalid params: mps(%" PRIu16 ") > mtu(%" PRIu16 ")", mps, mtu);
        return -1;
    }

    return 0;
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    l2cap_config_option_t conn_option = { 0 };
    l2cap_msg_t* msg;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!\n");
        return CMD_ERROR;
    }

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    msg = (l2cap_msg_t*)zalloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed");
        return CMD_ERROR;
    }

    conn_option.psm = strtoul(argv[1], NULL, 0);
    conn_option.transport = BT_TRANSPORT_BLE;
    conn_option.mode = L2CAP_CHANNEL_MODE_LE_CREDIT_BASED_FLOW_CONTROL;
    conn_option.mtu = (argc > 2) ? strtoul(argv[2], NULL, 0) : L2CAP_TRANS_MTU_CFG;
    conn_option.le_mps = (argc > 3) ? strtoul(argv[3], NULL, 0) : L2CAP_TRANS_MPS_CFG;
    conn_option.init_credits = (argc > 4) ? strtoul(argv[4], NULL, 0) : L2CAP_TRANS_CREDIT_CFG;
    if (validate_l2cap_params(conn_option.mtu, conn_option.le_mps, conn_option.init_credits) < 0) {
        free(msg);
        return CMD_INVALID_PARAM;
    }

    if (bt_l2cap_connect(handle, g_l2cap_handle, &addr, &conn_option) != BT_STATUS_SUCCESS) {
        PRINT("connect %s failed", argv[0]);
        free(msg);
        return CMD_ERROR;
    }

    PRINT("L2cap channel(id:%" PRIu16 ") connecting", conn_option.id);

    msg->id = conn_option.id;
    msg->psm = conn_option.psm;
    msg->proxy_name = strdup(conn_option.proxy_name);
    msg->is_listening = false;
    memcpy(&msg->addr, &addr, sizeof(bt_address_t));
    do_in_thread_loop(&g_l2cap_thread, add_l2cap_channel, msg);

    return CMD_OK;
}

static int listen_cmd(void* handle, int argc, char* argv[])
{
    l2cap_config_option_t conn_option = { 0 };
    l2cap_msg_t* msg;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!\n");
        return CMD_ERROR;
    }

    if (argc < 1)
        conn_option.psm = 0;
    else
        conn_option.psm = strtoul(argv[0], NULL, 0);

    msg = (l2cap_msg_t*)zalloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed");
        return CMD_ERROR;
    }

    conn_option.transport = BT_TRANSPORT_BLE;
    conn_option.mode = L2CAP_CHANNEL_MODE_LE_CREDIT_BASED_FLOW_CONTROL;
    conn_option.mtu = (argc > 1) ? strtoul(argv[1], NULL, 0) : L2CAP_TRANS_MTU_CFG;
    conn_option.le_mps = (argc > 2) ? strtoul(argv[2], NULL, 0) : L2CAP_TRANS_MPS_CFG;
    conn_option.init_credits = (argc > 3) ? strtoul(argv[3], NULL, 0) : L2CAP_TRANS_CREDIT_CFG;
    if (validate_l2cap_params(conn_option.mtu, conn_option.le_mps, conn_option.init_credits) < 0) {
        free(msg);
        return CMD_INVALID_PARAM;
    }

    if (bt_l2cap_listen(handle, g_l2cap_handle, &conn_option) != BT_STATUS_SUCCESS) {
        PRINT("listen 0x%" PRIx16 " failed", conn_option.psm);
        free(msg);
        return CMD_ERROR;
    }

    PRINT("L2cap channel(id:%" PRIu16 "/psm:0x%" PRIx16 ") start listen", conn_option.id, conn_option.psm);
    msg->id = conn_option.id;
    msg->psm = conn_option.psm;
    msg->is_listening = true;
    msg->proxy_name = strdup(conn_option.proxy_name);
    do_in_thread_loop(&g_l2cap_thread, add_l2cap_channel, msg);

    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    uint16_t id;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!\n");
        return CMD_ERROR;
    }

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    id = strtoul(argv[0], NULL, 10);
    if (bt_l2cap_disconnect(handle, g_l2cap_handle, id) != BT_STATUS_SUCCESS) {
        PRINT("disconnect %" PRIu16 " failed", id);
        return CMD_ERROR;
    }

    PRINT("L2cap channel(id:%" PRIu16 ") disconnecting", id);

    return CMD_OK;
}

static int write_cmd(void* handle, int argc, char* argv[])
{
    uint8_t* buf;
    l2cap_msg_t* msg;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!\n");
        return CMD_ERROR;
    }

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    buf = (uint8_t*)strdup(argv[1]);
    if (buf == NULL) {
        PRINT("allocate buf failed\n");
        return CMD_ERROR;
    }

    msg = (l2cap_msg_t*)malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed\n");
        free(buf);
        return CMD_ERROR;
    }

    msg->id = strtoul(argv[0], NULL, 10);
    msg->buf = buf;
    msg->len = strlen((char*)buf);
    do_in_thread_loop(&g_l2cap_thread, do_l2cap_write, msg);

    return CMD_OK;
}

static int stop_listen_cmd(void* handle, int argc, char* argv[])
{
    uint16_t psm;
    l2cap_msg_t* msg;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!");
        return CMD_ERROR;
    }

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    msg = (l2cap_msg_t*)zalloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed");
        return CMD_ERROR;
    }

    psm = strtoul(argv[0], NULL, 0);
    if (bt_l2cap_stop_listen_with_transport(handle, g_l2cap_handle, BT_TRANSPORT_BLE, psm) != BT_STATUS_SUCCESS) {
        PRINT("stop listen 0x%" PRIX16 " failed", psm);
        return CMD_ERROR;
    }

    PRINT("L2cap stop listen psm:0x%" PRIx16, psm);
    msg->psm = psm;
    do_in_thread_loop(&g_l2cap_thread, do_l2cap_stop_listen, msg);

    return CMD_OK;
}

static int speed_test_cmd(void* handle, int argc, char* argv[])
{
    l2cap_msg_t* msg;
    struct timespec ts;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!");
        return CMD_ERROR;
    }

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    msg = (l2cap_msg_t*)malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed");
        return CMD_ERROR;
    }

    msg->id = strtoul(argv[0], NULL, 10);
    msg->len = strtoul(argv[1], NULL, 10);
    if (msg->len == 0) {
        PRINT("invalid param");
        free(msg);
        return CMD_ERROR;
    }

    do_in_thread_loop(&g_l2cap_thread, do_l2cap_speed_test, msg);

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;
    if (sem_timedwait(&speed_tx_sem, &ts) < 0) {
        PRINT("wait speed test ack failed");
        l2cap_trans_reset();
        return CMD_ERROR;
    }

    return CMD_OK;
}

static int stop_receive_cmd(void* handle, int argc, char* argv[])
{
    l2cap_msg_t* msg;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!");
        return CMD_ERROR;
    }

    if (argc < 1) {
        return CMD_PARAM_NOT_ENOUGH;
    }

    msg = (l2cap_msg_t*)malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed");
        return CMD_ERROR;
    }

    msg->id = strtoul(argv[0], NULL, 10);
    msg->is_listening = false; /* stop receive*/
    do_in_thread_loop(&g_l2cap_thread, do_l2cap_flow_control, msg);

    return CMD_OK;
}

static int start_receive_cmd(void* handle, int argc, char* argv[])
{
    l2cap_msg_t* msg;

    if (!handle || !g_l2cap_handle) {
        PRINT("L2CAP tool not ready!");
        return CMD_ERROR;
    }

    if (argc < 1) {
        return CMD_PARAM_NOT_ENOUGH;
    }

    msg = (l2cap_msg_t*)malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed");
        return CMD_ERROR;
    }

    msg->id = strtoul(argv[0], NULL, 10);
    msg->is_listening = true; /* start receive*/
    do_in_thread_loop(&g_l2cap_thread, do_l2cap_flow_control, msg);

    return CMD_OK;
}

static bt_command_t g_l2cap_commands[] = {
    { "connect", connect_cmd, 0, "\"connect l2cap channel      param: <address> <psm> [mtu] [mps] [credits]\"" },
    { "listen", listen_cmd, 0, "\"listen l2cap channel        param: <psm> [mtu] [mps] [credits]\"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect l2cap channel  param: <id>\"" },
    { "stoplisten", stop_listen_cmd, 0, "\"stop listen l2cap channel  param: <psm>\"" },
    { "write", write_cmd, 0, "\"write data to peer   param: <id> <data>\"" },
    { "speed", speed_test_cmd, 0, "\"speed test l2cap channel    param: <id> <iteration>\"" },
    { "stoprecv", stop_receive_cmd, 0, "\"stop receive data from specified L2CAP channel  param: <id>\"" },
    { "startrecv", start_receive_cmd, 0, "\"start receive data from specified L2CAP channel  param: <id>\"" },
};

static void usage(void)
{
    int i;

    printf("Usage:\n");
    printf("\tpsm: Protocol/Service Multiplexer value(128~191, 0 only for start listen)\n");
    printf("\tid: L2CAP Sock id, which is returned by connect/listen command\n");
    printf("\tCommands:\n");
    for (i = 0; i < ARRAY_SIZE(g_l2cap_commands); i++) {
        printf("\t%-8s\t%s\n", g_l2cap_commands[i].cmd, g_l2cap_commands[i].help);
    }
}

int l2cap_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0) {
        ret = execute_command_in_table(handle, g_l2cap_commands, ARRAY_SIZE(g_l2cap_commands), argc, argv);
    }

    if (ret < 0)
        usage();

    return ret;
}

int l2cap_command_init(void* handle)
{
    sem_init(&speed_tx_sem, 0, 0);
    thread_loop_init(&g_l2cap_thread);
    thread_loop_run(&g_l2cap_thread, true, "bttool-l2cap");
    g_l2cap_handle = bt_l2cap_register_callbacks(handle, &l2cap_callback);

    return 0;
}

void l2cap_command_uninit(void* handle)
{
    do_in_thread_loop(&g_l2cap_thread, cleanup_l2cap_channel, NULL);
    bt_l2cap_unregister_callbacks(handle, g_l2cap_handle);
    thread_loop_exit(&g_l2cap_thread);
    sem_destroy(&speed_tx_sem);
    memset(&g_l2cap_thread, 0, sizeof(g_l2cap_thread));
}
