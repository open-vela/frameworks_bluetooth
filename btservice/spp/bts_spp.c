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
#define LOG_TAG "bts_spp"
/****************************************************************************
 * Included Files
 ****************************************************************************/
// stdlib
#include <fcntl.h>
#include <pty.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
// nuttx
#include <debug.h>
#include <nuttx/list.h>
// libuv
#include "uv.h"
// bluelet dependent
#include "stack_adapter_service_base.h"
#include "stack_adapter_spp.h"
// internel dependent
#include "btm_manager.h"
#include "btm_spp.h"
#include "bts_service.h"
#include "bts_spp.h"
#include "euv_pty.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "utils/uuid.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONNECTIONS_MAX CONFIG_BLUETOOTH_SPP_MAX_CONNECTIONS
#define SERVER_CONNECTION_MAX CONFIG_BLUETOOTH_SPP_SERVER_MAX_CONNECTIONS
#define CONNECTIONS_BASE (1 << 6)
#define INDEX_MAX (CONNECTIONS_MAX >> 5)
#define INVALID_FD -1

#define PACKET_SIZE (255)
#ifdef CONFIG_BLUETOOTH_SPP_WRITE_CREDITS
#define WRITE_CREDITS CONFIG_BLUETOOTH_SPP_WRITE_CREDITS
#else
#define WRITE_CREDITS 5
#endif

#ifdef CONFIG_BLUETOOTH_SPP_DUMPBUFFER
#define spp_dumpbuffer(m, a, n) lib_dumpbuffer(m, a, n)
#else
#define spp_dumpbuffer(m, a, n)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct
{
    uint32_t conn_id_map[INDEX_MAX];
    uint8_t conn_id_next;
    struct list_node dev_list;
    spp_service_callbacks_t* cbs;
} spp_handle_t;

typedef struct
{
    struct list_node node;
    euv_pty_t* handle;
#ifdef CONFIG_BLUETOOTH_SPP_LOOP_EN
    euv_pty_t* shandle;
#endif
    bool accept;
    bt_address addr;
    uint16_t svr_port;
    uint16_t conn_port;
    int mfd;
    int sfd;
    char pty_name[20];
    uint8_t credits;
    spp_connection_state_t state;
} spp_pty_device_t;

typedef struct
{
    enum {
        SERVER_START_REQ = 0,
        SERVER_STOP_REQ = 1,
        CLIENT_CONNECT_REQ,
        DISCONNECT_REQ,
        CLEANUP,
        STATE_CHANEG,
        DATA_SENT,
        DATA_RECEIVED,
        CONN_REQ_RECEIVED,
    } event;
    bt_address addr;
    uint16_t port;
    uint16_t uuid16;
    uint8_t* buffer;
    uint16_t length;
    uint16_t sent_length;
    spp_connection_state_t state;
} spp_msg_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int do_spp_write(spp_pty_device_t* device, uint8_t* buffer, uint16_t length);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static spp_handle_t g_spp_handle;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static const char* spp_event_to_string(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(SERVER_START_REQ)
        CASE_RETURN_STR(SERVER_STOP_REQ)
        CASE_RETURN_STR(CLIENT_CONNECT_REQ)
        CASE_RETURN_STR(DISCONNECT_REQ)
        CASE_RETURN_STR(STATE_CHANEG)
        CASE_RETURN_STR(DATA_SENT)
        CASE_RETURN_STR(DATA_RECEIVED)
        CASE_RETURN_STR(CONN_REQ_RECEIVED)
    default:
        return "UNKNOWN";
    }
}

static uint8_t alloc_connection_port(uint8_t svr_port)
{
    uint8_t conn_id = 0;

    for (; (conn_id < CONNECTIONS_MAX) && ((1 << conn_id) & g_spp_handle.conn_id_map[0]); conn_id++)
        ;
    if (conn_id < CONNECTIONS_MAX) {
        g_spp_handle.conn_id_map[0] |= (1 << conn_id);
        return (svr_port + (conn_id << 6));
    }

    return -ENOMEM;
}

static void free_connection_port(uint8_t conn_id)
{
    if (conn_id < CONNECTIONS_MAX) {
        g_spp_handle.conn_id_map[0] &= ~(1 << conn_id);
    }
}

static spp_pty_device_t* alloc_new_device(bt_address addr, uint16_t port, bool accept)
{
    spp_pty_device_t* device = (spp_pty_device_t*)malloc(sizeof(spp_pty_device_t));

    if (device == NULL)
        return NULL;

    device->svr_port = port;
    device->conn_port = alloc_connection_port(port);
    if (device->conn_port < 0) {
        free(device);
        return NULL;
    }
    device->accept = accept;
    device->handle = NULL;
    device->mfd = INVALID_FD;
    device->sfd = INVALID_FD;
    device->credits = WRITE_CREDITS;
    device->state = SPP_CONNECTION_STATE_DISCONNECTED;
    memcpy(device->addr, addr, sizeof(device->addr));
    list_add_tail(&g_spp_handle.dev_list, &device->node);

    return device;
}

static spp_pty_device_t* find_pty_device(uint16_t port)
{
    struct list_node* list = &g_spp_handle.dev_list;
    spp_pty_device_t* device;
    struct list_node* node;

    list_for_every(list, node)
    {
        device = (spp_pty_device_t*)node;
        if (port == device->conn_port)
            return device;
    }

    BT_LOGW("Device not found for port:%d", port);
    return NULL;
}

static spp_pty_device_t* check_and_update_conn_port(bt_address addr, uint16_t port)
{
    struct list_node* list = &g_spp_handle.dev_list;
    spp_pty_device_t* device;
    struct list_node* node;

    list_for_every(list, node)
    {
        device = (spp_pty_device_t*)node;
        if (memcmp(addr, device->addr, 6) == 0 && (port >> 6) == (device->conn_port >> 6)) {
            if (port != device->conn_port)
                device->conn_port = port;
            return device;
        }
    }

    BT_LOGW("Device not found for port:%d", port);
    return NULL;
}

static spp_pty_device_t* find_pty_device_by_handle(euv_pty_t* handle)
{
    struct list_node* list = &g_spp_handle.dev_list;
    spp_pty_device_t* device;
    struct list_node* node;

    list_for_every(list, node)
    {
        device = (spp_pty_device_t*)node;
        if (device->handle == handle)
            return device;
    }

    BT_LOGW("Device not found for handle:%p", handle);
    return NULL;
}

static void remove_pty_device(spp_pty_device_t* device)
{
    BT_LOGD("%s", __func__);
    free_connection_port(device->conn_port);

    list_delete(&device->node);
    free(device);
}

static spp_pty_device_t* spp_open_pty_device(bt_address addr, uint16_t port)
{
    int ret;
    spp_pty_device_t* device;

    device = check_and_update_conn_port(addr, port);
    if (device == NULL)
        return NULL;

    ret = openpty(&device->mfd, &device->sfd, device->pty_name, NULL, NULL);
    if (ret != 0) {
        BT_LOGE("pty create failed");
        goto error;
    }

    device->handle = euv_pty_init(get_service_loop(), device->mfd, UV_TTY_MODE_IO);
    if (!device->handle)
        goto error;

#ifdef CONFIG_BLUETOOTH_SPP_LOOP_EN
    device->shandle = euv_pty_init(get_service_loop(), device->sfd, UV_TTY_MODE_IO);
    if (!device->shandle) {
        euv_pty_close(device->handle);
        goto error;
    }
#endif

    BT_LOGD("pty create success, name:%s, master:%d, slave:%d",
        device->pty_name, device->mfd, device->sfd);
    return device;
error:
    close(device->mfd);
    close(device->sfd);
    remove_pty_device(device);
    return NULL;
}

static void spp_close_pty_device(spp_pty_device_t* device)
{
    if (device->mfd != INVALID_FD) {
        close(device->mfd);
        device->mfd = INVALID_FD;
    }

    if (device->sfd != INVALID_FD) {
        close(device->sfd);
        device->sfd = INVALID_FD;
    }

    euv_pty_close(device->handle);
#ifdef CONFIG_BLUETOOTH_SPP_LOOP_EN
    euv_pty_close(device->shandle);
    device->shandle = NULL;
#endif
    device->handle = NULL;
    if (device->state == SPP_CONNECTION_STATE_CONNECTED)
        service_adapter_spp_disconnect_by_port(device->conn_port);

    remove_pty_device(device);
}

static void spp_close_all_device(void)
{
    struct list_node* list = &g_spp_handle.dev_list;
    spp_pty_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(list, node, tmp)
    {
        device = (spp_pty_device_t*)node;
        spp_close_pty_device(device);
    }
}

static void spp_notify_connection_state(bt_address addr, uint16_t port, spp_connection_state_t state)
{
    if (g_spp_handle.cbs && g_spp_handle.cbs->connection_state_cb)
        g_spp_handle.cbs->connection_state_cb(addr, port, state);
}

static void spp_notify_pty_opened(bt_address addr, uint16_t port, char* name, int fd)
{
    if (g_spp_handle.cbs && g_spp_handle.cbs->pty_open_cb)
        g_spp_handle.cbs->pty_open_cb(addr, port, name, fd);
}

static void euv_read_complete(euv_pty_t* handle,
    const uint8_t* buf, ssize_t size)
{
    spp_pty_device_t* device;

    device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL)
        return;
    if (size < 0) {
        spp_close_pty_device(device);
        return;
    }
    spp_dumpbuffer("master read:", buf, size);
    do_spp_write(device, (uint8_t*)buf, size);
    if (!(--device->credits)) {
        euv_pty_read_stop(handle);
    }
}

#ifdef CONFIG_BLUETOOTH_SPP_LOOP_EN
static void euv_read_loop_complete(euv_pty_t* handle,
    const uint8_t* buf, ssize_t size)
{
    if (size > 0)
        spp_dumpbuffer("slave read:", buf, size);
}
#endif

static void euv_write_complete(euv_pty_t* handle, uint8_t* buf, int status)
{
    spp_pty_device_t* device;

    device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL)
        return;
    if (status != 0) {
        spp_close_pty_device(device);
    }

    service_adapter_spp_data_received_rsp(device->conn_port, buf);
}

#ifdef CONFIG_BLUETOOTH_SPP_LOOP_EN
static void euv_write_loop_complete(euv_pty_t* handle, uint8_t* buf, int status)
{
    free(buf);
}
#endif

static int do_spp_write(spp_pty_device_t* device, uint8_t* buffer, uint16_t length)
{
    SERVICE_BT_STATUS status;
    uint16_t remaining = length;
    uint16_t size;
    uint8_t* tmp;

    if (!device || buffer == NULL)
        return -1;

    do {
        size = (remaining > PACKET_SIZE) ? PACKET_SIZE : remaining;
        tmp = (uint8_t*)malloc(size);
        if (!tmp) {
            BT_LOGE("%s failed to allocate memory", __func__);
            return length - remaining;
        }
        memcpy(tmp, buffer, size);
        status = service_adapter_spp_write(device->conn_port, tmp, size);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("%s write to stack failed", __func__);
            return length - remaining;
        }
        remaining -= size;
        buffer += size;
    } while (remaining);

    return length;
}

static void spp_on_connection_state_chaneged(bt_address addr, uint16_t port,
    spp_connection_state_t state)
{
    spp_pty_device_t* device;
    int ret;

    BT_LOGD("%s, addr: %s, port: %d, state: %d", __func__, addr_str(addr), port, state);
    spp_notify_connection_state(addr, port, state);

    if (state == SPP_CONNECTION_STATE_CONNECTED) {
        device = spp_open_pty_device(addr, port);
        if (device == NULL)
            return;

        device->state = state;
        ret = euv_pty_read_start(device->handle, euv_read_complete);
#ifdef CONFIG_BLUETOOTH_SPP_LOOP_EN
        ret = euv_pty_read_start(device->shandle, euv_read_loop_complete);
#endif
        if (ret != 0) {
            spp_close_pty_device(device);
            return;
        }

        spp_notify_pty_opened(addr, port, device->pty_name, device->sfd);
    } else if (state == SPP_CONNECTION_STATE_DISCONNECTED) {
        device = find_pty_device(port);
        if (!device)
            return;
        device->state = state;
        euv_pty_read_stop(device->handle);
        spp_close_pty_device(device);
    }
}

static void spp_on_incoming_data_received(bt_address addr, uint16_t port,
    uint8_t* buffer, uint16_t length)
{
    spp_pty_device_t* device;
    int ret;

    device = find_pty_device(port);
    if (!device || buffer == NULL)
        return;

#ifdef CONFIG_BLUETOOTH_SPP_LOOP_EN
    uint8_t* loop_buf = (uint8_t*)malloc(length);
    memcpy(loop_buf, buffer, length);
    spp_dumpbuffer("slave write:", loop_buf, length);
    euv_pty_write(device->shandle, loop_buf, length, euv_write_loop_complete);
#endif
    spp_dumpbuffer("master write:", buffer, length);
    ret = euv_pty_write(device->handle, buffer, length, euv_write_complete);
    if (ret != 0) {
        spp_close_pty_device(device);
        BT_LOGE("Spp write to slave port %d failed", device->mfd);
    }
}

static void spp_on_outgoing_complete(uint16_t port, uint8_t* buffer, uint16_t length)
{
    spp_pty_device_t* device;

    device = find_pty_device(port);
    if (!device || buffer == NULL)
        return;

    if (!device->credits) {
        euv_pty_read_start(device->handle, euv_read_complete);
    }
    device->credits++;
    free(buffer);
}

static void spp_on_connect_request_received(bt_address addr, uint16_t port)
{
    spp_pty_device_t* device;

    device = alloc_new_device(addr, port, true);
    if (device) {
        BT_LOGD("CONN_REQ_RECEIVED: svr_port:%d, conn_port:%d", port, device->conn_port);
        service_adapter_spp_send_connection_rsp(addr, device->conn_port, true);
    } else {
        BT_LOGW("device alloc failed, reject connection: svr_port:%d,", port);
        service_adapter_spp_send_connection_rsp(addr, port, false);
    }
}

static void spp_server_start(uint16_t port, uint16_t uuid)
{
    struct bt_uuid_16 uuid_src;
    struct bt_uuid_128 uuid_128_dst;

    bt_uuid_create((struct bt_uuid*)&uuid_src, (uint8_t*)&uuid, 2);
    uuid_to_uuid128((struct bt_uuid*)&uuid_src, &uuid_128_dst);
    service_adapter_spp_server_open(port, uuid_128_dst.val, SERVER_CONNECTION_MAX);
}

static void spp_server_stop(uint16_t port)
{
    service_adapter_spp_server_close(port);
}

static void spp_client_connect(bt_address addr, uint16_t port, uint16_t uuid)
{
    SERVICE_BT_STATUS status;
    spp_pty_device_t* device;
    struct bt_uuid_16 uuid_src;
    struct bt_uuid_128 uuid_128_dst;

    bt_uuid_create((struct bt_uuid*)&uuid_src, (uint8_t*)&uuid, 2);
    uuid_to_uuid128((struct bt_uuid*)&uuid_src, &uuid_128_dst);

    device = alloc_new_device(addr, 0, false);
    if (!device)
        return;

    status = service_adapter_spp_client_open(addr, device->conn_port, uuid_128_dst.val);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        //spp_notify_connection_state(addr, device->conn_port, SPP_CONNECTION_STATE_DISCONNECTING);
        remove_pty_device(device);
        return;
    }
    // todo: start connect timer, release device if timeout
    device->state = SPP_CONNECTION_STATE_CONNECTING;
}

static void spp_disconnect(bt_address addr, uint16_t port)
{
    spp_pty_device_t* device = find_pty_device(port);
    if (device == NULL)
        return;

    device->state = SPP_CONNECTION_STATE_DISCONNECTING;
    service_adapter_spp_disconnect_by_port(port);
}

static void spp_cleanup(void)
{
    spp_close_all_device();
    list_delete(&g_spp_handle.dev_list);
    service_adapter_spp_cleanup();
    bts_unregister_profile_process(BT_PROFILE_SPP_ID);
}

static void spp_service_event_process(void* data, size_t size)
{
    if (!data || !size)
        return;
    spp_msg_t* msg = (spp_msg_t*)data;

    BT_LOGD("%s, event: %s", __func__, spp_event_to_string(msg->event));
    switch (msg->event) {
    case SERVER_START_REQ:
        spp_server_start(msg->port, msg->uuid16);
        break;

    case SERVER_STOP_REQ:
        spp_server_stop(msg->port);
        break;

    case CLIENT_CONNECT_REQ:
        spp_client_connect(msg->addr, msg->port, msg->uuid16);
        break;

    case DISCONNECT_REQ:
        spp_disconnect(msg->addr, msg->port);
        break;

    case CLEANUP:
        spp_cleanup();
        break;

    case STATE_CHANEG:
        spp_on_connection_state_chaneged(msg->addr, msg->port, msg->state);
        break;

    case DATA_SENT:
        spp_on_outgoing_complete(msg->port, msg->buffer, msg->length);
        break;

    case DATA_RECEIVED:
        spp_on_incoming_data_received(msg->addr, msg->port, msg->buffer, msg->length);
        break;

    case CONN_REQ_RECEIVED:
        spp_on_connect_request_received(msg->addr, msg->port);
        break;

    default:
        break;
    }

    free(data);
}

static void do_in_spp_service(spp_msg_t* msg)
{
    spp_msg_t* spp_msg = (spp_msg_t*)malloc(sizeof(spp_msg_t));

    memcpy(spp_msg, msg, sizeof(spp_msg_t));
    bts_send_uv_msg(BT_PROFILE_SPP_ID, spp_msg, sizeof(spp_msg_t));
}

static void adp_connection_state_changed_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    spp_msg_t msg;
    spp_connection_state_t conn_state;

    switch (state) {
    case SERVICE_PROFILE_DISCONNECTED:
        conn_state = SPP_CONNECTION_STATE_DISCONNECTED;
        break;
    case SERVICE_PROFILE_CONNECTING:
        conn_state = SPP_CONNECTION_STATE_CONNECTING;
        break;
    case SERVICE_PROFILE_CONNECTED:
        conn_state = SPP_CONNECTION_STATE_CONNECTED;
        break;
    case SERVICE_PROFILE_DISCONNECTING:
        conn_state = SPP_CONNECTION_STATE_DISCONNECTING;
        break;
    }
    msg.event = STATE_CHANEG;
    msg.state = conn_state;
    msg.port = conn_port;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));

    do_in_spp_service(&msg);
}

static void adp_data_sent_callback(SERVICE_SPP_PORT conn_port, uint8_t* buffer, uint16_t length,
    uint16_t sent_length)
{
    spp_msg_t msg;

    msg.event = DATA_SENT;
    msg.port = conn_port;
    msg.length = length;
    msg.sent_length = sent_length;
    msg.buffer = buffer;

    do_in_spp_service(&msg);
}

static void adp_data_received_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port, uint8_t* buffer, uint16_t length)
{
    spp_msg_t msg;

    msg.event = DATA_RECEIVED;
    msg.port = conn_port;
    msg.length = length;
    msg.buffer = buffer;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));

    do_in_spp_service(&msg);
}

static void adp_server_connection_req_received_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT svr_port)
{
    spp_msg_t msg;

    msg.event = CONN_REQ_RECEIVED;
    msg.port = svr_port;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));

    do_in_spp_service(&msg);
}

static void bts_spp_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    spp_service_event_process(data, size);
}

static SPP_CALLBACKS_S spp_adp_callbacks = {
    sizeof(SPP_CALLBACKS_S),
    adp_connection_state_changed_callback,
    adp_data_sent_callback,
    adp_data_received_callback,
    adp_server_connection_req_received_callback,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
bt_result_code bts_spp_init(spp_service_callbacks_t* callbacks)
{
    SERVICE_BT_STATUS status;

    g_spp_handle.cbs = callbacks;
    list_initialize(&g_spp_handle.dev_list);
    memset(&g_spp_handle.conn_id_map, 0, sizeof(g_spp_handle.conn_id_map));
    bts_register_profile_process(BT_PROFILE_SPP_ID, bts_spp_handle_service_msg);

    status = service_adapter_spp_init(&spp_adp_callbacks);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        list_delete(&g_spp_handle.dev_list);
        return BT_RESULT_FAILED;
    }

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_server_start(uint16_t port, uint16_t uuid)
{
    spp_msg_t msg;

    msg.event = SERVER_START_REQ;
    msg.port = port;
    msg.uuid16 = uuid;
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_server_stop(uint16_t port)
{
    spp_msg_t msg;

    msg.event = SERVER_STOP_REQ;
    msg.port = port;
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_client_connect(bt_address addr, uint16_t port, uint16_t uuid)
{
    spp_msg_t msg;

    msg.event = CLIENT_CONNECT_REQ;
    msg.port = port;
    msg.uuid16 = uuid;
    memcpy(msg.addr, addr, sizeof(bt_address));
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_disconnect(bt_address addr, uint16_t port)
{
    spp_msg_t msg;

    msg.event = DISCONNECT_REQ;
    msg.port = port;
    memcpy(msg.addr, addr, sizeof(bt_address));
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

void bts_spp_cleanup(void)
{
    spp_msg_t msg;

    msg.event = CLEANUP;
    do_in_spp_service(&msg);
}
