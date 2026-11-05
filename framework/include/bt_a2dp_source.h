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
#ifndef __BT_A2DP_SOURCE_H__
#define __BT_A2DP_SOURCE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "bt_a2dp.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief Callback for A2DP audio configuration.
 *
 * During the initialization process of the A2DP, callback functions are registered.
 * This callbacks is triggered whenever a change occurs in the audio configuration
 * of an A2DP connection.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 *
 * **Example:**
 * @code
static void on_audio_config_changed_cb(void* cookie, bt_address_t* addr)
{
    printf("A2DP audio source configuration is changed");
}
 * @endcode
 */
typedef void (*a2dp_audio_source_config_callback)(void* cookie, bt_address_t* addr);

/**
 * @cond
 */
/**
 * @brief A2DP source callback structure
 *
 */
typedef struct {
    /** set to sizeof(a2dp_source_callbacks_t) */
    size_t size;
    a2dp_connection_state_callback connection_state_cb;
    a2dp_audio_state_callback audio_state_cb;
    a2dp_audio_source_config_callback audio_source_config_cb;
} a2dp_source_callbacks_t;
/**
 * @endcond
 */

/**
 * @brief Register callback functions to A2DP Source service.
 *
 * When the A2DP Source is initialized, an application registers various callback
 * functions to the A2DP Source service, such as the connection state callback function,
 * audio state callback function, and audio source configuration callback function.
 * The A2DP Source service will notify the application of any state changes via the
 * registered callback functions.
 *
 * @param ins - Bluetooth client instance.
 * @param callbacks - A2DP Source callback functions.
 * @return void* - Callbacks cookie, if the callback is registered successfuly.
 *                 NULL if the callback is already registered or registration fails.
 *
 * **Example:**
 * @code
static const a2dp_source_callbacks_t a2dp_src_cbs = {
    sizeof(a2dp_src_cbs),
    a2dp_src_connection_state_cb,
    a2dp_src_audio_state_cb,
    a2dp_src_audio_source_config_cb,
};

void a2dp_source_uninit(void* ins)
{
    static void* src_cbks_cookie;

    src_cbks_cookie = bt_a2dp_source_register_callbacks(ins, &a2dp_src_cbs);
}
 * @endcode
 */
void* BTSYMBOLS(bt_a2dp_source_register_callbacks)(bt_instance_t* ins, const a2dp_source_callbacks_t* callbacks);

/**
 * @brief Unregister callback functions to A2DP Source service.
 *
 * An application may use this interface to stop listening on the A2DP Source
 * callbacks and to release the associated resources.
 *
 * @param ins - Bluetooth client instance.
 * @param cookie - Callbacks cookie.
 * @return true - Callback unregistration successful.
 * @return false - Callback cookie not found or callback unregistration failed.
 *
 * **Example:**
 * @code
static void* src_cbks_cookie = bt_a2dp_source_register_callbacks(ins, &a2dp_src_cbs);

void a2dp_source_uninit(void* ins)
{
    bt_a2dp_source_unregister_callbacks(ins, src_cbks_cookie);
}
 * @endcode
 */
bool BTSYMBOLS(bt_a2dp_source_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check if A2DP is already connected.
 *
 * This function serves the purpose of verifying the connection status of A2DP.
 * It is important to note that the A2DP profile is deemed connected solely when
 * the A2DP Source is either in the Opened or Started state.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - A2DP Source connected.
 * @return false - A2DP Source not connected.
 *
 * **Example:**
 * @code
void a2dp_source_connected(void* ins, bt_address_t* addr)
{
    bool ret = bt_a2dp_source_is_connected(ins, addr);

    printf("A2DP source is %s", ret? "connected" : "not connected");
}
 * @endcode
 */
bool BTSYMBOLS(bt_a2dp_source_is_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Check if the stream is being transmitted.
 *
 * This function is used to verify the streaming status of the A2DP Source.
 * The A2DP Source can only be deemed as playing when it is in the Started
 * state and the audio is also in the Started state.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - Stream is being transmitted.
 * @return false - No stream transmission.
 *
 * **Example**
 * @code
void a2dp_source_playing(void* ins, bt_address_t* addr)
{
    bool ret = bt_a2dp_source_is_playing(ins, addr);

    printf("A2DP source is %s", ret? "playing" : "not playing");
}
 * @endcode
 */
bool BTSYMBOLS(bt_a2dp_source_is_playing)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Obtain A2DP Source connection state.
 *
 * There are four states for A2DP connection, namely DISCONNECTED, CONNECTING,
 * CONNECTED, and DIACONNECTED. This function is used by the application of
 * the A2DP Source to obtain the current state of the A2DP profile connection.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return profile_connection_state_t - A2DP profile connection state.
 *
 * **Example**
 * @code
int get_state_cmd(void* ins, bt_address_t* addr)
{
    int state = bt_a2dp_source_get_connection_state(ins, addr);

    printf("A2DP source connection state is: %d", state);

    return state;
}
 * @endcode
 */
profile_connection_state_t BTSYMBOLS(bt_a2dp_source_get_connection_state)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish connection with peer A2DP device.
 *
 * This function is used to establish an A2DP profile connection with a designated
 * peer device. Upon successful establishment of the A2DP connection, the A2DP Source
 * transitions to an Opened state and notifies the application of its CONNECTED status.
 * Upon receipt of audio data from the media, the A2DP Source subsequently relays the
 * audio data to the A2DP Sink.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example**
 * @code
int a2dp_source_connect(void* ins, bt_address_t* addr)
{
    int ret = bt_a2dp_source_connect(ins, &addr);

    printf("A2DP source connect %s", ret ? "failed" : "success");

    return ret;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_a2dp_source_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from peer A2DP device.
 *
 * This function is used to remove an A2DP profile connection with a specific peer device. Once
 * the A2DP connection is removed, the A2DP Source turns into the Idle state and reports the
 * DISCONNECTED state to the application.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example**
 * @code
int a2dp_source_disconnect(void* ins, bt_address_t* addr)
{
    int ret = bt_a2dp_source_disconnect(ins, addr);

    printf("A2DP source disconnect %s", ret ? "failed" : "success");

    return ret;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_a2dp_source_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set a peer A2DP Sink device as silence device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @param silence - True, switch to silence mode to keep this device inactive.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example**
 * @code
int disable_sink_device(void* ins, bt_address_t* addr, bool silence)
{
    int ret = bt_a2dp_source_set_silence_device(ins, addr, silence);

    return ret;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_a2dp_source_set_silence_device)(bt_instance_t* ins, bt_address_t* addr, bool silence);

/**
 * @brief Set a peer A2DP Sink device as active device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example**
 * @code
int enable_sink_device(void* ins, bt_address_t* addr)
{
    int ret = bt_a2dp_source_set_active_device(ins, addr);

    return ret;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_a2dp_source_set_active_device)(bt_instance_t* ins, bt_address_t* addr);

#ifdef __cplusplus
}
#endif
#endif /* __BT_A2DP_SOURCE_H__ */
