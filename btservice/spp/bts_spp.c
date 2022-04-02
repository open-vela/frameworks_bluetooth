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
#include <nuttx/serial/pty.h>
// nuttx
#include <debug.h>
#include <nuttx/list.h>
#include <nuttx/mm/circbuf.h>
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
#define INDEX_MAX (CONNECTIONS_MAX >> 5)
#define INVALID_FD -1
#define DEFAULT_PACKET_SIZE (255)
#define SEND_FC_EN 1
#define SENDING_BUFS_QUOTA  13
#define CACHE_SEND_TIMEOUT 15
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
    uint8_t started;
    uint32_t conn_id_map[INDEX_MAX];
    uint8_t conn_id_next;
    struct list_node dev_list;
    spp_service_callbacks_t* cbs;
} spp_handle_t;

typedef struct {
    uint16_t length;
    uint8_t *buffer_head;
} cache_buf_t;

typedef struct
{
    struct list_node node;
    euv_pty_t* handle;
    uv_timer_t* timer;
    cache_buf_t cache_buf;
    bool accept;
    bt_address addr;
    uint16_t svr_port;
    uint16_t conn_port;
    uint16_t mfs;
    uint16_t next_to_read;
    int mfd;
    int sfd;
    char pty_name[20];
    uint8_t remaining_quota;
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
        UPDATE_MFS
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
static spp_handle_t g_spp_handle = { .started = 0 };
int g_send_cnt = -1;
int g_app_cnt = -1;
int g_spp_test_cnt = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if 0
static const char* spp_event_to_string(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(SERVER_START_REQ)
        CASE_RETURN_STR(SERVER_STOP_REQ)
        CASE_RETURN_STR(CLIENT_CONNECT_REQ)
        CASE_RETURN_STR(DISCONNECT_REQ)
        CASE_RETURN_STR(CLEANUP)
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

static void calc_trans_speed(const char *stage, int count, struct timespec* ts_start, struct timespec* ts_end)
{
    int consume_ms = 0;

    if ((ts_end->tv_nsec - ts_start->tv_nsec) > 0) {
        consume_ms =  (ts_end->tv_sec - ts_start->tv_sec) * 1000 + \
            (ts_end->tv_nsec - ts_start->tv_nsec)/1000000UL;
    } else {
        consume_ms =  (ts_end->tv_sec - ts_start->tv_sec - 1) * 1000 + \
            (1000000000LL + ts_end->tv_nsec - ts_start->tv_nsec)/1000000UL;
    }

    float seconds = (float)consume_ms / 1000.0f;
    float spd = (float)count / seconds;
    syslog(1, "%s trans bytes:%d, Seconds: %f, Speed: %fKb/s\n", stage, count * 990, seconds, spd);
}

void spp_test_start(int test_cnt)
{
    g_send_cnt = 0;
    g_app_cnt = 0;
    g_spp_test_cnt = test_cnt - 1;
}

static void spp_send_done_log(void)
{
    static struct timespec ts_start1, ts_end1;

    if (g_send_cnt < 0)
        return;

    if (g_send_cnt == 0) {
        clock_gettime(CLOCK_MONOTONIC, &ts_start1);
        g_send_cnt++;
    } else if (g_send_cnt == g_spp_test_cnt) {
        clock_gettime(CLOCK_MONOTONIC, &ts_end1);
        calc_trans_speed("Spp", g_send_cnt, &ts_start1, &ts_end1);
        g_send_cnt = -1;
    } else {
        g_send_cnt++;
    }
}

void spp_app_trans_done_log(void)
{
    static struct timespec ts_start2, ts_end2;

    if (g_app_cnt < 0)
        return;

    if (g_app_cnt == 0) {
        clock_gettime(CLOCK_MONOTONIC, &ts_start2);
        g_app_cnt++;
    } else if (g_app_cnt == g_spp_test_cnt) {
        clock_gettime(CLOCK_MONOTONIC, &ts_end2);
        calc_trans_speed("App", g_app_cnt, &ts_start2, &ts_end2);
        g_app_cnt = -1;
    } else {
        g_app_cnt++;
    }
}

static int alloc_connection_port(uint8_t svr_port, uint16_t* conn_port)
{
    uint8_t conn_id = 0;

    for (; (conn_id < CONNECTIONS_MAX) && ((1 << conn_id) & g_spp_handle.conn_id_map[0]); conn_id++)
        ;
    if (conn_id < CONNECTIONS_MAX) {
        g_spp_handle.conn_id_map[0] |= (1 << conn_id);
        *conn_port = (svr_port + (conn_id << 6));

        return 0;
    }

    return -ENOMEM;
}

static void free_connection_port(uint16_t conn_port)
{
    uint8_t conn_id = conn_port >> 6;

    if (conn_id < CONNECTIONS_MAX) {
        g_spp_handle.conn_id_map[0] &= ~(1 << conn_id);
    }
}

static spp_pty_device_t* alloc_new_device(bt_address addr, uint16_t port, bool accept)
{
    spp_pty_device_t* device = (spp_pty_device_t*)malloc(sizeof(spp_pty_device_t));

    if (device == NULL)
        return NULL;

    memset(device, 0, sizeof(spp_pty_device_t));
    device->svr_port = port;
    if (alloc_connection_port(port, &device->conn_port) < 0) {
        free(device);
        return NULL;
    }
    device->accept = accept;
    device->mfs = DEFAULT_PACKET_SIZE;
    device->mfd = INVALID_FD;
    device->sfd = INVALID_FD;
    device->remaining_quota = SENDING_BUFS_QUOTA;
    device->state = SPP_CONNECTION_STATE_DISCONNECTED;
    memcpy(device->addr, addr, sizeof(device->addr));
    list_add_tail(&g_spp_handle.dev_list, &device->node);

    return device;
}

static spp_pty_device_t* find_pty_device(uint16_t port)
{
    spp_pty_device_t* device;
    struct list_node* node;

    list_for_every(&g_spp_handle.dev_list, node)
    {
        device = (spp_pty_device_t*)node;
        if ((port >> 6) == (device->conn_port >> 6))
            return device;
    }

    BT_LOGW("%s, Device not found for port:%d", __func__, port);
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

    BT_LOGW("Device not found for handle:%p", handle);
    return NULL;
}

static void remove_pty_device(spp_pty_device_t* device)
{
    BT_LOGW("%s, for port:%d", __func__, device->conn_port);
    free_connection_port(device->conn_port);

    list_delete(&device->node);
    free(device);
}

static int open_pty(int *master, char *name)
{
    char buf[64];
    int ret;
    /* Open the pseudo terminal master */
    ret = posix_openpt(O_RDWR);
    if (ret < 0)
        return ret;

    *master = ret;

    /* Configure the pseudo terminal master */

    ret = grantpt(*master);
    if (ret < 0)
        goto err;

    ret = unlockpt(*master);
    if (ret < 0)
        goto err;

    /* Open the pseudo terminal slave */

    ret = ptsname_r(*master, buf, sizeof(buf));
    if (ret < 0)
        goto err;

    if (name != NULL)
        strcpy(name, buf);

    return 0;

err:
  close(*master);
  return ret;
}

static spp_pty_device_t* spp_open_pty_device(spp_pty_device_t* device, uint16_t new_port)
{
    int ret;

    if (new_port != device->conn_port)
        device->conn_port = new_port;

    ret = open_pty(&device->mfd, device->pty_name);
    device->sfd = INVALID_FD;
    if (ret != 0) {
        BT_LOGE("pty create failed");
        goto error;
    }

    device->handle = euv_pty_init(get_service_loop(), device->mfd, UV_TTY_MODE_IO);
    if (!device->handle)
        goto error;

    BT_LOGD("pty create success, name:%s, master:%d, slave:%d",
        device->pty_name, device->mfd, device->sfd);
    return device;
error:
    close(device->mfd);
    remove_pty_device(device);
    return NULL;
}

static void spp_close_pty_device(spp_pty_device_t* device)
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

    remove_pty_device(device);
}

static void spp_close_all_device(void)
{
    spp_pty_device_t* device;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_spp_handle.dev_list, node, tmp)
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

static void euv_alloc_buffer(euv_pty_t* handle, uint8_t** buf, size_t *len)
{
    spp_pty_device_t* device;

	device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL) {
        *len = 0;
        return;
    }

    if (device->cache_buf.length > 0) {
        *len = device->mfs - device->cache_buf.length;
        *buf = device->cache_buf.buffer_head + device->cache_buf.length;
    } else {
        *len = device->mfs;
        *buf = malloc(*len);
    }
}

static void euv_read_complete(euv_pty_t* handle,
    const uint8_t* buf, ssize_t size)
{
    spp_pty_device_t* device;

	device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL)
        return;

    if (size <= 0) {
        if (buf)
            free((void *)buf);

        if (size < 0)
            spp_close_pty_device(device);
        return;
    }
    if (size > 0) {
        spp_dumpbuffer("master read:", buf, size);
        do_spp_write(device, (uint8_t*)buf, size);
    }
}

static void euv_write_complete(euv_pty_t* handle, uint8_t* buf, int status)
{
    spp_pty_device_t* device;

    device = find_pty_device_by_handle(handle);
    if (!device || buf == NULL) {
        if (buf != NULL)
            BT_LOGW("%s, device closed, memory %p leak warning", __func__, buf);
        return;
    }

    service_adapter_spp_data_received_rsp(device->conn_port, buf);
    if (status != 0) {
        spp_close_pty_device(device);
    }
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

    BT_LOGD("%s, addr: %s, port: %d, state: %d", __func__, addr_str(addr), port, state);
    device = find_pty_device(port);
    if (device == NULL || memcmp(addr, device->addr, 6) != 0) {
        BT_LOGE("%s, port or address mismatch", __func__);
        return;
    }

    spp_notify_connection_state(addr, port, state);
    if (state == SPP_CONNECTION_STATE_CONNECTED) {
        BT_LOGD("PERFORMANCE-SPP-BTM-CONNECTED");
        device = spp_open_pty_device(device, port);
        if (device == NULL)
            return;

        device->state = state;
        spp_notify_pty_opened(addr, port, device->pty_name, device->sfd);
    } else if (state == SPP_CONNECTION_STATE_CONNECTING) {
        //update port by stack adapter , user port + peer svr_chnl << 1(sdp)
        if (port != device->conn_port)
            device->conn_port = port;
    } else if (state == SPP_CONNECTION_STATE_DISCONNECTED) {
        device->state = state;
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

    spp_dumpbuffer("master write:", buffer, length);
    ret = euv_pty_write(device->handle, buffer, length, euv_write_complete);
    if (ret != 0) {
        BT_LOGE("Spp write to slave port %d failed", device->mfd);
        spp_close_pty_device(device);
    }
}

static void spp_on_outgoing_complete(uint16_t port, uint8_t* buffer, uint16_t length)
{
#if SEND_FC_EN
    spp_pty_device_t* device;

    free(buffer);
    device = find_pty_device(port);
    if (!device)
        return;

    spp_send_done_log();
    if (!device->remaining_quota && device->handle != NULL) {
        euv_pty_read_start2(device->handle, device->next_to_read, euv_read_complete, euv_alloc_buffer);
    }
    device->remaining_quota++;
#endif
}

static void spp_on_connect_request_received(bt_address addr, uint16_t port)
{
    spp_pty_device_t* device;

    device = alloc_new_device(addr, port, true);
    if (device) {
        BT_LOGD("%s, CONN_REQ_RECEIVED: svr_port:%d, conn_port:%d", __func__, port, device->conn_port);
        service_adapter_spp_send_connection_rsp(addr, device->conn_port, true);
    } else {
        BT_LOGW("%s, device alloc failed, reject connection: svr_port:%d,", __func__, port);
        service_adapter_spp_send_connection_rsp(addr, port, false);
    }
}

static void spp_on_connection_update_mfs(uint16_t port, uint16_t mfs)
{
    int ret;
    spp_pty_device_t* device;

    BT_LOGD("%s, mfs:%d", __func__, mfs);
    device = find_pty_device(port);
    if (!device)
        return;

    device->mfs = mfs;
    device->next_to_read = mfs;
    ret = euv_pty_read_start2(device->handle, device->next_to_read, euv_read_complete, euv_alloc_buffer);
    if (ret != 0) {
        spp_close_pty_device(device);
    }
}

//spp server port must be odd number, range in (3~57) 3,5,7,9...57
static void spp_server_start(uint16_t port, uint16_t uuid)
{
    struct bt_uuid_16 uuid_src;
    struct bt_uuid_128 uuid_128_dst;

    bt_utils_uuid_create((struct bt_uuid*)&uuid_src, (uint8_t*)&uuid, 2);
    uuid_to_uuid128((struct bt_uuid*)&uuid_src, &uuid_128_dst);
    service_adapter_spp_server_open(port, uuid_128_dst.val, SERVER_CONNECTION_MAX);
}

static void spp_server_stop(uint16_t port)
{
    port = (port & 0x3E) + 1;
    service_adapter_spp_server_close(port);
}

static void spp_client_connect(bt_address addr, uint16_t port, uint16_t uuid)
{
    SERVICE_BT_STATUS status;
    spp_pty_device_t* device;
    struct bt_uuid_16 uuid_src;
    struct bt_uuid_128 uuid_128_dst;

    bt_utils_uuid_create((struct bt_uuid*)&uuid_src, (uint8_t*)&uuid, 2);
    uuid_to_uuid128((struct bt_uuid*)&uuid_src, &uuid_128_dst);

    device = alloc_new_device(addr, 0, false);
    if (!device)
        return;

    status = service_adapter_spp_client_open(addr, device->conn_port, uuid_128_dst.val);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        //spp_notify_connection_state(addr, device->conn_port, SPP_CONNECTION_STATE_DISCONNECTED);
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
    g_spp_handle.started = 0;
}

static void spp_service_event_process(spp_msg_t* msg)
{
    if (!msg)
        return;

    //BT_LOGD("%s, event: %s", __func__, spp_event_to_string(msg->event));
    switch (msg->event) {
    case SERVER_START_REQ:
        spp_server_start(msg->port, msg->uuid16);
        break;

    case SERVER_STOP_REQ:
        spp_server_stop(msg->port);
        break;

    case CLIENT_CONNECT_REQ:
        BT_LOGD("PERFORMANCE-SPP-BLUELET-CONNECT_START");
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

    case UPDATE_MFS:
        spp_on_connection_update_mfs(msg->port, msg->length);
        break;

    default:
        break;
    }
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
    spp_send_done_log();
    free(buffer);
#endif
}

static void adp_data_received_callback(BD_ADDR remote_addr, SERVICE_SPP_PORT conn_port, uint8_t* buffer, uint16_t length)
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

    g_spp_handle.cbs = callbacks;
    list_initialize(&g_spp_handle.dev_list);
    memset(&g_spp_handle.conn_id_map, 0, sizeof(g_spp_handle.conn_id_map));
    bts_register_profile_process(BT_PROFILE_SPP_ID, bts_spp_handle_service_msg);

    status = service_adapter_spp_init(&spp_adp_callbacks);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        list_delete(&g_spp_handle.dev_list);
        return BT_RESULT_FAILED;
    }

    g_spp_handle.started = 1;
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_server_start(uint16_t port, uint16_t uuid)
{
    spp_msg_t msg = {0};

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    msg.event = SERVER_START_REQ;
    msg.port = port;
    msg.uuid16 = uuid;
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_server_stop(uint16_t port)
{
    spp_msg_t msg = {0};

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    msg.event = SERVER_STOP_REQ;
    msg.port = port;
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_client_connect(bt_address addr, uint16_t port, uint16_t uuid)
{
    spp_msg_t msg = {0};

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    msg.event = CLIENT_CONNECT_REQ;
    msg.port = port;
    msg.uuid16 = uuid;
    memcpy(msg.addr, addr, sizeof(bt_address));
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_spp_disconnect(bt_address addr, uint16_t port)
{
    spp_msg_t msg = {0};

    if (!g_spp_handle.started)
        return BT_RESULT_FAILED;

    msg.event = DISCONNECT_REQ;
    msg.port = port;
    memcpy(msg.addr, addr, sizeof(bt_address));
    do_in_spp_service(&msg);

    return BT_RESULT_SUCCESS;
}

void bts_spp_cleanup(void)
{
    spp_msg_t msg = {0};

    if (!g_spp_handle.started)
        return;

    msg.event = CLEANUP;
    do_in_spp_service(&msg);
}

void bts_spp_state_dump(void)
{
    spp_pty_device_t* device;
    struct list_node* node;
    int i = 0;

    if (!g_spp_handle.started)
        return;

    list_for_every(&g_spp_handle.dev_list, node)
    {
        i++;
        device = (spp_pty_device_t*)node;
        printf("\tDevice[%d]: addr:%s, state:%d, svr_port:%d, conn_port:%d, mfs: %d, fds:[%d,%d], pty:%s\n", i,
            addr_str(device->addr), device->state, device->svr_port, device->conn_port, device->mfs,
            device->mfd, device->sfd, device->pty_name);
    }
    if (i == 0)
        printf("\tNo spp device found\n");
}
