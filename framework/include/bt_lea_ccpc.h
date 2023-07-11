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

#ifndef __BT_LEA_CCPC_H__
#define __BT_LEA_CCPC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include <stddef.h>

#define MAX_CALL_PROVIDER_NAME_SIZE 16
#define MAX_URI_SCHEMES_SIZE        16
#define MAX_CALL_URI_SIZE           16
#define MAX_UCI_SIZE                8
#define MAX_FRIENDLY_NAME_SIZE      8

/* LE Audio TBS struct */
typedef struct {
    uint32_t tbs_id; /**< ID of the TBS instance the telephone bearer attached to.*/
    void *bearer_ref; /**< Application specified bearer identity. */
    char provider_name[MAX_CALL_PROVIDER_NAME_SIZE]; /**< Initial Bearer Provider Name. Zero terminated UTF-8 string. */
    char uci[MAX_UCI_SIZE]; /**< Bearer UCI. Zero terminated UTF-8 string. */
    char uri_schemes[MAX_URI_SCHEMES_SIZE]; /**< Initial list of Bearer URI schemes supported. Zero terminated UTF-8 string. */
    uint8_t technology; /**< Initial Bearer Technology, one of #SERVICE_LEA_TBS_BEARER_TECHNLOGY.  */
    uint8_t signal_strength; /**< Initial Bearer Signal Strength, 0 indicates no service; 1 to 100 indicates the valid signal strength. 255 indicates that signal strength is unavailable or has no meaning.  */
    uint8_t signal_strength_report_interval; /**< Initial Signal Strength reporting interval in seconds. 0 to 255. 0 indicates that reporting signal strength only when it is changed. */
    uint16_t status_flags; /**< Server feature status. Bits of #SERVICE_LEA_TBS_STATUS_FLAGS. */
    uint16_t optional_opcodes_supported; /**< Call control point optional Opcodes supported. Bits of #SERVICE_LEA_TBS_SUPPORTED_CALL_CONTROL_OPCODES. */
} LEA_TELEPHONE_BEARER_S;

typedef struct {
    uint8_t index; /**< Call Index, 1 to 255. */
    uint8_t state; /**< Initial Call State, one of #SERVICE_LEA_TBS_CALL_STATE. */
    uint8_t flags; /**< Initial Call flags, bits of #SERVICE_LEA_TBS_CALL_FLAGS. */
    char call_uri[MAX_CALL_URI_SIZE]; /**< The Incoming Call URI or Outgoing Call URI. Zero terminated UTF-8 string. Set to NULL if the URI is unknown. */
    char incoming_target_uri[MAX_CALL_URI_SIZE]; /**< The Incoming Call Target Bearer URI. Zero terminated UTF-8 string. Set to NULL for an outgoing call or if the URI is unknown. */
    char friendly_name[MAX_FRIENDLY_NAME_SIZE]; /**< The Friendly Name of the incoming or outgoing call. Zero terminated UTF-8 string. Set to NULL if the URI is unknown. */
} LEA_TBS_CALL_S;

typedef struct {
    uint8_t index; /**< Call Index, 1 to 255. */
    uint8_t state; /**< Call State, one of #SERVICE_LEA_TBS_CALL_STATE. */
    uint8_t flags; /**< Call flags, bits of #SERVICE_LEA_TBS_CALL_FLAGS. */
} LEA_TBS_CALL_STATE_S;

typedef struct {
    uint8_t index; /**< Call Index, 1 to 255. */
    uint8_t state; /**< Call State, one of #SERVICE_LEA_TBS_CALL_STATE. */
    uint8_t flags; /**< Call flags, bits of #SERVICE_LEA_TBS_CALL_FLAGS. */
    char call_uri[0]; /**< The Incoming Call URI or Outgoing Call URI. Zero terminated UTF-8 string. Set to NULL if the URI is unknown. */
} LEA_TBS_CALLS_LIST_ITEM_S;

/** TBS call states. */
typedef enum {
    LEA_CCPC_CALL_STATE_INCOMING, /**< Incoming call: a remote party is calling. */
    LEA_CCPC_CALL_STATE_DIALING, /**< Dialing (outgoing call): Call the remote party, but remote party is not being alerted. */
    LEA_CCPC_CALL_STATE_ALERTING, /**< Alerting (outgoing call): Remote party is being alerted. */
    LEA_CCPC_CALL_STATE_ACTIVE, /**< Active (ongoing call): The call is in an active conversation. */
    LEA_CCPC_CALL_STATE_LOCALLY_HELD, /**< Locally Held: The call is held locally. */
    LEA_CCPC_CALL_STATE_REMOTELY_HELD, /**< Remotely Held: The call is held by the remote party. */
    LEA_CCPC_CALL_STATE_BOTH_HELD, /**< Locally and Remotely Held: The call is held both locally and remotely. */
} LEA_TBS_CALL_STATE;

/** Call control point opcodes. */
typedef enum {
    LEA_CCPC_CALL_CONTROL_ACCEPT, /**< Accept the specified incoming call. */
    LEA_CCPC_CALL_CONTROL_TERMINATE, /**< End the specified active, alerting, dialing, incoming or held call. */
    LEA_CCPC_CALL_CONTROL_LOCAL_HOLD, /**< Place the specified active or incoming call on local hold. */
    LEA_CCPC_CALL_CONTROL_LOCAL_RETRIEVE, /**< If the specified call is locally held, move it to an active call. Or, if it is locally and remotely held, move it to a remotely held call.*/
    LEA_CCPC_CALL_CONTROL_ORIGINATE, /**< Initiate a call to the remote party identified by the URI. */
    LEA_CCPC_CALL_CONTROL_JOIN, /**< Put calls (not in remotely held state) in the list to active and join the calls. Any calls in one of the remotely held state move to remotely held state and are joined with the other calls. */
} LEA_CALL_CONTROL_OPCODE;

/**
 * @brief LE Audio ccpc test callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer LE Audio device.
 */
typedef void (*lea_ccpc_test_callback)(void *cookie, bt_address_t *addr);

typedef struct
{
    size_t size;
    lea_ccpc_test_callback test_cb;
} lea_ccpc_callbacks_t;

/**
 * @brief Register LE Audio ccpc callback functions
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - LE Audio ccpc callback functions.
 * @return void* - callback cookie.
 */
void *bt_lea_ccpc_register_callbacks(bt_instance_t *ins, const lea_ccpc_callbacks_t *callbacks);

/**
 * @brief Unregister LE Audio ccpc callback functions
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callback cookie.
 * @return true - on unregister success.
 * @return false - on callback cookie not found.
 */
bool bt_lea_ccpc_unregister_callbacks(bt_instance_t *ins, void *cookie);

/**
 * @brief Read bearer provider name of a remote TBS. Value is returned by
 * #lea_tbc_bearer_provider_name_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_bearer_provider_name(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read bearer uci of a remote TBS. Value is returned by
 * #lea_tbc_bearer_uci_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_bearer_uci(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read bearer technology of a remote TBS. Value is returned by
 * #lea_tbc_bearer_technology_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_bearer_technology(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read bearer uri schemes supported list of a remote TBS. Value is returned by
 * #lea_tbc_bearer_uri_schemes_supported_list_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_bearer_uri_schemes_supported_list(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read bearer signal strength of a remote TBS. Value is returned by
 * #lea_tbc_bearer_signal_strength_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_bearer_signal_strength(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read bearer signal strength report interval of a remote TBS. Value is returned by
 * #lea_tbc_bearer_signal_strength_report_interval_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_bearer_signal_strength_report_interval(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read content control id of a remote TBS. Value is returned by
 * #lea_tbc_content_control_id_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_content_control_id(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read status flags of a remote TBS. Value is returned by
 * #lea_tbc_status_flags_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_status_flags(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read call control optional opcodes of a remote TBS. Value is returned by
 * #lea_tbc_call_control_optional_opcodes_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_call_control_optional_opcodes(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read incoming call of a remote TBS. Value is returned by
 * #lea_tbc_incoming_call_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_incoming_call(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read incoming call target bearer uri of a remote TBS. Value is returned by
 * #lea_tbc_incoming_call_target_bearer_uri_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_incoming_call_target_bearer_uri(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read call state of a remote TBS. Value is returned by
 * #lea_tbc_call_state_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_call_state(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read bearer list current calls of a remote TBS. Value is returned by
 * #lea_tbc_bearer_list_current_calls_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_bearer_list_current_calls(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Read call friendly name of a remote TBS. Value is returned by
 * #lea_tbc_call_friendly_name_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_read_call_friendly_name(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Write an opcode with call index to the call control point of a remote
 * TBS. Response is returned by #lea_tbc_call_control_result_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_call_control_by_index(bt_instance_t *ins, bt_address_t *addr, uint8_t opcode);

/**
 * @brief Write Originate opcode to the call control point of a remote TBS.
 * Response is returned by #lea_tbc_call_control_result_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_originate_call(bt_instance_t *ins, bt_address_t *addr, uint8_t *uri);

/**
 * @brief Write Join opcode to the call control point of a remote TBS.
 * Response is returned by #lea_tbc_call_control_result_callback.
 * @param ins - bluetooth client instance.
 * @param addr - Address of the remote server.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t bt_lea_ccpc_join_calls(bt_instance_t *ins, bt_address_t *addr, uint8_t number, uint8_t *call_indexes);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LEA_CCPC_H__ */
