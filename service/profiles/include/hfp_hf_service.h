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
#ifndef __HFP_HF_SERVICE_H__
#define __HFP_HF_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "audio_transport.h"
#include "bt_device.h"
#include "bt_hfp_hf.h"
#include "hfp_define.h"
#include "hfp_hf_event.h"

typedef enum {
    HFP_HF_STATE_DISCONNECTED = 0,
    HFP_HF_STATE_CONNECTING,
    HFP_HF_STATE_DISCONNECTING,
    HFP_HF_STATE_CONNECTED,
    HFP_HF_STATE_AUDIO_CONNECTED
} hfp_hf_state_t;

typedef struct {
    hfp_call_t call_status;
    hfp_callsetup_t callsetup_status;
    hfp_callheld_t callheld_status;
} hfp_hf_call_cache_t;

typedef struct {
    hfp_call_t call_status;
    uint64_t call_timestamp_us;
    hfp_callsetup_t callsetup_status;
    uint64_t callsetup_timestamp_us;
    hfp_callheld_t callheld_status;
    uint64_t callheld_timestamp_us;
    uint64_t dialing_timestamp_us;
    hfp_hf_call_cache_t last_reported;
#ifdef CONFIG_HFP_HF_WEBCHAT_BLOCKER
    uint64_t webchat_flag_timestamp_us;
#endif
} hfp_hf_call_status_t;

/*
 * sal callback
 */
void hfp_hf_on_connection_state_changed(bt_address_t* addr, profile_connection_state_t state,
    profile_connection_reason_t reason, uint32_t remote_features);
void hfp_hf_on_audio_connection_state_changed(bt_address_t* addr,
    hfp_audio_state_t state,
    uint16_t sco_connection_handle);
void hfp_hf_on_codec_changed(bt_address_t* addr, hfp_codec_config_t* config);
void hfp_hf_on_call_setup_state_changed(bt_address_t* addr, hfp_callsetup_t setup);
void hfp_hf_on_call_active_state_changed(bt_address_t* addr, hfp_call_t state);
void hfp_hf_on_call_held_state_changed(bt_address_t* addr, hfp_callheld_t state);
void hfp_hf_on_volume_changed(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
void hfp_hf_on_ring_active_state_changed(bt_address_t* addr, bool active, hfp_in_band_ring_state_t inband_ring);
void hfp_hf_on_voice_recognition_state_changed(bt_address_t* addr, bool started);
void hfp_hf_on_received_at_cmd_resp(bt_address_t* addr, char* response, uint16_t response_length);
void hfp_hf_on_received_sco_connection_req(bt_address_t* addr);
void hfp_hf_on_clip(bt_address_t* addr, const char* number, const char* name);
void hfp_hf_on_current_call_response(bt_address_t* addr, uint32_t idx,
    hfp_call_direction_t dir,
    hfp_hf_call_state_t status,
    hfp_call_mpty_type_t mpty,
    const char* number, uint32_t type);
void hfp_hf_on_at_command_result_response(bt_address_t* addr, uint32_t at_cmd_code, uint32_t result);

/*
 *  statemachine callbacks
 */

void hf_service_notify_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);
void hf_service_notify_audio_state_changed(bt_address_t* addr, hfp_audio_state_t state);
void hf_service_notify_vr_state_changed(bt_address_t* addr, bool started);
void hf_service_notify_call_state_changed(bt_address_t* addr, hfp_current_call_t* call);
void hf_service_notify_cmd_complete(bt_address_t* addr, const char* resp);
void hf_service_notify_ring_indication(bt_address_t* addr, bool inband_ring_tone);
void hf_service_notify_volume_changed(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
void hf_service_notify_call(bt_address_t* addr, hfp_call_t call);
void hf_service_notify_callsetup(bt_address_t* addr, hfp_callsetup_t callsetup);
void hf_service_notify_callheld(bt_address_t* addr, hfp_callheld_t callheld);

/*
 * service api
 */

bt_status_t hfp_hf_send_event(bt_address_t* addr, hfp_hf_event_t evt);
bool hfp_hf_on_sco_start(void);
bool hfp_hf_on_sco_stop(void);

typedef struct hf_interface {
    size_t size;
    void* (*register_callbacks)(void* remote, const hfp_hf_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bool (*is_connected)(bt_address_t* addr);
    bool (*is_audio_connected)(bt_address_t* addr);
    profile_connection_state_t (*get_connection_state)(bt_address_t* addr);
    bt_status_t (*connect)(bt_address_t* addr);
    bt_status_t (*disconnect)(bt_address_t* addr);
    bt_status_t (*set_connection_policy)(bt_address_t* addr, connection_policy_t policy);
    bt_status_t (*connect_audio)(bt_address_t* addr);
    bt_status_t (*disconnect_audio)(bt_address_t* addr);
    bt_status_t (*start_voice_recognition)(bt_address_t* addr);
    bt_status_t (*stop_voice_recognition)(bt_address_t* addr);
    bt_status_t (*dial)(bt_address_t* addr, const char* number);
    bt_status_t (*dial_memory)(bt_address_t* addr, uint32_t memory);
    bt_status_t (*redial)(bt_address_t* addr);
    bt_status_t (*accept_call)(bt_address_t* addr, hfp_call_accept_t flag);
    bt_status_t (*reject_call)(bt_address_t* addr);
    bt_status_t (*hold_call)(bt_address_t* addr);
    bt_status_t (*terminate_call)(bt_address_t* addr);
    bt_status_t (*control_call)(bt_address_t* addr, hfp_call_control_t chld, uint8_t index);
    bt_status_t (*query_current_calls)(bt_address_t* addr, hfp_current_call_t** calls, int* num, bt_allocator_t allocator);
    bt_status_t (*send_at_cmd)(bt_address_t* addr, const char* cmd);
    bt_status_t (*update_battery_level)(bt_address_t* addr, uint8_t level);
    bt_status_t (*volume_control)(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume);
    bt_status_t (*send_dtmf)(bt_address_t* addr, char dtmf);
} hfp_hf_interface_t;

/*
 * register profile to service manager
 */
void register_hfp_hf_service(void);

#endif /* __HFP_HF_SERVICE_H__ */