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

#ifdef __BT_MESSAGE_CODE__
BT_SCAN_MESSAGE_START,
    BT_LE_SCAN_START,
    BT_LE_SCAN_START_SETTINGS,
    BT_LE_SCAN_START_WITH_FILTERS,
    BT_LE_SCAN_STOP,
    BT_LE_SCAN_IS_SUPPORT,
    BT_SCAN_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_SCAN_CALLBACK_START,
    BT_LE_ON_SCAN_RESULT,
    BT_LE_ON_SCAN_START_STATUS,
    BT_LE_ON_SCAN_STOPPED,
    BT_SCAN_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_SCAN_H__
#define _BT_MESSAGE_SCAN_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bluetooth.h"
#include "bt_le_scan.h"

    typedef union {
        bt_status_t status;
        uint32_t remote;
        bool vbool;
    } bt_scan_result_t;

    typedef struct {
        uint32_t remote;
        union {
            bt_instance_t* ins;
            scanner_callbacks_t* callback;
        };
    } bt_scan_remote_t;

    typedef union {
        struct {
            uint32_t remote;
            ble_scan_settings_t settings;
        } _bt_le_start_scan,
            _bt_le_stop_scan,
            _bt_le_start_scan_settings;

        struct {
            uint32_t remote;
            ble_scan_settings_t settings;
            uint8_t filter_data[256];
            uint16_t filter_length;
        } _bt_le_start_scan_with_filters;
    } bt_message_scan_t;

    typedef struct
    {
        struct {
            uint32_t scanner;
            ble_scan_result_t result;
            uint8_t adv_data[256];
        } _on_scan_result_cb;

        struct {
            uint32_t scanner;
            uint32_t status;
        } _on_scan_status_cb;

        struct {
            uint32_t scanner;
        } _on_scan_stopped_cb;
    } bt_message_scan_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_SCAN_H__ */
