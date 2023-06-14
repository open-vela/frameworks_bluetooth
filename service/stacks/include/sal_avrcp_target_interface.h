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
#ifndef __SAL_AVRCP_TARGET_INTERFACE_H__
#define __SAL_AVRCP_TARGET_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "avrcp_target_service.h"

#include "avrcp_msg.h"

bt_status_t bt_sal_avrcp_target_init(void);
void bt_sal_avrcp_target_cleanup(void);

bt_status_t bt_sal_avrcp_target_get_play_status_rsp(bt_address_t *addr,
    avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos);
bt_status_t bt_sal_avrcp_target_play_status_notify(bt_address_t *addr, avrcp_play_status_t status);
bt_status_t bt_sal_avrcp_target_set_absolute_volume(bt_address_t *addr, uint8_t volume);

bt_status_t bt_sal_avrcp_target_notify_track_changed(bt_address_t *addr, bool selected);
bt_status_t bt_sal_avrcp_target_notify_play_position_changed(bt_address_t *addr, uint32_t position);
bt_status_t bt_sal_avrcp_target_register_volume_changed(bt_address_t *addr);

void bt_sal_avrcp_target_event_callback(avrcp_msg_t *msg);

#endif /* __SAL_AVRCP_TARGET_INTERFACE_H__ */
