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

#ifndef __BT_LEA_CCP_H__
#define __BT_LEA_CCP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include "lea_audio_common.h"
#include <stddef.h>

/**
 * @brief LE Audio ccp test callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer LE Audio device.
 */
typedef void (*lea_ccp_test_callback)(void* cookie, bt_address_t* addr);

typedef struct
{
    size_t size;
    lea_ccp_test_callback test_cb;
} lea_ccp_callbacks_t;

/**
 * @brief Register LE Audio ccp callback functions
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - LE Audio ccp callback functions.
 * @return void* - callback cookie.
 */
void* bt_lea_ccp_register_callbacks(bt_instance_t* ins, const lea_ccp_callbacks_t* callbacks);

/**
 * @brief Unregister LE Audio ccp callback functions
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callback cookie.
 * @return true - on unregister success.
 * @return false - on callback cookie not found.
 */
bool bt_lea_ccp_unregister_callbacks(bt_instance_t* ins, void* cookie);

/**
 * @brief Read bearer provider name of a remote TBS. Value is returned by
 * #lea_tbc_bearer_provider_name_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_bearer_provider_name(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read bearer uci of a remote TBS. Value is returned by
 * #lea_tbc_bearer_uci_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_bearer_uci(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read bearer technology of a remote TBS. Value is returned by
 * #lea_tbc_bearer_technology_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_bearer_technology(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read bearer uri schemes supported list of a remote TBS. Value is returned by
 * #lea_tbc_bearer_uri_schemes_supported_list_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_bearer_uri_schemes_supported_list(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read bearer signal strength of a remote TBS. Value is returned by
 * #lea_tbc_bearer_signal_strength_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_bearer_signal_strength(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read bearer signal strength report interval of a remote TBS. Value is returned by
 * #lea_tbc_bearer_signal_strength_report_interval_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_bearer_signal_strength_report_interval(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read content control id of a remote TBS. Value is returned by
 * #lea_tbc_content_control_id_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_content_control_id(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read status flags of a remote TBS. Value is returned by
 * #lea_tbc_status_flags_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_status_flags(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read call control optional opcodes of a remote TBS. Value is returned by
 * #lea_tbc_call_control_optional_opcodes_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_call_control_optional_opcodes(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read incoming call of a remote TBS. Value is returned by
 * #lea_tbc_incoming_call_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_incoming_call(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read incoming call target bearer uri of a remote TBS. Value is returned by
 * #lea_tbc_incoming_call_target_bearer_uri_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_incoming_call_target_bearer_uri(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read call state of a remote TBS. Value is returned by
 * #lea_tbc_call_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_call_state(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read bearer list current calls of a remote TBS. Value is returned by
 * #lea_tbc_bearer_list_current_calls_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_bearer_list_current_calls(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Read call friendly name of a remote TBS. Value is returned by
 * #lea_tbc_call_friendly_name_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_read_call_friendly_name(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Write an opcode with call index to the call control point of a remote
 * TBS. Response is returned by #lea_tbc_call_control_result_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_call_control_by_index(bt_instance_t* ins, bt_address_t* addr, uint8_t opcode);

/**
 * @brief Write Originate opcode to the call control point of a remote TBS.
 * Response is returned by #lea_tbc_call_control_result_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_originate_call(bt_instance_t* ins, bt_address_t* addr, uint8_t* uri);

/**
 * @brief Write Join opcode to the call control point of a remote TBS.
 * Response is returned by #lea_tbc_call_control_result_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccp_join_calls(bt_instance_t* ins, bt_address_t* addr, uint8_t number, uint8_t* call_indexes);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LEA_CCP_H__ */
