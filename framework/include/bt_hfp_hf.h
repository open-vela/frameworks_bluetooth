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

#ifndef __BT_HFP_HF_H__
#define __BT_HFP_HF_H__

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
 * @brief HFP HF call state
 */
typedef enum {
    HFP_HF_CALL_STATE_ACTIVE = 0,
    HFP_HF_CALL_STATE_HELD,
    HFP_HF_CALL_STATE_DIALING,
    HFP_HF_CALL_STATE_ALERTING,
    HFP_HF_CALL_STATE_INCOMING,
    HFP_HF_CALL_STATE_WAITING,
    HFP_HF_CALL_STATE_HELD_BY_RESP_HOLD,
    HFP_HF_CALL_STATE_DISCONNECTED
} hfp_hf_call_state_t;

/**
 * @brief HFP HF channel type
 */
typedef enum {
    HFP_HF_CHANNEL_TYP_PHONE = 0,
    HFP_HF_CHANNEL_TYP_WEBCHAT,
} hfp_hf_channel_type_t;

/**
 * @brief HFP call info structure
 */
typedef struct {
    uint32_t index;
    uint8_t dir; /* hfp_call_direction_t */
    uint8_t state; /* hfp_hf_call_state_t */
    uint8_t mpty; /* hfp_call_mpty_type_t */
    uint8_t pad[1];
    char number[HFP_PHONENUM_DIGITS_MAX];
    char name[HFP_NAME_DIGITS_MAX];
} hfp_current_call_t;

/**
 * @brief HFP HF connection state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param state - connection state.
 */
typedef void (*hfp_hf_connection_state_callback)(void* cookie, bt_address_t* addr, profile_connection_state_t state);

/**
 * @brief HFP HF audio connection state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param state - hfp audio state.
 */
typedef void (*hfp_hf_audio_state_callback)(void* cookie, bt_address_t* addr, hfp_audio_state_t state);

/**
 * @brief Voice recognition state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param started - is voice recognition started, true:started, false:stopped.
 */
typedef void (*hfp_hf_vr_cmd_callback)(void* cookie, bt_address_t* addr, bool started);

/**
 * @brief Call state chanaged callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param call - call infomation.
 */
typedef void (*hfp_hf_call_state_change_callback)(void* cookie, bt_address_t* addr, hfp_current_call_t* call);

/**
 * @brief At command callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param resp - at command response string.
 */
typedef void (*hfp_hf_cmd_complete_callback)(void* cookie, bt_address_t* addr, const char* resp);

/**
 * @brief Ring indication callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param inband_ring_tone - true:in-band.
 */
typedef void (*hfp_hf_ring_indication_callback)(void* cookie, bt_address_t* addr, bool inband_ring_tone);

/**
 * @brief Network roaming state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param status - roaming state, 0:not roaming, 1:roaming.
 */
typedef void (*hfp_hf_roaming_changed_callback)(void* cookie, bt_address_t* addr, int status);

/**
 * @brief Network registration state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param status - roaming state, 0:not available, 1:available.
 */
typedef void (*hfp_hf_network_state_changed_callback)(void* cookie, bt_address_t* addr, int status);

/**
 * @brief Network registration signale strength changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param signal - signale strength, range 0-5.
 */
typedef void (*hfp_hf_signal_strength_changed_callback)(void* cookie, bt_address_t* addr, int signal);

/**
 * @brief Network operator name changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param name - network operator name.
 */
typedef void (*hfp_hf_operator_changed_callback)(void* cookie, bt_address_t* addr, char* name);

/**
 * @brief HFP HF volume control callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param type - the type of volume, 0:gain of speaker, 1:gain of microphone.
 * @param volume - the gain level, range 0-15.
 */
typedef void (*hfp_hf_volume_changed_callback)(void* cookie, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief HFP HF call indicator callback.
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param call - the call indicator.
 */
typedef void (*hfp_hf_call_callback)(void* cookie, bt_address_t* addr, hfp_call_t call);

/**
 * @brief HFP HF callsetup indicator callback.
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param callsetup - the callsetup indicator.
 */
typedef void (*hfp_hf_callsetup_callback)(void* cookie, bt_address_t* addr, hfp_callsetup_t callsetup);

/**
 * @brief HFP HF callheld indicator callback.
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer AG device.
 * @param callheld - the callheld indicator.
 */
typedef void (*hfp_hf_callheld_callback)(void* cookie, bt_address_t* addr, hfp_callheld_t callheld);

/**
 * @brief HFP HF callback structure
 *
 */
typedef struct
{
    size_t size;
    hfp_hf_connection_state_callback connection_state_cb;
    hfp_hf_audio_state_callback audio_state_cb;
    hfp_hf_vr_cmd_callback vr_cmd_cb;
    hfp_hf_call_state_change_callback call_state_changed_cb;
    hfp_hf_cmd_complete_callback cmd_complete_cb;
    hfp_hf_ring_indication_callback ring_indication_cb;
    hfp_hf_volume_changed_callback volume_changed_cb;
    hfp_hf_call_callback call_cb;
    hfp_hf_callsetup_callback callsetup_cb;
    hfp_hf_callheld_callback callheld_cb;
} hfp_hf_callbacks_t;

/**
 * @brief Register HFP HF callback functions
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - HFP HF callback functions.
 * @return void* - callback cookie.
 */
void* BTSYMBOLS(bt_hfp_hf_register_callbacks)(bt_instance_t* ins, const hfp_hf_callbacks_t* callbacks);

/**
 * @brief Unregister HFP AG callback functions
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callback cookie.
 * @return true - on unregister success.
 * @return false - on callback cookie not found.
 */
bool BTSYMBOLS(bt_hfp_hf_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check HFP HF is connected
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return true - connected.
 * @return false - not connected.
 */
bool BTSYMBOLS(bt_hfp_hf_is_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Check HFP HF audio connection is connected
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return true - connected.
 * @return false - not connected.
 */
bool BTSYMBOLS(bt_hfp_hf_is_audio_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get HFP HF connection state
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return profile_connection_state_t - connection state.
 */
profile_connection_state_t BTSYMBOLS(bt_hfp_hf_get_connection_state)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish SLC with peer AG device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from HFP SLC
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set HF Connection policy
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param policy - connection policy.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_set_connection_policy)(bt_instance_t* ins, bt_address_t* addr, connection_policy_t policy);

/**
 * @brief Establish audio connection with peer AG device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_connect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect audio connection
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_disconnect_audio)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Start voice recognition
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_start_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Stop voice recognition
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_stop_voice_recognition)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Dial number
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param number - phone number.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_dial)(bt_instance_t* ins, bt_address_t* addr, const char* number);

/**
 * @brief Dial memory
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param memory - memory location.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_dial_memory)(bt_instance_t* ins, bt_address_t* addr, uint32_t memory);

/**
 * @brief Dial last number
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_redial)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Accept an voice call
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param flag - accept flag, (0:none, 1:hold, 2:release).
 * 0: Accept an incoming call, invalid on no incoming call.
 * 1: Places all active calls (if any exist) on hold and accepts the other (held or waiting) call.
 * 2: Releases all active calls (if any exist) and accepts the other (held or waiting) call.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_accept_call)(bt_instance_t* ins, bt_address_t* addr, hfp_call_accept_t flag);

/**
 * @brief Reject voice call
 *  reject an incoming call if any exist, otherwise then releases all held calls or a waiting call.
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_reject_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Hold voice call
 *
 * hold active call when three-way calling.
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_hold_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Terminate voice call
 * release all calls if any active/dialing/alerting voice call exist, otherwise then releases all held calls,
 * if don't want release all calls  use bt_hfp_hf_control_call instead.
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_terminate_call)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Enhanced call control
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param chld - call control.
 * 0: Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call.
 * 1: Releases all active calls (if any exist) and accepts the other (held or waiting) call.
 * 2: Places all active calls (if any exist) on hold and accepts the other (held or waiting) call.
 * 3: Adds a held call to the conversation.
 * 4: Connects the two calls and disconnects the subscriber from both calls (Explicit Call Transfer).
 * Support for this value and its associated functionality is optional for the HF.
 * @param index - call index, it does not work.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_control_call)(bt_instance_t* ins, bt_address_t* addr, hfp_call_control_t chld, uint8_t index);

/**
 * @brief Query current calls
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param[out] calls - out calls infomation array.
 * @param[out] num - out calls array size.
 * @param allocator - array allocator.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_query_current_calls)(bt_instance_t* ins, bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator);

/**
 * @brief Send AT command
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param cmd - AT command.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_send_at_cmd)(bt_instance_t* ins, bt_address_t* addr, const char* cmd);

/**
 * @brief Update battery level
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param level - the battery level, valid from 0 to 100.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_update_battery_level)(bt_instance_t* ins, bt_address_t* addr, uint8_t level);

/**
 * @brief Send volume control
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param type - the type of volume, 0:gain of speaker, 1:gain of microphone.
 * @param volume - the gain level, range 0-15.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_volume_control)(bt_instance_t* ins, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);

/**
 * @brief Send Dual Tone Multi-Frequency (DTMF) code
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer AG device.
 * @param dtmf - the DTMF code, one of ['0'-'9', 'A'-'D', '*', '#'].
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hfp_hf_send_dtmf)(bt_instance_t* ins, bt_address_t* addr, char dtmf);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HFP_HF_H__ */
