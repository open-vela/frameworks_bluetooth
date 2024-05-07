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
#ifndef __SAL_LEA_TBS_INTERFACE_H__
#define __SAL_LEA_TBS_INTERFACE_H__

#include "bt_status.h"
#include "stack_adapter_lea_ccp.h"
#include <stdint.h>

bt_status_t bt_sal_lea_tbs_add(uint32_t tbs_id);
bt_status_t bt_sal_lea_tbs_remove(uint32_t tbs_id);
bt_status_t bt_sal_lea_tbs_set_telephone_bearer_info(SERVICE_LEA_TELEPHONE_BEARER_S* bearer);
bt_status_t bt_sal_lea_tbs_add_call(SERVICE_LEA_TBS_CALL_S* call_s);
bt_status_t bt_sal_lea_tbs_remove_call(uint32_t tbs_id, uint8_t call_index);
bt_status_t bt_sal_lea_tbs_provider_name_changed(uint32_t tbs_id, uint8_t* name);
bt_status_t bt_sal_lea_tbs_bearer_technology_changed(uint32_t tbs_id, lea_adpt_bearer_technology_t technology);
bt_status_t bt_sal_lea_tbs_uri_schemes_supported_list_changed(uint32_t tbs_id, uint8_t* uri_schemes);
bt_status_t bt_sal_lea_tbs_rssi_value_changed(uint32_t tbs_id, uint8_t strength);
bt_status_t bt_sal_lea_tbs_rssi_interval_changed(uint32_t tbs_id, uint8_t interval);
bt_status_t bt_sal_lea_tbs_status_flags_changed(uint32_t tbs_id, uint8_t status_flags);
bt_status_t bt_sal_lea_tbs_call_state_changed(uint32_t tbs_id, uint8_t number, SERVICE_LEA_TBS_CALL_STATE_S* state_s);
bt_status_t bt_sal_lea_tbs_notify_termination_reason(uint32_t tbs_id, uint8_t call_index, lea_adpt_termination_reason_t reason);
bt_status_t bt_sal_lea_tbs_call_control_response(uint32_t tbs_id, uint8_t call_index, lea_adpt_call_control_result_t result);

#endif /* __SAL_LEA_TBS_INTERFACE_H__ */