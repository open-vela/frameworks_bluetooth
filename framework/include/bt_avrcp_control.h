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
#ifndef __BT_AVRCP_CONTROL_H__
#define __BT_AVRCP_CONTROL_H__

#include "bt_avrcp.h"

typedef void (*avrcp_passthrough_rsp_callback)(void *cookie, bt_address_t *addr, avrcp_passthr_cmd_t key_code,
                                               avrcp_key_state_t key_state, uint8_t response);
typedef void (*avrcp_play_position_changed_callback)(void *cookie, bt_address_t *addr, uint32_t song_len, uint32_t song_pos);
typedef void (*avrcp_play_status_changed_callback)(void *cookie, bt_address_t *addr, avrcp_play_status_t play_status);
typedef void (*avrcp_register_notification_absvol_callback)(void *cookie, bt_address_t *addr);
typedef void (*avrcp_set_volume_callback)(void *cookie, bt_address_t *addr, uint8_t volume);

typedef struct {
    size_t size;
    avrcp_connection_state_callback connection_state_cb;
    avrcp_passthrough_rsp_callback passthrough_rsp_cb;
    avrcp_play_position_changed_callback play_position_changed_cb;
    avrcp_play_status_changed_callback play_status_changed_cb;
    avrcp_register_notification_absvol_callback register_notification_absvol_cb;
    avrcp_set_volume_callback set_volume_cb;
} avrcp_control_callbacks_t;

void *bt_avrcp_control_register_callbacks(bt_instance_t *ins, const avrcp_control_callbacks_t *callbacks);
bool bt_avrcp_control_unregister_callbacks(bt_instance_t *ins, void *cookie);
bt_status_t bt_avrcp_control_send_pass_through_cmd(bt_address_t *bd_addr,
                                                   avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state);
bt_status_t bt_avrcp_control_get_playback_state(bt_address_t *bd_addr);
bt_status_t bt_avrcp_control_volume_changed_notify(bt_address_t *bd_addr, uint8_t volume);

#endif /* __BT_AVRCP_CONTROL_H__ */
