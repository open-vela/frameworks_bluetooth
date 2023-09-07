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
#ifndef __SAL_LEA_CCP_INTERFACE_H__
#define __SAL_LEA_CCP_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_lea_ccp.h"
#include "bt_status.h"
#include "stack_adapter_common.h"
#include "stack_adapter_lea_ccp.h"

bt_status_t bt_sal_lea_tbc_read_bearer_provider_name(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_bearer_uci(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_bearer_technology(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_bearer_uri_schemes_supported_list(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_bearer_signal_strength(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_bearer_signal_strength_report_interval(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_content_control_id(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_status_flags(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_call_control_optional_opcodes(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_incoming_call(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_incoming_call_target_bearer_uri(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_call_state(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_bearer_list_current_calls(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_read_call_friendly_name(bt_address_t *addr, uint32_t tbs_id);
bt_status_t bt_sal_lea_tbc_call_control_by_index(bt_address_t *addr, uint32_t tbs_id, uint8_t opcode, uint8_t call_index);
bt_status_t bt_sal_lea_tbc_originate_call(bt_address_t *addr, uint32_t tbs_id, uint8_t *uri);
bt_status_t bt_sal_lea_tbc_join_calls(bt_address_t *addr, uint32_t tbs_id, uint8_t number, uint8_t *call_indexes);

#endif /* __SAL_LEA_CCP_INTERFACE_H__ */