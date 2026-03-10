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
#ifndef _BT_MESSAGE_PA_SYNC_H__
#define _BT_MESSAGE_PA_SYNC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_ipc_code.h"
#include "bt_pa_sync.h"

#define PA_SYNC_SUBCODE_CREATE_SYNC 1
#define PA_SYNC_SUBCODE_TERMINATE_SYNC 2

#define BT_IPC_CODE_COMMAND_PA_SYNC_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PA_SYNC, 0)
#define BT_PA_SYNC_CREATE_SYNC BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PA_SYNC, PA_SYNC_SUBCODE_CREATE_SYNC)
#define BT_PA_SYNC_TERMINATE_SYNC BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PA_SYNC, PA_SYNC_SUBCODE_TERMINATE_SYNC)
#define BT_IPC_CODE_COMMAND_PA_SYNC_END BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PA_SYNC, BT_IPC_CODE_SUBCODE_MAX_NUM)

// TODO: Add new BT IPC Code sequentially
#define PA_SYNC_SUBCODE_SYNC_ESTABLISHED_CALLBACK 1
#define PA_SYNC_SUBCODE_SYNC_TERMINATED_CALLBACK 2
#define PA_SYNC_SUBCODE_SYNC_REPORT_CALLBACK 3

#define BT_IPC_CODE_CALLBACK_PA_SYNC_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PA_SYNC, 0)
#define BT_PA_SYNC_ON_SYNC_ESTABLISHED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PA_SYNC, PA_SYNC_SUBCODE_SYNC_ESTABLISHED_CALLBACK)
#define BT_PA_SYNC_ON_SYNC_TERMINATED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PA_SYNC, PA_SYNC_SUBCODE_SYNC_TERMINATED_CALLBACK)
#define BT_PA_SYNC_ON_SYNC_REPORT BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PA_SYNC, PA_SYNC_SUBCODE_SYNC_REPORT_CALLBACK)
#define BT_IPC_CODE_CALLBACK_PA_SYNC_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PA_SYNC, BT_IPC_CODE_SUBCODE_MAX_NUM)

typedef union {
    bt_status_t status;
} bt_pa_sync_result_t;

typedef union {
    struct {
        bt_le_address_t addr;
        uint8_t sid;
        uint8_t have_params; /* true if params is provided */
        bt_pa_sync_create_param_t params;
        uint64_t cbs; /* bt_pa_sync_callbacks_t* */
        uint64_t context; /* void* */
    } _bt_pa_sync_create;

    struct {
        bt_le_address_t addr;
        uint8_t sid;
    } _bt_pa_sync_terminate;
} bt_message_pa_sync_t;

typedef struct {
    uint64_t cbs; /* bt_pa_sync_callbacks_t* */
    uint64_t context; /* void* */
    union {
        struct {
            bt_le_address_t addr;
            uint8_t sid;
            /** TODO: add other parameters, e.g., subevents */
        } _on_sync_established;

        struct {
            bt_le_address_t addr;
            uint8_t sid;
        } _on_sync_terminated;

        struct {
            bt_le_address_t addr;
            uint8_t sid;
            int8_t tx_power;
            int8_t rssi;
            uint16_t cnt;
            uint16_t adv_data_len;
            uint8_t subevent;
            uint8_t pad[1];
            uint8_t data[BT_PA_SYNC_DATA_LEN_MAX];
        } _on_sync_report;
    };
} bt_message_pa_sync_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_PA_SYNC_H__ */
