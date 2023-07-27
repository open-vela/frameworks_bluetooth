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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_lea_ccp.h"

#include "bluetooth.h"
#include "lea_tbs_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_tbs_interface.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS

void adpt_lea_tbs_state_callback(uint32_t tbs_id, uint8_t ccid, bool added)
{
    lea_tbs_on_state_changed(tbs_id, ccid, added);
}

void adpt_lea_tbs_bearer_set_callback(uint32_t tbs_id, void *bearer_ref, bool result)
{
    lea_tbs_on_bearer_info_set(tbs_id, bearer_ref, result);
}

void adpt_lea_tbs_call_added_callback(uint32_t tbs_id, uint8_t call_index,
                                      bool result)
{
    lea_tbs_on_call_added(tbs_id, call_index, result);
}

void adpt_lea_tbs_call_removed_callback(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_on_call_removed(tbs_id, call_index);
}

void adpt_lea_tbs_accept_callback(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_on_accept_call(tbs_id, call_index);
}

void adpt_lea_tbs_terminate_callback(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_on_terminate_call(tbs_id, call_index);
}

void adpt_lea_tbs_local_hold_callback(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_on_local_hold_call(tbs_id, call_index);
}

void adpt_lea_tbs_local_retrieve_callback(uint32_t tbs_id, uint8_t call_index)
{
    lea_tbs_on_local_retrieve_call(tbs_id, call_index);
}

void adpt_lea_tbs_originate_callback(uint32_t tbs_id, SERVICE_LEA_UTF8_STR *uri)
{
    lea_tbs_on_originate_call(tbs_id, (size_t)uri->length + 1, (char *)uri->string);
}

void adpt_lea_tbs_join_callback(uint32_t tbs_id, uint8_t index_number,
                                uint8_t *index_list)
{
    lea_tbs_on_join_call(tbs_id, index_number, strlen((char *)index_list) + 1, (char *)index_list);
}

/****************************************************************************
 *
 ****************************************************************************/
bt_status_t bt_sal_lea_tbs_add(uint32_t tbs_id)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_add(tbs_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_remove(uint32_t tbs_id)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_remove(tbs_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_set_telephone_bearer_info(SERVICE_LEA_TELEPHONE_BEARER_S *bearer)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_set_telephone_bearer(bearer), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_add_call(SERVICE_LEA_TBS_CALL_S *call_s)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_add_call(call_s), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_remove_call(uint32_t tbs_id, uint8_t call_index)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_remove_call(tbs_id, call_index), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_provider_name_changed(uint32_t tbs_id, uint8_t *name)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_provider_name_changed(tbs_id, name), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_bearer_technology_changed(uint32_t tbs_id, lea_adpt_bearer_technology_t technology)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_bearer_technology_changed(tbs_id, (SERVICE_LEA_TBS_BEARER_TECHNLOGY)technology),
                  SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_uri_schemes_supported_list_changed(uint32_t tbs_id, uint8_t *uri_schemes)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_uri_schemes_supported_list_changed(tbs_id, uri_schemes), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_rssi_value_changed(uint32_t tbs_id, uint8_t strength)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_signal_strength_changed(tbs_id, strength), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_rssi_interval_changed(uint32_t tbs_id, uint8_t interval)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_signal_strength_report_interval_changed(tbs_id, interval), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_status_flags_changed(uint32_t tbs_id, uint8_t status_flags)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_status_flags_changed(tbs_id, status_flags), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_call_state_changed(uint32_t tbs_id, uint8_t number, SERVICE_LEA_TBS_CALL_STATE_S *state_s)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_call_state_changed(tbs_id, number, state_s),
                  SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_notify_termination_reason(uint32_t tbs_id, uint8_t call_index, lea_adpt_termination_reason_t reason)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_notify_termination_reason(tbs_id, call_index, reason), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbs_call_control_response(uint32_t tbs_id, uint8_t call_index, lea_adpt_call_control_result_t result)
{
    SAL_CHECK_RET(stack_adapter_lea_tbs_call_control_response(tbs_id, call_index, result), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

#endif