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

#include "utils/btsnoop_log.h"
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
static uint8_t g_hci_rxbuf[2048];
static uint16_t g_hci_rxlen = 0;

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
    g_hci_rxlen = 0;
    g_tlfd = open(CONFIG_OBELISK_HCI_UART_NAME, O_RDWR | O_BINARY | O_CLOEXEC);
    BT_LOGI("%s: g_tlfd = %d", __func__, g_tlfd);

    return g_tlfd;
}

void bt_sal_hci_transport_recv(void)
{
    int ret;
    uint16_t pkt_len = 0;
    union hci_header {
        struct bt_hci_cmd_hdr_s cmd;
        struct bt_hci_acl_hdr_s acl;
        struct bt_hci_evt_hdr_s evt;
        struct bt_hci_iso_hdr_s iso;
    } * hdr;

    ret = read(g_tlfd, &g_hci_rxbuf[g_hci_rxlen], sizeof(g_hci_rxbuf) - g_hci_rxlen);
    if (ret < 0)
        return;

    g_hci_rxlen += ret;

    while (g_hci_rxlen) {
        hdr = (union hci_header *)&g_hci_rxbuf[1];
        switch (g_hci_rxbuf[0]) {
        case HCI_DATATYPE_EVENT: {
            if (g_hci_rxlen < 1 + sizeof(struct bt_hci_evt_hdr_s))
                return;

            pkt_len = 1 + sizeof(struct bt_hci_evt_hdr_s) + hdr->evt.len;
        } break;
        case HCI_DATATYPE_ACL: {
            if (g_hci_rxlen < 1 + sizeof(struct bt_hci_acl_hdr_s))
                return;

            pkt_len = 1 + sizeof(struct bt_hci_acl_hdr_s) + hdr->acl.len;
        } break;
        case HCI_DATATYPE_ISO_DATA: {
            if (g_hci_rxlen < 1 + sizeof(struct bt_hci_iso_hdr_s))
                return;

            pkt_len = 1 + sizeof(struct bt_hci_iso_hdr_s) + hdr->iso.len;
        } break;
        default:
            return;
        }

        if (g_hci_rxlen < pkt_len)
            return;

        btsnoop_log_capture(1, g_hci_rxbuf, pkt_len);
        service_adapter_gap_receive_hci_packet(g_hci_rxbuf, pkt_len);
        g_hci_rxlen -= pkt_len;
        memmove(g_hci_rxbuf, g_hci_rxbuf + pkt_len, g_hci_rxlen);
    }
}

int bt_sal_hci_send_packet(uint8_t *buf, uint32_t len)
{
    btsnoop_log_capture(0, buf, len);

    return h4_send_data(buf, len);
}

void bt_sal_hci_transport_cleanup(void)
{
    close(g_tlfd);
    g_hci_rxlen = 0;
    g_tlfd = -1;
}
