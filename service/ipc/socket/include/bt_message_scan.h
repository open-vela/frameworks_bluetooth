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
#include "bt_ipc_code.h"
#include "bt_le_scan.h"

    enum {
        BLE_SCAN_SUBCODE_START = 0,
        BLE_SCAN_SUBCODE_BATCH_SCAN_CALLBACK = 1,
        BLE_SCAN_SUBCODE_END = BT_IPC_CODE_SUBCODE_MAX_NUM,
    };

#define BT_IPC_CODE_COMMAND_BLE_SCAN_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_BLE_SCAN, 0)
// TODO: Add new BT IPC Code sequentially
#define BT_IPC_CODE_COMMAND_BLE_SCAN_END BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_BLE_SCAN, BT_IPC_CODE_SUBCODE_MAX_NUM)

#define BT_IPC_CODE_CALLBACK_BLE_SCAN_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_BLE_SCAN, 0)
// TODO: Add new BT IPC Code sequentially
#define BT_LE_ON_BATCH_SCAN_RESULT BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_BLE_SCAN, BLE_SCAN_SUBCODE_BATCH_SCAN_CALLBACK)
#define BT_IPC_CODE_CALLBACK_BLE_SCAN_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_BLE_SCAN, BT_IPC_CODE_SUBCODE_MAX_NUM)

#define MAX_SCAN_RESULTS_PER_PACKET 13
#define MAX_LEGACY_SCAN_RESULTS_LENGTH 31
#define MAX_EXT_SCAN_RESULTS_LENGTH 256
#define SCAN_FLUSH_INTERVAL_MS 150

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t vbool; /* boolean */
        uint8_t pad[2];
        uint64_t remote;
    } bt_scan_result_t;

    typedef struct {
        uint16_t count;
        uint64_t scanner;
        struct {
            ble_scan_result_t result;
            uint8_t adv_data[MAX_LEGACY_SCAN_RESULTS_LENGTH];
        } results[MAX_SCAN_RESULTS_PER_PACKET];
    } bt_message_batch_scan_result_callbacks_t;

    typedef struct {
        uint64_t remote;
        bt_instance_t* ins;
        scanner_callbacks_t* callbacks;
        bt_message_batch_scan_result_callbacks_t scan_result_cache;
        void* flush_ctrl;
    } bt_scan_remote_t;

    typedef union {
        struct {
            uint64_t remote;
            ble_scan_settings_t settings;
        } _bt_le_start_scan,
            _bt_le_stop_scan,
            _bt_le_start_scan_settings;

        struct {
            uint64_t remote;
            ble_scan_settings_t settings;
            ble_scan_filter_t filter;
        } _bt_le_start_scan_with_filters;
    } bt_message_scan_t;

    typedef struct
    {
        struct {
            uint64_t scanner; /* bt_scan_remote_t* */
            ble_scan_result_t result;
            uint8_t adv_data[MAX_EXT_SCAN_RESULTS_LENGTH];
        } _on_scan_result_cb;

        struct {
            uint64_t scanner; /* bt_scan_remote_t* */
            uint32_t status;
        } _on_scan_status_cb;

        struct {
            uint64_t scanner; /* bt_scan_remote_t* */
        } _on_scan_stopped_cb;
    } bt_message_scan_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_SCAN_H__ */
