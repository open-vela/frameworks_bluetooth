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
#include "stack_adapter_lea_gaf.h"

#include "bluetooth.h"
#include "bt_lea_ccp.h"
#include "lea_ccp_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_ccp_interface.h"

#define UNKNOWN_INFO "unknown"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP

static void adpt_lea_tbc_bearer_provider_name_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t *name);
static void adpt_lea_tbc_bearer_uci_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t *uci);
static void adpt_lea_tbc_bearer_technology_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t technology);
static void adpt_lea_tbc_bearer_uri_schemes_supported_list_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t *uri_schemes);
static void adpt_lea_tbc_bearer_signal_strength_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t strength);
static void adpt_lea_tbc_bearer_signal_strength_report_interval_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t interval);
static void adpt_lea_tbc_content_control_id_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t ccid);
static void adpt_lea_tbc_status_flags_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint16_t status_flags);
static void adpt_lea_tbc_call_control_optional_opcodes_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint16_t opcodes);
static void adpt_lea_tbc_incoming_call_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t call_index, uint8_t *uri);
static void adpt_lea_tbc_incoming_call_target_bearer_uri_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t call_index, uint8_t *uri);
static void adpt_lea_tbc_call_state_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint32_t number, SERVICE_LEA_TBS_CALL_STATE_S *states_s);
static void adpt_lea_tbc_bearer_list_current_calls_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint32_t number, SERVICE_LEA_TBS_CALLS_LIST_ITEM_S *calls);
static void adpt_lea_tbc_call_friendly_name_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t call_index, uint8_t *name);
static void adpt_lea_tbc_termination_reason_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t call_index, uint8_t reason);
static void adpt_lea_tbc_call_control_result_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t opcode, uint8_t call_index, uint8_t result);

const LEA_TBC_CALLBACK_S adpt_lea_ccp_client_callbacks = {
    .lea_tbc_bearer_provider_name_cb = adpt_lea_tbc_bearer_provider_name_callback,
    .lea_tbc_bearer_uci_cb = adpt_lea_tbc_bearer_uci_callback,
    .lea_tbc_bearer_technology_cb = adpt_lea_tbc_bearer_technology_callback,
    .lea_tbc_bearer_uri_schemes_supported_list_cb = adpt_lea_tbc_bearer_uri_schemes_supported_list_callback,
    .lea_tbc_bearer_signal_strength_cb = adpt_lea_tbc_bearer_signal_strength_callback,
    .lea_tbc_bearer_signal_strength_report_interval_cb = adpt_lea_tbc_bearer_signal_strength_report_interval_callback,
    .lea_tbc_content_control_id_cb = adpt_lea_tbc_content_control_id_callback,
    .lea_tbc_status_flags_cb = adpt_lea_tbc_status_flags_callback,
    .lea_tbc_call_control_optional_opcodes_cb = adpt_lea_tbc_call_control_optional_opcodes_callback,

    .lea_tbc_incoming_call_cb = adpt_lea_tbc_incoming_call_callback,
    .lea_tbc_incoming_call_target_bearer_uri_cb = adpt_lea_tbc_incoming_call_target_bearer_uri_callback,
    .lea_tbc_call_state_cb = adpt_lea_tbc_call_state_callback,
    .lea_tbc_bearer_list_current_calls_cb = adpt_lea_tbc_bearer_list_current_calls_callback,
    .lea_tbc_call_friendly_name_cb = adpt_lea_tbc_call_friendly_name_callback,
    .lea_tbc_termination_reason_cb = adpt_lea_tbc_termination_reason_callback,

    .lea_tbc_call_control_result_cb = adpt_lea_tbc_call_control_result_callback,
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_tbc_bearer_provider_name_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t *name)
{
    bt_address_t addr;
    const char *nullname = UNKNOWN_INFO;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    if (name == NULL) {
        lea_ccp_on_bearer_provider_name(&addr, tbs_id, strlen(nullname) + 1, nullname);
    } else {
        lea_ccp_on_bearer_provider_name(&addr, tbs_id, strlen((const char *)name) + 1,
                                         (const char *)name);
    }
}

static void adpt_lea_tbc_bearer_uci_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t *uci)
{
    bt_address_t addr;
    const char *nulluri = UNKNOWN_INFO;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    if (uci == NULL) {
        lea_ccp_on_bearer_uci(&addr, tbs_id, strlen(nulluri) + 1, nulluri);
    } else {
        lea_ccp_on_bearer_uci(&addr, tbs_id, strlen((const char *)uci) + 1, (const char *)uci);
    }
}

static void adpt_lea_tbc_bearer_technology_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t technology)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_bearer_technology(&addr, tbs_id, technology);
}

static void adpt_lea_tbc_bearer_uri_schemes_supported_list_callback(BD_ADDR tbs_addr, uint32_t tbs_id,
                                                                    uint8_t *uri_schemes)
{
    bt_address_t addr;
    const char *nulluri_schemes = UNKNOWN_INFO;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    if (uri_schemes == NULL) {
        lea_ccp_on_bearer_uri_schemes_supported_list(&addr, tbs_id, strlen(nulluri_schemes) + 1,
                                                      nulluri_schemes);
    } else {
        lea_ccp_on_bearer_uri_schemes_supported_list(&addr, tbs_id, strlen((const char *)uri_schemes) + 1,
                                                      (const char *)uri_schemes);
    }
}

static void adpt_lea_tbc_bearer_signal_strength_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t strength)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_bearer_signal_strength(&addr, tbs_id, strength);
}

static void adpt_lea_tbc_bearer_signal_strength_report_interval_callback(BD_ADDR tbs_addr, uint32_t tbs_id,
                                                                         uint8_t interval)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_bearer_signal_strength_report_interval(&addr, tbs_id, interval);
}

static void adpt_lea_tbc_content_control_id_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t ccid)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_content_control_id(&addr, tbs_id, ccid);
}

static void adpt_lea_tbc_status_flags_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint16_t status_flags)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_status_flags(&addr, tbs_id, status_flags);
}

static void adpt_lea_tbc_call_control_optional_opcodes_callback(BD_ADDR tbs_addr, uint32_t tbs_id,
                                                                uint16_t opcodes)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_call_control_optional_opcodes(&addr, tbs_id, opcodes);
}

static void adpt_lea_tbc_incoming_call_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t call_index,
                                                uint8_t *uri)
{
    bt_address_t addr;
    const char *nulluri = UNKNOWN_INFO;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    if (uri == NULL) {
        lea_ccp_on_incoming_call(&addr, tbs_id, call_index, strlen(nulluri) + 1, nulluri);
    } else {
        lea_ccp_on_incoming_call(&addr, tbs_id, call_index, strlen((const char *)uri) + 1,
                                  (const char *)uri);
    }
}

static void adpt_lea_tbc_incoming_call_target_bearer_uri_callback(BD_ADDR tbs_addr, uint32_t tbs_id,
                                                                  uint8_t call_index, uint8_t *uri)
{
    bt_address_t addr;
    const char *nulluri = UNKNOWN_INFO;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    if (uri == NULL) {
        lea_ccp_on_incoming_call_target_bearer_uri(&addr, tbs_id, call_index, strlen(nulluri) + 1, nulluri);
    } else {
        lea_ccp_on_incoming_call_target_bearer_uri(&addr, tbs_id, call_index, strlen((const char *)uri) + 1,
                                                    (const char *)uri);
    }
}

static void adpt_lea_tbc_call_state_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint32_t number,
                                             SERVICE_LEA_TBS_CALL_STATE_S *states_s)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_call_state(&addr, tbs_id, number, (lea_tbs_call_state_t *)states_s);

    stack_adapter_lea_mem_free(states_s);
}

static void adpt_lea_tbc_bearer_list_current_calls_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint32_t number,
                                                            SERVICE_LEA_TBS_CALLS_LIST_ITEM_S *calls)
{
    bt_address_t addr;
    const char *uri;
    uint8_t size;
    lea_tbs_call_list_item_t *calls_list;

    if (number < 1) {
        BT_LOGW("%s ,the number of bearer list current call is zero!", __func__);
        return;
    }

    for (int i = 0; i < number; i++) {
        uri = (const char *)(calls + i)->call_uri;
        size += sizeof(lea_tbs_call_list_item_t) + strlen(uri) + 1;
    }
    calls_list = (lea_tbs_call_list_item_t *)malloc(size);
    lea_tbs_call_list_item_t *sub_call = calls_list;
    void *p = sub_call;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);

    for (int i = 0; i < number; i++) {
        sub_call->index = (calls + i)->index;
        sub_call->state = (calls + i)->state;
        sub_call->flags = (calls + i)->flags;
        uri = (const char *)(calls + i)->call_uri;
        strcpy((sub_call)->call_uri, uri);
        p += sizeof(lea_tbs_call_list_item_t) + strlen(uri) + 1;
        sub_call = p;
    }
    lea_ccp_on_bearer_list_current_calls(&addr, tbs_id, number, size, calls_list);

    stack_adapter_lea_ccp_recycle_calls_list_s(number, calls);
}

static void adpt_lea_tbc_call_friendly_name_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t call_index,
                                                     uint8_t *name)
{
    bt_address_t addr;
    const char *nullname = UNKNOWN_INFO;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    if (name == NULL) {
        lea_ccp_on_call_friendly_name(&addr, tbs_id, call_index, strlen(nullname) + 1, nullname);
    } else {
        lea_ccp_on_call_friendly_name(&addr, tbs_id, call_index, strlen((const char *)name) + 1,
                                       (const char *)name);
    }
}

static void adpt_lea_tbc_termination_reason_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t call_index,
                                                     uint8_t reason)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_termination_reason(&addr, tbs_id, call_index, reason);
}

static void adpt_lea_tbc_call_control_result_callback(BD_ADDR tbs_addr, uint32_t tbs_id, uint8_t opcode,
                                                      uint8_t call_index, uint8_t result)
{
    bt_address_t addr;

    memcpy(addr.addr, tbs_addr, BD_ADDR_SIZE);
    lea_ccp_on_call_control_result(&addr, tbs_id, opcode, call_index, result);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bt_status_t bt_sal_lea_tbc_read_bearer_provider_name(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_bearer_provider_name(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_bearer_uci(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_bearer_uci(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_bearer_technology(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_bearer_technology(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_bearer_uri_schemes_supported_list(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_bearer_uri_schemes_supported_list(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_bearer_signal_strength(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_bearer_signal_strength(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_bearer_signal_strength_report_interval(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_bearer_signal_strength_report_interval(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_content_control_id(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_content_control_id(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_status_flags(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_status_flags(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_call_control_optional_opcodes(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_call_control_optional_opcodes(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_incoming_call(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_incoming_call(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_incoming_call_target_bearer_uri(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_incoming_call_target_bearer_uri(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_call_state(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_call_state(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_bearer_list_current_calls(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_bearer_list_current_calls(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_read_call_friendly_name(bt_address_t *addr, uint32_t tbs_id)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_read_call_friendly_name(addr->addr, tbs_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_call_control_by_index(bt_address_t *addr, uint32_t tbs_id, uint8_t opcode,
                                                 uint8_t call_index)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_call_control_by_index(addr->addr, tbs_id, opcode, call_index), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_originate_call(bt_address_t *addr, uint32_t tbs_id, uint8_t *uri)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_originate_call(addr->addr, tbs_id, uri), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_tbc_join_calls(bt_address_t *addr, uint32_t tbs_id, uint8_t number, uint8_t *call_indexes)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(stack_adapter_lea_tbc_join_calls(addr->addr, tbs_id, number, call_indexes), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif