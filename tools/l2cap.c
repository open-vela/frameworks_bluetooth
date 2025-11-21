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

static uv_loop_t g_l2cap_thread;
static void* g_l2cap_handle;
static struct list_node channel_list = LIST_INITIAL_VALUE(channel_list);

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

static l2cap_chnl_t* find_channel_by_pipe(euv_pipe_t* pipe)
{
    struct list_node* node;
    struct list_node* list = &channel_list;
    l2cap_chnl_t* channel;

    list_for_every(list, node)
    {
        channel = (l2cap_chnl_t*)node;
        if (channel->pipe == pipe) {
            return channel;
        }
    }

    return NULL;
}

static void write_complete_cb(euv_pipe_t* handle, uint8_t* buf, int status)
{
    free(buf);
}

static void read_complete_cb(euv_pipe_t* pipe, const uint8_t* buf, ssize_t nread)
{
    l2cap_chnl_t* channel;

    // need lock
    if (nread < 0) {
        PRINT("read failed:%s\n", uv_strerror(nread));
        euv_pipe_read_stop(pipe);
        channel = find_channel_by_pipe(pipe);
        if (channel == NULL) {
            PRINT("channel not found\n");
            return;
        }

        euv_pipe_disconnect(channel->pipe);
        channel->pipe = NULL;
        list_delete(&channel->node);
        free(channel);
    } else if (nread == 0) {
        if (buf)
            free((void*)buf);
    } else {
        lib_dumpbuffer("read data:", buf, nread);
    }
}

static void data_path_connected_cb(euv_pipe_t* pipe, int status, void* data)
{
    l2cap_chnl_t* channel = (l2cap_chnl_t*)data;
    // need lock
    PRINT("l2cap channel(id:%" PRIu16 ") data path establish status:%d\n", channel->id, status); // euv thread

    // do nothing
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
    l2cap_msg_t* msg;
    l2cap_chnl_t* channel;

    if (!data) {
        PRINT("invalid arg\n");
        return;
    }

    msg = (l2cap_msg_t*)data;
    channel = find_channel_by_id(msg->id);
    if (channel == NULL) {
        PRINT("channel not found\n");
        free(msg);
        return;
    }

    PRINT("free channel(id:%" PRIu16 ")\n", msg->id);
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

    conn_option.psm = atoi(argv[1]);
    // defaule param
    conn_option.transport = BT_TRANSPORT_BLE;
    conn_option.mode = L2CAP_CHANNEL_MODE_LE_CREDIT_BASED_FLOW_CONTROL;
    conn_option.mtu = 128;
    conn_option.le_mps = 128;
    conn_option.init_credits = 0xffff;
    if (bt_l2cap_connect(handle, g_l2cap_handle, &addr, &conn_option) != BT_STATUS_SUCCESS) {
        PRINT("connect %s failed\n", argv[0]);
        return CMD_ERROR;
    }

    PRINT("L2cap channel(id:%" PRIu16 ") connecting\n", conn_option.id);
    msg = (l2cap_msg_t*)malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed\n");
        return CMD_ERROR;
    }

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
        return CMD_PARAM_NOT_ENOUGH;

    conn_option.psm = atoi(argv[0]);
    // default param
    conn_option.transport = BT_TRANSPORT_BLE;
    conn_option.mode = L2CAP_CHANNEL_MODE_LE_CREDIT_BASED_FLOW_CONTROL;
    conn_option.mtu = 128;
    conn_option.le_mps = 128;
    conn_option.init_credits = 0xffff;
    if (bt_l2cap_listen(handle, g_l2cap_handle, &conn_option) != BT_STATUS_SUCCESS) {
        PRINT("listen 0x%" PRIx16 " failed", conn_option.psm);
        return CMD_ERROR;
    }

    PRINT("L2cap channel(id:%" PRIu16 "/psm:0x%x) start listen\n", conn_option.id, conn_option.psm);
    msg = (l2cap_msg_t*)malloc(sizeof(l2cap_msg_t));
    if (!msg) {
        PRINT("allocate msg failed\n");
        return CMD_ERROR;
    }

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

    id = atoi(argv[0]);
    PRINT("L2cap channel(id:%" PRIu16 ") disconnecting\n", id);
    bt_l2cap_disconnect(handle, g_l2cap_handle, id);

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

    msg->id = atoi(argv[0]);
    msg->buf = buf;
    msg->len = strlen(argv[1]);
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

static bt_command_t g_l2cap_commands[] = {
    { "connect", connect_cmd, 0, "\"connect l2cap channel      param: <address> <psm>\"" },
    { "listen", listen_cmd, 0, "\"listen l2cap channel        param: <psm>\"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect l2cap channel  param: <id>\"" },
    { "stoplisten", stop_listen_cmd, 0, "\"stop listen l2cap channel  param: <psm>\"" },
    { "write", write_cmd, 0, "\"write data to peer   param: <id> <data>\"" },
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
    thread_loop_init(&g_l2cap_thread);
    thread_loop_run(&g_l2cap_thread, true, "bttool-l2cap");
    g_l2cap_handle = bt_l2cap_register_callbacks(handle, &l2cap_callback);

    return 0;
}

void l2cap_command_uninit(void* handle)
{
    bt_l2cap_unregister_callbacks(handle, g_l2cap_handle);
    thread_loop_exit(&g_l2cap_thread);
    memset(&g_l2cap_thread, 0, sizeof(g_l2cap_thread));
}
