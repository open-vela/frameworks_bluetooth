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

#include "bt_a2dp.h"

typedef void (*a2dp_audio_sink_config_callback)(void *cookie, bt_address_t *addr);

typedef struct {
    /** set to sizeof(a2dp_sink_callbacks_t) */
    size_t size;
    a2dp_connection_state_callback connection_state_cb;
    a2dp_audio_state_callback audio_state_cb;
    a2dp_audio_sink_config_callback audio_sink_config_cb;
} a2dp_sink_callbacks_t;

void *bt_a2dp_sink_register_callbacks(bt_instance_t *ins, const a2dp_sink_callbacks_t *callbacks);
bool bt_a2dp_sink_unregister_callbacks(bt_instance_t *ins, void *cookie);
bt_status_t bt_a2dp_sink_connect(bt_instance_t *ins, bt_address_t *addr);
bt_status_t bt_a2dp_sink_disconnect(bt_instance_t *ins, bt_address_t *addr);
bt_status_t bt_a2dp_sink_set_active_device(bt_instance_t *ins, bt_address_t *addr);

#endif /* __BT_A2DP_SINK_H__ */
