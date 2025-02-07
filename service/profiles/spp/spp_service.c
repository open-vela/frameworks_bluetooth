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
#define LOG_TAG "spp"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "bt_addr.h"
#include "bt_debug.h"
#include "bt_device.h"
#include "bt_list.h"
#include "bt_profile.h"
#include "bt_uuid.h"
#include "euv_pipe.h"
#include "index_allocator.h"
#include "power_manager.h"
#include "sal_spp_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "spp_service.h"
#include "utils/log.h"
#include "uv.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define REGISTER_MAX 5
#define CONNECTIONS_MAX CONFIG_BLUETOOTH_SPP_MAX_CONNECTIONS
#define SERVER_CONNECTION_MAX CONFIG_BLUETOOTH_SPP_SERVER_MAX_CONNECTIONS
#define INVALID_FD -1
#define DEFAULT_PACKET_SIZE (255)
#define SENDING_BUFS_QUOTA 13
#define CACHE_SEND_TIMEOUT 15
#ifdef CONFIG_BLUETOOTH_SPP_DUMPBUFFER
#define spp_dumpbuffer(m, a, n) lib_dumpbuffer(m, a, n)
#else
#define spp_dumpbuffer(m, a, n)
#endif

#define STACK_SVR_PORT(scn) (((scn << 1) & 0x3E) + 1)
#define STACK_CONN_PORT(scn, conn_id, accept) \
    ((conn_id << 6) + (accept ? STACK_SVR_PORT(scn) : ((scn << 1) & 0x3E)))
#define SERVICE_SCN(port) ((port & 0x3E) >> 1)
#define SERVICE_CONN_ID(conn_port) (conn_port >> 6)

#ifdef CONFIG_RPMSG_UART
#define SPP_UART_DEV "/dev/ttyDROID"
#endif

#define SPP_PROXY_SERVER_PREF "btspp-srv"
/****************************************************************************
 * Private Types
 ****************************************************************************/

struct spp_service_global {
    uint8_t started;
    uint8_t registered;
    index_allocator_t* allocator;
    uint32_t server_channel_map;
    struct list_node devices;
    struct list_node servers;
    struct list_node apps;
    pthread_mutex_t spp_lock;
};

typedef struct spp_handle {
    struct list_node node;
    bt_instance_t* ins;
    char name[64];
    void* remote;
    const spp_callbacks_t* cbs;
} spp_handle_t;

typedef struct {
    uint16_t length;
    uint8_t* buffer_head;
} cache_buf_t;

typedef struct {
    struct list_node node;
    uint16_t scn;
    bt_uuid_t uuid;
    spp_handle_t* app_handle;
} spp_server_t;

typedef struct
{
    struct list_node node;
    uint8_t* buffer;
    uint16_t length;
} spp_rx_buf_t;

typedef struct {
    struct list_node node;
    spp_server_t* server;
    euv_pipe_t* handle;
    service_timer_t* timer;
    cache_buf_t cache_buf;
    struct list_node rx_list;
    bool accept;
    bt_address_t addr;
    int16_t scn;
    uint16_t conn_port;
    uint16_t conn_id;
    bt_uuid_t uuid;
    uint16_t mfs;
    uint16_t next_to_read;
    char proxy_name[20];
    uint8_t remaining_quota;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    spp_handle_t* app_handle;
    /* connection state */
    profile_connection_state_t state;
    spp_proxy_state_t proxy_state;
} spp_device_t;

typedef struct {
    enum {
        STATE_CHANEG = 0,
        DATA_SENT,
        DATA_RECEIVED,
        CONN_REQ_RECEIVED,
        UPDATE_MFS
    } event;
    uint16_t port;
    uint16_t length;
    uint16_t sent_length;
    uint8_t* buffer;
    bt_address_t addr;
    /* connection state */
    profile_connection_state_t state;
} spp_msg_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static int do_spp_write(spp_device_t* device, uint8_t* buffer, uint16_t length);
static void spp_server_cleanup_devices(spp_server_t* server);
static void spp_proxy_connection_callback(euv_pipe_t* handle, int status, void* user_data);
static bt_status_t spp_unregister_app(void** remote, void* handle);

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

static void spp_notify_connection_state(spp_device_t* device, profile_connection_state_t state)
{
    assert(device);
    if (!device->app_handle)
        return;

    void* handle = device->app_handle->remote ? device->app_handle->remote : device->app_handle;

    if (device->app_handle->cbs && device->app_handle->cbs->connection_state_cb)
        device->app_handle->cbs->connection_state_cb(handle, &device->addr,
            device->scn, device->conn_id, state);
}

static void spp_notify_proxy_state(spp_device_t* device, spp_proxy_state_t state)
{
    assert(device);
    if (!device->app_handle)
        return;

    void* handle = device->app_handle->remote ? device->app_handle->remote : device->app_handle;

    if (device->app_handle->cbs && device->app_handle->cbs->proxy_state_cb)
        device->app_handle->cbs->proxy_state_cb(handle, &device->addr, state, device->scn,
            device->conn_id, device->proxy_name);
}

static int scn_bit_check(uint16_t scn)
{
    /* 29 and 30 for HFP */
    if (scn < 1 || scn > 28)
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
    /* 29 and 30 for HFP */
    if (scn < 1 || scn > 28)
        return -EINVAL;

    g_spp_handle.server_channel_map &= ~(1 << scn);

    return 0;
}

static spp_server_t* alloc_new_server(uint16_t scn, bt_uuid_t* uuid, spp_handle_t* handle)
{
    if (scn_bit_alloc(scn) != 0)
        return NULL;

    spp_server_t* server = malloc(sizeof(spp_server_t));
    if (!server)
        return NULL;

    server->scn = scn;
    bt_uuid_to_uuid128(uuid, &server->uuid);
    server->app_handle = handle;
    list_add_tail(&g_spp_handle.servers, &server->node);

    return server;
}

static void free_server_resource(spp_server_t* server)
{
    spp_server_cleanup_devices(server);
    scn_bit_free(server->scn);
    list_delete(&server->node);
    free(server);
}

static spp_server_t* find_server(uint16_t scn)
{
    spp_server_t* server;
    struct list_node* node;

    list_for_every(&g_spp_handle.servers, node)
    {
        server = (spp_server_t*)node;
        if (server->scn == scn)
            return server;
    }

    BT_LOGW("%s, server not found: %d", __func__, scn);
    return NULL;
}

static spp_device_t* alloc_new_device(bt_address_t* addr, int16_t scn,
    bt_uuid_t* uuid, bool accept,
    spp_handle_t* handle)
{
    int conn_id;
    spp_device_t* device;

    conn_id = index_alloc(g_spp_handle.allocator);
    if (conn_id < 0)
        return NULL;

    device = malloc(sizeof(spp_device_t));
    if (device == NULL)
        return NULL;

    memset(device, 0, sizeof(spp_device_t));
    device->conn_id = conn_id;
    device->scn = scn;
    device->app_handle = handle;
    device->conn_port = STACK_CONN_PORT(scn, device->conn_id, accept);
    device->accept = accept;
    device->mfs = DEFAULT_PACKET_SIZE;
    bt_uuid_to_uuid128(uuid, &device->uuid);
    device->tx_bytes = 0;
    device->rx_bytes = 0;
    device->remaining_quota = SENDING_BUFS_QUOTA;
    device->state = PROFILE_STATE_DISCONNECTED;
    device->proxy_state = SPP_PROXY_STATE_DISCONNECTED;
    memcpy(&device->addr, addr, sizeof(bt_address_t));
    list_initialize(&device->rx_list);
    list_add_tail(&g_spp_handle.devices, &device->node);

    return device;
}

static spp_device_t* find_spp_device_by_conn(uint16_t conn_id)
{
    spp_device_t* device;
    struct list_node* node;

    if (!g_spp_handle.started)
        return NULL;

    list_for_every(&g_spp_handle.devices, node)
    {
        device = (spp_device_t*)node;
        if (conn_id == device->conn_id)
            return device;
    }

    BT_LOGW("Device not found for conn_id:%d", conn_id);
    return NULL;
}

static spp_device_t* find_spp_device_by_handle(euv_pipe_t* handle)
{
    spp_device_t* device;
    struct list_node* node;

    if (!g_spp_handle.started)
        return NULL;

    list_for_every(&g_spp_handle.devices, node)
    {
        device = (spp_device_t*)node;
        if (device->handle == handle)
            return device;
    }

    BT_LOGW("Device not found for handle: %p", handle);
    return NULL;
}

static void remove_spp_device(spp_device_t* device)
{
    BT_LOGI("spp device remove, conn_id: %d", device->conn_id);
    index_free(g_spp_handle.allocator, device->conn_id);
    list_delete(&device->node);
    free(device);
}

static bool spp_app_is_exist(void* handle)
{
    struct list_node* node;

    list_for_every(&g_spp_handle.apps, node)
    {
        if ((void*)node == handle)
            return true;
    }

    BT_LOGW("spp app not found: %p", handle);
    return false;
}

static spp_device_t* spp_device_open(spp_device_t* device)
{
    if (!device) {
        BT_LOGE("%s, device null", __func__);
        return NULL;
    }

    sprintf(device->proxy_name, "%s-%d", SPP_PROXY_SERVER_PREF, device->conn_id);
    device->handle = euv_pipe_open(get_service_uv_loop(), device->proxy_name, spp_proxy_connection_callback, device);
    if (!device->handle) {
        BT_LOGE("spp proxy open fail, proxy_name: %s", device->proxy_name);
        goto error;
    }

    BT_LOGD("spp proxy open success, name: %s", device->proxy_name);
    return device;

error:
    remove_spp_device(device);
    return NULL;
}

static void spp_device_close(spp_device_t* device)
{
    if (device->timer != NULL) {
        service_loop_cancel_timer(device->timer);
        device->timer = NULL;
    }

    if (device->state == PROFILE_STATE_CONNECTED || device->state == PROFILE_STATE_CONNECTING)
        bt_sal_spp_disconnect(device->conn_port);

    if (device->handle) {
        euv_pipe_close(device->handle);
        device->handle = NULL;
    }

    device->app_handle = NULL;
}

static void spp_device_cleanup(spp_device_t* device, bool notify)
{
    if (notify)
        spp_notify_connection_state(device, PROFILE_STATE_DISCONNECTED);

    spp_device_close(device);
    remove_spp_device(device);
}

static void spp_server_cleanup_devices(spp_server_t* server)
{
    spp_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_spp_handle.devices, node, tmp)
    {
        device = (spp_device_t*)node;
        if (device->server == server)
            spp_device_cleanup(device, true);
    }
}

static void spp_app_cleanup_servers(spp_handle_t* app)
{
    spp_server_t* server;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_spp_handle.servers, node, tmp)
    {
        server = (spp_server_t*)node;
        if (server->app_handle == app) {
            bt_sal_spp_server_stop(STACK_SVR_PORT(server->scn));
            free_server_resource(server);
        }
    }
}

static void spp_app_cleanup_devices(spp_handle_t* app)
{
    spp_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    // cleanup all device
    list_for_every_safe(&g_spp_handle.devices, node, tmp)
    {
        device = (spp_device_t*)node;
        if (device->app_handle == app) {
            bt_pm_conn_close(PROFILE_SPP, &device->addr);
            spp_device_cleanup(device, true);
        }
    }
}

static void spp_cleanup_app(spp_handle_t* app)
{
    /* cleanpup local server */
    spp_app_cleanup_servers(app);

    /* cleanpup all initiator device */
    spp_app_cleanup_devices(app);
}

static void spp_cleanup_all_apps(void)
{
    struct list_node* node;
    struct list_node* tmp;

    // cleanup all device
    list_for_every_safe(&g_spp_handle.apps, node, tmp)
    {
        spp_cleanup_app((spp_handle_t*)node);
        list_delete(&((spp_handle_t*)node)->node);
        free(node);
    }
}

static void euv_alloc_buffer(euv_pipe_t* handle, uint8_t** buf, size_t* len)
{
    spp_device_t* device;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_spp_device_by_handle(handle);
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

static void euv_read_complete(euv_pipe_t* handle, const uint8_t* buf, ssize_t size)
{
    spp_device_t* device;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_spp_device_by_handle(handle);
    if (!device || buf == NULL)
        goto unlock;

    if (size <= 0) {
        if (buf && (device->cache_buf.length == 0))
            free((void*)buf);

        if (size < 0)
            spp_device_close(device);

        goto unlock;
    }

    spp_dumpbuffer("master read:", buf, size);
    do_spp_write(device, (uint8_t*)buf, size);

unlock:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

static void euv_write_complete(euv_pipe_t* handle, uint8_t* buf, int status)
{
    spp_device_t* device;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_spp_device_by_handle(handle);
    if (!device || buf == NULL)
        goto unlock;

    bt_sal_spp_data_received_response(device->conn_port, buf);
    if (status != 0)
        spp_device_close(device);

unlock:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

static void spp_rx_buffer_send(spp_device_t* device)
{
    struct list_node *node, *tmp;
    spp_rx_buf_t* buf;

    BT_LOGD("spp_rx_buffer_send, rx_list: %d, rx_bytes: %" PRIu32 "", list_length(&device->rx_list), device->rx_bytes);
    list_for_every_safe(&device->rx_list, node, tmp)
    {
        buf = (spp_rx_buf_t*)node;
        device->rx_bytes += buf->length;
        if (euv_pipe_write(device->handle, buf->buffer, buf->length, euv_write_complete) != 0) {
            BT_LOGE("Spp write to slave port %d failed", device->conn_port);
            break;
        }
        spp_dumpbuffer("master buffer write:", buf->buffer, buf->length);
        list_delete(node);
        free(node);
    }
}

static bool spp_rx_buffer_empty(spp_device_t* device)
{
    return list_is_empty(&device->rx_list);
}

static void spp_rx_buffer_cache(spp_device_t* device, uint8_t* buffer, uint16_t length)
{
    spp_rx_buf_t* rx_buf = (spp_rx_buf_t*)malloc(sizeof(spp_rx_buf_t));

    if (!rx_buf) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    rx_buf->buffer = buffer;
    rx_buf->length = length;
    list_add_tail(&device->rx_list, &rx_buf->node);
}

static void spp_proxy_connection_callback(euv_pipe_t* handle, int status, void* user_data)
{
    spp_device_t* device;
    int ret;

    if (status < 0) {
        BT_LOGE("%s,uv listen error: %d", __func__, status);
        return;
    }

#ifdef CONFIG_NET_RPMSG
    /* close unused pipe */
    euv_pipe_close2(handle);
#endif

    device = find_spp_device_by_handle(handle);
    if (!device->handle) {
        BT_LOGE("%s, handle null", __func__);
        return;
    }

    BT_LOGD("spp proxy connected, status: %d", status);
    device->proxy_state = SPP_PROXY_STATE_CONNECTED;
    spp_rx_buffer_send(device);

    ret = euv_pipe_read_start(device->handle, device->next_to_read, euv_read_complete, euv_alloc_buffer);
    if (ret != 0) {
        BT_LOGE("%s, read start fail", __func__);
        spp_device_close(device);
    }
}

static void spp_cache_timeout(service_timer_t* timer, void* data)
{
    spp_device_t* device;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    device = find_spp_device_by_handle((euv_pipe_t*)data);
    if (!device)
        goto unlock;

    if (device->cache_buf.length == 0)
        goto unlock;

    do_spp_write(device, NULL, 0);
unlock:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
}

static void spp_cache_fragement(spp_device_t* device, uint8_t* buffer, uint16_t length)
{
    device->cache_buf.buffer_head = buffer;
    device->cache_buf.length = length;
    /* cache timer */
    device->timer = service_loop_timer_no_repeating(CACHE_SEND_TIMEOUT,
        spp_cache_timeout,
        device->handle);
    device->next_to_read = device->mfs - length;
}

static void spp_cache_stop(spp_device_t* device)
{
    service_loop_cancel_timer(device->timer);
    device->timer = NULL;
    device->next_to_read = device->mfs;
}

static int do_spp_write(spp_device_t* device, uint8_t* buffer, uint16_t length)
{
    bt_status_t status;
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

        bt_pm_busy(PROFILE_SPP, &device->addr);
        status = bt_sal_spp_write(device->conn_port, tmpbuf, size);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("%s write to stack failed", __func__);
            bt_pm_idle(PROFILE_SPP, &device->addr);
            free(tmpbuf);
            return length - remaining;
        }
        device->tx_bytes += size;

        if (!(--device->remaining_quota)) {
            euv_pipe_read_stop(device->handle);
        }

        remaining -= size;
        buffer += size;
        assert(remaining == 0);
    } while (remaining);

    return length;
}

static void spp_on_connection_state_chaneged(bt_address_t* addr, uint16_t port,
    profile_connection_state_t state)
{
    spp_device_t* device;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    device = find_spp_device_by_conn(SERVICE_CONN_ID(port));
    if (device == NULL || memcmp(addr, &device->addr, 6) != 0) {
        BT_LOGE("%s, port or address mismatch", __func__);
        return;
    }

    if (!device->accept && device->scn == UNKNOWN_SERVER_CHANNEL_NUM) {
        device->scn = SERVICE_SCN(port);
        device->conn_port = port;
    }

    bt_addr_ba2str(&device->addr, addr_str);
    BT_LOGD("%s, addr: %s, scn: %d, port: %d, state: %d",
        __func__, addr_str, device->scn, device->conn_id, state);
    device->state = state;
    spp_notify_connection_state(device, state);

    if (state == PROFILE_STATE_CONNECTED) {
        device = spp_device_open(device);
        if (device == NULL) {
            BT_LOGE("spp device open fail, disconnect port:%d", port);
            bt_sal_spp_disconnect(port);
            return;
        }

        spp_notify_proxy_state(device, SPP_PROXY_STATE_CONNECTED);
        bt_pm_conn_open(PROFILE_SPP, &device->addr);
    } else if (state == PROFILE_STATE_DISCONNECTED) {
        bt_pm_conn_close(PROFILE_SPP, &device->addr);
        spp_notify_proxy_state(device, SPP_PROXY_STATE_DISCONNECTED);
        spp_device_cleanup(device, false);
    }
}

static void spp_on_incoming_data_received(bt_address_t* addr, uint16_t port,
    uint8_t* buffer, uint16_t length)
{
    spp_device_t* device;
    int ret;

    device = find_spp_device_by_conn(SERVICE_CONN_ID(port));
    if (!device || buffer == NULL) {
        BT_LOGE("%s, port or address mismatch", __func__);
        return;
    }

    if (device->proxy_state != SPP_PROXY_STATE_CONNECTED) {
        BT_LOGW("spp proxy cache, port: %d, buf:%p, length:%d", port, buffer, length);
        spp_rx_buffer_cache(device, buffer, length);
        return;
    }

    if (!spp_rx_buffer_empty(device)) {
        BT_LOGW("spp proxy handle cache, port: %d, buf:%p, length:%d", port, buffer, length);
        spp_rx_buffer_cache(device, buffer, length);
        spp_rx_buffer_send(device);
        return;
    }

    spp_dumpbuffer("master write:", buffer, length);
    device->rx_bytes += length;
    ret = euv_pipe_write(device->handle, buffer, length, euv_write_complete);
    if (ret != 0) {
        BT_LOGE("Spp write to slave port %d failed", device->conn_port);
        spp_device_close(device);
    }
}

static void spp_on_outgoing_complete(uint16_t port, uint8_t* buffer, uint16_t length)
{
    spp_device_t* device;

    free(buffer);
    device = find_spp_device_by_conn(SERVICE_CONN_ID(port));
    if (!device)
        return;

    if (!device->remaining_quota && device->handle != NULL) {
        euv_pipe_read_start(device->handle, device->next_to_read, euv_read_complete, euv_alloc_buffer);
    }
    device->remaining_quota++;
}

static void spp_on_connect_request_received(bt_address_t* addr, uint16_t port)
{
    spp_server_t* server;
    spp_device_t* device;

    server = find_server(SERVICE_SCN(port));
    if (!server)
        return;

    device = alloc_new_device(addr, server->scn, &server->uuid, true, server->app_handle);
    if (device) {
        char uuid_str[40] = { 0 };
        bt_uuid_to_string(&server->uuid, uuid_str, 40);
        BT_LOGD("CONN_REQ_RECEIVED scn:%d, uuid:%s, conn_id:%d", server->scn, uuid_str, device->conn_id);
        device->server = server;
        bt_sal_spp_connect_request_reply(addr, device->conn_port, true);
    } else {
        BT_LOGW("CONN_REQ_RECEIVED scn: %d, reject connection", port);
        bt_sal_spp_connect_request_reply(addr, port, false);
    }
}

static void spp_on_connection_update_mfs(uint16_t port, uint16_t mfs)
{
    spp_device_t* device;

    device = find_spp_device_by_conn(SERVICE_CONN_ID(port));
    if (!device)
        return;

    device->mfs = mfs;
    device->next_to_read = mfs;
}

static void spp_service_event_process(void* data)
{
    if (!data)
        return;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        return;
    }

    spp_msg_t* msg = data;
    switch (msg->event) {
    case STATE_CHANEG:
        spp_on_connection_state_chaneged(&msg->addr, msg->port, msg->state);
        break;
    case DATA_SENT:
        spp_on_outgoing_complete(msg->port, msg->buffer, msg->length);
        bt_pm_idle(PROFILE_SPP, &msg->addr);
        break;
    case DATA_RECEIVED:
        bt_pm_busy(PROFILE_SPP, &msg->addr);
        spp_on_incoming_data_received(&msg->addr, msg->port, msg->buffer, msg->length);
        bt_pm_idle(PROFILE_SPP, &msg->addr);
        break;
    case CONN_REQ_RECEIVED:
        spp_on_connect_request_received(&msg->addr, msg->port);
        break;
    case UPDATE_MFS:
        spp_on_connection_update_mfs(msg->port, msg->length);
        break;
    default:
        break;
    }

    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    free(data);
}

static bt_status_t spp_init(void)
{
    pthread_mutexattr_t attr;

    memset(&g_spp_handle, 0, sizeof(g_spp_handle));
    g_spp_handle.started = 0;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_spp_handle.spp_lock, &attr) < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

static bt_status_t spp_startup(profile_on_startup_t cb)
{
    bt_status_t status;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (g_spp_handle.started) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        cb(PROFILE_SPP, true);
        return BT_STATUS_SUCCESS;
    }

    g_spp_handle.server_channel_map = 0;
    g_spp_handle.registered = 0;
    g_spp_handle.allocator = index_allocator_create(CONNECTIONS_MAX);
    list_initialize(&g_spp_handle.devices);
    list_initialize(&g_spp_handle.servers);
    list_initialize(&g_spp_handle.apps);
    status = bt_sal_spp_init();
    if (status != BT_STATUS_SUCCESS) {
        list_delete(&g_spp_handle.devices);
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        cb(PROFILE_SPP, false);
        return BT_STATUS_FAIL;
    }

    g_spp_handle.started = 1;
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    cb(PROFILE_SPP, true);

    return BT_STATUS_SUCCESS;
}

static bt_status_t spp_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        cb(PROFILE_SPP, false);
        return BT_STATUS_NOT_ENABLED;
    }

    g_spp_handle.started = 0;
    g_spp_handle.registered = 0;
    g_spp_handle.server_channel_map = 0;
    spp_cleanup_all_apps();
    index_allocator_delete(&g_spp_handle.allocator);
    list_delete(&g_spp_handle.devices);
    list_delete(&g_spp_handle.servers);
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    /* cleanup spp stack */
    bt_sal_spp_cleanup();
    cb(PROFILE_SPP, true);

    return BT_STATUS_SUCCESS;
}

static void spp_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_REMOTE_DETACH: {
        bt_instance_t* ins = msg->data.data;

        if (ins->spp_cookie) {
            BT_LOGD("%s PROFILE_EVT_REMOTE_DETACH", __func__);
            void* handle = NULL;
            spp_unregister_app(&handle, ins->spp_cookie);
            ins->spp_cookie = NULL;
        }
        break;
    }
    default:
        break;
    }
}

static int spp_get_state(void)
{
    return 1;
}

static void* spp_register_app(void* remote, const char* name, const spp_callbacks_t* callbacks)
{
    spp_handle_t* hdl = NULL;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        BT_LOGE("%s, SPP not started", __func__);
        return NULL;
    }

    if (g_spp_handle.registered == REGISTER_MAX) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        BT_LOGE("%s, spp register reach MAX number: %d", __func__, REGISTER_MAX);
        return NULL;
    }

    hdl = zalloc(sizeof(spp_handle_t));
    if (hdl == NULL) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        BT_LOGE("%s, spp handle malloc error", __func__);
        return NULL;
    }

    if (name)
        strlcpy(hdl->name, name, sizeof(hdl->name));
    hdl->ins = NULL;
    hdl->remote = remote;
    hdl->cbs = callbacks;
    g_spp_handle.registered++;
    list_add_tail(&g_spp_handle.apps, &hdl->node);

    pthread_mutex_unlock(&g_spp_handle.spp_lock);

    return hdl;
}

static bt_status_t spp_unregister_app(void** remote, void* handle)
{
    spp_handle_t* app = handle;

    if (!app || !spp_app_is_exist(handle))
        return BT_STATUS_FAIL;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        return BT_STATUS_NOT_ENABLED;
    }

    g_spp_handle.registered--;

    /* TODO: release all port bind on this handle */
    if (remote)
        *remote = app->remote;
    spp_cleanup_app(app);
    list_delete(&app->node);
    free(app);
    pthread_mutex_unlock(&g_spp_handle.spp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t spp_server_start(void* handle, uint16_t scn, bt_uuid_t* uuid, uint8_t max_connection)
{
    bt_uuid_t uuid_128_dst;
    spp_server_t* server;
    bt_status_t ret = BT_STATUS_SUCCESS;

    /* TODO: check handle are valid */
    if (!handle)
        return BT_STATUS_FAIL;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        ret = BT_STATUS_NOT_ENABLED;
        goto unlock_exit;
    }

    /* uuid any to uuid128 */
    bt_uuid_to_uuid128(uuid, &uuid_128_dst);

    /* alloc server */
    server = alloc_new_server(scn, uuid, handle);
    if (!server) {
        ret = BT_STATUS_NO_RESOURCES;
        goto unlock_exit;
    }

    char uuid_str[40] = { 0 };
    bt_uuid_to_string(&uuid_128_dst, uuid_str, 40);
    BT_LOGI("%s, scn:%d, uuid:%s", __func__, scn, uuid_str);
    bt_sal_spp_server_start(STACK_SVR_PORT(scn), &uuid_128_dst, MIN(max_connection, SERVER_CONNECTION_MAX));

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return ret;
}

static bt_status_t spp_server_stop(void* handle, uint16_t scn)
{
    spp_server_t* server;
    bt_status_t ret = BT_STATUS_SUCCESS;

    /* TODO: check handle are valid */
    if (!handle)
        return BT_STATUS_FAIL;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        ret = BT_STATUS_NOT_ENABLED;
        goto unlock_exit;
    }

    server = find_server(scn);
    if (!server) {
        ret = BT_STATUS_FAIL;
        goto unlock_exit;
    }

    bt_sal_spp_server_stop(STACK_SVR_PORT(scn));
    free_server_resource(server);

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return ret;
}

static bt_status_t spp_connect(void* handle, bt_address_t* addr, int16_t scn, bt_uuid_t* uuid, uint16_t* port)
{
    bt_status_t status = BT_STATUS_SUCCESS;
    spp_device_t* device;
    bt_uuid_t uuid_128_dst;

    /* TODO: check handle are valid */
    if (!handle)
        return BT_STATUS_FAIL;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        status = BT_STATUS_NOT_ENABLED;
        goto unlock_exit;
    }

    bt_uuid_to_uuid128(uuid, &uuid_128_dst);
    device = alloc_new_device(addr, scn == UNKNOWN_SERVER_CHANNEL_NUM ? 0 : scn,
        uuid, false, handle);
    if (!device) {
        status = BT_STATUS_NO_RESOURCES;
        goto unlock_exit;
    }

    status = bt_sal_spp_connect(addr, device->conn_port, &uuid_128_dst);
    if (status != BT_STATUS_SUCCESS) {
        // spp_notify_connection_state(device, SPP_CONNECTION_STATE_DISCONNECTED);
        remove_spp_device(device);
        status = BT_STATUS_FAIL;
        goto unlock_exit;
    }

    // todo: start connect timer, release device if timeout
    *port = device->conn_id;
    device->state = PROFILE_STATE_CONNECTING;

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return status;
}

static bt_status_t spp_disconnect(void* handle, bt_address_t* addr, uint16_t port)
{
    spp_device_t* device;
    bt_status_t ret = BT_STATUS_SUCCESS;

    /* TODO: check handle are valid */
    if (!handle)
        return BT_STATUS_FAIL;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    if (!g_spp_handle.started) {
        pthread_mutex_unlock(&g_spp_handle.spp_lock);
        return BT_STATUS_NOT_ENABLED;
    }

    device = find_spp_device_by_conn(port);
    if (device == NULL) {
        ret = BT_STATUS_DEVICE_NOT_FOUND;
        goto unlock_exit;
    }

    if (bt_addr_compare(&device->addr, addr)) {
        ret = BT_STATUS_DEVICE_NOT_FOUND;
        BT_LOGE("%s, addr not match", __func__);
        goto unlock_exit;
    }

    device->state = PROFILE_STATE_DISCONNECTING;
    bt_sal_spp_disconnect(device->conn_port);

unlock_exit:
    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    return ret;
}

static void spp_cleanup(void)
{
    pthread_mutex_destroy(&g_spp_handle.spp_lock);
}

static int spp_dump(void)
{
    spp_device_t* device;
    spp_server_t* server = NULL;
    struct list_node* node;
    int i = 0;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    char uuid_str[40] = { 0 };

    if (!g_spp_handle.started)
        return 0;

    pthread_mutex_lock(&g_spp_handle.spp_lock);
    list_for_every(&g_spp_handle.servers, node)
    {
        i++;
        server = (spp_server_t*)node;
        bt_uuid_to_string(&server->uuid, uuid_str, 40);
        printf("\tServer[%d]: Scn:%d, UUID:%s" PRIx16 "\n", i, server->scn, uuid_str);
    }
    if (i == 0)
        printf("\tNo spp Server found\n");

    i = 0;
    list_for_every(&g_spp_handle.devices, node)
    {
        i++;
        device = (spp_device_t*)node;
        bt_addr_ba2str(&device->addr, addr_str);
        if (server)
            bt_uuid_to_string(&server->uuid, uuid_str, 40);
        printf("\tDevice[%d]: ID:%d, Addr:%s, State:%d, Scn:%d, UUID:%s" PRIx16
               ", MFS:%d, Proxy:[%d,%s], Rx:%" PRIu32 ", Tx:%" PRIu32 "\n",
            i, device->conn_id, addr_str, device->state,
            device->scn, uuid_str, device->mfs, device->conn_port,
            device->proxy_name, device->rx_bytes, device->tx_bytes);
    }

    pthread_mutex_unlock(&g_spp_handle.spp_lock);
    if (i == 0)
        printf("\tNo spp device found\n");

    return 0;
}

static spp_interface_t sppInterface = {
    .size = sizeof(sppInterface),
    .register_app = spp_register_app,
    .unregister_app = spp_unregister_app,
    .server_start = spp_server_start,
    .server_stop = spp_server_stop,
    .connect = spp_connect,
    .disconnect = spp_disconnect,
};

static const void* get_spp_profile_interface(void)
{
    return (void*)&sppInterface;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void spp_on_connection_state_changed(bt_address_t* addr, uint16_t conn_port,
    profile_connection_state_t state)
{
    spp_msg_t* msg = malloc(sizeof(spp_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = STATE_CHANEG;
    msg->state = state;
    msg->port = conn_port;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));

    do_in_service_loop(spp_service_event_process, msg);
}

void spp_on_data_sent(uint16_t conn_port, uint8_t* buffer, uint16_t length,
    uint16_t sent_length)
{
    spp_device_t* device;
    spp_msg_t* msg;

    device = find_spp_device_by_conn(SERVICE_CONN_ID(conn_port));
    if (!device) {
        BT_LOGE("%s port:%d not exist", __func__, conn_port);
        return;
    }

    msg = malloc(sizeof(spp_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = DATA_SENT;
    msg->port = conn_port;
    msg->length = length;
    msg->sent_length = sent_length;
    msg->buffer = buffer;
    memcpy(&msg->addr, &device->addr, sizeof(bt_address_t));

    do_in_service_loop(spp_service_event_process, msg);
}

void spp_on_data_received(bt_address_t* addr, uint16_t conn_port,
    uint8_t* buffer, uint16_t length)
{
    spp_msg_t* msg = malloc(sizeof(spp_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = DATA_RECEIVED;
    msg->port = conn_port;
    msg->length = length;
    msg->buffer = buffer;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));

    do_in_service_loop(spp_service_event_process, msg);
}

void spp_on_server_recieve_connect_request(bt_address_t* addr, uint16_t scn)
{
    spp_msg_t* msg = malloc(sizeof(spp_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = CONN_REQ_RECEIVED;
    msg->port = scn;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));

    do_in_service_loop(spp_service_event_process, msg);
}

void spp_on_connection_mfs_update(uint16_t conn_port, uint16_t mfs)
{
    spp_msg_t* msg = malloc(sizeof(spp_msg_t));
    if (!msg) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    msg->event = UPDATE_MFS;
    msg->port = conn_port;
    msg->length = mfs;

    do_in_service_loop(spp_service_event_process, msg);
}

static const profile_service_t spp_service = {
    .auto_start = true,
    .name = PROFILE_SPP_NAME,
    .id = PROFILE_SPP,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = spp_init,
    .startup = spp_startup,
    .shutdown = spp_shutdown,
    .process_msg = spp_process_msg,
    .get_state = spp_get_state,
    .get_profile_interface = get_spp_profile_interface,
    .cleanup = spp_cleanup,
    .dump = spp_dump,
};

void register_spp_service(void)
{
    register_service(&spp_service);
}
