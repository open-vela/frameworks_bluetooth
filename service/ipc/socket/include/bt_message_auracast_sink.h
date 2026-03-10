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
#ifndef _BT_MESSAGE_AURACAST_SINK_H__
#define _BT_MESSAGE_AURACAST_SINK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_ipc_code.h"
#include "bt_auracast_sink.h"

#define AURACAST_SINK_SUBCODE_REGISTER_CALLBACKS 1
#define AURACAST_SINK_SUBCODE_UNREGISTER_CALLBACKS 2
#define AURACAST_SINK_SUBCODE_CREATE_SYNC 3
#define AURACAST_SINK_SUBCODE_TERMINATE_SYNC 4

#define BT_IPC_CODE_COMMAND_AURACAST_SINK_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AURACAST_SINK, 0)
#define BT_AURACAST_SINK_REGISTER_CALLBACKS BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AURACAST_SINK, AURACAST_SINK_SUBCODE_REGISTER_CALLBACKS)
#define BT_AURACAST_SINK_UNREGISTER_CALLBACKS BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AURACAST_SINK, AURACAST_SINK_SUBCODE_UNREGISTER_CALLBACKS)
#define BT_AURACAST_SINK_CREATE_SYNC BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AURACAST_SINK, AURACAST_SINK_SUBCODE_CREATE_SYNC)
#define BT_AURACAST_SINK_TERMINATE_SYNC BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AURACAST_SINK, AURACAST_SINK_SUBCODE_TERMINATE_SYNC)
#define BT_IPC_CODE_COMMAND_AURACAST_SINK_END BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AURACAST_SINK, BT_IPC_CODE_SUBCODE_MAX_NUM)

// TODO: Add new BT IPC Code sequentially
#define AURACAST_SINK_SUBCODE_SYNC_ESTABLISHED_CALLBACK 1
#define AURACAST_SINK_SUBCODE_SYNC_TERMINATED_CALLBACK 2

#define BT_IPC_CODE_CALLBACK_AURACAST_SINK_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_AURACAST_SINK, 0)
#define BT_AURACAST_SINK_ON_SYNC_ESTABLISHED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_AURACAST_SINK, AURACAST_SINK_SUBCODE_SYNC_ESTABLISHED_CALLBACK)
#define BT_AURACAST_SINK_ON_SYNC_TERMINATED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_AURACAST_SINK, AURACAST_SINK_SUBCODE_SYNC_TERMINATED_CALLBACK)
#define BT_IPC_CODE_CALLBACK_AURACAST_SINK_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_AURACAST_SINK, BT_IPC_CODE_SUBCODE_MAX_NUM)

typedef union {
    bt_status_t status;
} bt_auracast_sink_result_t;

typedef union {
    struct {
        bt_le_address_t addr;
        uint8_t sid;
        uint32_t bitfield;
        uint8_t encrypted;
        uint8_t broadcast_code[BT_AURACAST_BROADCAST_CODE_LEN];
    } _bt_auracast_sink_create_sync;

    struct {
        bt_le_address_t addr;
        uint8_t sid;
    } _bt_auracast_sink_terminate_sync;
} bt_message_auracast_sink_t;

typedef struct {
    union {
        struct {
            bt_le_address_t addr;
            uint8_t sid;
        } _on_sync_established,
            _on_sync_terminated;
    };
} bt_message_auracast_sink_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_AURACAST_SINK_H__ */
