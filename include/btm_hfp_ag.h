/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#ifndef __BT_AG_SERVER_H__
#define __BT_AG_SERVER_H__

#include "btm_manager.h"
#include "bts_ag_server_define.h"

typedef enum {
    AG_SERVER_STATE_DISCONNECTED = 0,
    AG_SERVER_STATE_CONNECTING,
    AG_SERVER_STATE_DISCONNECTING,
    AG_SERVER_STATE_CONNECTED,
    AG_SERVER_STATE_AUDIO_CONNECTING,
    AG_SERVER_STATE_AUDIO_CONNECTED,
    AG_SERVER_STATE_AUDIO_DISCONNECTING
} ag_server_state_t;

typedef struct {
    hfp_network_state_t network; /* 0: unavailable, 1: available */
    hfp_roaming_state_t roam; /* 0: no roaming, 1: roaming */
    uint8_t signal; /* range in 0-5 */
    uint8_t battery; /* range in 0-5 */
    hfp_call_t call; /* 0: no call, 1: call in progress */
    hfp_callsetup_t call_setup; /* 0: no call setup, 1: incoming, 2: outgoing, 3: alerting */
    hfp_callheld_t call_held; /* 0: no call held, 1: callheld */
} ag_server_cind_resopnse_t;

typedef enum {
    AG_SERVER_VR_STATE_STOPPED = 0,
    AG_SERVER_VR_STATE_STARTED
} ag_server_vr_state_t;

typedef enum {
    AG_SERVER_VOLUME_TYPE_SPK = 0,
    AG_SERVER_VOLUME_TYPE_MIC
} ag_server_volume_type_t;

typedef enum {
    HFP_HF_CALL_CONTROL_CHLD_0, /* Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call */
    HFP_HF_CALL_CONTROL_CHLD_1, /* Releases all active calls (if any exist) and accepts the other (held or waiting) call */
    HFP_HF_CALL_CONTROL_CHLD_2, /* Places all active calls (if any exist) on hold and accepts the other (held or waiting) call */
    HFP_HF_CALL_CONTROL_CHLD_3, /* Adds a held call to the conversation */
    HFP_HF_CALL_CONTROL_CHLD_4 /* Connects the two calls and disconnects the subscriber from both calls (Explicit Call Transfer).
                                  Support for this value and its associated functionality is optional for the HF */
} ag_server_call_control_t;

typedef enum {
    AG_SERVER_CALL_STATE_ACTIVE = 0,
    AG_SERVER_CALL_STATE_HELD,
    AG_SERVER_CALL_STATE_DIALING,
    AG_SERVER_CALL_STATE_ALERTING,
    AG_SERVER_CALL_STATE_INCOMING,
    AG_SERVER_CALL_STATE_WAITING,
    AG_SERVER_CALL_STATE_IDLE,
    AG_SERVER_CALL_STATE_DISCONNECTED
} ag_server_call_state_t;

typedef enum {
    AG_SERVER_CALL_ADDRTYPE_UNKNOWN = 0x81,
    AG_SERVER_CALL_ADDRTYPE_INTERNATIONAL = 0x91,
    AG_SERVER_CALL_ADDRTYPE_NATIONAL = 0xA1,
} ag_server_call_addrtype_t;

typedef enum {
    AG_SERVER_CALL_NO_CALLS_IN_PROGRESS = 0,
    AG_SERVER_CALL_CALLS_IN_PROGRESS
} ag_server_call_t;

typedef enum {
    AG_SERVER_CALLSETUP_NONE = 0,
    AG_SERVER_CALLSETUP_INCOMING,
    AG_SERVER_CALLSETUP_OUTGOING,
    AG_SERVER_CALLSETUP_ALERTING
} ag_server_callsetup_t;

typedef enum {
    AG_SERVER_CALLHELD_NONE = 0,
    AG_SERVER_CALLHELD_HELD,
} ag_server_callheld_t;

typedef enum {
    AG_SERVER_CALL_DIRECTION_OUTGOING = 0,
    AG_SERVER_CALL_DIRECTION_INCOMING
} ag_server_call_direction_t;

typedef enum {
    AG_SERVER_CALL_MPTY_TYPE_SINGLE = 0,
    AG_SERVER_CALL_MPTY_TYPE_MULTI
} ag_server_call_mpty_type_t;

typedef enum {
    AG_SERVER_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    AG_SERVER_IN_BAND_RINGTONE_PROVIDED,
} ag_server_in_band_ring_state_t;

typedef void (*hfp_ag_connection_state_callback)(
    bt_address addr, profile_connection_state_t state);
typedef void (*hfp_ag_audio_state_callback)(
    bt_address addr, hfp_audio_state_t state);
typedef void (*hfp_ag_vr_cmd_callback)(
    bt_address addr, bool started);
typedef void (*hfp_ag_battery_update_callback)(
    bt_address addr, uint8_t value);
typedef void (*hfp_ag_answer_call_callback)(
    bt_address addr);
typedef void (*hfp_ag_reject_call_callback)(
    bt_address addr);
typedef void (*hfp_ag_hangup_call_callback)(
    bt_address addr);
typedef void (*hfp_ag_dial_number_callback)(
    bt_address addr, char* number);
typedef void (*hfp_ag_call_control_callback)(
    bt_address addr, uint8_t chld);
typedef void (*hfp_ag_at_command_callback)(
    bt_address addr, char* at_command);
typedef void (*hfp_ag_cind_callback)(
    bt_address addr);
typedef void (*hfp_ag_clcc_callback)(
    bt_address addr);
typedef void (*hfp_ag_cops_callback)(
    bt_address addr);

typedef struct
{
    size_t size;
    hfp_ag_connection_state_callback connection_state_cb;
    hfp_ag_audio_state_callback audio_state_cb;
    hfp_ag_vr_cmd_callback vr_cmd_cb;
    hfp_ag_battery_update_callback ag_battery_update_cb;
    hfp_ag_answer_call_callback answer_call_cb;
    hfp_ag_reject_call_callback reject_call_cb;
    hfp_ag_hangup_call_callback hangup_call_cb;
    hfp_ag_dial_number_callback dial_number_cb;
    hfp_ag_call_control_callback call_control_cb;
    hfp_ag_at_command_callback at_command_cb;

    hfp_ag_cind_callback cind_cb;
    hfp_ag_clcc_callback clcc_cb;
    hfp_ag_cops_callback cops_cb;

} ag_server_callbacks_t;

typedef struct
{
    size_t size;
    bool (*is_connected)(void* handle, bt_address addr);
    bool (*is_audio_connected)(void* handle, bt_address addr);
    ag_server_state_t (*get_connection_state)(void* handle, bt_address addr);

    /**
     * @brief Connect to the audio gateway.
     * @param[in] handle    the hand-free handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*connect)(void* handle, bt_address addr);

    /**
     * @brief Dis-connect from the audio gateway.
     * @param[in] handle    the hand-free handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*disconnect)(void* handle, bt_address addr);

    /**
     * @brief Create an audio connection, SCO link.
     * @param[in] handle    the hand-free handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*connect_audio)(void* handle, bt_address addr);

    /**
     * @brief Close audio connection, SCO link.
     * @param[in] handle    the hand-free handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*disconnect_audio)(void* handle, bt_address addr);

    /**
     * @brief Start voice recognition.
     * @param[in] handle    the hand-free handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*start_voice_recognition)(void* handle, bt_address addr);

    /**
     * @brief Stop voice recognition.
     * @param[in] handle    the hand-free handle (unused).
     * @param[in] addr      address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*stop_voice_recognition)(void* handle, bt_address addr);

    /**
     * @brief Notify the phone state change.
     * @param[in] bd_addr    address of peer device.
     * @param[in] num_active number of active calls.
     * @param[in] num_held   number of held calls.
     * @param[in] call_state state of the current call.
     * @param[in] type       type of the phone number (unknown, international, national, etc.).
     * @param[in] number     phone number of the current call (optional).
     * @param[in] name       name of the current call (optional).
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*phone_state_change)(bt_address bd_addr,
        uint8_t num_active, uint8_t num_held,
        ag_server_call_state_t call_state,
        ag_server_call_addrtype_t type, const char* number,
        const char* name);

    /**
     * @brief Notify the device status changed.
     * @param[in] bd_addr    address of peer device.
     * @param[in] network    network state of the device (not available, available, etc.).
     * @param[in] roam       roaming state of the device (not roaming, roaming, etc.).
     * @param[in] signal     signal strength of the device (0-5).
     * @param[in] battery    battery level of the device (0-5).
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*device_status_changed)(bt_address bd_addr,
        hfp_network_state_t network,
        hfp_roaming_state_t roam,
        uint8_t signal, uint8_t battery);

    /**
     * @brief Set the inband ring enable state of the device.
     * @param[in] bd_addr    address of peer device.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*set_inband_ring_enable)(bt_address bd_addr);

    /**
     * @brief Send an AT command to the device.
     * @param[in] bd_addr    address of peer device.
     * @param[in] at_command the AT command to send.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*send_at_command)(bt_address bd_addr, char* at_command);

    /**
     * @brief Notify the dial result.
     * @param[in] bd_addr    address of peer device.
     * @param[in] result     dial result (0: success, 1: fail).
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*dial_result)(bt_address bd_addr, uint8_t result);

    /**
     * @brief Notify the indicator status response.
     * @param[in] bd_addr    address of peer device.
     * @param[in] service    network state of the device (not available, available, etc.).
     * @param[in] signal     signal strength of the device (0-5).
     * @param[in] roam       roaming state of the device (not roaming, roaming, etc.).
     * @param[in] battery    battery level of the device (0-5).
     * @param[in] call       call state of the device (no call, active call, etc.).
     * @param[in] call_setup call setup state of the device (no call setup, incoming call, outgoing call, etc.).
     * @param[in] call_held  call held state of the device (no calls held, calls held and active, calls held and no active).
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*cind_response)(bt_address bd_addr,
        hfp_network_state_t service,
        uint8_t signal,
        hfp_roaming_state_t roam,
        uint8_t battery,
        hfp_call_t call,
        hfp_callsetup_t call_setup,
        hfp_callheld_t call_held);

    /**
     * @brief Notify the current calls response.
     * @param[in] bd_addr    address of peer device.
     * @param[in] index      index of the current call (1-7).
     * @param[in] dir        direction of the current call (0: outgoing, 1: incoming).
     * @param[in] status     status of the current call (active, held, dialing, alerting, incoming, waiting).
     * @param[in] mode       mode of the current call (0: voice, 1: data).
     * @param[in] mpty       multiparty flag of the current call (0: not multiparty, 1: multiparty).
     * @param[in] number     phone number of the current call (optional).
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*clcc_response)(bt_address bd_addr,
        uint32_t index,
        uint8_t dir,
        ag_server_call_state_t status,
        uint8_t mode,
        uint8_t mpty,
        const char* number);

    /**
     * @brief Notify the operator name response.
     * @param[in] bd_addr         address of peer device.
     * @param[in] operator_name   operator name of the device.
     * @param[in] length          length of the operator name.
     * @return BT_RESULT_SUCCESS on success; a negated errno value on failure.
     */
    bt_result_code (*cops_response)(bt_address bd_addr, char* operator_name, uint16_t length);

    /**
     * @brief Set the hand-free event callback
     * @param[in] handle    the hand-free handle (unused).
     * @param[in] callbacks hand-free event callback function.
     */
    void (*set_callbacks)(void* handle, ag_server_callbacks_t* callbacks);
} ag_server_interface_t;

/**
 * @brief Get the hand-free interface
 * @return Pointer to hand-free interface.
 */
const ag_server_interface_t* get_ag_server_interface(void);

#endif
