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
BT_HFP_AG_MESSAGE_START,
    BT_HFP_AG_REGISTER_CALLBACK,
    BT_HFP_AG_UNREGISTER_CALLBACK,
    BT_HFP_AG_IS_CONNECTED,
    BT_HFP_AG_IS_AUDIO_CONNECTED,
    BT_HFP_AG_GET_CONNECTION_STATE,
    BT_HFP_AG_CONNECT,
    BT_HFP_AG_DISCONNECT,
    BT_HFP_AG_CONNECT_AUDIO,
    BT_HFP_AG_DISCONNECT_AUDIO,
    BT_HFP_AG_START_VIRTUAL_CALL,
    BT_HFP_AG_STOP_VIRTUAL_CALL,
    BT_HFP_AG_START_VOICE_RECOGNITION,
    BT_HFP_AG_STOP_VOICE_RECOGNITION,
    BT_HFP_AG_PHONE_STATE_CHANGE,
    BT_HFP_AG_NOTIFY_DEVICE_STATUS,
    BT_HFP_AG_VOLUME_CONTROL,
    BT_HFP_AG_SEND_AT_COMMAND,
    BT_HFP_AG_SEND_VENDOR_SPECIFIC_AT_COMMAND,
    BT_HFP_AG_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_HFP_AG_CALLBACK_START,
    BT_HFP_AG_ON_CONNECTION_STATE_CHANGED,
    BT_HFP_AG_ON_AUDIO_STATE_CHANGED,
    BT_HFP_AG_ON_VOICE_RECOGNITION_STATE_CHANGED,
    BT_HFP_AG_ON_BATTERY_LEVEL_CHANGED,
    BT_HFP_AG_ON_VOLUME_CONTROL,
    BT_HFP_AG_ON_ANSWER_CALL,
    BT_HFP_AG_ON_REJECT_CALL,
    BT_HFP_AG_ON_HANGUP_CALL,
    BT_HFP_AG_ON_DIAL_CALL,
    BT_HFP_AG_ON_AT_COMMAND_RECEIVED,
    BT_HFP_AG_ON_VENDOR_SPECIFIC_AT_COMMAND_RECEIVED,
    BT_HFP_AG_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_HFP_AG_H__
#define _BT_MESSAGE_HFP_AG_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_hfp_ag.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t profile_conn_state; /* profile_connection_state_t */
        uint8_t value_bool; /* boolean */
    } bt_hfp_ag_result_t;

    typedef union {
        struct {
            bt_address_t addr;
        } _bt_hfp_ag_is_connected,
            _bt_hfp_ag_is_audio_connected,
            _bt_hfp_ag_get_connection_state,
            _bt_hfp_ag_connect,
            _bt_hfp_ag_disconnect,
            _bt_hfp_ag_connect_audio,
            _bt_hfp_ag_disconnect_audio,
            _bt_hfp_ag_start_virtual_call,
            _bt_hfp_ag_stop_virtual_call,
            _bt_hfp_ag_start_voice_recognition,
            _bt_hfp_ag_stop_voice_recognition;

        struct {
            bt_address_t addr;
            uint8_t num_active;
            uint8_t num_held;
            uint8_t call_state; /* hfp_ag_call_state_t */
            uint8_t type; /* hfp_call_addrtype_t */
            uint8_t pad0[2];
            char number[HFP_PHONENUM_DIGITS_MAX + 1];
            uint8_t pad1[(HFP_PHONENUM_DIGITS_MAX + 1 + 3) / 4 * 4 - (HFP_PHONENUM_DIGITS_MAX + 1)];
            char name[HFP_NAME_DIGITS_MAX + 1];
        } _bt_hfp_ag_phone_state_change;

        struct {
            bt_address_t addr;
            uint8_t network; /* hfp_network_state_t */
            uint8_t roam; /* hfp_roaming_state_t */
            uint8_t signal;
            uint8_t battery;
        } _bt_hfp_ag_notify_device_status;

        struct {
            bt_address_t addr;
            uint8_t type; /* hfp_volume_type_t */
            uint8_t volume;
        } _bt_hfp_ag_volume_control;

        struct {
            bt_address_t addr;
            uint8_t pad[2];
            char cmd[HFP_AT_LEN_MAX + 1];
        } _bt_hfp_ag_send_at_cmd;

        struct {
            bt_address_t addr;
            uint8_t pad[2];
            char cmd[HFP_COMPANY_PREFIX_LEN_MAX + 1];
            uint8_t pad1[(HFP_COMPANY_PREFIX_LEN_MAX + 1 + 3) / 4 * 4 - (HFP_COMPANY_PREFIX_LEN_MAX + 1)];
            char value[HFP_AT_LEN_MAX + 1];
        } _bt_hfp_ag_send_vendor_specific_at_cmd;
    } bt_message_hfp_ag_t;

    typedef union {
        struct {
            bt_address_t addr;
        } _on_answer_call,
            _on_reject_call,
            _on_hangup_call;

        struct {
            bt_address_t addr;
            uint8_t state; /* profile_connection_state_t */
        } _on_connection_state_changed;

        struct {
            bt_address_t addr;
            uint8_t state; /* hfp_audio_state_t */
        } _on_audio_state_changed;

        struct {
            bt_address_t addr;
            uint8_t started; /* boolean */
        } _on_voice_recognition_state_changed;

        struct {
            bt_address_t addr;
            uint8_t value;
        } _on_battery_level_changed;

        struct {
            bt_address_t addr;
            uint8_t type; /* hfp_volume_type_t */
            uint8_t volume;
        } _on_volume_control;

        struct {
            bt_address_t addr;
            uint8_t pad[2];
            char number[HFP_PHONENUM_DIGITS_MAX + 1];
        } _on_dial_call;

        struct {
            bt_address_t addr;
            uint8_t pad[2];
            char cmd[HFP_AT_LEN_MAX + 1];
        } _on_at_cmd_received;

        struct {
            bt_address_t addr;
            uint16_t company_id;
            char command[HFP_COMPANY_PREFIX_LEN_MAX + 1];
            uint8_t pad1[(HFP_COMPANY_PREFIX_LEN_MAX + 1 + 3) / 4 * 4 - (HFP_COMPANY_PREFIX_LEN_MAX + 1)];
            char value[HFP_AT_LEN_MAX + 1];
        } _on_vend_spec_at_cmd_received;
    } bt_message_hfp_ag_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_HFP_AG_H__ */
