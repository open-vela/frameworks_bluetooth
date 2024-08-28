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
#ifndef __BT_SCAN_MANAGER_H__
#define __BT_SCAN_MANAGER_H__

#include <stdio.h>

#include "bt_le_scan.h"

enum scan_state {
    SCAN_STATE_STARTED,
    SCAN_STATE_STOPPED
};

void scan_on_state_changed(uint8_t state);
void scan_on_result_data_update(ble_scan_result_t* result_info, char* adv_data);
bt_scanner_t* scanner_start_scan(void* remote, const scanner_callbacks_t* cbs);
bt_scanner_t* scanner_start_scan_settings(void* remote,
    ble_scan_settings_t* settings,
    const scanner_callbacks_t* cbs);
bt_scanner_t* scanner_start_scan_with_filters(void* remote,
    ble_scan_settings_t* settings,
    ble_scan_filter_t* filter,
    const scanner_callbacks_t* cbs);
void scanner_stop_scan(bt_scanner_t* scanner);
bool scan_is_supported(void);
void scan_manager_init(void);
void scan_manager_cleanup(void);
void scanner_dump(bt_scanner_t* scanner);

#endif /* __BT_SCAN_MANAGER_H__ */
