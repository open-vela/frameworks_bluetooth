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
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
// nuttx
#include <debug.h>
#include <nuttx/list.h>
// libuv
#include "uv.h"
// bluelet dependent
#include "stack_adapter_service_base.h"
#include "stack_adapter_spp.h"

#include "btm_manager.h"
#include "btm_spp.h"
#include "bts_service.h"
#include "bts_spp.h"
#include "euv_pty.h"
#include "openpty.h"
#include "utils/log.h"
#include "utils/utils.h"
#include "utils/uuid.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define REGISTER_MAX          5
#define CONNECTIONS_MAX       CONFIG_BLUETOOTH_SPP_MAX_CONNECTIONS
#define SERVER_CONNECTION_MAX CONFIG_BLUETOOTH_SPP_SERVER_MAX_CONNECTIONS
#define INDEX_MAX             (CONNECTIONS_MAX >> 5)
#define INVALID_FD            -1
#define DEFAULT_PACKET_SIZE   (255)
#define SEND_FC_EN            1
#define SENDING_BUFS_QUOTA    13
#define CACHE_SEND_TIMEOUT    15
#ifdef CONFIG_BLUETOOTH_SPP_DUMPBUFFER
#define spp_dumpbuffer(m, a, n) lib_dumpbuffer(m, a, n)
#else
#define spp_dumpbuffer(m, a, n)
#endif

#define STACK_SVR_PORT(scn)           (((scn << 1) & 0x3E) + 1)
#define STACK_CONN_PORT(scn, conn_id, accept) \
                                      ((conn_id << 6) + (accept ? STACK_SVR_PORT(scn) : ((scn << 1) & 0x3E)))
#define SERVICE_SCN(port)             ((port & 0x3E) >> 1)
#define SERVICE_CONN_ID(conn_port)    (conn_port >> 6)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct spp_service_global {
    uint8_t          started;
    uint8_t          registered;
    uint8_t          conn_id_next;
    uint32_t         server_channel_map;
    uint32_t         conn_id_map[INDEX_MAX];
    struct list_node dev_list;
    struct list_node server_list;
    pthread_mutex_t  spp_lock;
    spp_service_callbacks_t* cbs;
};

typedef struct spp_handle {
    int              app_id;
    spp_callbacks_t* cbs;
} spp_handle_t;

typedef struct {
    uint16_t length;
    uint8_t* buffer_head;
} cache_buf_t;

typedef struct {
    struct list_node node;
    uint16_t         scn;
    uint16_t         uuid;
    spp_callbacks_t* cbs;
} spp_server_t;

typedef struct {
    struct list_node node;
    spp_server_t*    server;
    euv_pty_t*       handle;
    uv_timer_t*      timer;
    cache_buf_t      cache_buf;
    bool             accept;
    bt_address       addr;
    int16_t          scn;
    uint16_t         conn_port;
    uint16_t         conn_id;
    uint16_t         uuid;
    uint16_t         mfs;
    uint16_t         next_to_read;
    int              mfd;
    char             pty_name[20];
    uint8_t          remaining_quota;
    uint32_t         rx_bytes;
    uint32_t         tx_bytes;
    spp_connection_state_t state;
    spp_callbacks_t* cbs;
} spp_pty_device_t;

typedef struct {
    enum {
        STATE_CHANEG = 0,
        DATA_SENT,
        DATA_RECEIVED,
        CONN_REQ_RECEIVED,
        UPDATE_MFS
    } event;
    uint16_t   port;
    uint16_t   length;
    uint16_t   sent_length;
    uint8_t*   buffer;
    bt_address addr;
    spp_connection_state_t state;
} spp_msg_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int do_spp_write(spp_pty_device_t* device, uint8_t* buffer, uint16_t length);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static struct spp_service_global g_spp_handle = { .started = 0 };

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if 0
static const char* spp_event_to_string(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(STATE_CHANEG)
        CASE_RETURN_STR(DATA_SENT)
        CASE_RETURN_STR(DATA_RECEIVED)
        CASE_RETURN_STR(CONN_REQ_RECEIVED)
        CASE_RETURN_STR(UPDATE_MFS)
    default:
        return "UNKNOWN";
    }
}
#endif

static int alloc_connection_id(void)
{
    uint8_t start = g_spp_handle.conn_id_next;
    uint8_t minor;
    int index;
    int bitno;

    for (; ; ) {
        minor = g_spp_handle.conn_id_next;
        if (g_spp_handle.conn_id_next >= CONNECTIONS_MAX)
            g_spp_handle.conn_id_next = 0;
        else
            g_spp_handle.conn_id_next++;

        index = minor >> 5;
        bitno = minor & 31;
        if ((g_spp_handle.conn_id_map[index] & (1 << bitno)) == 0) {
            g_spp_handle.conn_id_map[index] |= (1 << bitno);
            return (int)minor;
        }

        if (start == minor)
            return -ENOMEM;
    }

    return -ENOMEM;
}

static void free_connection_id(uint16_t conn_id)
{
    int index;
    int bitno;

    index = conn_id >> 5;
    bitno = conn_id & 31;

    assert((g_spp_handle.conn_id_map[index] & (1 << bitno)) != 0);
    g_spp_handle.conn_id_map[index] &= ~(1 << bitno);
    if (conn_id < g_spp_handle.conn_id_next)
        g_spp_handle.conn_id_next = conn_id;
}

static int scn_bit_check(uint16_t scn)
{
    if (scn < 1 || scn > 31)
        return -EINVAL;

    return g_spp_handle.server_channel_map & (1 << scn);
}

static int scn_bit_alloc(uint16_t scn)
{
    if (scn_bit_check(scn) != 0)
        return -ENOMEM;

    g_spp_handle.server_channel_map |= (1 << scn);

    return 0;
}

static int scn_bit_free(uint16_t scn)
{
    if (scn < 1 || scn > 31)
        return -EINVAL;

    g_spp_handle.server_channel_map &= ~(1 << scn);

    return 0;
}

static spp_server_t *alloc_new_server(uint16_t scn, uint16_t uuid, spp_callbacks_t* cbs)
{
    if (scn_bit_alloc(scn) != 0)
        return NULL;

    spp_server_t *server = malloc(sizeof(spp_server_t));
    if (!server)
        return NULL;

    server->scn  = scn;
    server->uuid = uuid;
    server->cbs  = cbs;
    list_add_tail(&g_spp_handle.server_list, &server->node);

    return server;
}

static void free_server_resource(spp_server_t *server)
{
    scn_bit_free(server->scn);
    list_delete(&server->node);
    free(server);
}

static void spp_cleanup_all_server(void)
{
    spp_server_t* server;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_spp_handle.dev_list, node, tmp)
    {
        server = (spp_server_t*)node;
        free_server_resource(server);
    }
}

static spp_server_t *find_server(uint16_t scn)
{
    spp_server_t* server;
    struct list_node* node;

    list_for_every(&g_spp_handle.server_list, node)
    {
        server = (spp_server_t*)node;
        if (server->scn == scn)
            return server;
    }

    BT_LOGW("%s, server not found: %d", __func__, scn);
    return NULL;
}

static spp_pty_device_t* alloc_new_device(bt_address addr, int16_t scn, uint16_t uuid, bool accept, spp_callbacks_t* cbs)
{
    spp_pty_device_t* device = (spp_pty_device_t*)malloc(sizeof(spp_pty_device_t));

    if (device == NULL)
        return NULL;

    memset(device, 0, sizeof(spp_pty_device_t));
    device->scn = scn;
    device->conn_id = alloc_connection_id();
    if (device->conn_id < 0) {
        free(device);
        return NULL;
    }

    device->cbs = cbs;
    device->conn_port = STACK_CONN_PORT(scn, device->conn_id, accept);
    device->accept = accept;
    device->mfs = DEFAULT_PACKET_SIZE;
    device->mfd = INVALID_FD;
    device->uuid = uuid;
    device->tx_bytes = 0;
    device->rx_bytes = 0;
    device->remaining_quota = SENDING_BUFS_QUOTA;
    device->state = SPP_CONNECTION_STATE_DISCONNECTED;
    memcpy(device->addr, addr, sizeof(device->addr));
    list_add_tail(&g_spp_handle.dev_list, &device->node);

    return device;
}

static spp_pty_device_t* find_pty_device(uint16_t conn_id)
{
    spp_pty_device_t* device;
    struct list_node* node;

    list_for_every(&g_spp_handle.dev_list, node)
    {
        device = (spp_pty_device_t*)node;
        if (conn_id == device->conn_id)
            return device;
    }

    BT_LOGW("Device not found for conn_id:%d", conn_id);
    return NULL;
}

static spp_pty_device_t* find_pty_device_by_handle(euv_pty_t* handle)
{
    spp_pty_device_t* device;
    struct list_node* node;

    list_for_every(&g_spp_handle.dev_list, node)
    {
        device = (spp_pty_device_t*)node;
        if (device->handle == handle)
            return device;
    }

    BT_LOGW("Device not found for handle: %p", handle);
    return NULL;
}

static void remove_pty_device(spp_pty_device_t* device)
{
    BT_LOGI("spp device remove, conn_id: %d, uuid:%04x", device->conn_id, device->uuid);
    free_connection_id(device->conn_id);
    list_delete(&device->node);
    free(device);
}

static spp_pty_device_t* spp_pty_device_open(spp_pty_device_t* device)
{
    int ret;

    ret = open_pty(&device->mfd, device->pty_name);
    if (ret != 0) {
        BT_LOGE("pty create failed");
        goto error;
    }

    device->handle = euv_pty_init(get_service_loop(), device->mfd, UV_TTY_MODE_IO);
    if (!device->handle)
        goto error;

    BT_LOGD("pty create success, name: %s, master: %d", device->pty_name, device->mfd);
    return device;
error:
    close(device->mfd);
    remove_pty_device(device);
    return NULL;
}

static void spp_pty_device_close(spp_pty_device_t* device)
{
    if (device->timer != NULL)
        stop_timer(device->timer);

    if (device->handle) {
        euv_pty_close(device->handle);
        device->handle = NULL;
        device->mfd = INVALID_FD;
    }

    if (device->state == SPP_CONNECTION_STATE_CONNECTED)
        service_adapter_spp_disconnect_by_port(device->conn_port);
}

static void spp_device_cleanup(spp_pty_device_t* device)
{
    spp_pty_device_close(device);
    remove_pty_device(device);
}

static void spp_cleanup_all_device(void)
{
    spp_pty_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_spp_handle.dev_list, node, tmp)
    {
        device = (spp_pty_device_t*)node;
        spp_device_cleanup(device);
    }
}

static void spp_notify_connection_state(spp_pty_device_t* device, spp_connection_state_t state)
{
    assert(device);

    if (device->cbs && device->cbs->connection_state_cb)
        device->cbs->connection_state_cb(device->addr, device->scn, device->conn_id, state);
    else if (g_spp_handle.cbs && g_spp_handle.cbs->connection_state_cb)
        g_spp_handle.cbs->connection_state_cb(device->addr, device->scn, device->conn_id, state);
}

static void spp_notify_pty_opened(spp_pty_device_t* device)
{
    assert(device);

    if (device->cbs && device->cbs->pty_open_cb)
        device->cbs->pty_open_cb(device->addr, device->conn_id, device->pty_name);
    else if (g_spp_handle.cbs && g_spp_handle.cbs->pty_open_cb)
        g_spp_handle.cbs->pty_open_cb(device->addr, device->conn_id, device->pty_name);
}

static void euv_alloc_buffer(euv_pty_t* handle, uint8_t** buf, size_t *len)
{
    spp_pty_device_t* device;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL) {
        *len = 0;
        goto unlock;
    }

    if (device->cache_buf.length > 0) {
        *len = device->mfs - device->cache_buf.length;
        *buf = device->cache_buf.buffer_head + device->cache_buf.length;
    } else {
        *len = device->mfs;
        *buf = malloc(*len);
    }

unlock:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

static void euv_read_complete(euv_pty_t* handle, const uint8_t* buf, ssize_t size)
{
    spp_pty_device_t* device;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL)
        goto unlock;

    if (size <= 0) {
        if (buf && (device->cache_buf.length == 0))
            free((void *)buf);

        if (size < 0)
            spp_pty_device_close(device);

        goto unlock;
    }

    spp_dumpbuffer("master read:", buf, size);
    do_spp_write(device, (uint8_t*)buf, size);

unlock:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

static void euv_write_complete(euv_pty_t* handle, uint8_t* buf, int status)
{
    spp_pty_device_t* device;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL)
        goto unlock;

    service_adapter_spp_data_received_rsp(device->conn_port, buf);
    if (status != 0)
        spp_pty_device_close(device);

unlock:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

static void spp_cache_timeout(char* data)
{
    spp_pty_device_t* device = (spp_pty_device_t*)data;

    if (device->cache_buf.length == 0)
        return;

    do_spp_write(device, NULL, 0);
}

static void spp_cache_fragement(spp_pty_device_t* device, uint8_t* buffer, uint16_t length)
{
    device->cache_buf.buffer_head = buffer;
    device->cache_buf.length = length;
    device->timer = start_timer(CACHE_SEND_TIMEOUT, 0, spp_cache_timeout, device);
    device->next_to_read = device->mfs - length;
}

static void spp_cache_stop(spp_pty_device_t* device)
{
    stop_timer(device->timer);
    device->timer = NULL;
    device->next_to_read = device->mfs;
}

static int do_spp_write(spp_pty_device_t* device, uint8_t* buffer, uint16_t length)
{
    SERVICE_BT_STATUS status;
    uint16_t remaining;
    uint16_t cache_size;
    uint16_t size;
    uint8_t* tmpbuf;

    if (!device)
        return -EINVAL;

    cache_size = device->cache_buf.length;
    remaining = length + cache_size;

    do {
        size = (remaining > device->mfs) ? device->mfs : remaining;
        if (cache_size == 0 && size < device->mfs) {
            spp_cache_fragement(device, buffer, size);
            return 0;
        }

        if (cache_size > 0) {
            spp_cache_stop(device);
            tmpbuf = device->cache_buf.buffer_head;
            device->cache_buf.buffer_head = NULL;
            device->cache_buf.length = 0;
            cache_size = 0;
        } else {
            tmpbuf = buffer;
        }

        status = service_adapter_spp_write(device->conn_port, tmpbuf, size);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("%s write to stack failed", __func__);
            free(tmpbuf);
            return length - remaining;
        }
        device->tx_bytes += size;
#if SEND_FC_EN
        if (!(--device->remaining_quota)) {
            euv_pty_read_stop(device->handle);
        }
#endif
        remaining -= size;
        buffer += size;
        assert(remaining == 0);
    } while (remaining);

    return length;
}

static void spp_on_connection_state_chaneged(bt_address addr, uint16_t port,
                                             spp_connection_state_t state)
{
    spp_pty_device_t* device;

    device = find_pty_device(SERVICE_CONN_ID(port));
    if (device == NULL || memcmp(addr, device->addr, 6) != 0) {
        BT_LOGE("%s, port or address mismatch", __func__);
        return;
    }

    if (!device->accept && device->scn == UNKNOWN_SERVER_CHANNEL_NUM) {
        device->scn = SERVICE_SCN(port);
        device->conn_port = port;
    }

    BT_LOGD("%s, addr: %s, scn: %d, port: %d, state: %d", 
            __func__, addr_str(addr), device->scn, device->conn_id, state);
    device->state = state;
    spp_notify_connection_state(device, state);
    if (state == SPP_CONNECTION_STATE_CONNECTED) {
        BT_LOGD("PERFORMANCE-SPP-BTM-CONNECTED");
        device = spp_pty_device_open(device);
        if (device == NULL)
            return;

        spp_notify_pty_opened(device);
    } else if (state == SPP_CONNECTION_STATE_DISCONNECTED)
        spp_device_cleanup(device);
}

static void spp_on_incoming_data_received(bt_address addr, uint16_t port,
                                          uint8_t* buffer, uint16_t length)
{
    spp_pty_device_t* device;
    int ret;

    device = find_pty_device(SERVICE_CONN_ID(port));
    if (!device || buffer == NULL)
        return;

    spp_dumpbuffer("master write:", buffer, length);
    device->rx_bytes += length;
    ret = euv_pty_write(device->handle, buffer, length, euv_write_complete);
    if (ret != 0) {
        BT_LOGE("Spp write to slave port %d failed", device->mfd);
        spp_pty_device_close(device);
    }
}

static void spp_on_outgoing_complete(uint16_t port, uint8_t* buffer, uint16_t length)
{
#if SEND_FC_EN
    spp_pty_device_t* device;

    free(buffer);
    device = find_pty_device(SERVICE_CONN_ID(port));
    if (!device)
        return;

    if (!device->remaining_quota && device->handle != NULL) {
        euv_pty_read_start2(device->handle, device->next_to_read, euv_read_complete, euv_alloc_buffer);
    }
    device->remaining_quota++;
#endif
}

static void spp_on_connect_request_received(bt_address addr, uint16_t port)
{
    spp_server_t* server;
    spp_pty_device_t* device;

    server = find_server(SERVICE_SCN(port));
    if (!server)
        return;

    device = alloc_new_device(addr, server->scn, server->uuid, true, server->cbs ? server->cbs : NULL);
    if (device) {
        BT_LOGD("CONN_REQ_RECEIVED scn:%d, uuid:%04x, conn_id:%d", server->scn, server->uuid, device->conn_id);
        device->server = server;
        service_adapter_spp_send_connection_rsp(addr, device->conn_port, true);
    } else {
        BT_LOGW("CONN_REQ_RECEIVED scn: %d, reject connection", port);
        service_adapter_spp_send_connection_rsp(addr, port, false);
    }
}

static void spp_on_connection_update_mfs(uint16_t port, uint16_t mfs)
{
    int ret;
    spp_pty_device_t* device;

    device = find_pty_device(SERVICE_CONN_ID(port));
    if (!device)
        return;

    device->mfs = mfs;
    device->next_to_read = mfs;
    ret = euv_pty_read_start2(device->handle, device->next_to_read, euv_read_complete, euv_alloc_buffer);
    if (ret != 0) {
        spp_pty_device_close(device);
    }
}

static void spp_service_event_process(spp_msg_t* msg)
{
    if (!msg)
        return;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    switch (msg->event) {
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

    case UPDATE_MFS:
        spp_on_connection_update_mfs(msg->port, msg->length);
        break;

    default:
        break;
    }
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

static void do_in_spp_service(spp_msg_t* msg)
{
    spp_msg_t* spp_msg = (spp_msg_t*)malloc(sizeof(spp_msg_t));
    if (spp_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    memcpy(spp_msg, msg, sizeof(spp_msg_t));
    bts_send_uv_msg(BT_PROFILE_SPP_ID, spp_msg, sizeof(spp_msg_t));
}

static void adp_connection_state_changed_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
                                                  SERVICE_PROFILE_CONNECTION_STATE state)
{
    spp_msg_t msg = {0};
    spp_connection_state_t conn_state = SPP_CONNECTION_STATE_DISCONNECTED;

    switch (state) {
    case PROFILE_DISCONNECTED:
        conn_state = SPP_CONNECTION_STATE_DISCONNECTED;
        break;
    case PROFILE_CONNECTING:
        conn_state = SPP_CONNECTION_STATE_CONNECTING;
        break;
    case PROFILE_CONNECTED:
        BT_LOGD("PERFORMANCE-SPP-BLUELET-CONNECTED");
        conn_state = SPP_CONNECTION_STATE_CONNECTED;
        break;
    case PROFILE_DISCONNECTING:
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
#if SEND_FC_EN
    spp_msg_t msg = {0};

    msg.event = DATA_SENT;
    msg.port = conn_port;
    msg.length = length;
    msg.sent_length = sent_length;
    msg.buffer = buffer;
    do_in_spp_service(&msg);
#else
    free(buffer);
#endif
}

static void adp_data_received_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port,
                                       uint8_t* buffer, uint16_t length)
{
    spp_msg_t msg = {0};

    msg.event = DATA_RECEIVED;
    msg.port = conn_port;
    msg.length = length;
    msg.buffer = buffer;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));

    do_in_spp_service(&msg);
}

static void adp_server_connection_req_received_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT svr_port)
{
    spp_msg_t msg = {0};

    msg.event = CONN_REQ_RECEIVED;
    msg.port = svr_port;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));

    do_in_spp_service(&msg);
}

static void adp_connection_mfs_callback(SERVICE_SPP_PORT conn_port, uint16_t mfs)
{
    spp_msg_t msg = {0};

    msg.event = UPDATE_MFS;
    msg.port = conn_port;
    msg.length = mfs;

    do_in_spp_service(&msg);
}

static void bts_spp_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    if (data == NULL)
        return;

    spp_service_event_process((spp_msg_t*)data);
    free(data);
}

static SPP_CALLBACKS_S spp_adp_callbacks = {
    sizeof(SPP_CALLBACKS_S),
    adp_connection_state_changed_callback,
    adp_data_sent_callback,
    adp_data_received_callback,
    adp_server_connection_req_received_callback,
    adp_connection_mfs_callback,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/
bt_result_code bts_spp_init(spp_service_callbacks_t* callbacks)
{
    SERVICE_BT_STATUS status;

    if (g_spp_handle.started)
        return BT_RESULT_SUCCESS;

    if (pthread_mutex_init(&g_spp_handle.spp_lock, NULL) < 0)
        return BT_RESULT_FAILED;

    g_spp_handle.cbs = callbacks;
    g_spp_handle.server_channel_map = 0;
    memset(&g_spp_handle.conn_id_map, 0, sizeof(g_spp_handle.conn_id_map));
    list_initialize(&g_spp_handle.dev_list);
    list_initialize(&g_spp_handle.server_list);
    status = service_adapter_spp_init(&spp_adp_callbacks);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        pthread_mutex_destroy(&g_spp_handle.spp_lock);
        list_delete(&g_spp_handle.dev_list);
        return BT_RESULT_FAILED;
    }

    bts_register_profile_process(BT_PROFILE_SPP_ID, bts_spp_handle_service_msg);
    g_spp_handle.started = 1;

    return BT_RESULT_SUCCESS;
}

spp_handle_t* bts_spp_register_app(int app_id, spp_callbacks_t *callbacks)
{
    spp_handle_t *handle = NULL;

    if (!g_spp_handle.started)
        return NULL;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (g_spp_handle.registered == REGISTER_MAX)
        goto unlock;

    handle = malloc(sizeof(spp_handle_t));
    if (handle == NULL)
        goto unlock;

    handle->app_id = app_id;
    handle->cbs = callbacks;
    g_spp_handle.registered++;

unlock:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return handle;
}

bt_result_code bts_spp_server_start(spp_handle_t* handle, uint16_t scn, uint16_t uuid)
{
    struct bt_uuid_16 uuid_src;
    struct bt_uuid_128 uuid_128_dst;
    spp_server_t *server;
    bt_result_code ret = BT_RESULT_SUCCESS;

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    bt_utils_uuid_create((struct bt_uuid*)&uuid_src, (uint8_t*)&uuid, 2);
    uuid_to_uuid128((struct bt_uuid*)&uuid_src, &uuid_128_dst);
    pthread_mutex_lock(&g_spp_handle.spp_lock);
    server = alloc_new_server(scn, uuid, (handle && handle->cbs) ? handle->cbs : NULL);
    if (!server) {
        ret = BT_RESULT_FAILED;
        goto unlock_exit;
    }

    service_adapter_spp_server_open(STACK_SVR_PORT(scn), uuid_128_dst.val, SERVER_CONNECTION_MAX);

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return ret;
}

bt_result_code bts_spp_server_stop(spp_handle_t* handle, uint16_t scn)
{
    spp_server_t *server;
    bt_result_code ret = BT_RESULT_SUCCESS;

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    server = find_server(scn);
    if (!server) {
        ret = BT_RESULT_FAILED;
        goto unlock_exit;
    }

    service_adapter_spp_server_close(STACK_SVR_PORT(scn));
    free_server_resource(server);

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return ret;
}

bt_result_code bts_spp_client_connect(spp_handle_t* handle, bt_address addr, int16_t scn, uint16_t uuid, uint16_t *port)
{
    SERVICE_BT_STATUS status;
    spp_pty_device_t* device;
    struct bt_uuid_16 uuid_src;
    struct bt_uuid_128 uuid_128_dst;
    bt_result_code ret = BT_RESULT_SUCCESS;

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    bt_utils_uuid_create((struct bt_uuid*)&uuid_src, (uint8_t*)&uuid, 2);
    uuid_to_uuid128((struct bt_uuid*)&uuid_src, &uuid_128_dst);
    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = alloc_new_device(addr, scn == UNKNOWN_SERVER_CHANNEL_NUM ? 0 : scn,
                              uuid, false, (handle && handle->cbs) ? handle->cbs : NULL);
    if (!device) {
        ret = BT_RESULT_FAILED;
        goto unlock_exit;
    }

    status = service_adapter_spp_client_open(addr, device->conn_port, uuid_128_dst.val);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        //spp_notify_connection_state(device, SPP_CONNECTION_STATE_DISCONNECTED);
        remove_pty_device(device);
        ret = BT_RESULT_FAILED;
        goto unlock_exit;
    }

    // todo: start connect timer, release device if timeout
    *port = device->conn_id;
    device->state = SPP_CONNECTION_STATE_CONNECTING;

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return ret;
}

bt_result_code bts_spp_disconnect(spp_handle_t* handle, bt_address addr, uint16_t port)
{
    spp_pty_device_t* device;
    bt_result_code ret = BT_RESULT_SUCCESS;

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_pty_device(port);
    if (device == NULL) {
        ret = BT_RESULT_FAILED;
        goto unlock_exit;
    }

    device->state = SPP_CONNECTION_STATE_DISCONNECTING;
    service_adapter_spp_disconnect_by_port(device->conn_port);

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return ret;
}

void bts_spp_unregister_app(spp_handle_t *handle)
{
    if (!handle || !g_spp_handle.started)
        return;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    free(handle);
    g_spp_handle.registered--;
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

void bts_spp_cleanup(void)
{
    if (!g_spp_handle.started)
        return ;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    spp_cleanup_all_device();
    spp_cleanup_all_server();
    list_delete(&g_spp_handle.dev_list);
    list_delete(&g_spp_handle.server_list);
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    pthread_mutex_destroy(&g_spp_handle.spp_lock);
    service_adapter_spp_cleanup();
    bts_unregister_profile_process(BT_PROFILE_SPP_ID);
    g_spp_handle.started = 0;
}

void bts_spp_state_dump(void)
{
    spp_pty_device_t* device;
    spp_server_t* server;
    struct list_node* node;
    int i = 0;

    if (!g_spp_handle.started)
        return;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    list_for_every(&g_spp_handle.server_list, node)
    {
        i++;
        server = (spp_server_t*)node;
        printf("\tServer[%d]: Scn:%d, UUID:%04" PRIx16"\n", i, server->scn, server->uuid);
    }
    if (i == 0)
        printf("\tNo spp Server found\n");

    i = 0;
    list_for_every(&g_spp_handle.dev_list, node)
    {
        i++;
        device = (spp_pty_device_t*)node;
        printf("\tDevice[%d]: ID:%d, Addr:%s, State:%d, Scn:%d, UUID:%04" PRIx16
                ", MFS:%d, Pty:[%d,%s], Rx:%" PRIu32 ", Tx:%" PRIu32"\n",
                i, device->conn_id, addr_str(device->addr), device->state,
                device->scn, device->uuid, device->mfs, device->mfd,
                device->pty_name, device->rx_bytes, device->tx_bytes);
    }
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    if (i == 0)
        printf("\tNo spp device found\n");
}
