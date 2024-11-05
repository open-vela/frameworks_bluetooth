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
#ifndef __HFP_AG_SERVICE_H__
#define __HFP_AG_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "audio_transport.h"
#include "bt_device.h"
#include "bt_hfp_ag.h"
#include "hfp_define.h"

typedef enum {
    HFP_AG_STATE_DISCONNECTED = 0,
    HFP_AG_STATE_CONNECTING,
    HFP_AG_STATE_DISCONNECTING,
    HFP_AG_STATE_CONNECTED,
    HFP_AG_STATE_AUDIO_CONNECTING,
    HFP_AG_STATE_AUDIO_CONNECTED,
    HFP_AG_STATE_AUDIO_DISCONNECTING
} hfp_ag_state_t;

typedef struct {
    hfp_network_state_t network; /* 0: unavailable, 1: available */
    hfp_roaming_state_t roam; /* 0: no roaming, 1: roaming */
    uint8_t signal; /* range in 0-5 */
    uint8_t battery; /* range in 0-5 */
    hfp_call_t call; /* 0: no call, 1: call in progress */
    hfp_callsetup_t call_setup; /* 0: no call setup, 1: incoming, 2: outgoing, 3: alerting */
    hfp_callheld_t call_held; /* 0: no call held, 1: callheld */
} hfp_ag_cind_resopnse_t;

/*
 * sal callback
 */
void hfp_ag_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state,
    profile_connection_reason_t reason, uint32_t remote_features);
void hfp_ag_on_audio_state_changed(bt_address_t* addr, hfp_audio_state_t state, uint16_t sco_connection_handle);
void hfp_ag_on_codec_changed(bt_address_t* addr, hfp_codec_config_t* config);
void hfp_ag_on_volume_changed(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
void hfp_ag_on_received_cind_request(bt_address_t* addr);
void hfp_ag_on_received_clcc_request(bt_address_t* addr);
void hfp_ag_on_received_cops_request(bt_address_t* addr);
void hfp_ag_on_voice_recognition_state_changed(bt_address_t* addr, bool started);
void hfp_ag_on_remote_battery_level_update(bt_address_t* addr, uint8_t value);
void hfp_ag_on_answer_call(bt_address_t* addr);
void hfp_ag_on_reject_call(bt_address_t* addr);
void hfp_ag_on_hangup_call(bt_address_t* addr);
void hfp_ag_on_received_at_cmd(bt_address_t* addr, char* at_string, uint16_t at_length);
void hfp_ag_on_audio_connect_request(bt_address_t* addr);
void hfp_ag_on_dial_number(bt_address_t* addr, char* number, uint32_t length);
void hfp_ag_on_dial_memory(bt_address_t* addr, uint32_t location);
void hfp_ag_on_call_control(bt_address_t* addr, hfp_call_control_t control);
void hfp_ag_on_received_dtmf(bt_address_t* addr, char tone);
void hfp_ag_on_received_manufacture_request(bt_address_t* addr);
void hfp_ag_on_received_model_id_request(bt_address_t* addr);
void hfp_ag_on_received_nrec_request(bt_address_t* addr, uint8_t nrec);

/*
 *  statemachine callbacks
 */
void ag_service_notify_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);
void ag_service_notify_audio_state_changed(bt_address_t* addr, hfp_audio_state_t state);
void ag_service_notify_vr_state_changed(bt_address_t* addr, bool started);
void ag_service_notify_hf_battery_update(bt_address_t* addr, uint8_t value);
void ag_service_notify_volume_changed(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
void ag_service_notify_call_answered(bt_address_t* addr);
void ag_service_notify_call_rejected(bt_address_t* addr);
void ag_service_notify_call_hangup(bt_address_t* addr);
void ag_service_notify_call_dial(bt_address_t* addr, const char* number);
void ag_service_notify_cmd_received(bt_address_t* addr, const char* at_cmd);
void ag_service_notify_vendor_specific_cmd(bt_address_t* addr, const char* command, uint16_t company_id, const char* value);

/*
 * telephony
 */
bt_status_t hfp_ag_phone_state_change(bt_address_t* addr, uint8_t num_active, uint8_t num_held,
    hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name);
bt_status_t hfp_ag_device_status_changed(bt_address_t* addr, hfp_network_state_t network,
    hfp_roaming_state_t roam, uint8_t signal, uint8_t battery);
bt_status_t hfp_ag_dial_result(uint8_t result);

/*
 * service api
 */

bool hfp_ag_on_sco_start(void);
bool hfp_ag_on_sco_stop(void);

typedef struct ag_interface {
    size_t size;
    void* (*register_callbacks)(void* remote, const hfp_ag_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bool (*is_connected)(bt_address_t* addr);
    bool (*is_audio_connected)(bt_address_t* addr);
    profile_connection_state_t (*get_connection_state)(bt_address_t* addr);
    bt_status_t (*connect)(bt_address_t* addr);
    bt_status_t (*disconnect)(bt_address_t* addr);
    bt_status_t (*connect_audio)(bt_address_t* addr);
    bt_status_t (*disconnect_audio)(bt_address_t* addr);
    bt_status_t (*start_virtual_call)(bt_address_t* addr);
    bt_status_t (*stop_virtual_call)(bt_address_t* addr);
    bt_status_t (*start_voice_recognition)(bt_address_t* addr);
    bt_status_t (*stop_voice_recognition)(bt_address_t* addr);
    bt_status_t (*phone_state_change)(bt_address_t* addr, uint8_t num_active, uint8_t num_held,
        hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
        const char* number, const char* name);
    bt_status_t (*device_status_changed)(bt_address_t* addr, hfp_network_state_t network,
        hfp_roaming_state_t roam, uint8_t signal, uint8_t battery);
    bt_status_t (*volume_control)(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
    bt_status_t (*dial_response)(uint8_t result);
    bt_status_t (*send_at_command)(bt_address_t* addr, const char* at_command);
    bt_status_t (*send_vendor_specific_at_command)(bt_address_t* addr, const char* command, const char* value);
} hfp_ag_interface_t;

/*
 * register audio gate-way to service manager
 */
void register_hfp_ag_service(void);

/*
 * get local supported features
 */
uint32_t hfp_ag_get_local_features(void);

#endif /* __HFP_AG_SERVICE_H__ */