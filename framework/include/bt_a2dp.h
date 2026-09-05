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
#ifndef __BT_A2DP_H__
#define __BT_A2DP_H__

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"

/**
 * @cond
 */
/**
 * @brief A2DP audio state
 */
typedef enum {
    A2DP_AUDIO_STATE_REMOTE_SUSPEND = 0,
    A2DP_AUDIO_STATE_STOPPED,
    A2DP_AUDIO_STATE_STARTED,
} a2dp_audio_state_t;
/**
 * @endcond
 */

/**
 * @brief Callback for A2DP connection state changed.
 *
 * There are four states for A2DP connection, namely DISCONNECTED, CONNECTING,
 * CONNECTED, and DISCONNECTING. During the initialization phase of the A2DP,
 * it is necessary to register callback functions. This callback is triggered
 * when there is a change in the state of the A2DP connection.
 *
 * Stable States:
 *    DISCONNECTED: The initial state.
 *    CONNECTED: The A2DP connection is established.
 * Transient states:
 *    CONNECTING: The A2DP connection is being established.
 *    DISCONNECTING: The A2DP connection is being terminated.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - A2DP profile connection state.
 *
 * **Example:**
 * @code
static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    printf("A2DP connection state is: %d", state);
}
 * @endcode
 */
typedef void (*a2dp_connection_state_callback)(void* cookie, bt_address_t* addr,
    profile_connection_state_t state);

/**
 * @brief Callback for A2DP audio state changed.
 *
 * There are three states for A2DP audio, namely SUSPEND, STOPPED,
 * and STARTED. It is important to note that a callback function
 * is triggered whenever a change occurs in the audio state.
 *
 * Stable States:
 *    SUSPEND: The stream is suspended.
 *    STOPPED: The stream is stopped.
 *    STARTED: The stream is started.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - A2DP audio connection state.
 *
 * **Example:**
 * @code
static void on_audio_state_changed_cb(void* cookie, bt_address_t* addr, a2dp_audio_state_t state)
{
    printf("A2DP audio state is: %d", state);
}
 * @endcode
 */
typedef void (*a2dp_audio_state_callback)(void* cookie, bt_address_t* addr,
    a2dp_audio_state_t state);

#endif /* __BT_A2DP_H__ */
