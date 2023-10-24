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

typedef void (*avrcp_get_play_status_callback)(void *cookie, bt_address_t *addr);
typedef void (*avrcp_playback_register_notification_callback)(void *cookie, bt_address_t *addr);
typedef void (*avrcp_volume_changed_callback)(void *cookie, bt_address_t *addr, uint8_t volume);

typedef struct {
    size_t size;
    avrcp_connection_state_callback connection_state_cb;
} avrcp_target_callbacks_t;

void *bt_avrcp_target_register_callbacks(bt_instance_t *ins, const avrcp_target_callbacks_t *callbacks);
bool bt_avrcp_target_unregister_callbacks(bt_instance_t *ins, void *cookie);
bt_status_t bt_avrcp_target_get_play_status_rsp(bt_address_t *addr, avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos);
bt_status_t bt_avrcp_target_play_status_notify(bt_address_t *addr, avrcp_play_status_t status);
bt_status_t bt_avrcp_target_set_absolute_volume(bt_address_t *addr, uint8_t volume);

#endif /* __BT_AVRCP_TARGET_H__ */
