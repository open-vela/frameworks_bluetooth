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
#ifndef __LEA_CCP_SERVICE_H__
#define __LEA_CCP_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_lea_ccp.h"
#include "sal_lea_ccp_interface.h"
#include "stddef.h"

typedef struct {
    uint8_t num;
    uint32_t sid;
} bts_tbs_info_s;

typedef struct {
    uint32_t tbs_id;
    char provider_name[MAX_PROVIDER_NAME_LENGTH];
    char uci[MAX_UCI_LENGTH];
    uint8_t technology;
    char uri_schemes[MAX_URI_SCHEMES_LENGTH];
    uint8_t strength;
    uint8_t interval;
    uint8_t ccid;
    uint16_t status_flags;
    uint16_t opcodes;
    uint8_t call_index;
    char uri[MAX_CALL_URI_LENGTH];
    char friendly_name[MAX_PROVIDER_NAME_LENGTH];
} bearer_tele_info_t;

/*
 * sal callback
 */
void lea_ccp_on_bearer_provider_name(bt_address_t* addr, uint32_t tbs_id, size_t size, const char* name);
void lea_ccp_on_bearer_uci(bt_address_t* addr, uint32_t tbs_id, size_t size, const char* uci);
void lea_ccp_on_bearer_technology(bt_address_t* addr, uint32_t tbs_id, uint8_t technology);
void lea_ccp_on_bearer_uri_schemes_supported_list(bt_address_t* addr, uint32_t tbs_id, size_t size, const char* uri_schemes);
void lea_ccp_on_bearer_signal_strength(bt_address_t* addr, uint32_t tbs_id, uint8_t strength);
void lea_ccp_on_bearer_signal_strength_report_interval(bt_address_t* addr, uint32_t tbs_id, uint8_t interval);
void lea_ccp_on_content_control_id(bt_address_t* addr, uint32_t tbs_id, uint8_t ccid);
void lea_ccp_on_status_flags(bt_address_t* addr, uint32_t tbs_id, uint16_t status_flags);
void lea_ccp_on_call_control_optional_opcodes(bt_address_t* addr, uint32_t tbs_id, uint16_t opcodes);
void lea_ccp_on_incoming_call(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index, size_t size, const char* uri);
void lea_ccp_on_incoming_call_target_bearer_uri(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index, size_t size, const char* uri);
void lea_ccp_on_call_state(bt_address_t* addr, uint32_t tbs_id, uint32_t number, lea_tbs_call_state_t* states_s);
void lea_ccp_on_bearer_list_current_calls(bt_address_t* addr, uint32_t tbs_id, uint32_t number, size_t size, lea_tbs_call_list_item_t* calls);
void lea_ccp_on_call_friendly_name(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index, size_t size, const char* name);
void lea_ccp_on_termination_reason(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index, lea_adpt_termination_reason_t reason);
void lea_ccp_on_call_control_result(bt_address_t* addr, uint32_t tbs_id, uint8_t opcode, uint8_t call_index, lea_adpt_call_control_result_t result);

typedef struct {
    size_t size;
    bt_status_t (*read_bearer_provider_name)(bt_address_t* tbs_addr);
    bt_status_t (*read_bearer_uci)(bt_address_t* tbs_addr);
    bt_status_t (*read_bearer_technology)(bt_address_t* tbs_addr);
    bt_status_t (*read_bearer_uri_schemes_supported_list)(bt_address_t* tbs_addr);
    bt_status_t (*read_bearer_signal_strength)(bt_address_t* tbs_addr);
    bt_status_t (*read_bearer_signal_strength_report_interval)(bt_address_t* tbs_addr);
    bt_status_t (*read_content_control_id)(bt_address_t* tbs_addr);
    bt_status_t (*read_status_flags)(bt_address_t* tbs_addr);
    bt_status_t (*read_call_control_optional_opcodes)(bt_address_t* tbs_addr);
    bt_status_t (*read_incoming_call)(bt_address_t* tbs_addr);
    bt_status_t (*read_incoming_call_target_bearer_uri)(bt_address_t* tbs_addr);
    bt_status_t (*read_call_state)(bt_address_t* tbs_addr);
    bt_status_t (*read_bearer_list_current_calls)(bt_address_t* tbs_addr);
    bt_status_t (*read_call_friendly_name)(bt_address_t* tbs_addr);
    bt_status_t (*call_control_by_index)(bt_address_t* tbs_addr, uint8_t opcode);
    bt_status_t (*originate_call)(bt_address_t* tbs_addr, uint8_t* uri);
    bt_status_t (*join_calls)(bt_address_t* tbs_addr, uint8_t number, uint8_t* call_indexes);
    void* (*register_callbacks)(void* handle, lea_ccp_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** handle, void* cookie);
} lea_ccp_interface_t;

/*
 * register profile to service manager
 */
void register_lea_ccp_service(void);

/*
 * set tbs id infomation
 */
void adpt_tbs_sid_changed(uint32_t sid);

#endif /* __LEA_CCP_SERVICE_H__ */
