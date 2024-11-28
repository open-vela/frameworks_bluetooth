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
#define LOG_TAG "panu"

#include <fcntl.h>
#include <net/if.h>
#include <nuttx/net/ethernet.h>
#include <nuttx/net/netdev.h>
#include <nuttx/net/tun.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "callbacks_list.h"
#include "netutils/netlib.h"
#include "power_manager.h"
#include "sal_pan_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#define PAN_MAX_CONNECTIONS 1
#define PAN_DEV_NAME "bt-pan"

#define PAN_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, pan_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node conn_list;
    bool enable;
    int tun_fd;
    int tun_packet_size;
    char tun_devname[16];
    int local_role;
    bt_address_t peer_addr;
    service_poll_t* poll_handle;
    pthread_mutex_t pan_lock;
    callbacks_list_t* callbacks;
} pan_global_t;

typedef struct {
    struct list_node node;
    bt_address_t addr;
    uint8_t local_role;
    uint8_t peer_role;
    uint8_t state;
} pan_conn_t;

typedef struct {
    /* role */
    pan_role_t remote_role;
    pan_role_t local_role;
    /* pan connection state */
    profile_connection_state_t state;
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
    bt_address_t addr;
    union {
        pan_conn_evt_t conn_evt;
        pan_data_evt_t data_evt;
    };
} pan_msg_t;

typedef struct eth_hdr {
    uint8_t h_dest[6];
    uint8_t h_src[6];
    short h_proto;
} eth_hdr_t;

static pan_global_t g_pan = { 0 };
static uint8_t* pan_read_buf = NULL;

static pan_conn_t* pan_find_conn(bt_address_t* addr);
static void pan_conn_close(pan_conn_t* conn);
static bool pan_unregister_callbacks(void** remote, void* cookie);

static uint8_t pan_conns(void)
{
    return list_length(&g_pan.conn_list);
}

static pan_conn_t* pan_new_conn(bt_address_t* addr)
{
    pan_conn_t* conn;

    if (pan_conns() == PAN_MAX_CONNECTIONS) {
        BT_LOGD("%s, PAN_MAX_CONNECTIONS", __func__);
        return NULL;
    }

    if (pan_find_conn(addr))
        return NULL;

    conn = malloc(sizeof(pan_conn_t));
    memcpy(&conn->addr, addr, sizeof(bt_address_t));
    list_add_tail(&g_pan.conn_list, &conn->node);

    return conn;
}

static void pan_free_conn(pan_conn_t* conn)
{
    list_delete(&conn->node);
    free(conn);
}

static pan_conn_t* pan_find_conn(bt_address_t* addr)
{
    pan_conn_t* conn;
    struct list_node* node;

    list_for_every(&g_pan.conn_list, node)
    {
        conn = (pan_conn_t*)node;
        if (!memcmp(addr, &conn->addr, sizeof(bt_address_t)))
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
        bt_pm_conn_close(PROFILE_PANU, &conn->addr);
        pan_conn_close(conn);
    }
}

static int pan_tap_bridge_open(const char* devname)
{
    struct ifreq ifr;
    bt_address_t local_addr, ethaddr;
    int errcode;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
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
    adapter_get_address(&local_addr);
    bt_addr_swap(&local_addr, &ethaddr);
    netlib_setmacaddr(ifr.ifr_name, ethaddr.addr);
    netlib_ifup(g_pan.tun_devname);

    bt_addr_ba2str(&ethaddr, addr_str);
    BT_LOGI("Created Tap device: %s, Mac address: %s", ifr.ifr_name, addr_str);

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

static void pan_tap_poll_data(service_poll_t* poll, int revent, void* userdata)
{
    eth_hdr_t ethhdr;

    if (revent & POLL_READABLE) {
        int ret = read(g_pan.tun_fd, pan_read_buf, g_pan.tun_packet_size);
        if (ret > 0) {
            memcpy(&ethhdr, pan_read_buf, sizeof(eth_hdr_t));
            bt_pm_busy(PROFILE_PANU, &g_pan.peer_addr);
            bt_sal_pan_write(&g_pan.peer_addr, ntohs(ethhdr.h_proto),
                ethhdr.h_dest, ethhdr.h_src,
                pan_read_buf + sizeof(eth_hdr_t),
                ret - sizeof(eth_hdr_t));
            bt_pm_idle(PROFILE_PANU, &g_pan.peer_addr);
        }
        return;
    }

    if (revent & POLL_WRITABLE) {
        /* not implemented */
        return;
    }

    BT_LOGE("%s poll disconnected", __func__);
    /* any poll error, need close all pan connection */
    pan_close_all_conn();
}

static int pan_get_tun_packet_size(const char* devname)
{
    int errcode, ret, sockfd;
    struct ifreq ifr = { 0 };

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        BT_LOGE("ERROR: Can't open socket: %d\n", sockfd);
        return -1;
    }

    strlcpy(ifr.ifr_name, devname, IFNAMSIZ);
    ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
    ret = ioctl(sockfd, SIOCGIFMTU, &ifr);
    if (ret < 0) {
        errcode = errno;
        BT_LOGE("ERROR: ioctl SIOCGIFMTU failed: %d\n", errcode);
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return ifr.ifr_mtu - sizeof(eth_hdr_t);
}

static pan_conn_t* pan_new_conn_open(bt_address_t* addr, uint8_t local, uint8_t remote)
{
    pan_conn_t* conn;
    int ret;

    memcpy(&g_pan.peer_addr, addr, sizeof(bt_address_t));
    conn = pan_find_conn(addr);
    if (!conn) {
        conn = pan_new_conn(addr);
        if (!conn)
            goto open_fail;
    }

    conn->local_role = local;
    conn->peer_role = remote;
    conn->state = PROFILE_STATE_CONNECTED;
    if (g_pan.tun_fd < 0) {
        ret = pan_tap_bridge_open(PAN_DEV_NAME);
        if (ret < 0)
            goto open_fail;

        ret = pan_get_tun_packet_size(PAN_DEV_NAME);
        if (ret < 0)
            goto open_fail;

        g_pan.tun_packet_size = ret;
        pan_read_buf = malloc(g_pan.tun_packet_size);
        if (pan_read_buf == NULL) {
            BT_LOGE("%s packet malloc failed", __func__);
            goto open_fail;
        }

        g_pan.poll_handle = service_loop_poll_fd(g_pan.tun_fd,
            POLL_DISCONNECT | POLL_READABLE,
            pan_tap_poll_data, NULL);
        if (!g_pan.poll_handle)
            goto open_fail;

        PAN_CALLBACK_FOREACH(g_pan.callbacks, netif_state_cb, PAN_STATE_ENABLED, g_pan.local_role, g_pan.tun_devname);
    }

    return conn;

open_fail:
    if (conn)
        pan_conn_close(conn);
    else
        bt_sal_pan_disconnect(addr);
    return NULL;
}

static void pan_conn_close(pan_conn_t* conn)
{
    if (conn == NULL)
        return;

    if (conn->state == PROFILE_STATE_CONNECTED)
        bt_sal_pan_disconnect(&conn->addr);

    pan_free_conn(conn);
    if (pan_conns() == 0) {
        if (g_pan.poll_handle) {
            service_loop_remove_poll(g_pan.poll_handle);
            g_pan.poll_handle = NULL;
        }

        if (pan_read_buf) {
            free(pan_read_buf);
            pan_read_buf = NULL;
        }

        if (g_pan.tun_fd) {
            pan_tap_bridge_close();
            PAN_CALLBACK_FOREACH(g_pan.callbacks, netif_state_cb, PAN_STATE_DISABLED, g_pan.local_role, g_pan.tun_devname);
        }
    }
}

static void on_pan_connection_state_changed(bt_address_t* addr, pan_conn_evt_t* evt)
{
    pan_conn_t* conn;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s, addr: %s, remote_role: %d, local_role: %d, state: %d",
        __func__, addr_str, evt->remote_role,
        evt->local_role, evt->state);

    switch (evt->state) {
    case PROFILE_STATE_DISCONNECTED: {
        conn = pan_find_conn(addr);
        pan_conn_close(conn);
        bt_pm_conn_close(PROFILE_PANU, addr);
        break;
    }
    case PROFILE_STATE_CONNECTED:
        bt_pm_conn_open(PROFILE_PANU, addr);
        conn = pan_new_conn_open(addr, evt->local_role, evt->remote_role);
        break;
    case PROFILE_STATE_CONNECTING:
    case PROFILE_STATE_DISCONNECTING:
    default:
        break;
    }

    PAN_CALLBACK_FOREACH(g_pan.callbacks, connection_state_cb, evt->state, addr, evt->local_role, evt->remote_role);
}

static int on_pan_data_incoming(bt_address_t* addr, uint16_t protocol,
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

static void pan_service_event_process(void* data)
{
    pan_msg_t* msg = data;

    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        return;
    }

    switch (msg->evt_id) {
    case CONNECTION_EVT:
        on_pan_connection_state_changed(&msg->addr, &msg->conn_evt);
        break;
    case DATA_IND_EVT: {
        pan_data_evt_t* evt = &msg->data_evt;

        bt_pm_busy(PROFILE_PANU, &msg->addr);
        on_pan_data_incoming(&msg->addr, evt->protocol,
            evt->packet, evt->length);
        bt_pm_idle(PROFILE_PANU, &msg->addr);
        free(evt->packet);
        break;
    }
    default:
        break;
    }
    pthread_mutex_unlock(&g_pan.pan_lock);

    free(data);
}

void pan_on_connection_state_changed(bt_address_t* addr, pan_role_t remote_role,
    pan_role_t local_role, profile_connection_state_t state)
{
    pan_msg_t* pan_msg = (pan_msg_t*)malloc(sizeof(pan_msg_t));
    if (pan_msg == NULL) {
        BT_LOGE("%s malloc failed", __func__);
        return;
    }

    pan_msg->evt_id = CONNECTION_EVT;
    pan_msg->conn_evt.state = state;
    pan_msg->conn_evt.remote_role = remote_role;
    pan_msg->conn_evt.local_role = local_role;
    memcpy(&pan_msg->addr, addr, sizeof(bt_address_t));

    do_in_service_loop(pan_service_event_process, pan_msg);
}

void pan_on_data_received(bt_address_t* addr, uint16_t protocol,
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
    memcpy(&pan_msg->addr, addr, sizeof(bt_address_t));
    pan_msg->data_evt.protocol = protocol;

    /* build eth header */
    memcpy(ethhdr.h_dest, dst_addr, 6);
    memcpy(ethhdr.h_src, src_addr, 6);
    ethhdr.h_proto = htons(protocol);

    /* malloc packet with eth header */
    packet = malloc(g_pan.tun_packet_size + sizeof(ethhdr));
    if (packet == NULL) {
        free(pan_msg);
        BT_LOGE("%s packet malloc failed", __func__);
        return;
    }

    /* copy eth header to packet buffer */
    memcpy(packet, &ethhdr, sizeof(eth_hdr_t));

    /* copy protocol data to packet buffer */
    if (length > g_pan.tun_packet_size) {
        free(packet);
        free(pan_msg);
        BT_LOGE("send eth packet size:%d is exceeded limit!", length);
        return;
    }
    memcpy(packet + sizeof(eth_hdr_t), data, length);

    /* set pan packet */
    pan_msg->data_evt.length = length + sizeof(eth_hdr_t);
    pan_msg->data_evt.packet = packet;

    do_in_service_loop(pan_service_event_process, pan_msg);
}

static bt_status_t pan_init(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_pan.pan_lock, &attr) < 0)
        return BT_STATUS_FAIL;

    g_pan.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    return BT_STATUS_SUCCESS;
}

static void pan_cleanup(void)
{
    bt_callbacks_list_free(g_pan.callbacks);
    g_pan.callbacks = NULL;
    pthread_mutex_destroy(&g_pan.pan_lock);
}

static bt_status_t pan_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_pan.pan_lock);
    if (g_pan.enable) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        cb(PROFILE_PANU, true);
        return BT_STATUS_NOT_ENABLED;
    }

    g_pan.tun_fd = -1;
    g_pan.local_role = PAN_ROLE_PANU;
    list_initialize(&g_pan.conn_list);
    if (bt_sal_pan_init(PAN_MAX_CONNECTIONS, PAN_ROLE_PANU) != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        list_delete(&g_pan.conn_list);
        cb(PROFILE_PANU, false);
        return BT_STATUS_FAIL;
    }

    g_pan.enable = true;
    pthread_mutex_unlock(&g_pan.pan_lock);
    cb(PROFILE_PANU, true);

    return BT_STATUS_SUCCESS;
}

static bt_status_t pan_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        pthread_mutex_unlock(&g_pan.pan_lock);
        cb(PROFILE_PANU, true);
        return BT_STATUS_SUCCESS;
    }

    g_pan.enable = false;
    pan_close_all_conn();
    list_delete(&g_pan.conn_list);
    pthread_mutex_unlock(&g_pan.pan_lock);
    bt_sal_pan_cleanup();
    cb(PROFILE_PANU, true);

    return BT_STATUS_SUCCESS;
}

static void pan_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_REMOTE_DETACH: {
        bt_instance_t* ins = msg->data.data;

        if (ins->panu_cookie) {
            BT_LOGD("%s PROFILE_EVT_REMOTE_DETACH", __func__);
            pan_unregister_callbacks((void**)&ins, ins->panu_cookie);
            ins->panu_cookie = NULL;
        }
        break;
    }
    default:
        break;
    }
}

static int pan_get_state(void)
{
    return 1;
}

static void* pan_register_callbacks(void* remote, const pan_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_pan.callbacks, remote, (void*)callbacks);
}

static bool pan_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_pan.callbacks, remote, cookie);
}

static bt_status_t pan_connect(bt_address_t* addr, uint8_t dst_role, uint8_t src_role)
{
    pan_conn_t* conn;
    bt_status_t status;

    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    conn = pan_new_conn(addr);
    if (!conn) {
        status = BT_STATUS_NO_RESOURCES;
        goto exit;
    }

    status = bt_sal_pan_connect(addr, dst_role, src_role);
    if (status != BT_STATUS_SUCCESS) {
        pan_free_conn(conn);
        goto exit;
    }

    conn->state = PROFILE_STATE_CONNECTING;

exit:
    pthread_mutex_unlock(&g_pan.pan_lock);
    return status;
}

static bt_status_t pan_disconnect(bt_address_t* addr)
{
    pan_conn_t* conn;
    bt_status_t status;

    pthread_mutex_lock(&g_pan.pan_lock);
    if (!g_pan.enable) {
        status = BT_STATUS_NOT_ENABLED;
        goto exit;
    }

    conn = pan_find_conn(addr);
    if (!conn) {
        status = BT_STATUS_DEVICE_NOT_FOUND;
        goto exit;
    }

    status = bt_sal_pan_disconnect(addr);
    if (status != BT_STATUS_SUCCESS)
        goto exit;

    conn->state = PROFILE_STATE_DISCONNECTING;

exit:
    pthread_mutex_unlock(&g_pan.pan_lock);
    return status;
}

static const pan_interface_t panInterface = {
    .size = sizeof(panInterface),
    .register_callbacks = pan_register_callbacks,
    .unregister_callbacks = pan_unregister_callbacks,
    .connect = pan_connect,
    .disconnect = pan_disconnect,
};

static const void* get_pan_profile_interface(void)
{
    return (void*)&panInterface;
}

static int pan_dump(void)
{
    BT_LOGD("%s", __func__);

    return 0;
}

static const profile_service_t pan_service = {
    .auto_start = true,
    .name = PROFILE_PANU_NAME,
    .id = PROFILE_PANU,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = pan_init,
    .startup = pan_startup,
    .shutdown = pan_shutdown,
    .process_msg = pan_process_msg,
    .get_state = pan_get_state,
    .get_profile_interface = get_pan_profile_interface,
    .cleanup = pan_cleanup,
    .dump = pan_dump,
};

void register_pan_service(void)
{
    register_service(&pan_service);
}
