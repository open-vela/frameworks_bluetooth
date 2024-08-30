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
BT_AVRCP_TARGET_MESSAGE_START,
    BT_AVRCP_TARGET_REGISTER_CALLBACKS,
    BT_AVRCP_TARGET_UNREGISTER_CALLBACKS,
    BT_AVRCP_TARGET_GET_PLAY_STATUS_RESPONSE,
    BT_AVRCP_TARGET_PLAY_STATUS_NOTIFY,
    BT_AVRCP_TARGET_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_AVRCP_TARGET_CALLBACK_START,
    BT_AVRCP_TARGET_ON_CONNECTION_STATE_CHANGED,
    BT_AVRCP_TARGET_ON_GET_PLAY_STATUS_REQUEST,
    BT_AVRCP_TARGET_ON_REGISTER_NOTIFICATION_REQUEST,
    BT_AVRCP_TARGET_ON_PANEL_OPERATION_RECEIVED,
    BT_AVRCP_TARGET_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_AVRCP_TARGET_H__
#define _BT_MESSAGE_AVRCP_TARGET_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_avrcp_target.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t value_bool; /* boolean */
    } bt_avrcp_target_result_t;

    typedef union {
        struct {
            bt_address_t addr;
            uint8_t play_status; /* avrcp_play_status_t */
            uint8_t pad[1];
            uint32_t song_len;
            uint32_t song_pos;
        } _bt_avrcp_target_get_play_status_response;

        struct {
            bt_address_t addr;
            uint8_t play_status; /* avrcp_play_status_t */
        } _bt_avrcp_target_play_status_notify;
    } bt_message_avrcp_target_t;

    typedef union {
        struct {
            bt_address_t addr;
            uint8_t state; /* profile_connection_state_t */
        } _on_connection_state_changed;

        struct {
            bt_address_t addr;
        } _on_get_play_status_request;

        struct {
            bt_address_t addr;
            uint8_t event; /* avrcp_notification_event_t */
            uint8_t pad[1];
            uint32_t interval;
        } _on_register_notification_request;

        struct {
            bt_address_t addr;
            uint8_t op;
            uint8_t state;
        } _on_received_panel_operator_cb;
    } bt_message_avrcp_target_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_AVRCP_TARGET_H__ */
