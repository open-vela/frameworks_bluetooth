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

#ifndef _BT_MESSAGE_CS_H__
#define _BT_MESSAGE_CS_H__

#include "bt_cs.h"
#include "bt_ipc_code.h"

#define BT_IPC_CODE_COMMAND_CS_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, 0)
// TODO: Add new BT IPC Code sequentially
#define CS_SUBCODE_REGISTER_CALLBACKS 1
#define BT_CS_REGISTER_CALLBACKS BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, CS_SUBCODE_REGISTER_CALLBACKS)
#define CS_SUBCODE_UNREGISTER_CALLBACKS 2
#define BT_CS_UNREGISTER_CALLBACKS BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, CS_SUBCODE_UNREGISTER_CALLBACKS)
#define CS_SUBCODE_START_DISTANCE_MEASUREMENT 3
#define BT_CS_START_DISTANCE_MEASUREMENT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, CS_SUBCODE_START_DISTANCE_MEASUREMENT)
#define CS_SUBCODE_STOP_DISTANCE_MEASUREMENT 4
#define BT_CS_STOP_DISTANCE_MEASUREMENT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, CS_SUBCODE_STOP_DISTANCE_MEASUREMENT)
#define CS_SUBCODE_SET_CONFIG 5
#define BT_CS_SET_CONFIG BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, CS_SUBCODE_SET_CONFIG)

#ifdef CONFIG_BT_CS_RAS_TEST
#define CS_SUBCODE_TEST 6
#define BT_CS_TEST BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, CS_SUBCODE_TEST)
#endif /* CONFIG_BT_CS_RAS_TEST */

#define BT_IPC_CODE_COMMAND_CS_END BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_CS, BT_IPC_CODE_SUBCODE_MAX_NUM)

#define BT_IPC_CODE_CALLBACK_CS_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_CS, 0)
// TODO: Add new BT IPC Code sequentially

#define BT_IPC_CODE_CALLBACK_CS_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_CS, BT_IPC_CODE_SUBCODE_MAX_NUM)

typedef union {
    uint8_t status; /* bt_status_t */
    // uint8_t profile_conn_state; /* profile_connection_state_t */
    // uint8_t value_bool; /* boolean */
} bt_cs_result_t;

typedef union {
    struct {
        bt_distance_measurement_params_t params;
    } _bt_cs_start_distance_measurement;

    struct {
        bt_address_t addr;
        uint8_t method;
        uint8_t timeout_bool; /* boolean */
    } _bt_cs_stop_distance_measurement;

    struct {
        bt_address_t addr;
        bt_cs_set_params_t params;
    } _bt_cs_set_config;

#ifdef CONFIG_BT_CS_RAS_TEST
    struct {
        uint16_t len;
        uint8_t data[MAX_TEST_DATA];
    } _bt_cs_test;
#endif /* CONFIG_BT_CS_RAS_TEST */
} bt_message_cs_t;

typedef union {
    struct {
        bt_address_t addr;
        uint8_t state; /* profile_connection_state_t */
    } _on_connection_state_changed;
} bt_message_cs_callbacks_t;

#endif /* _BT_MESSAGE_CS_H__ */
