/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#ifndef __SCAN_MANAGER_INTERNAL_H__
#define __SCAN_MANAGER_INTERNAL_H__

#include "bt_list.h"
#include "scan_manager.h"
#include "service_loop.h"

#ifndef CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM
#define CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM 2
#endif

#ifndef CONFIG_BT_LE_ADV_REPORT_SIZE
#define CONFIG_BT_LE_ADV_REPORT_SIZE 10
#endif

typedef struct scanner {
    struct list_node scanning_node;
    void* remote;
    uint8_t scanner_id;
    bool is_scanning;
    ble_scan_filter_policy_t policy;
    ble_scan_settings_t settings;
    ble_scan_filter_t filter;
    const scanner_callbacks_t* callbacks;

    /** advertising report counters for `BT_SCAN_MODE_LOW_POWER`, `BT_SCAN_MODE_BALANCED`,
     *  and `BT_SCAN_MODE_LOW_LATENCY`, respectively */
    uint32_t result_cnt[3];
} scanner_t;

typedef struct scanner_manager {
    scanner_t* scanner_list[CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM];
    struct list_node scanning_list;
    uint32_t hash_table[CONFIG_BT_LE_ADV_REPORT_SIZE];
    bt_list_t* devices;
    uint8_t scanner_cnt;
    uint8_t curr_scan_mode;
    bool is_scanning;
} scanner_manager_t;

scanner_manager_t* scan_manager_get_interface(void);

#endif /* __SCAN_MANAGER_INTERNAL_H__ */
