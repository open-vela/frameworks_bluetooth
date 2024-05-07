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

#ifndef __BT_LEA_TBS_H__
#define __BT_LEA_TBS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include "lea_audio_common.h"
#include <stddef.h>

typedef void (*lea_tbs_test_callback)(void* cookie, uint8_t value, bool added);

typedef struct
{
    size_t size;
    lea_tbs_test_callback tbs_test_cb;
} lea_tbs_callbacks_t;

/**
 * @brief Register tbs server callback functions
 *
 * @param ins - bluetooth server instance.
 * @param callbacks - ccp server callback functions.
 * @return void* - callback cookie.
 */
void* bt_lea_tbs_register_callbacks(bt_instance_t* ins, const lea_tbs_callbacks_t* callbacks);

/**
 * @brief Unregister tbs server callback functions
 *
 * @param ins - bluetooth server instance.
 * @param cookie - callback cookie.
 * @return true - on unregister success.
 * @return false - on callback cookie not found.
 */
bool bt_lea_tbs_unregister_callbacks(bt_instance_t* ins, void* cookie);

/**
 * @brief TBS instance add. Value is returned by
 * #lea_tbs_state_callback.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_service_add(bt_instance_t* ins);

/**
 * @brief TBS instance remove. Value is returned by
 * #lea_tbs_state_callback.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_service_remove(bt_instance_t* ins);

/**
 * @brief TBS set telephone bearer information. Value is returned by
 * #lea_tbs_bearer_set_callback.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_set_telephone_bearer_info(bt_instance_t* ins, lea_tbs_telephone_bearer_t* bearer);

/**
 * @brief TBS add a new call state member. Value is returned by
 * #lea_tbs_call_added_callback.
 * @param[in] call_s Call information.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_add_call(bt_instance_t* ins, lea_tbs_calls_t* call_s);

/**
 * @brief TBS remove a call when it is terminated by either side. Value is returned by
 * #lea_tbs_call_removed_callback.
 * @param[in] call_index Index of the call to remove, 1 to 255.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_remove_call(bt_instance_t* ins, uint8_t call_index);

/**
 * @brief Provider name is changed.
 * @param[in] name The new provider name, zero terminated UTF-8 string. Set to NULL if
 * not available.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_provider_name_changed(bt_instance_t* ins, uint8_t* name);

/**
 * @brief Bearer technology is changed.
 * @param[in] technology The new Bearer technology.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_bearer_technology_changed(bt_instance_t* ins, lea_adpt_bearer_technology_t technology);

/**
 * @brief Bearer URI schemes supported list is changed.
 * @param[in] uri_schemes The updated list of Bearer URI schemes supported.
 * Zero terminated UTF-8 string.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_uri_schemes_supported_list_changed(bt_instance_t* ins, uint8_t* uri_schemes);

/**
 * @brief Signal strength is changed.
 * @param[in] strength Current bearer signal strength, 0 indicates no service;
 * 1 to 100 indicates the valid signal strength. 255 indicates that signal
 * strength is unavailable or has no meaning for this bearer.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_rssi_value_changed(bt_instance_t* ins, uint8_t strength);

/**
 * @brief Signal strength reporting interval is changed.
 * @param[in] interval The updated reporting interval in seconds, 0 to 255.
 * 0 indicates that reporting signal strength only when it is changed.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_rssi_interval_changed(bt_instance_t* ins, uint8_t interval);

/**
 * @brief Status flags is changed.
 * @param[in] status_flags The new status flags. Bits of #SERVICE_LEA_TBS_STATUS_FLAGS.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_status_flags_changed(bt_instance_t* ins, uint16_t status_flags);

/**
 * @brief Call state is changed locally or by remote action request.
 * @param[in] number Number of call states in state_s. At least 1.
 * @param[in] state_s List of call states of calls that are changed.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_call_state_changed(bt_instance_t* ins, uint8_t number, lea_tbs_call_state_t* state_s);

/**
 * @brief Notify the clients that a call is terminated.
 * @param[in] call_index Index of the call terminated.
 * @param[in] reason Termination reason code, one of
 * #SERVICE_LEA_TBS_TERMINATION_REASON. Other extended reason may also be set.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_notify_termination_reason(bt_instance_t* ins, uint8_t call_index, lea_adpt_termination_reason_t reason);

/**
 * @brief Call control response.
 * @param[in] call_index Call index. Set to 0 if result is not success. Set to
 * the Call Index value assigned to the new call for the Originate operation.
 * Set to the first Call index in the provided Call Indexes list for the Join
 * operation. Set to the provided Call Index for the other operations
 * @param[in] result Result of the control operation.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_tbs_call_control_response(bt_instance_t* ins, uint8_t call_index, lea_adpt_call_control_result_t result);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LEA_TBS_H__ */