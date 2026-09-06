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

#ifndef _BT_MESSAGE_HID_HOST_H__
#define _BT_MESSAGE_HID_HOST_H__

#include "bt_addr.h"
#include "bt_ipc_code.h"

/* ---- Command codes ---- */

#define BT_IPC_CODE_COMMAND_HID_HOST_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, 0)

#define HIDH_SUBCODE_REGISTER_CALLBACK 1
#define BT_HID_HOST_REGISTER_CALLBACK BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_REGISTER_CALLBACK)
#define HIDH_SUBCODE_UNREGISTER_CALLBACK 2
#define BT_HID_HOST_UNREGISTER_CALLBACK BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_UNREGISTER_CALLBACK)
#define HIDH_SUBCODE_CONNECT 3
#define BT_HID_HOST_CONNECT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_CONNECT)
#define HIDH_SUBCODE_DISCONNECT 4
#define BT_HID_HOST_DISCONNECT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_DISCONNECT)
#define HIDH_SUBCODE_GET_REPORT 5
#define BT_HID_HOST_GET_REPORT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_GET_REPORT)
#define HIDH_SUBCODE_SET_REPORT 6
#define BT_HID_HOST_SET_REPORT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_SET_REPORT)
#define HIDH_SUBCODE_SET_PROTOCOL 7
#define BT_HID_HOST_SET_PROTOCOL BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_SET_PROTOCOL)
#define HIDH_SUBCODE_SUSPEND 8
#define BT_HID_HOST_SUSPEND BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_SUSPEND)
#define HIDH_SUBCODE_EXIT_SUSPEND 9
#define BT_HID_HOST_EXIT_SUSPEND BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_EXIT_SUSPEND)
#define HIDH_SUBCODE_SET_MODE 10
#define BT_HID_HOST_SET_MODE BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, HIDH_SUBCODE_SET_MODE)

#define BT_IPC_CODE_COMMAND_HID_HOST_END BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_HID_HOST, BT_IPC_CODE_SUBCODE_MAX_NUM)

/* ---- Callback codes ---- */

#define BT_IPC_CODE_CALLBACK_HID_HOST_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, 0)

#define HIDH_CB_SUBCODE_CONNECTION_STATE 1
#define BT_HID_HOST_CONNECTION_STATE BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, HIDH_CB_SUBCODE_CONNECTION_STATE)
#define HIDH_CB_SUBCODE_REPORT_MAP 2
#define BT_HID_HOST_REPORT_MAP BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, HIDH_CB_SUBCODE_REPORT_MAP)
#define HIDH_CB_SUBCODE_INPUT_REPORT 3
#define BT_HID_HOST_INPUT_REPORT BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, HIDH_CB_SUBCODE_INPUT_REPORT)
#define HIDH_CB_SUBCODE_GET_REPORT_RESULT 4
#define BT_HID_HOST_GET_REPORT_RESULT BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, HIDH_CB_SUBCODE_GET_REPORT_RESULT)
#define HIDH_CB_SUBCODE_PNP_ID 5
#define BT_HID_HOST_PNP_ID BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, HIDH_CB_SUBCODE_PNP_ID)
#define HIDH_CB_SUBCODE_BATTERY_LEVEL 6
#define BT_HID_HOST_BATTERY_LEVEL BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, HIDH_CB_SUBCODE_BATTERY_LEVEL)
#define HIDH_CB_SUBCODE_MODE_CHANGED 7
#define BT_HID_HOST_MODE_CHANGED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, HIDH_CB_SUBCODE_MODE_CHANGED)

#define BT_IPC_CODE_CALLBACK_HID_HOST_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_HID_HOST, BT_IPC_CODE_SUBCODE_MAX_NUM)

/* ---- Payload / Result types ---- */

#define BT_HID_HOST_MAX_REPORT_SIZE 256
#define BT_HID_HOST_MAX_REPORT_MAP_SIZE 512

typedef union {
    uint8_t status;
    uint8_t value_bool;
} bt_hid_host_result_t;

typedef union {
    struct {
        bt_address_t addr;
        uint8_t transport;
    } _bt_hid_host_connect;

    struct {
        bt_address_t addr;
    } _bt_hid_host_disconnect, _bt_hid_host_suspend, _bt_hid_host_exit_suspend;

    struct {
        bt_address_t addr;
        uint8_t report_id;
        uint8_t report_type;
    } _bt_hid_host_get_report;

    struct {
        bt_address_t addr;
        uint8_t report_id;
        uint8_t report_type;
        uint16_t len;
        uint8_t data[BT_HID_HOST_MAX_REPORT_SIZE];
    } _bt_hid_host_set_report;

    struct {
        bt_address_t addr;
        uint8_t protocol_mode;
    } _bt_hid_host_set_protocol;

    struct {
        bt_address_t addr;
        uint8_t mode;
        uint8_t level;
    } _bt_hid_host_set_mode;

} bt_message_hid_host_t;

typedef union {
    struct {
        bt_address_t addr;
        uint8_t transport;
        uint8_t state;
    } _connection_state;

    struct {
        bt_address_t addr;
        uint8_t service_index;
        uint16_t len;
        uint8_t data[BT_HID_HOST_MAX_REPORT_MAP_SIZE];
    } _report_map;

    struct {
        bt_address_t addr;
        uint8_t service_index;
        uint8_t report_id;
        uint16_t len;
        uint8_t data[BT_HID_HOST_MAX_REPORT_SIZE];
    } _input_report;

    struct {
        bt_address_t addr;
        uint8_t report_id;
        uint8_t report_type;
        uint16_t len;
        uint8_t data[BT_HID_HOST_MAX_REPORT_SIZE];
    } _get_report_result;

    struct {
        bt_address_t addr;
        uint8_t vid_src;
        uint16_t vid;
        uint16_t pid;
        uint16_t version;
    } _pnp_id;

    struct {
        bt_address_t addr;
        uint8_t bat_index;
        uint8_t level;
    } _battery_level;

    struct {
        bt_address_t addr;
        uint8_t mode;
        int status;
    } _mode_changed;

} bt_message_hid_host_callbacks_t;

#endif /* _BT_MESSAGE_HID_HOST_H__ */
