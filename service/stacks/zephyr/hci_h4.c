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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "service_loop.h"

#include <debug.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth.h>

#define DT_DRV_COMPAT zephyr_bt_hci_ttyHCI

#include "hci_h4.h"
#include "vhal/bt_vhal.h"

#define LOG_TAG "h4"
#include "utils/log.h"

/* Datatype in HCI_TL_RecvData */
enum {
    HCI_DATATYPE_COMMAND = 1,
    HCI_DATATYPE_ACL = 2,
    HCI_DATATYPE_SCO = 3,
    HCI_DATATYPE_EVENT = 4,
    HCI_DATATYPE_ISO_DATA = 5
};

struct h4_data {
    int fd;
    pthread_mutex_t mutex;
    bt_hci_recv_t recv;
    void* hci_data;
};

static const struct device* bt_dev;
static service_poll_t* hci_handle;

static void h4_data_dump(const char* tag, uint8_t type, uint8_t* data, uint32_t len)
{
#ifdef CONFIG_BT_HCI_H4_DEBUG
    struct iovec bufs[2];

    bufs[0].iov_base = &type;
    bufs[0].iov_len = 1;
    bufs[1].iov_base = data;
    bufs[1].iov_len = len;

    lib_dumpvbuffer(tag, bufs, 2);
#endif
}

static int h4_send_data(int fd, uint8_t* buf, int count)
{
    int ret, nwritten = 0;

    while (nwritten != count) {
        ret = write(fd, buf + nwritten, count - nwritten);
        if (ret < 0) {
            if (errno == EAGAIN) {
                usleep(1000);
                continue;
            } else
                return ret;
        }

        nwritten += ret;
    }

    return nwritten;
}

static struct net_buf* get_rx(const uint8_t* buf)
{
    bool discardable = false;
    k_timeout_t timeout = K_FOREVER;

    switch (buf[0]) {
    case BT_HCI_H4_EVT:
        if (buf[1] == BT_HCI_EVT_LE_META_EVENT && (buf[3] == BT_HCI_EVT_LE_ADVERTISING_REPORT)) {
            discardable = true;
            timeout = K_NO_WAIT;
        }

        return bt_buf_get_evt(buf[1], discardable, timeout);
    case BT_HCI_H4_ACL:
        return bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
    case BT_HCI_H4_ISO:
        if (IS_ENABLED(CONFIG_BT_ISO)) {
            return bt_buf_get_rx(BT_BUF_ISO_IN, K_FOREVER);
        }
        break;
    default:
        BT_LOGE("RX unknown packet type: %u", buf[0]);
        break;
    }

    return NULL;
}

static int32_t hci_packet_complete(const uint8_t* buf, uint16_t buf_len)
{
    uint16_t payload_len = 0;
    const uint8_t type = buf[0];
    uint8_t header_len = sizeof(type);
    const uint8_t* hdr = &buf[sizeof(type)];

    switch (type) {
    case BT_HCI_H4_CMD: {
        const struct bt_hci_cmd_hdr* cmd = (const struct bt_hci_cmd_hdr*)hdr;

        if (buf_len < header_len + BT_HCI_CMD_HDR_SIZE) {
            return 0;
        }

        /* Parameter Total Length */
        payload_len = cmd->param_len;
        header_len += BT_HCI_CMD_HDR_SIZE;
        break;
    }
    case BT_HCI_H4_ACL: {
        const struct bt_hci_acl_hdr* acl = (const struct bt_hci_acl_hdr*)hdr;

        if (buf_len < header_len + BT_HCI_ACL_HDR_SIZE) {
            return 0;
        }

        /* Data Total Length */
        payload_len = sys_le16_to_cpu(acl->len);
        header_len += BT_HCI_ACL_HDR_SIZE;
        break;
    }
    case BT_HCI_H4_SCO: {
        const struct bt_hci_sco_hdr* sco = (const struct bt_hci_sco_hdr*)hdr;

        if (buf_len < header_len + BT_HCI_SCO_HDR_SIZE) {
            return 0;
        }

        /* Data_Total_Length */
        payload_len = sco->len;
        header_len += BT_HCI_SCO_HDR_SIZE;
        break;
    }
    case BT_HCI_H4_EVT: {
        const struct bt_hci_evt_hdr* evt = (const struct bt_hci_evt_hdr*)hdr;

        if (buf_len < header_len + BT_HCI_EVT_HDR_SIZE) {
            return 0;
        }

        /* Parameter Total Length */
        payload_len = evt->len;
        header_len += BT_HCI_EVT_HDR_SIZE;
        break;
    }
    case BT_HCI_H4_ISO: {
        const struct bt_hci_iso_hdr* iso = (const struct bt_hci_iso_hdr*)hdr;

        if (buf_len < header_len + BT_HCI_ISO_HDR_SIZE) {
            return 0;
        }

        /* ISO_Data_Load_Length parameter */
        payload_len = bt_iso_hdr_len(sys_le16_to_cpu(iso->len));
        header_len += BT_HCI_ISO_HDR_SIZE;
        break;
    }
    /* If no valid packet type found */
    default:
        BT_LOGE("H4: Unknown packet type 0x%02x", type);
        return -1;
    }

    /* Request more data */
    if (buf_len < header_len + payload_len) {
        return 0;
    }

    return (int32_t)header_len + payload_len;
}

static void bt_sal_hci_transport_recv(void)
{
    struct h4_data* h4 = bt_dev->data;
    static uint8_t frame[1026];
    struct net_buf* buf;
    size_t buf_tailroom;
    size_t buf_add_len;
    ssize_t len;
    const uint8_t* frame_start = frame;
    static ssize_t frame_size = 0;

    len = read(h4->fd, frame + frame_size, sizeof(frame) - frame_size);
    if (len < 0) {
        BT_LOGE("Reading hci failed, errno %d", errno);
        close(h4->fd);
        h4->fd = -1;
        return;
    }

    frame_size += len;

    while (frame_size > 0) {
        const uint8_t* buf_add;
        const uint8_t packet_type = frame_start[0];
        const int32_t decoded_len = hci_packet_complete(frame_start, frame_size);

        if (decoded_len == -1) {
            BT_LOGE("HCI Packet type is invalid, length could not be decoded");
            frame_size = 0; /* Drop buffer */
            break;
        }

        if (decoded_len == 0) {
            if (frame_size == sizeof(frame)) {
                BT_LOGE("HCI Packet is too big for frame");
                frame_size = 0; /* Drop buffer */
                break;
            }
            if (frame_start != frame) {
                memmove(frame, frame_start, frame_size);
            }
            /* Read more */
            break;
        }

        buf_add = frame_start + sizeof(packet_type);
        buf_add_len = decoded_len - sizeof(packet_type);

        buf = get_rx(frame_start);

        frame_size -= decoded_len;
        frame_start += decoded_len;

        if (!buf) {
            BT_LOGD("Discard adv report due to insufficient buf");
            continue;
        }

        buf_tailroom = net_buf_tailroom(buf);
        if (buf_tailroom < buf_add_len) {
            BT_LOGE("Not enough space in buffer %zu/%zu", buf_add_len,
                buf_tailroom);
            net_buf_unref(buf);
            continue;
        }

        net_buf_add_mem(buf, buf_add, buf_add_len);

        h4_data_dump("BT RX", packet_type, buf->data, buf_add_len);
        h4->recv(bt_dev, buf, h4->hci_data);
    }
}

int bt_sal_hci_transport_init(const bt_vhal_interface* vhal)
{
    return 0;
}

void bt_sal_hci_transport_cleanup(void)
{
    struct h4_data* h4 = bt_dev->data;

    close(h4->fd);
    h4->fd = -1;
}

static void hci_remove_recv(void* data)
{
    (void)data;

    BT_LOGD("%s", __func__);
    service_loop_remove_poll(hci_handle);
    hci_handle = NULL;
}

static void hci_poll_recv(service_poll_t* poll, int revent, void* userdata)
{
    (void)poll;
    (void)userdata;

    if (revent & (POLL_ERROR | POLL_DISCONNECT))
        hci_remove_recv(NULL);

    if (revent & POLL_READABLE)
        bt_sal_hci_transport_recv();
}

static int h4_open(const struct device* dev, bt_hci_recv_t recv, void* hci_data)
{
    int fd;
    struct h4_data* h4;

    fd = open(CONFIG_BT_UART_ON_DEV_NAME, O_RDWR | O_BINARY | O_CLOEXEC);
    if (fd < 0) {
        BT_LOGE("H4: Failed to open %s: %d", CONFIG_BT_UART_ON_DEV_NAME, errno);
        return fd;
    }

    h4 = dev->data;
    h4->fd = fd;
    h4->recv = recv;
    h4->hci_data = hci_data;

    bt_dev = dev;
    BT_LOGE("H4: %s opened as fd:%d", CONFIG_BT_UART_ON_DEV_NAME, h4->fd);

    hci_handle = service_loop_poll_fd(h4->fd, POLL_READABLE, hci_poll_recv, NULL);
    if (!hci_handle) {
        BT_LOGD("hci fd:%d add poll failed", h4->fd);
        return -1;
    }

    return 0;
}

static int h4_send(const struct device* dev, struct net_buf* buf)
{
    int len;
    int ret;
    struct h4_data* h4 = bt_dev->data;

    switch (bt_buf_get_type(buf)) {
    case BT_BUF_ACL_OUT:
        net_buf_push_u8(buf, BT_HCI_H4_ACL);
        break;
    case BT_BUF_CMD:
        net_buf_push_u8(buf, BT_HCI_H4_CMD);
        break;
    case BT_BUF_ISO_OUT:
        if (IS_ENABLED(CONFIG_BT_ISO)) {
            net_buf_push_u8(buf, BT_HCI_H4_ISO);
            break;
        }
    default:
        BT_LOGE("Unknown buffer type");
        return -EINVAL;
    }

    h4_data_dump("BT TX", buf->data[0], buf->data, buf->len);

    len = buf->len;
    ret = h4_send_data(h4->fd, buf->data, buf->len);
    if (ret != len) {
        BT_LOGE("H4: Failed to send %u bytes: %d", len, ret);
        ret = -EINVAL;
    }

    net_buf_unref(buf);

    return ret < 0 ? ret : 0;
}

const struct bt_hci_driver_api h4_drv_api = {
    .open = h4_open,
    .send = h4_send,
};

static int h4_init(const struct device* dev)
{
    BT_LOGD("Bluetooth H4 driver");
    return 0;
}

#define DT_HCI_INST(node, inst) DT_CAT(node, inst)

#define H4_DEVICE_INIT(inst)                                                                                  \
    static struct h4_data h4_data_##inst = {                                                                  \
        .fd = -1,                                                                                             \
        .mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP,                                                      \
    };                                                                                                        \
    DEVICE_DT_DEFINE(DT_HCI_INST(DT_DRV_INST(inst), inst), h4_init, NULL, &h4_data_##inst, NULL, POST_KERNEL, \
        CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &h4_drv_api)

H4_DEVICE_INIT(0);
H4_DEVICE_INIT(1);
