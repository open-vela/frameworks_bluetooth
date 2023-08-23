/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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

#include "stack_adapter_gap.h"
#include <nuttx/wireless/bluetooth/bt_ioctl.h>

#include "hci_h4.h"

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

static int g_tlfd = -1;

static int h4_recv_data(uint8_t *buf, int count)
{
    int ret, nread = 0;

    while (count != nread) {
        ret = read(g_tlfd, buf + nread, count - nread);
        if (ret < 0) {
            if (ret == -EAGAIN) {
                usleep(1000);
                continue;
            } else
                return ret;
        }

        nread += ret;
    }

    return nread;
}

static int h4_send_data(uint8_t *buf, int count)
{
    int ret, nwritten = 0;

    while (nwritten != count) {
        ret = write(g_tlfd, buf + nwritten, count - nwritten);
        if (ret < 0) {
            if (ret == -EAGAIN) {
                usleep(1000);
                continue;
            } else
                return ret;
        }

        nwritten += ret;
    }

    return nwritten;
}

int bt_sal_hci_transport_init(void)
{
    g_tlfd = open(CONFIG_OBELISK_HCI_UART_NAME, O_RDWR | O_BINARY | O_CLOEXEC);
    BT_LOGI("%s: g_tlfd = %d", __func__, g_tlfd);

    return g_tlfd;
}

void bt_sal_hci_transport_recv(void)
{
    uint8_t data[2048];
    int data_len;
    int hdr_len;
    int ret;
    union hci_header {
        struct bt_hci_cmd_hdr_s cmd;
        struct bt_hci_acl_hdr_s acl;
        struct bt_hci_evt_hdr_s evt;
        struct bt_hci_iso_hdr_s iso;
    } * hdr;

    ret = h4_recv_data(data, 1);
    if (ret != 1)
        return;

    if (data[0] == HCI_DATATYPE_EVENT)
        hdr_len = sizeof(struct bt_hci_evt_hdr_s);
    else if (data[0] == HCI_DATATYPE_ACL)
        hdr_len = sizeof(struct bt_hci_acl_hdr_s);
    else if (data[0] == HCI_DATATYPE_ISO_DATA)
        hdr_len = sizeof(struct bt_hci_iso_hdr_s);
    else
        return;

    ret = h4_recv_data(data + 1, hdr_len);
    if (ret != hdr_len)
        return;

    hdr = (union hci_header *)(data + 1);
    if (data[0] == HCI_DATATYPE_EVENT)
        data_len = hdr->evt.len;
    else if (data[0] == HCI_DATATYPE_ACL)
        data_len = hdr->acl.len;
    else if (data[0] == HCI_DATATYPE_ISO_DATA)
        data_len = hdr->iso.len;
    else
        return;

    ret = h4_recv_data(data + 1 + hdr_len, data_len);
    if (ret != data_len)
        return;

    service_adapter_gap_receive_hci_packet(data, 1 + hdr_len + data_len);
}

int bt_sal_hci_send_packet(uint8_t *buf, uint32_t len)
{
    return h4_send_data(buf, len);
}

void bt_sal_hci_transport_cleanup(void)
{
    close(g_tlfd);
    g_tlfd = -1;
}
