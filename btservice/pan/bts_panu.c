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
#include <fcntl.h>
#include <net/if.h>
#include <nuttx/list.h>
#include <nuttx/net/ethernet.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/tun.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "bts_panu.h"
#include "bts_service.h"
#include "netutils/netlib.h"
#include "stack_adapter_gap.h"
#include "stack_adapter_pan.h"
#include "utils/utils.h"
#include "uv.h"

#define LOG_TAG "bts_panu"
#include "utils/log.h"

#define PAN_MAX_CONNECTIONS 1
#define TAP_MAX_PKT_WRITE_LEN (CONFIG_NET_TUN_PKTSIZE - sizeof(eth_hdr_t))
#define PAN_DEV_NAME "bt-pan"

typedef struct {
    struct list_node conn_list;
    bool enable;
    int tun_fd;
    char tun_devname[16];
    int local_role;
    uint8_t peer_addr[6];
    uv_poll_t* poll_handle;
    pthread_mutex_t pan_lock;
    pan_callbacks_t* pan_cbs;
} pan_global_t;

typedef struct {
    struct list_node node;
    bt_address addr;
    bt_address eth_addr;
    uint8_t local_role;
    uint8_t peer_role;
    uint8_t state;
} pan_conn_t;

typedef struct {
    pan_role_t remote_role;
    pan_role_t local_role;
    pan_connection_state_t state;
} pan_conn_evt_t;

typedef struct {
    uint16_t protocol;
    uint8_t* packet;
    uint16_t length;
} pan_data_evt_t;

typedef struct {
    enum {
        CONNECTION_EVT,
        DATA_IND_EVT,
    } evt_id;
    bt_address addr;
    union {
        pan_conn_evt_t conn_evt;
        pan_data_evt_t data_evt;
    };
} pan_msg_t;

typedef struct eth_hdr {
    bt_address h_dest;
    bt_address h_src;
    short h_proto;
} eth_hdr_t;

static pan_global_t g_pan = { 0 };
static uint8_t pan_read_buf[TAP_MAX_PKT_WRITE_LEN];

static pan_conn_t* pan_find_conn(bt_address addr);
static void pan_conn_close(pan_conn_t* conn);

static uint8_t pan_conns(void)
{
    return list_length(&g_pan.conn_list);
}

static pan_conn_t* pan_new_conn(bt_address addr)
{
    pan_conn_t* conn;

    if (pan_conns() == PAN_MAX_CONNECTIONS) {
        BT_LOGD("%s, PAN_MAX_CONNECTIONS", __func__);
        return NULL;
    }

    if (pan_find_conn(addr))
        return NULL;

    conn = malloc(sizeof(pan_conn_t));
    memcpy(conn->addr, addr, 6);
    list_add_tail(&g_pan.conn_list, &conn->node);

    return conn;
}

static void pan_free_conn(pan_conn_t* conn)
{
    list_delete(&conn->node);
    free(conn);
}

static pan_conn_t* pan_find_conn(bt_address addr)
{
    pan_conn_t* conn;
    struct list_node* node;

    list_for_every(&g_pan.conn_list, node)
    {
        conn = (pan_conn_t*)node;
        if (!memcmp(addr, conn->addr, sizeof(bt_address)))
            return conn;
    }

    return NULL;
}

static void pan_close_all_conn(void)
{
    pan_conn_t* conn;
    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_pan.conn_list, node, tmp)
    {
        conn = (pan_conn_t*)node;
        pan_conn_close(conn);
    }
}

static int pan_tap_bridge_open(const char* devname)
{
    struct ifreq ifr;
    uint8_t local_addr[6], ethaddr[6];
    int errcode;
    int ret;

    g_pan.tun_fd = open("/dev/tun", O_RDWR | O_CLOEXEC);
    if (g_pan.tun_fd < 0) {
        errcode = errno;
        BT_LOGE("ERROR: Failed to open /dev/tun: %d\n", errcode);
        return -errcode;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strlcpy(ifr.ifr_name, devname, IFNAMSIZ);
    ret = ioctl(g_pan.tun_fd, TUNSETIFF, (unsigned long)&ifr);
    if (ret < 0) {
        errcode = errno;
        BT_LOGE("ERROR: ioctl TUNSETIFF failed: %d\n", errcode);
        close(g_pan.tun_fd);
        return -errcode;
    }

    memset(g_pan.tun_devname, 0, sizeof(g_pan.tun_devname));
    strncpy(g_pan.tun_devname, ifr.ifr_name, IFNAMSIZ);
    service_adapter_gap_get_local_address(local_addr);
    ethaddr[0] = local_addr[5];
    ethaddr[1] = local_addr[4];
    ethaddr[2] = local_addr[3];
    ethaddr[3] = local_addr[2];
    ethaddr[4] = local_addr[1];
    ethaddr[5] = local_addr[0];
    netlib_setmacaddr(ifr.ifr_name, ethaddr);
    netlib_ifup(g_pan.tun_devname);
    BT_LOGI("Created Tap device: %s, Mac address: %s", ifr.ifr_name, addr_str(ethaddr));

    return 0;
}

static void pan_tap_bridge_close(void)
{
    if (g_pan.tun_fd) {
        BT_LOGD("TUN device: %s Closing", g_pan.tun_devname);
        netlib_ifdown(g_pan.tun_devname);
        close(g_pan.tun_fd);
        g_pan.tun_fd = -1;
    }
}

void pan_tap_poll_data(uv_poll_t* handle, int status, int events)
{
    eth_hdr_t ethhdr;

    if (events & UV_READABLE) {
        if (status == 0) {
            int ret = read(g_pan.tun_fd, pan_read_buf, TAP_MAX_PKT_WRITE_LEN);
            if (ret > 0) {
                memcpy(&ethhdr, pan_read_buf, sizeof(eth_hdr_t));
                service_adapter_pan_write(g_pan.peer_addr, ntohs(ethhdr.h_proto),
                    ethhdr.h_dest, ethhdr.h_src,
                    pan_read_buf + sizeof(eth_hdr_t),
                    ret - sizeof(eth_hdr_t));
            }
            return;
        } else {
            BT_LOGE("%s poll status:%d", __func__, status);
            return;
        }
    }

    if (events & UV_WRITABLE) {
        /* not implemented */
        return;
    }

    BT_LOGE("%s poll disconnected", __func__);
    /* any poll error, need close all pan connection */
    pan_close_all_conn();
}

static pan_conn_t* pan_new_conn_open(bt_address addr, uint8_t local, uint8_t remote)
{
    pan_conn_t* conn;
    int ret;

    memcpy(g_pan.peer_addr, addr, 6);
    conn = pan_find_conn(addr);
    if (!conn) {
        conn = pan_new_conn(addr);
        if (!conn)
            goto open_fail;
    }

    conn->local_role = local;
    conn->peer_role = remote;
    conn->state = PAN_STATE_CONNECTED;
    if (g_pan.tun_fd < 0) {
        ret = pan_tap_bridge_open(PAN_DEV_NAME);
        if (ret < 0)
            goto open_fail;
        g_pan.poll_handle = bts_uv_poll_start(g_pan.tun_fd,
            UV_DISCONNECT | UV_READABLE,
            pan_tap_poll_data, NULL);
        if (!g_pan.poll_handle)
            goto open_fail;
        if (g_pan.pan_cbs)
            g_pan.pan_cbs->netif_state_cb(PAN_STATE_ENABLED, g_pan.local_role, g_pan.tun_devname);
    }

    return conn;

open_fail:
    if (conn)
        pan_conn_close(conn);
    else
        service_adapter_pan_disconnect(addr);
    return NULL;
}

static void pan_conn_close(pan_conn_t* conn)
{
    if (conn == NULL)
        return;

    if (conn->state == PAN_STATE_CONNECTED)
        service_adapter_pan_disconnect(conn->addr);

    pan_free_conn(conn);
    if (pan_conns() == 0) {
        if (g_pan.poll_handle) {
            bts_uv_poll_stop(g_pan.poll_handle);
            g_pan.poll_handle = NULL;
        }

        if (g_pan.tun_fd) {
            pan_tap_bridge_close();
            if (g_pan.pan_cbs)
                g_pan.pan_cbs->netif_state_cb(PAN_STATE_DISABLED, g_pan.local_role, g_pan.tun_devname);
        }
    }
}

static void on_pan_connection_state_changed(bt_address addr, pan_conn_evt_t* evt)
{
    pan_conn_t* conn;

    BT_LOGD("%s, addr: %s, remote_role: %d, local_role: %d, state: %d",
        __func__, addr_str(addr), evt->remote_role,
        evt->local_role, evt->state);
    if (g_pan.pan_cbs)
        g_pan.pan_cbs->connection_state_cb(evt->state, addr, evt->local_role, evt->remote_role);

    switch (evt->state) {
    case SERVICE_PROFILE_DISCONNECTED: {
        conn = pan_find_conn(addr);
        pan_conn_close(conn);
        break;
    }
    case SERVICE_PROFILE_CONNECTED:
        conn = pan_new_conn_open(addr, evt->local_role, evt->remote_role);
        break;
    case SERVICE_PROFILE_CONNECTING:
    case SERVICE_PROFILE_DISCONNECTING:
    default:
        break;
    }
}

static int on_pan_data_incoming(bt_address remote_addr, uint16_t protocol,
    uint8_t* packet, uint16_t length)
{
    if (g_pan.tun_fd > 0) {
        /* Send data to network interface */
        ssize_t ret;
        do {
            ret = write(g_pan.tun_fd, packet, length);
        } while (ret == -1 && errno == EINTR);

        return (int)ret;
    }

    return -1;
}

void pan_service_event_process(pan_msg_t* msg)
{
    switch (msg->evt_id) {
    case CONNECTION_EVT:
        on_pan_connection_state_changed(msg->addr, &msg->conn_evt);
        break;
    case DATA_IND_EVT: {
        pan_data_evt_t* evt = &msg->data_evt;
        on_pan_data_incoming(msg->addr, evt->protocol,
            evt->packet, evt->length);
        free(evt->packet);
        break;
    }
    default:
        break;
    }
}

static void bts_pan_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    if (data == NULL)
        return;

    pan_service_event_process((pan_msg_t*)data);
    free(data);
}

static void adp_pan_connection_state_cb(BD_ADDR remote_addr,
    SERVICE_PAN_ROLE_TYPE remote_role,
    SERVICE_PAN_ROLE_TYPE local_role,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    pan_msg_t* pan_msg = (pan_msg_t*)malloc(sizeof(pan_msg_t));
    if (pan_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    pan_msg->conn_evt.state = PAN_STATE_DISCONNECTED;
    switch (state) {
    case SERVICE_PROFILE_CONNECTING:
        pan_msg->conn_evt.state = PAN_STATE_CONNECTING;
        break;
    case SERVICE_PROFILE_CONNECTED:
        pan_msg->conn_evt.state = PAN_STATE_CONNECTED;
        break;
    case SERVICE_PROFILE_DISCONNECTING:
        pan_msg->conn_evt.state = PAN_STATE_DISCONNECTING;
        break;
    default:
        break;
    }
    pan_msg->evt_id = CONNECTION_EVT;
    pan_msg->conn_evt.remote_role = remote_role;
    pan_msg->conn_evt.local_role = local_role;
    memcpy(pan_msg->addr, remote_addr, 6);

    bts_send_uv_msg(BT_PROFILE_PAN_ID, pan_msg, sizeof(pan_msg_t));
}

static void adp_pan_data_received_cb(BD_ADDR remote_addr, uint16_t protocol,
    uint8_t* dst_addr, uint8_t* src_addr,
    uint8_t* data, uint16_t length)
{
    pan_msg_t* pan_msg;
    eth_hdr_t ethhdr;
    uint8_t* packet;

    pan_msg = (pan_msg_t*)malloc(sizeof(pan_msg_t));
    if (pan_msg == NULL) {
        BT_LOGE("%s msg malloc failed", __func__);
        return;
    }
    /* fill pan message */
    pan_msg->evt_id = DATA_IND_EVT;
    memcpy(pan_msg->addr, remote_addr, 6);
    pan_msg->data_evt.protocol = protocol;

    /* build eth header */
    memcpy(ethhdr.h_dest, dst_addr, 6);
    memcpy(ethhdr.h_src, src_addr, 6);
    ethhdr.h_proto = htons(protocol);

    /* malloc packet with eth header */
    packet = malloc(TAP_MAX_PKT_WRITE_LEN + sizeof(ethhdr));
    if (packet == NULL) {
        free(pan_msg);
        BT_LOGE("%s packet malloc failed", __func__);
        return;
    }
    /* copy eth header to packet buffer */
    memcpy(packet, &ethhdr, sizeof(eth_hdr_t));

    /* copy protocol data to packet buffer */
    if (length > TAP_MAX_PKT_WRITE_LEN) {
        free(packet);
        free(pan_msg);
        BT_LOGE("send eth packet size:%d is exceeded limit!", length);
        return;
    }
    memcpy(packet + sizeof(eth_hdr_t), data, length);

    /* set pan packet */
    pan_msg->data_evt.length = length + sizeof(eth_hdr_t);
    pan_msg->data_evt.packet = packet;

    bts_send_uv_msg(BT_PROFILE_PAN_ID, pan_msg, sizeof(pan_msg_t));
}

static const PAN_CALLBACKS_S pan_adp_callbacks = {
    sizeof(PAN_CALLBACKS_S),
    .pan_connection_state_cb = adp_pan_connection_state_cb,
    .pan_data_received_cb = adp_pan_data_received_cb,
    .pan_protocol_filter_cb = NULL,
    .pan_multicast_filter_cb = NULL,
};

bt_result_code bts_pan_init(pan_callbacks_t* callbacks)
{
    SERVICE_BT_STATUS status;

    if (g_pan.enable)
        return BT_RESULT_SUCCESS;

    if (pthread_mutex_init(&g_pan.pan_lock, NULL) < 0)
        return BT_RESULT_FAILED;

    g_pan.tun_fd = -1;
    g_pan.local_role = PAN_ROLE_PANU;
    g_pan.pan_cbs = callbacks;
    list_initialize(&g_pan.conn_list);
    status = service_adapter_pan_init(PAN_MAX_CONNECTIONS, SERVICE_PAN_ROLE_PANU,
        (PAN_CALLBACKS_S*)&pan_adp_callbacks);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        pthread_mutex_destroy(&g_pan.pan_lock);
        list_delete(&g_pan.conn_list);
        return BT_RESULT_FAILED;
    }
    g_pan.enable = true;
    bts_register_profile_process(BT_PROFILE_PAN_ID, bts_pan_handle_service_msg);

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_pan_connect(bt_address addr, uint8_t dst_role, uint8_t src_role)
{
    pan_conn_t* conn;
    SERVICE_BT_STATUS status;
    bt_result_code ret = BT_RESULT_FAILED;

    if (!g_pan.enable)
        return ret;

    pthread_mutex_lock(&g_pan.pan_lock);
    conn = pan_new_conn(addr);
    if (conn == NULL)
        goto exit;

    status = service_adapter_pan_connect(addr, dst_role, src_role);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        pan_free_conn(conn);
        goto exit;
    }
    conn->state = PAN_STATE_CONNECTING;
    ret = BT_RESULT_SUCCESS;
exit:
    pthread_mutex_unlock(&g_pan.pan_lock);
    return ret;
}

bt_result_code bts_pan_disconnect(bt_address addr)
{
    pan_conn_t* conn;
    SERVICE_BT_STATUS status;
    bt_result_code ret = BT_RESULT_FAILED;

    if (!g_pan.enable)
        return ret;

    pthread_mutex_lock(&g_pan.pan_lock);
    conn = pan_find_conn(addr);
    if (!conn)
        goto exit;

    status = service_adapter_pan_disconnect(addr);
    if (status != SERVICE_BT_STATUS_SUCCESS)
        goto exit;
    conn->state = PAN_STATE_DISCONNECTING;
    ret = BT_RESULT_SUCCESS;
exit:
    pthread_mutex_unlock(&g_pan.pan_lock);
    return ret;
}

void bts_pan_cleanup(void)
{
    if (!g_pan.enable)
        return;

    g_pan.enable = false;
    pthread_mutex_lock(&g_pan.pan_lock);
    pan_close_all_conn();
    list_delete(&g_pan.conn_list);
    pthread_mutex_unlock(&g_pan.pan_lock);
    pthread_mutex_destroy(&g_pan.pan_lock);
    bts_unregister_profile_process(BT_PROFILE_PAN_ID);
    service_adapter_pan_cleanup();
}
