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
 * @brief Callback for playing status request from AVRCP Controller.
 *
 * During the initialization process of an AVRCP target, callback functions are registered.
 * This callback is triggered when a request for the current player status is received by
 * the AVRCP Target from an AVRCP Controller.
 *
 * The status of the player includes: playing, paused, stopped, seek forward, and seek rewind.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 *
 * **Example:**
 * @code
static void on_get_play_status_request_cb(void* cookie, bt_address_t* addr)
{
    avrcp_play_status_t status = AVRCP_PLAY_STATUS_PLAYING;
    uint32_t song_len = 180000;
    uint32_t song_pos = 10000;
    bt_instance_t* ins = cookie;
    bt_avrcp_target_get_play_status_response(ins, addr, status, song_len, song_pos);
}
 * @endcode
 */
typedef void (*avrcp_received_get_play_status_request_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Callback for register notification request.
 *
 * All player application attributes can be registered as events by an AVRCP Controller.
 * When an AVRCP Controller has registered a specific event to an AVRCP Target, the Target
 * shall notify the Controller on change in value of the registered event. In particular,
 * a notify command terminates after providing a corresponding change.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param event - The event that the AVRCP Controller wants to be notified about.
 * @param interval - Applicable only for PLAY_POS_CHANGED event.
 *
 * **Example:**
 * @code
uint16_t notify_event = 0;
static void on_register_notification_request_cb(void* cookie, bt_address_t* addr, avrcp_notification_event_t event, uint32_t interval)
{
    notify_event |= event;
}
 * @endcode
 */
typedef void (*avrcp_received_register_notification_request_callback)(void* cookie, bt_address_t* addr, avrcp_notification_event_t event, uint32_t interval);

/**
 * @brief Callback for panel operation.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param op - Panel operation.
 * @param state - Key state.
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
 * @brief Register callback functions to AVRCP Target service.
 *
 * When initializing an AVRCP Target, an application should register callback functions
 * to the AVRCP Target service. Subsequently, the AVRCP Target service will notify the
 * application of any state changes via the registered callback functions.
 *
 * Callback functions includes:
 *   * connection_state_cb
 *   * received_get_play_status_request_cb
 *   * received_register_notification_request_cb
 *   * received_panel_operation_cb
 *
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - AVRCP Target callback functions.
 * @return void* - Callbacks cookie, if the callback is registered successfuly. NULL,
 *                 if the callback is already registered or registration fails.
 *                 To obtain more information, refer to bt_remote_callbacks_register().
 *
 * **Example:**
 * @code
const static avrcp_target_callbacks_t g_avrcp_target_cbs = {
    .size = sizeof(avrcp_target_callbacks_t),
    .connection_state_cb = on_connection_state_changed_cb,
    .received_get_play_status_request_cb = on_get_play_status_request_cb,
    .received_register_notification_request_cb = on_register_notification_request_cb,
    .received_panel_operation_cb = on_received_panel_operation_cb,
};

void avrcp_target_init(void* ins)
{
    static void* target_cbks_cookie;

    target_cbks_cookie = bt_avrcp_target_register_callbacks(ins, &g_avrcp_target_cbs);
}
 * @endcode
 */
void* BTSYMBOLS(bt_avrcp_target_register_callbacks)(bt_instance_t* ins, const avrcp_target_callbacks_t* callbacks);

/**
 * @brief Unregister callback functions to AVRCP Target service.
 *
 * An application may use this interface to stop listening on the AVRCP Target
 * callbacks and to release the associated resources.
 *
 * @param ins - Bluetooth client instance.
 * @param cookie - Callbacks cookie.
 * @return true - Callback unregistration successful.
 * @return false - Callback cookie not found or callback unregistration failed.
 *
 * **Example:**
 * @code
void avrcp_target_uninit(void* ins)
{
    bt_avrcp_target_unregister_callbacks(ins, target_cbks_cookie);
}
 */
bool BTSYMBOLS(bt_avrcp_target_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Tell the AVRCP Controller the current state of the player.
 *
 * An application of an AVRCP Target will call this interface to send the current
 * status of the player when "received_get_play_status_request_cb" event is triggered.
 * Additionally, the total length and the position of the current song should also
 * be returned as described in the AVRCP profile. In particular, if this Target does
 * not support total length and current position of the song, then the Target shall
 * return 0xFFFFFFFF.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param status - Current play status.
 * @param song_len - Song length in ms.
 * @param song_pos - Current position in ms.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
static void on_get_play_status_request_cb(void* cookie, bt_address_t* addr)
{
    avrcp_play_status_t status = AVRCP_PLAY_STATUS_PLAYING;
    uint32_t song_len = 0xFFFFFFFF;
    uint32_t song_pos = 0xFFFFFFFF;
    bt_instance_t* ins = cookie;
    bt_avrcp_target_get_play_status_response(ins, addr, status, song_len, song_pos);
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_avrcp_target_get_play_status_response)(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t status,
    uint32_t song_len, uint32_t song_pos);

/**
 * @brief Notify the status of a player if peer device has registered the corresponding event.
 *
 * If the status of a player changes and an AVRCP Controller has registered for that event,
 * the AVRCP target should use that interface to send a change notification to the Controller
 * with the current status. In particular, after this notification, the registered notification
 * event by the AVRCP Controller becomes invalid.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param status - Current play status.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example:**
 * @code
uint16_t notify_event;

static void avrcp_target_play_status_changed(void* cookie, bt_address_t* addr, avrcp_play_status_t status)
{
    bt_instance_t* ins = cookie;
    if (notify_event & AVRCP_NOTIFICATION_EVENT_PLAY_STATUS_CHANGED) {
        bt_avrcp_target_play_status_notify(ins, addr, status);

        notify_event &= ~AVRCP_NOTIFICATION_EVENT_PLAY_STATUS_CHANGED;
    }
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_avrcp_target_play_status_notify)(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t status);

#ifdef __cplusplus
}
#endif
#endif /* __BT_AVRCP_TARGET_H__ */
