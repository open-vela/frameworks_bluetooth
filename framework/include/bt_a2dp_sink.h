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
 * @brief A2DP audio sink config changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer A2DP source device.
 */
typedef void (*a2dp_audio_sink_config_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief A2DP sink callback structure
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
 * @brief Register callback functions to A2DP sink service
 *
 * @param ins - bluetooth client instance.
 * @param id - A2DP sink callback functions.
 * @return void* - callbacks cookie.
 */
void* BTSYMBOLS(bt_a2dp_sink_register_callbacks)(bt_instance_t* ins, const a2dp_sink_callbacks_t* callbacks);

/**
 * @brief Unregister callback functions to a2dp sink service
 *
 * @param ins - bluetooth client instance.
 * @param id - callbacks cookie.
 * @return true - on callback unregister success
 * @return false - on callback cookie not found
 */
bool BTSYMBOLS(bt_a2dp_sink_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check A2DP sink is connected
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer A2DP source device.
 * @return true - connected.
 * @return false - not connected.
 */
bool BTSYMBOLS(bt_a2dp_sink_is_connected)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Check A2DP sink is playing
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer A2DP source device.
 * @return true - playing.
 * @return false - not playing.
 */
bool BTSYMBOLS(bt_a2dp_sink_is_playing)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief get A2DP sink connection state
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer A2DP source device.
 * @return profile_connection_state_t - connection state.
 */
profile_connection_state_t BTSYMBOLS(bt_a2dp_sink_get_connection_state)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Establish connection with peer A2DP device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer A2DP source device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_a2dp_sink_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from peer A2DP device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer A2DP source device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_a2dp_sink_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief set a peer A2DP source device as active device
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer A2DP source device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_a2dp_sink_set_active_device)(bt_instance_t* ins, bt_address_t* addr);

#ifdef __cplusplus
}
#endif
#endif /* __BT_A2DP_SINK_H__ */
