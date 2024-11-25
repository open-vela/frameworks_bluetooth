/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#include "bt_vhal.h"

#include <debug.h>
#include <nuttx/wireless/bluetooth/bt_hci.h>
#include <nuttx/wireless/bluetooth/bt_ioctl.h>
#include <string.h>

#include "bt_addr.h"
#include "bt_hash.h"
#include "bt_hci_filter.h"
#include "utils/log.h"

enum {
    HCI_TYPE_COMMAND = 1,
    HCI_TYPE_ACL = 2,
    HCI_TYPE_SCO = 3,
    HCI_TYPE_EVENT = 4,
    HCI_TYPE_ISO_DATA = 5
};

static int bt_vhal_open(int fd)
{
    return 0;
}

static int bt_vhal_close(int fd)
{
    return 0;
}

static int bt_vhal_send(uint8_t* value, uint32_t size)
{
    switch (*value) {
    case HCI_TYPE_COMMAND: {
        bt_hci_filter_can_send(value, size);
        break;
    }
    default:
        break;
    }

    return 0;
}

static int bt_vhal_recv(uint8_t* value, uint32_t size)
{
    int ret = 0;

    switch (*value) {
    case HCI_TYPE_EVENT: {
        ret = bt_hci_filter_can_recv(value, size);
        break;
    }
    default:
        break;
    }

    return ret;
}

static const bt_vhal_interface g_vhal_interface = {
    .open = bt_vhal_open,
    .close = bt_vhal_close,
    .send = bt_vhal_send,
    .recv = bt_vhal_recv,
};

const void* get_bt_vhal_interface()
{
    return &g_vhal_interface;
}
