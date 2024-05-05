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

#ifdef __BT_MESSAGE_CODE__
BT_A2DP_SINK_MESSAGE_START,
    BT_A2DP_SINK_REGISTER_CALLBACKS,
    BT_A2DP_SINK_UNREGISTER_CALLBACKS,
    BT_A2DP_SINK_IS_CONNECTED,
    BT_A2DP_SINK_IS_PLAYING,
    BT_A2DP_SINK_GET_CONNECTION_STATE,
    BT_A2DP_SINK_CONNECT,
    BT_A2DP_SINK_DISCONNECT,
    BT_A2DP_SINK_SET_ACTIVE_DEVICE,
    BT_A2DP_SINK_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_A2DP_SINK_CALLBACK_START,
    BT_A2DP_SINK_CONNECTION_STATE_CHANGE,
    BT_A2DP_SINK_AUDIO_STATE_CHANGE,
    BT_A2DP_SINK_CONFIG_CHANGE,
    BT_A2DP_SINK_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_A2DP_SINK_H__
#define _BT_MESSAGE_A2DP_SINK_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_a2dp_sink.h"

    typedef union {
        uint8_t bbool; /* boolean */
        uint8_t state; /* profile_connection_state_t */
        uint8_t status; /* bt_status_t */
    } bt_a2dp_sink_result_t;

    typedef union {
        union {
            bt_address_t addr;
        } _bt_a2dp_sink_is_connected,
            _bt_a2dp_sink_is_playing,
            _bt_a2dp_sink_get_connection_state,
            _bt_a2dp_sink_connect,
            _bt_a2dp_sink_disconnect,
            _bt_a2dp_sink_set_active_device;
    } bt_message_a2dp_sink_t;

    typedef union {
        struct {
            bt_address_t addr;
            uint8_t state; /* profile_connection_state_t */
        } _connection_state_changed;
        struct {
            bt_address_t addr;
            uint8_t state; /* a2dp_audio_state_t */
        } _audio_state_changed;
        struct {
            bt_address_t addr;
        } _config_state_changed;
    } bt_message_a2dp_sink_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_A2DP_SINK_H__ */
