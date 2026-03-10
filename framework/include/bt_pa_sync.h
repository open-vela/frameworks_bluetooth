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

#define BT_PA_SYNC_SKIP_MAX (0x01F3)
#define BT_PA_SYNC_TIMEOUT_MIN (0x000A)
#define BT_PA_SYNC_TIMEOUT_MAX (0x4000)
#define BT_PA_SYNC_SUBEVENT_NONE (0xFF)
#define BT_PA_SYNC_DATA_LEN_MAX (251)

/**
 * @brief Information about the periodic advertising sync.
 */
typedef struct bt_pa_sync_info {
    /** Present if available: Broadcast_ID from Broadcast Audio Announcement.
     *  Otherwise, `BT_INVALID_BROADCAST_ID` */
    uint32_t broadcast_id;

    /** UTF-8 string of the remote device name */
    char name[BT_REM_NAME_MAX_LEN + 1];

    /** UTF-8 string of the `Broadcast_Name` field in `AdvData` (if present) */
    char broadcast_name[BT_BROADCAST_NAME_MAX_LEN + 1];
} bt_pa_sync_info_t;

/** @brief Periodic advertising report structure */
typedef struct bt_pa_sync_report {
    /** TxPower in dBm (-127 to +20). `BT_POWER_UNAVAILABLE` if unavailable */
    int8_t tx_power; 

    /** RSSI in dBm (-127 to +20). `BT_POWER_UNAVAILABLE` if unavailable */
    int8_t rssi;

    /** Periodic_Event_Counter, the value of `paEventCounter` for the reported periodic advertising
     *  packet */
    uint16_t cnt;

    /** The subevent number, range from 0x00 to 0x7F, `BT_PA_SYNC_SUBEVENT_NONE` if no subevents */
    uint8_t subevent;

    /** Length of `adv_data` */
    uint16_t adv_data_len;

    /** Data received from a Periodic Advertising packet, NULL if `adv_data_len` is zero */
    const uint8_t* data;
} bt_pa_sync_report_t;

typedef struct bt_pa_sync_create_param {
    /** The maximum number of periodic advertising events that can be skipped after a successful
     *  reception. Range from 0x0000 to 0x01F3 */
    uint16_t skip;

    /** Synchronization timeout for the periodic advertising train, measured in 10 ms units.
     *  Range from 0x000A to 0x4000 */
    uint16_t timeout;

    /** `true` to enable duplicate filtering, `false` by default */
    bool filter;

    /** `true` to disable periodic advertising reports, `false` by default  */
    bool no_report;
} bt_pa_sync_create_param_t;

/**
 * @brief Periodic advertising sync established.
 *
 *  * @param addr The Bluetooth address and address type of the remote device
 * @param sid The advertising set id (0x00-0x0F) to identify the periodic advertising
 * @param context User context
 */
typedef void (*on_pa_sync_established_callback)(const bt_le_address_t* addr, uint8_t sid,
    void* context);

/**
 * @brief Periodic advertising terminated.
 *
 * @param addr The Bluetooth address and address type of the remote device
 * @param sid The advertising set id (0x00-0x0F) to identify the periodic advertising
 * @param context User context
 */
typedef void (*on_pa_sync_terminated_callback)(const bt_le_address_t* addr, uint8_t sid,
    void* context);

/**
 * @brief Periodic advertising report.
 *
 * @param addr The Bluetooth address and address type of the remote device
 * @param sid The advertising set id (0x00-0x0F) to identify the periodic advertising
 * @param report The received periodic advertising report, see @ref bt_pa_sync_report_t
 * @param context User context
 */
typedef void (*on_pa_sync_report_callback)(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_report_t* report, void* context);

/**
 * @brief Callback when auracast is ready to received.
 *
 * @param addr The Bluetooth address and address type of the remote device
 * @param sid The advertising set id (0x00-0x0F) to identify the periodic advertising
 * @param encrypted `true` if the auracast stream is encrypted, `false` otherwise
 * @param context User context
 */
typedef void (*on_auracast_ready_callback)(const bt_le_address_t* addr, uint8_t sid, bool encrypted,
     void* context);

typedef struct {
    on_pa_sync_established_callback on_sync_established;
    on_pa_sync_terminated_callback on_sync_terminated;
    on_pa_sync_report_callback on_sync_report;
    on_auracast_ready_callback on_auracast_ready;
} bt_pa_sync_callbacks_t;

/**
 * @brief Parse an advertising report and check if periodic advertising is present.
 *
 * @param[out] info Buffer to store the parsed periodic advertising info
 * @param[in] result Advertising report from the scan result callback
 *
 * @return `BT_STATUS_SUCCESS` if periodic advertising is found.
 * @return `BT_STATUS_NOT_FOUND` if no periodic advertising is found.
 * @return Other error codes on failure.
 */
bt_status_t bt_pa_sync_parse_adv_data(bt_pa_sync_info_t* info, const ble_scan_result_t* result);

/**
 * @brief Synchronize to periodic advertising via extended advertising report.
 *
 * @param[in] ins The Bluetooth instance, see @ref bt_instance_t
 * @param[in] addr The Bluetooth address and address type of the remote device
 * @param[in] sid The advertising set id subfield to identify the periodic advertising, range from
 *                0x00 to 0x0F
 * @param[in] params Optional parameters for periodic sync, set to `NULL` to use default parameters
 * @param[in] cbs Callbacks for periodic advertising sync, see @ref bt_pa_sync_callbacks_t
 * @param[in] context User context
 *
 * @return `BT_STATUS_SUCCESS` on success.
 * @return Other error codes on failure.
 */
bt_status_t BTSYMBOLS(bt_pa_sync_create)(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, const bt_pa_sync_create_param_t* params, const bt_pa_sync_callbacks_t* cbs,
    const void* context);

/**
 * @brief Stop reception of the periodic advertising train.
 *
 * @param[in] ins The Bluetooth instance, see @ref bt_instance_t
 * @param[in] addr The Bluetooth address and address type of the remote device
 * @param[in] sid The advertising set id subfield to identify the periodic advertising, range from
 *                0x00 to 0x0F
 *
 * @return `BT_STATUS_SUCCESS` on success.
 * @return Other error codes on failure.
 */
bt_status_t BTSYMBOLS(bt_pa_sync_terminate)(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid);

#ifdef __cplusplus
}
#endif

#endif
