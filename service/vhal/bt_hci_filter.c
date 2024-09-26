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

#include "bt_hci_filter.h"

#include <debug.h>
#include <nuttx/wireless/bluetooth/bt_hci.h>
#include <nuttx/wireless/bluetooth/bt_ioctl.h>
#include <string.h>

#include "bt_addr.h"
#include "bt_hash.h"
#include "utils/log.h"

#ifndef CONFIG_BT_LE_ADV_REPORT_SIZE
#define CONFIG_BT_LE_ADV_REPORT_SIZE 300
#endif
#define BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE 0x2042

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#undef BT_HCI_FILTER_LOG_ENABLE

enum {
    HCI_TYPE_COMMAND = 1,
    HCI_TYPE_ACL = 2,
    HCI_TYPE_SCO = 3,
    HCI_TYPE_EVENT = 4,
    HCI_TYPE_ISO_DATA = 5
};

typedef struct __attribute__((packed)) {
    uint16_t evt_type;
    bt_addr_le_t addr;
    uint8_t prim_phy;
    uint8_t sec_phy;
    uint8_t sid;
    int8_t tx_power;
    int8_t rssi;
    uint16_t interval;
    bt_addr_le_t direct_addr;
    uint8_t length;
    uint8_t data[0];
} bt_hci_evt_le_ext_advertising_info_t;

static uint32_t g_adv_htable[CONFIG_BT_LE_ADV_REPORT_SIZE];

static bool scan_hsearch_find(const void* keyarg, size_t len)
{
    uint32_t hash;
    int i;

    hash = bt_hash4(keyarg, len);
    for (i = 0; i < ARRAY_SIZE(g_adv_htable); i++) {
        if (g_adv_htable[i] == hash) {
            return true;
        }

        if (g_adv_htable[i] == 0) {
            break;
        }
    }

    return false;
}

static void scan_hsearch_add(const void* keyarg, size_t len)
{
    uint32_t hash;
    int i;

    hash = bt_hash4(keyarg, len);
    for (i = 0; i < ARRAY_SIZE(g_adv_htable); i++) {
        if (g_adv_htable[i] == 0) {
            g_adv_htable[i] = hash;
            break;
        }
    }
}

static void scan_hsearch_free()
{
    BT_LOGD("%s", __func__);
    memset(&g_adv_htable, 0, sizeof(g_adv_htable));
}

static bool le_adv_report_filter(uint8_t* data, uint32_t size)
{
    struct bt_hci_ev_le_advertising_report_s* info;
#ifdef BT_HCI_FILTER_LOG_ENABLE
    bt_addr_le_t* addr;
#endif
    void* adv_data;
    uint8_t adv_len;
    uint8_t num_reports = data[0];

    data += sizeof(num_reports);

    if (num_reports > 1) {
        BT_LOGE("%s, num_reports:%d", __func__, num_reports);
        return false;
    }

    info = (struct bt_hci_ev_le_advertising_report_s*)data;
    adv_data = info->data;
    adv_len = info->length;

#ifdef BT_HCI_FILTER_LOG_ENABLE
    addr = &info->addr;
    lib_dumpbuffer("le_advdata:", adv_data, adv_len);
#endif

    if (scan_hsearch_find(adv_data, adv_len)) {
#ifdef BT_HCI_FILTER_LOG_ENABLE
        BT_LOGD("scan_hsearch_find addr:%s", bt_addr_str((bt_address_t*)addr->val));
#endif
        return true;
    }

    scan_hsearch_add(adv_data, adv_len);
    return false;
}

static bool le_ext_adv_report_filter(uint8_t* data, uint32_t size)
{
    bt_hci_evt_le_ext_advertising_info_t* info;
#ifdef BT_HCI_FILTER_LOG_ENABLE
    bt_addr_le_t* addr;
#endif
    void* adv_data;
    uint8_t adv_len;
    uint8_t num_reports = data[0];

    data += sizeof(num_reports);

    if (num_reports > 1) {
        BT_LOGE("%s, num_reports:%d", __func__, num_reports);
        return false;
    }

    info = (bt_hci_evt_le_ext_advertising_info_t*)data;
    adv_data = info->data;
    adv_len = info->length;

#ifdef BT_HCI_FILTER_LOG_ENABLE
    addr = &info->addr;
    lib_dumpbuffer("leext_advdata:", adv_data, adv_len);
#endif

    if (scan_hsearch_find(adv_data, adv_len)) {
#ifdef BT_HCI_FILTER_LOG_ENABLE
        BT_LOGD("scan_hsearch_find addr:%s", bt_addr_str((bt_address_t*)addr->val));
#endif
        return true;
    }

    scan_hsearch_add(adv_data, adv_len);
    return false;
}

bool bt_hci_filter_can_recv(uint8_t* value, uint32_t size)
{
    if (value[1] == BT_HCI_EVT_LE_META_EVENT && value[3] == BT_HCI_EVT_LE_ADVERTISING_REPORT) {
        return le_adv_report_filter(value + 4, size - 4);
    } else if (value[1] == BT_HCI_EVT_LE_META_EVENT && value[3] == BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT) {
        return le_ext_adv_report_filter(value + 4, size - 4);
    }

    return false;
}

bool bt_hci_filter_can_send(uint8_t* value, uint32_t size)
{
    struct bt_hci_cmd_hdr_s* hdr = (struct bt_hci_cmd_hdr_s*)&value[1];

    if (hdr->opcode == BT_HCI_OP_LE_SET_SCAN_ENABLE || hdr->opcode == BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE) {
        scan_hsearch_free();
    }

    return true;
}