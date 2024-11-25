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
 * @brief A2DP audio state
 */
typedef enum {
    A2DP_AUDIO_STATE_REMOTE_SUSPEND = 0,
    A2DP_AUDIO_STATE_STOPPED,
    A2DP_AUDIO_STATE_STARTED,
} a2dp_audio_state_t;

/**
 * @brief A2DP connection state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer A2DP device.
 * @param state - connection state.
 */
typedef void (*a2dp_connection_state_callback)(void* cookie, bt_address_t* addr,
    profile_connection_state_t state);

/**
 * @brief A2DP audio connection state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer A2DP device.
 * @param state - audio connection state.
 */
typedef void (*a2dp_audio_state_callback)(void* cookie, bt_address_t* addr,
    a2dp_audio_state_t state);

#endif /* __BT_A2DP_H__ */
