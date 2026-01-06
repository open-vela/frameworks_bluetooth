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
#ifndef __BT_PA_SYNC_H__
#define __BT_PA_SYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_le_scan.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief Information about the periodic advertising sync.
 */
typedef struct bt_pa_sync_info {
    /** Advertising Set ID (SID) subfield of the `AdvDataInfo` (ADI) */
    uint8_t sid;

    /** TxPower in dBm (-127 to +20). 0x7F if unavailable */
    int8_t tx_power;

    /** Present if available: Broadcast_ID from Broadcast Audio Announcement.
     *  Otherwise, `BT_INVALID_BROADCAST_ID` */
    uint32_t broadcast_id;

    /** UTF-8 string of the `Broadcast_Name` field in `AdvData` (if present) */
    char broadcast_name[BT_BROADCAST_NAME_MAX_LEN + 1];
} bt_pa_sync_info_t;

/**
 * @brief Parse an advertising report and check if periodic advertising is present.
 *
 * @param[out] info Buffer to store the parsed periodic advertising info
 * @param[in] result Advertising report from the scan result callback
 *
 * @return `BT_STATUS_SUCCESS` if periodic advertising is found
 * @return `BT_STATUS_NOT_FOUND` if no periodic advertising is found
 * @return Other negative `bt_status_t` error codes on failure.
 */
bt_status_t bt_pa_sync_parse_adv_data(bt_pa_sync_info_t* info, const ble_scan_result_t* result);

#ifdef __cplusplus
}
#endif

#endif
