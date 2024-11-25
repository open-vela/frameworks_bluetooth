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
#define LOG_TAG "adv"

#include <stdlib.h>

#include "bluetooth.h"
#include "bt_internal.h"
#include "bt_le_scan.h"
#include "bt_list.h"
#include "scan_manager.h"
#include "utils/log.h"

bt_scanner_t* BTSYMBOLS(bt_le_start_scan)(bt_instance_t* ins, const scanner_callbacks_t* cbs)
{
    return scanner_start_scan(NULL, cbs);
}

bt_scanner_t* BTSYMBOLS(bt_le_start_scan_settings)(bt_instance_t* ins,
    ble_scan_settings_t* settings,
    const scanner_callbacks_t* cbs)
{
    return scanner_start_scan_settings(NULL, settings, cbs);
}

bt_scanner_t* BTSYMBOLS(bt_le_start_scan_with_filters)(bt_instance_t* ins,
    ble_scan_settings_t* settings,
    ble_scan_filter_t* filter,
    const scanner_callbacks_t* cbs)
{
    return scanner_start_scan_with_filters(NULL, settings, filter, cbs);
}

void BTSYMBOLS(bt_le_stop_scan)(bt_instance_t* ins, bt_scanner_t* scanner)
{
    scanner_stop_scan(scanner);
}

bool BTSYMBOLS(bt_le_scan_is_supported)(bt_instance_t* ins)
{
    return scan_is_supported();
}
