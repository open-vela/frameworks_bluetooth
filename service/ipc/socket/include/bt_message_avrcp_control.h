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

#ifdef __BT_MESSAGE_CODE__
BT_AVRCP_CONTROL_MESSAGE_START,
    BT_AVRCP_CONTROL_REGISTER_CALLBACKS,
    BT_AVRCP_CONTROL_UNREGISTER_CALLBACKS,
    BT_AVRCP_CONTROL_GET_ELEMENT_ATTRIBUTES,
    BT_AVRCP_CONTROL_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_AVRCP_CONTROL_CALLBACK_START,
    BT_AVRCP_CONTROL_ON_CONNECTION_STATE_CHANGED,
    BT_AVRCP_CONTROL_ON_GET_ELEMENT_ATTRIBUTES_REQUEST,
    BT_AVRCP_CONTROL_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_AVRCP_CONTROL_H__
#define _BT_MESSAGE_AVRCP_CONTROL_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_avrcp_control.h"
#include "bt_ipc_code.h"

#define BT_IPC_CODE_COMMAND_AVRCP_CT_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AVRCP_CT, 0)
// TODO: Add new BT IPC Code sequentially
#define AVRCP_CT_SUBCODE_SEND_PASSTHROUGH_CMD 1
#define BT_AVRCP_CT_SEND_PASSTHROUGH_CMD BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AVRCP_CT, AVRCP_CT_SUBCODE_SEND_PASSTHROUGH_CMD)
#define AVRCP_CT_SUBCODE_GET_UNIT_INFO_CMD 2
#define BT_AVRCP_CT_GET_UNIT_INFO_CMD BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AVRCP_CT, AVRCP_CT_SUBCODE_GET_UNIT_INFO_CMD)
#define AVRCP_CT_SUBCODE_GET_SUBUNIT_INFO_CMD 3
#define BT_AVRCP_CT_GET_SUBUNIT_INFO_CMD BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AVRCP_CT, AVRCP_CT_SUBCODE_GET_SUBUNIT_INFO_CMD)
#define AVRCP_CT_SUBCODE_GET_PLAYBACK_STATE_CMD 4
#define BT_AVRCP_CT_GET_PLAYBACK_STATE_CMD BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AVRCP_CT, AVRCP_CT_SUBCODE_GET_PLAYBACK_STATE_CMD)
#define AVRCP_CT_SUBCODE_REGISTER_NOTIFICATION_CMD 5
#define BT_AVRCP_CT_REGISTER_NOTIFICATION_CMD BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AVRCP_CT, AVRCP_CT_SUBCODE_REGISTER_NOTIFICATION_CMD)
#define BT_IPC_CODE_COMMAND_AVRCP_CT_END BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_AVRCP_CT, BT_IPC_CODE_SUBCODE_MAX_NUM)

#define BT_IPC_CODE_CALLBACK_AVRCP_CT_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_AVRCP_CT, 0)
// TODO: Add new BT IPC Code sequentially
#define BT_IPC_CODE_CALLBACK_AVRCP_CT_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_AVRCP_CT, BT_IPC_CODE_SUBCODE_MAX_NUM)

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t value_bool; /* boolean */
    } bt_avrcp_control_result_t;

    typedef union {
        struct {
            bt_address_t addr;
        } _bt_avrcp_control_get_element_attribute;

        struct {
            bt_address_t addr;
            uint8_t cmd;
            uint8_t state;
        } _bt_avrcp_control_send_passthrough_cmd;

        struct {
            bt_address_t addr;
        } _bt_avrcp_control_get_unit_info,
            _bt_avrcp_control_get_subunit_info,
            _bt_avrcp_control_get_playback_state;

        struct {
            bt_address_t addr;
            uint8_t event;
            uint8_t pad[1];
            uint32_t interval;
        } _bt_avrcp_control_register_notification;
    } bt_message_avrcp_control_t;

    typedef union {
        struct {
            bt_address_t addr;
            uint8_t state; /* profile_connection_state_t */
        } _on_connection_state_changed;

        struct {
            bt_address_t addr;
            uint8_t attrs_count;
            uint32_t types[AVRCP_MAX_ATTR_COUNT];
            uint16_t chr_sets[AVRCP_MAX_ATTR_COUNT];
            avrcp_socket_element_attr_val_t values;
        } _bt_avrcp_control_get_element_attribute;
    } bt_message_avrcp_control_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_AVRCP_CONTROL_H__ */
