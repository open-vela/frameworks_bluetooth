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
#ifndef __BT_HFP_AG_H__
#define __BT_HFP_AG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_hfp.h"
#include <stddef.h>

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief HFP AG call state
 *
 */
typedef enum {
    HFP_AG_CALL_STATE_ACTIVE,
    HFP_AG_CALL_STATE_HELD,
    HFP_AG_CALL_STATE_DIALING,
    HFP_AG_CALL_STATE_ALERTING,
    HFP_AG_CALL_STATE_INCOMING,
    HFP_AG_CALL_STATE_WAITING,
    HFP_AG_CALL_STATE_IDLE,
    HFP_AG_CALL_STATE_DISCONNECTED
} hfp_ag_call_state_t;

/**
 * @brief HFP AG connection state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 * @param state - connection state.
 */
typedef void (*hfp_ag_connection_state_callback)(void* cookie, bt_address_t* addr, profile_connection_state_t state);

/**
 * @brief HFP AG audio connection state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 * @param state - hfp audio state.
 */
typedef void (*hfp_ag_audio_state_callback)(void* cookie, bt_address_t* addr, hfp_audio_state_t state);

/**
 * @brief voice recognition state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 * @param started - is voice recognition started, true:started, false:stopped.
 */
typedef void (*hfp_ag_vr_cmd_callback)(void* cookie, bt_address_t* addr, bool started);

/**
 * @brief battery update callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 * @param value - battery level.
 */
typedef void (*hfp_ag_battery_update_callback)(void* cookie, bt_address_t* addr, uint8_t value);

/**
 * @brief HFP AG volume control callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 * @param type - the type of volume, 0:gain of speaker, 1:gain of microphone.
 * @param volume - the gain level, range 0-15.
 */
typedef void (*hfp_ag_volume_control_callback)(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief HFP AG received answer incoming call
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 */
typedef void (*hfp_ag_answer_call_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief HFP AG received reject incoming call
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 */
typedef void (*hfp_ag_reject_call_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief HFP AG received hangup call
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 */
typedef void (*hfp_ag_hangup_call_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief HFP AG received dial an outgoing call
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 * @param number - dialed number, if number is NULL, redial.
 */
typedef void (*hfp_ag_dial_call_callback)(void* cookie, bt_address_t* addr, const char* number);

/**
 * @brief HFP AT command received callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer HF device.
 * @param at_command - AT command.
 */
typedef void (*hfp_ag_at_cmd_received_callback)(void* cookie, bt_address_t* addr, const char* at_command);

/**
 * @brief HFP vendor specific AT command received callback
 *
 * @param cookie - callback cookie.
 * @param command - The prefix of the AT command.
 * @param company_id - Bluetooth company ID.
 * @param value - AT command value.
 * @param addr - address of peer HF device.
 */
typedef void (*hfp_ag_vend_spec_at_cmd_received_callback)(void* cookie, bt_address_t* addr, const char* command, uint16_t company_id, const char* value);

/**
 * @brief HFP AG callback structure
 *
 */
typedef struct
{
    size_t size;
    hfp_ag_connection_state_callback connection_state_cb;
    hfp_ag_audio_state_callback audio_state_cb;
    hfp_ag_vr_cmd_callback vr_cmd_cb;
    hfp_ag_battery_update_callback hf_battery_update_cb;
    hfp_ag_volume_control_callback volume_control_cb;
    hfp_ag_answer_call_callback answer_call_cb;
    hfp_ag_reject_call_callback reject_call_cb;
    hfp_ag_hangup_call_callback hangup_call_cb;
    hfp_ag_dial_call_callback dial_call_cb;
    hfp_ag_at_cmd_received_callback at_cmd_cb;
    hfp_ag_vend_spec_at_cmd_received_callback vender_specific_at_cmd_cb;
} hfp_ag_callbacks_t;

/**
 * @brief Register HFP AG callback functions
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - HFP AG callback functions.
 * @return void* - callback cookie.
 */
void* BTSYMBOLS(bt_hfp_ag_register_callbacks)(bt_instance_t* ins, const hfp_ag_callbacks_t* callbacks);

/**
 * @brief Unregister HFP AG callback functions
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callback cookie.
 * @return true - on unregister success.
 * @return false - on callback cookie not found.
 */
bool BTSYMBOLS(bt_hfp_ag_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check HFP AG is connected
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return true - connected.
 * @return false - not connected.
 */
bool BTSYMBOLS(bt_hfp_ag_is_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Check HFP AG audio connection is connected
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return true - connected.
 * @return false - not connected.
 */
bool BTSYMBOLS(bt_hfp_ag_is_audio_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get HFP AG connection state
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return profile_connection_state_t - connection state.
 */
profile_connection_state_t BTSYMBOLS(bt_hfp_ag_get_connection_state)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish SLC with peer HF device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from HFP SLC
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish audio connection with peer HF device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_connect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect audio connection
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_disconnect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Start SCO using virtual voice call
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_start_virtual_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Stop SCO using virtual voice call
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_stop_virtual_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Start voice recognition
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_start_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Stop voice recognition
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_stop_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Send phone state change
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @param num_active - active call count
 * @param num_held - held call count
 * @param call_state - call state
 * @param number - call number
 * @param name - call name
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_phone_state_change)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t num_active, uint8_t num_held,
    hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name);

/**
 * @brief Notify device status
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @param network - network service state
 * @param roam - roam state
 * @param signal - signal strength
 * @param battery - battery level
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_notify_device_status)(bt_instance_t* ins, bt_address_t* addr,
    hfp_network_state_t network, hfp_roaming_state_t roam,
    uint8_t signal, uint8_t battery);

/**
 * @brief Send volume control
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @param type - the type of volume, 0:gain of speaker, 1:gain of microphone.
 * @param volume - the gain level, range 0-15.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_volume_control)(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief Send AT Command
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @param at_command - the AT command to be send.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_send_at_command)(bt_instance_t* ins, bt_address_t* addr, const char* at_command);

/**
 * @brief Send vendor specific AT Command
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer HF device.
 * @param command - the prefix of the AT command to be send.
 * @param value - the value of the AT command to be send.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_ag_send_vendor_specific_at_command)(bt_instance_t* ins, bt_address_t* addr, const char* command, const char* value);
#ifdef __cplusplus
}
#endif

#endif /* __BT_HFP_AG_H__ */
