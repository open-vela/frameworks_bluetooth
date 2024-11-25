/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#ifndef __BT_AVRCP_TARGET_H__
#define __BT_AVRCP_TARGET_H__

#include "bt_avrcp.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief Received Get Play Status request from CT.
 *
 * @param cookie - Callback cookie.
 * @param addr - Remote BT address.
 * @return void.
 */
typedef void (*avrcp_received_get_play_status_request_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Received register notification request callback.
 *
 * @param cookie - Callback cookie.
 * @param addr - Remote BT address.
 * @param event - The event for which the CT requires notifications.
 * @param interval - Only works in PLAY_POS_CHANGED event.
 */
typedef void (*avrcp_received_register_notification_request_callback)(void* cookie, bt_address_t* addr, avrcp_notification_event_t event, uint32_t interval);

/**
 * @brief Received panel operation from CT.
 *
 * @param cookie - Callback cookie.
 * @param addr - Remote BT address.
 * @param op - Panel operation.
 * @param state - Key state
 */
typedef void (*avrcp_received_panel_operation_callback)(void* cookie, bt_address_t* addr, uint8_t op, uint8_t state);

/**
 * @cond
 */

typedef struct {
    size_t size;
    avrcp_connection_state_callback connection_state_cb;
    avrcp_received_get_play_status_request_callback received_get_play_status_request_cb;
    avrcp_received_register_notification_request_callback received_register_notification_request_cb;
    avrcp_received_panel_operation_callback received_panel_operation_cb;
} avrcp_target_callbacks_t;

/**
 * @endcond
 */

/**
 * @brief Register callback functions to AVRCP Target
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - AVRCP Target callback functions.
 * @return void* - Callbacks cookie.
 */
void* BTSYMBOLS(bt_avrcp_target_register_callbacks)(bt_instance_t* ins, const avrcp_target_callbacks_t* callbacks);

/**
 * @brief Unregister callback functions to AVRCP Target
 *
 * @param ins - Bluetooth client instance.
 * @param cookie - Callbacks cookie.
 * @return bool - True, if unregister success, false otherwise.
 */
bool BTSYMBOLS(bt_avrcp_target_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Send response of Get Play Status request to CT.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - Remote BT address.
 * @param status - Current play status.
 * @param song_len - Song length in ms.
 * @param song_pos - Current position in ms.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_avrcp_target_get_play_status_response)(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t status,
    uint32_t song_len, uint32_t song_pos);

/**
 * @brief Notify playback status if peer device had register playback notification
 *
 * @param ins - Bluetooth client instance.
 * @param addr - Remote BT address.
 * @param status - current play status.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_avrcp_target_play_status_notify)(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t status);

#ifdef __cplusplus
}
#endif
#endif /* __BT_AVRCP_TARGET_H__ */
