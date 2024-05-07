/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef __SAL_LEA_MCP_INTERFACE_H__
#define __SAL_LEA_MCP_INTERFACE_H__

#include "bt_status.h"
#include "stack_adapter_lea_common.h"
#include <stdint.h>

bt_status_t bt_sal_lea_mcs_add(uint32_t mcs_id);
bt_status_t bt_sal_lea_mcs_remove(uint32_t mcs_id);
bt_status_t bt_sal_lea_mcs_set_media_player_info(uint32_t mcs_id);
bt_status_t bt_sal_lea_mcs_add_object(uint32_t mcs_id, uint8_t type, uint8_t* name, void* obj_ref);
bt_status_t bt_sal_lea_mcs_playing_order_changed(uint32_t mcs_id, uint8_t order);
bt_status_t bt_sal_lea_mcs_media_state_changed(uint32_t mcs_id, SERVICE_LEA_MCS_MEDIA_STATE state);
bt_status_t bt_sal_lea_mcs_playback_speed_changed(uint32_t mcs_id, int8_t speed);
bt_status_t bt_sal_lea_mcs_seeking_speed_changed(uint32_t mcs_id, int8_t speed);
bt_status_t bt_sal_lea_mcs_track_title_changed(uint32_t mcs_id, uint8_t* title);
bt_status_t bt_sal_lea_mcs_track_duration_changed(uint32_t mcs_id, int32_t duration);
bt_status_t bt_sal_lea_mcs_track_position_changed(uint32_t mcs_id, int32_t position);
bt_status_t bt_sal_lea_mcs_current_track_changed(uint32_t mcs_id, LEA_OBJ_ID track_id);
bt_status_t bt_sal_lea_mcs_next_track_changed(uint32_t mcs_id, LEA_OBJ_ID track_id);
bt_status_t bt_sal_lea_mcs_current_group_changed(uint32_t mcs_id, LEA_OBJ_ID group_id);
bt_status_t bt_sal_lea_mcs_parent_group_changed(uint32_t mcs_id, LEA_OBJ_ID group_id);
bt_status_t bt_sal_lea_mcs_media_control_response(uint32_t mcs_id, SERVICE_LEA_MEDIA_CONTROL_RESULT result);

#endif /* __SAL_LEA_MCP_INTERFACE_H__ */