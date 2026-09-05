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
#ifndef __BT_A2DP_SINK_H__
#define __BT_A2DP_SINK_H__
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
 * @param cookie - Callbacks cookie.
 * @param addr - The Bluetooth address of the peer device.
 *
 * **Example:**
 * @code
static void on_audio_config_changed_cb(void* cookie, bt_address_t* addr)
{
    printf("A2DP audio sink configuration is changed");
}
 * @endcode
 */
typedef void (*a2dp_audio_sink_config_callback)(void* cookie, bt_address_t* addr);

/**
 * @cond
 */
/**
 * @brief A2DP Sink callback structure
 *
 */
typedef struct {
    /** set to sizeof(a2dp_sink_callbacks_t) */
    size_t size;
    a2dp_connection_state_callback connection_state_cb;
    a2dp_audio_state_callback audio_state_cb;
    a2dp_audio_sink_config_callback audio_sink_config_cb;
} a2dp_sink_callbacks_t;
/**
 * @endcond
 */

/**
 * @brief Register callback functions to A2DP Sink service.
 *
 * When initializing the A2DP Sink, an application should register callback functions
 * to the A2DP Sink service, which includes a connection state callback function, an
 * audio state callback function, and an audio sink configuration callback function.
 * Subsequently, the A2DP Sink service will notify the application of any state
 * changes via the registered callback functions.
 *
 * @param ins - Buetooth client instance.
 * @param callbacks - A2DP Sink callback functions.
 * @return void* - Callbacks cookie, if the callback is registered successfuly. NULL
 *                 if the callback is already registered or registration fails.
 *                 To obtain more information, refer to bt_remote_callbacks_register().
 *
 * **Example:**
 * @code
static const a2dp_sink_callbacks_t a2dp_sink_cbs = {
    sizeof(a2dp_sink_cbs),
    a2dp_sink_connection_state_cb,
    a2dp_sink_audio_state_cb,
    a2dp_sink_audio_config_cb,
};

void a2dp_sink_init(void* ins)
{
    static void* sink_cbks_cookie;

    sink_cbks_cookie = bt_a2dp_sink_register_callbacks(ins, &a2dp_sink_cbs);
}
 * @endcode
 */
void* BTSYMBOLS(bt_a2dp_sink_register_callbacks)(bt_instance_t* ins, const a2dp_sink_callbacks_t* callbacks);

/**
 * @brief Unregister callback functions from A2DP Sink service.
 *
 * An application may use this interface to stop listening on the A2DP Sink
 * callbacks and to release the associated resources.
 *
 * @param ins - Buetooth client instance.
 * @param cookie - Callbacks cookie.
 * @return true - Callback unregistration successful.
 * @return false - Callback cookie not found or callback unregistration failed.
 *
 * **Example:**
 * @code
static void* sink_cbks_cookie = bt_a2dp_sink_register_callbacks(ins, &a2dp_sink_cbs);

void a2dp_sink_uninit(void* ins)
{
    bt_a2dp_sink_unregister_callbacks(ins, sink_cbks_cookie);
}
 * @endcode
 */
bool BTSYMBOLS(bt_a2dp_sink_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check if A2DP is already connected.
 *
 * This function serves the purpose of verifying the connection status of A2DP.
 * It is important to note that the A2DP profile is deemed connected solely when
 * the A2DP Sink is either in the Opened or Started state.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - A2DP Sink connected.
 * @return false - A2DP Sink not connected.
 *
 * **Example:**
 * @code
void a2dp_sink_connected(void* ins, bt_address_t* addr)
{
    bool ret = bt_a2dp_sink_is_connected(ins, addr);

    printf("A2DP sink is %s", ret? "connected" : "not connected");
}
 * @endcode
 */
bool BTSYMBOLS(bt_a2dp_sink_is_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Check if the stream is being transmitted.
 *
 * This function is used to verify the streaming status of the A2DP Sink.
 * The A2DP Sink can only be deemed as playing when it is in the Started
 * state and the audio is also in the Started state.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return true - Stream is being transmitted .
 * @return false - No stream transmission.
 *
 * **Example**
 * @code
void a2dp_sink_playing(void* ins, bt_address_t* addr)
{
    bool ret = bt_a2dp_sink_is_playing(ins, addr);

    printf("A2DP sink is %s", ret? "playing" : "not playing");
}
 * @endcode
 */
bool BTSYMBOLS(bt_a2dp_sink_is_playing)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Obtain A2DP Sink connection state.
 *
 * There are four states for A2DP connection, namely DISCONNECTED, CONNECTING,
 * CONNECTED, and DIACONNECTED. This function is used by the application of
 * the A2DP Sink to obtain the current state of the A2DP profile connection.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return profile_connection_state_t - A2DP profile connection state.
 *
 * **Example**
 * @code
int get_state_cmd(void* ins, bt_address_t* addr)
{
    int state = bt_a2dp_sink_get_connection_state(ins, addr);

    printf("A2DP sink connection state is: %d", state);

    return state;
}
 * @endcode
 */
profile_connection_state_t BTSYMBOLS(bt_a2dp_sink_get_connection_state)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish connection with peer A2DP device.
 *
 * This function is used to establish an A2DP profile connection with a designated
 * peer device. Following the successful creation of the A2DP connection, the A2DP
 * Sink transitions to an Opened state and subsequently notifies the application of
 * its CONNECTED state. Upon reception of audio data from the A2DP Source device,
 * the A2DP Sink then proceeds to forward the audio data to the Media.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example**
 * @code
int a2dp_sink_connect(void* ins, bt_address_t* addr)
{
    int ret = bt_a2dp_sink_connect(ins, addr);

    printf("A2DP sink connect %s", ret ? "failed" : "success");

    return ret;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_a2dp_sink_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from peer A2DP device.
 *
 * This function is utilized for the purpose of disconnecting an A2DP profile connection
 * with a designated peer device. Upon successful disconnection of the A2DP connection,
 * the A2DP Sink transitions into an Idle state and subsequently notifies the application
 * of the DISCONNECTED state.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example**
 * @code
int a2dp_sink_disconnect(void* ins, bt_address_t* addr)
{
    int ret = bt_a2dp_sink_disconnect(ins, addr);

    printf("A2DP sink disconnect %s", ret ? "failed" : "success");

    return ret;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_a2dp_sink_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/*
 * @brief Set a peer A2DP Source device as active device.
 *
 * @param ins - Bluetooth client instance.
 * @param addr - The Bluetooth address of the peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 *
 * **Example**
 * @code
int enable_source_device(void* ins, bt_address_t* addr)
{
    int ret = bt_a2dp_sink_set_active_device(ins, addr);

    return ret;
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_a2dp_sink_set_active_device)(bt_instance_t* ins, bt_address_t* addr);

#ifdef __cplusplus
}
#endif
#endif /* __BT_A2DP_SINK_H__ */
