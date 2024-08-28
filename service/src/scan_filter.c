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
#define LOG_TAG "scan_filter"

#include "scan_filter.h"

static bool match_uuid(uint16_t uuid, const uint16_t* uuids)
{
    int i;

    for (i = 0; i < BLE_SCAN_FILTER_UUID_MAX_NUM; i++) {
        if (uuids[i] != 0 && uuid == uuids[i]) {
            return true;
        }
    }

    return false;
}

bool scanner_match_filter(scan_record_t* record, ble_scan_filter_t* filter)
{
    if (record->uuid && match_uuid(record->uuid, filter->uuids)) {
        return true;
    }

    /* TODO: add other filter match */

    return false;
}
