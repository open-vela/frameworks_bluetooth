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
BT_HFP_HF_MESSAGE_START,
    BT_HFP_HF_REGISTER_CALLBACK,
    BT_HFP_HF_UNREGISTER_CALLBACK,
    BT_HFP_HF_IS_CONNECTED,
    BT_HFP_HF_IS_AUDIO_CONNECTED,
    BT_HFP_HF_GET_CONNECTION_STATE,
    BT_HFP_HF_CONNECT,
    BT_HFP_HF_SET_CONNECTION_POLICY,
    BT_HFP_HF_DISCONNECT,
    BT_HFP_HF_CONNECT_AUDIO,
    BT_HFP_HF_DISCONNECT_AUDIO,
    BT_HFP_HF_START_VOICE_RECOGNITION,
    BT_HFP_HF_STOP_VOICE_RECOGNITION,
    BT_HFP_HF_DIAL,
    BT_HFP_HF_DIAL_MEMORY,
    BT_HFP_HF_REDIAL,
    BT_HFP_HF_ACCEPT_CALL,
    BT_HFP_HF_REJECT_CALL,
    BT_HFP_HF_HOLD_CALL,
    BT_HFP_HF_TERMINATE_CALL,
    BT_HFP_HF_CONTROL_CALL,
    BT_HFP_HF_QUERY_CURRENT_CALLS,
    BT_HFP_HF_SEND_AT_CMD,
    BT_HFP_HF_UPDATE_BATTERY_LEVEL,
    BT_HFP_HF_VOLUME_CONTROL,
    BT_HFP_HF_SEND_DTMF,
    BT_HFP_HF_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_HFP_HF_CALLBACK_START,
    BT_HFP_HF_ON_CONNECTION_STATE_CHANGED,
    BT_HFP_HF_ON_AUDIO_STATE_CHANGED,
    BT_HFP_HF_ON_VOICE_RECOGNITION_STATE_CHANGED,
    BT_HFP_HF_ON_CALL_STATE_CHANGED,
    BT_HFP_HF_ON_AT_CMD_COMPLETE,
    BT_HFP_HF_ON_RING_INDICATION,
    BT_HFP_HF_ON_VOLUME_CHANGED,
    BT_HFP_HF_ON_CALL_IND_RECEIVED,
    BT_HFP_HF_ON_CALLSETUP_IND_RECEIVED,
    BT_HFP_HF_ON_CALLHELD_IND_RECEIVED,
    BT_HFP_HF_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_HFP_HF_H__
#define _BT_MESSAGE_HFP_HF_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_hfp_hf.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t profile_conn_state; /* profile_connection_state_t */
        uint8_t value_bool; /* boolean */
    } bt_hfp_hf_result_t;

    typedef union {
        struct {
            bt_address_t addr;
        } _bt_hfp_hf_is_connected,
            _bt_hfp_hf_is_audio_connected,
            _bt_hfp_hf_get_connection_state,
            _bt_hfp_hf_connect,
            _bt_hfp_hf_disconnect,
            _bt_hfp_hf_connect_audio,
            _bt_hfp_hf_disconnect_audio,
            _bt_hfp_hf_start_voice_recognition,
            _bt_hfp_hf_stop_voice_recognition,
            _bt_hfp_hf_redial,
            _bt_hfp_hf_reject_call,
            _bt_hfp_hf_hold_call,
            _bt_hfp_hf_terminate_call;

        struct {
            bt_address_t addr;
            uint8_t policy;
        } _bt_hfp_hf_set_connection_policy;

        struct {
            bt_address_t addr;
            uint8_t pad[2];
            char number[HFP_PHONENUM_DIGITS_MAX + 1];
        } _bt_hfp_hf_dial;

        struct {
            uint32_t memory;
            bt_address_t addr;
        } _bt_hfp_hf_dial_memory;

        struct {
            bt_address_t addr;
            uint8_t flag; /* hfp_call_accept_t */
        } _bt_hfp_hf_accept_call;

        struct {
            bt_address_t addr;
            uint8_t chld; /* hfp_call_control_t */
            uint8_t index;
        } _bt_hfp_hf_control_call;

        struct {
            bt_address_t addr; /* @param[in]  */
            uint8_t pad[2];
            uint32_t num; /* @param[out] */
            hfp_current_call_t calls[HFP_CALL_LIST_MAX]; /* @param[out] */
        } _bt_hfp_hf_query_current_calls;

        struct {
            bt_address_t addr;
            uint8_t pad[2];
            char cmd[HFP_AT_LEN_MAX + 1];
        } _bt_hfp_hf_send_at_cmd;

        struct {
            bt_address_t addr;
            uint8_t level;
        } _bt_hfp_hf_update_battery_level;

        struct {
            bt_address_t addr;
            uint8_t type; /* hfp_volume_type_t */
            uint8_t volume;
        } _bt_hfp_hf_volume_control;

        struct {
            bt_address_t addr;
            uint8_t dtmf;
        } _bt_hfp_hf_send_dtmf;
    } bt_message_hfp_hf_t;

    typedef union {
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
            hfp_current_call_t call;
        } _on_call_state_changed_cb;

        struct {
            bt_address_t addr;
            uint8_t pad[2];
            char resp[HFP_AT_LEN_MAX + 1];
        } _on_at_cmd_complete_cb;

        struct {
            bt_address_t addr;
            uint8_t inband_ring_tone; /* boolean */
        } _on_ring_indication_cb;

        struct {
            bt_address_t addr;
            uint8_t type; /* hfp_volume_type_t */
            uint8_t volume;
        } _on_volume_changed_cb;

        struct {
            bt_address_t addr;
            uint16_t value;
        } _on_call_cb,
            _on_callsetup_cb,
            _on_callheld_cb;
    } bt_message_hfp_hf_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_HFP_HF_H__ */
