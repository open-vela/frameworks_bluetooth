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
#ifndef __LEA_TBS_SERVICE_H__
#define __LEA_TBS_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_lea_tbs.h"
#include "sal_lea_tbs_interface.h"
#include "stddef.h"

/****************************************************************************
 * sal callback
 ****************************************************************************/
void lea_tbs_on_state_changed(uint32_t tbs_id, uint8_t ccid, bool added);
void lea_tbs_on_bearer_info_set(uint32_t tbs_id, char* bearer_ref, bool result);
void lea_tbs_on_call_added(uint32_t tbs_id, uint8_t call_index, bool result);
void lea_tbs_on_call_removed(uint32_t tbs_id, uint8_t call_index);
void lea_tbs_on_accept_call(uint32_t tbs_id, uint8_t call_index);
void lea_tbs_on_terminate_call(uint32_t tbs_id, uint8_t call_index);
void lea_tbs_on_local_hold_call(uint32_t tbs_id, uint8_t call_index);
void lea_tbs_on_local_retrieve_call(uint32_t tbs_id, uint8_t call_index);
void lea_tbs_on_originate_call(uint32_t tbs_id, size_t size, char* uri);
void lea_tbs_on_join_call(uint32_t tbs_id, uint8_t index_number, size_t size, char* index_list);

/****************************************************************************
 * lea tbs interface
 ****************************************************************************/
bt_status_t lea_tbs_set_telephone_bearer_info(lea_tbs_telephone_bearer_t* bearer);
bt_status_t lea_tbs_add_call(lea_tbs_calls_t* call_s);
bt_status_t lea_tbs_remove_call(uint8_t call_index);
bt_status_t lea_tbs_provider_name_changed(uint8_t* name);
bt_status_t lea_tbs_bearer_technology_changed(lea_adpt_bearer_technology_t technology);
bt_status_t lea_tbs_uri_schemes_supported_list_changed(uint8_t* uri_schemes);
bt_status_t lea_tbs_rssi_value_changed(uint8_t strength);
bt_status_t lea_tbs_rssi_interval_changed(uint8_t interval);
bt_status_t lea_tbs_status_flags_changed(uint8_t status_flags);
bt_status_t lea_tbs_call_state_changed(uint8_t number, lea_tbs_call_state_t* state_s);
bt_status_t lea_tbs_notify_termination_reason(uint8_t call_index, lea_adpt_termination_reason_t reason);

typedef struct {
    size_t size;
    bt_status_t (*tbs_add)();
    bt_status_t (*tbs_remove)();
    bt_status_t (*set_telephone_bearer)(lea_tbs_telephone_bearer_t* bearer);
    bt_status_t (*add_call)(lea_tbs_calls_t* call_s);
    bt_status_t (*remove_call)(uint8_t call_index);
    bt_status_t (*provider_name_changed)(uint8_t* name);
    bt_status_t (*bearer_technology_changed)(lea_adpt_bearer_technology_t technology);
    bt_status_t (*uri_schemes_supported_list_changed)(uint8_t* uri_schemes);
    bt_status_t (*rssi_value_changed)(uint8_t strength);
    bt_status_t (*rssi_interval_changed)(uint8_t interval);
    bt_status_t (*status_flags_changed)(uint8_t status_flags);
    bt_status_t (*call_state_changed)(uint8_t number, lea_tbs_call_state_t* state_s);
    bt_status_t (*notify_termination_reason)(uint8_t call_index, lea_adpt_termination_reason_t reason);
    bt_status_t (*call_control_response)(uint8_t call_index, lea_adpt_call_control_result_t result);
    void* (*register_callbacks)(void* handle, lea_tbs_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** handle, void* cookie);
} lea_tbs_interface_t;

/*
 * register profile to service manager
 */
void register_lea_tbs_service(void);

#endif /* __LEA_TBS_SERVICE_H__ */