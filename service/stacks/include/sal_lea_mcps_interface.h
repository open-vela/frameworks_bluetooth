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
#ifndef __SAL_LEA_MCPS_INTERFACE_H__
#define __SAL_LEA_MCPS_INTERFACE_H__

#include <stdint.h>
#include "bt_status.h"
#include "stack_adapter_lea_common.h"

bt_status_t bt_sal_lea_mcs_add(uint32_t mcs_id);
bt_status_t bt_sal_lea_mcs_remove(uint32_t mcs_id);
bt_status_t bt_sal_lea_mcs_set_media_player_info(uint32_t mcs_id);
bt_status_t bt_sal_lea_mcs_add_object(uint32_t mcs_id, uint8_t type, uint8_t *name, void *obj_ref);
bt_status_t bt_sal_lea_mcs_playing_order_changed(uint32_t mcs_id, uint8_t order);
bt_status_t bt_sal_lea_mcs_media_state_changed(uint32_t mcs_id, SERVICE_LEA_MCS_MEDIA_STATE state);
bt_status_t bt_sal_lea_mcs_playback_speed_changed(uint32_t mcs_id, int8_t speed);
bt_status_t bt_sal_lea_mcs_seeking_speed_changed(uint32_t mcs_id, int8_t speed);
bt_status_t bt_sal_lea_mcs_track_title_changed(uint32_t mcs_id, uint8_t *title);
bt_status_t bt_sal_lea_mcs_track_duration_changed(uint32_t mcs_id, int32_t duration);
bt_status_t bt_sal_lea_mcs_track_position_changed(uint32_t mcs_id, int32_t position);
bt_status_t bt_sal_lea_mcs_current_track_changed(uint32_t mcs_id, LEA_OBJ_ID track_id);
bt_status_t bt_sal_lea_mcs_next_track_changed(uint32_t mcs_id, LEA_OBJ_ID track_id);
bt_status_t bt_sal_lea_mcs_current_group_changed(uint32_t mcs_id, LEA_OBJ_ID group_id);
bt_status_t bt_sal_lea_mcs_parent_group_changed(uint32_t mcs_id, LEA_OBJ_ID group_id);
bt_status_t bt_sal_lea_mcs_media_control_response(uint32_t mcs_id, SERVICE_LEA_MEDIA_CONTROL_RESULT result);

void adpt_lea_mcs_state_callback(uint32_t mcs_id, uint8_t ccid, bool added);
void adpt_lea_mcs_player_set_callback(uint32_t mcs_id, void* player_ref, bool result);
void adpt_lea_mcs_object_added_callback(uint32_t mcs_id, void* obj_ref, LEA_OBJ_ID obj_id);
void adpt_lea_mcs_set_position_callback(uint32_t mcs_id, int32_t position);
void adpt_lea_mcs_set_playback_speed_callback(uint32_t mcs_id, int8_t speed);
void adpt_lea_mcs_set_current_track_callback(uint32_t mcs_id, LEA_OBJ_ID track_id);
void adpt_lea_mcs_set_next_track_callback(uint32_t mcs_id, LEA_OBJ_ID track_id);
void adpt_lea_mcs_set_current_group_callback(uint32_t mcs_id, LEA_OBJ_ID group_id);
void adpt_lea_mcs_set_playing_order_callback(uint32_t mcs_id, uint8_t order);
void adpt_lea_mcs_play_callback(uint32_t mcs_id);
void adpt_lea_mcs_pause_callback(uint32_t mcs_id);
void adpt_lea_mcs_fast_rewind_callback(uint32_t mcs_id);
void adpt_lea_mcs_fast_forward_callback(uint32_t mcs_id);
void adpt_lea_mcs_stop_callback(uint32_t mcs_id);
void adpt_lea_mcs_move_callback(uint32_t mcs_id, int32_t offset);
void adpt_lea_mcs_previous_segment_callback(uint32_t mcs_id);
void adpt_lea_mcs_next_segment_callback(uint32_t mcs_id);
void adpt_lea_mcs_first_segment_callback(uint32_t mcs_id);
void adpt_lea_mcs_last_segment_callback(uint32_t mcs_id);
void adpt_lea_mcs_goto_segment_callback(uint32_t mcs_id, int32_t n_segment);
void adpt_lea_mcs_previous_track_callback(uint32_t mcs_id);
void adpt_lea_mcs_next_track_callback(uint32_t mcs_id);
void adpt_lea_mcs_first_track_callback(uint32_t mcs_id);
void adpt_lea_mcs_last_track_callback(uint32_t mcs_id);
void adpt_lea_mcs_goto_track_callback(uint32_t mcs_id, int32_t n_track);
void adpt_lea_mcs_previous_group_callback(uint32_t mcs_id);
void adpt_lea_mcs_next_group_callback(uint32_t mcs_id);
void adpt_lea_mcs_first_group_callback(uint32_t mcs_id);
void adpt_lea_mcs_last_group_callback(uint32_t mcs_id);
void adpt_lea_mcs_goto_group_callback(uint32_t mcs_id, int32_t n_group);
void adpt_lea_mcs_search_track_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR* name, bool last_condition);
void adpt_lea_mcs_search_artist_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR* name, bool last_condition);
void adpt_lea_mcs_search_album_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR* name, bool last_condition);
void adpt_lea_mcs_search_group_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR* name, bool last_condition);
void adpt_lea_mcs_search_earliest_year_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR* year, bool last_condition);
void adpt_lea_mcs_search_latest_year_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR* year, bool last_condition);
void adpt_lea_mcs_search_genre_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR* name, bool last_condition);
void adpt_lea_mcs_search_tracks_callback(uint32_t mcs_id, bool last_condition);
void adpt_lea_mcs_search_groups_callback(uint32_t mcs_id, bool last_condition);

#endif /* __SAL_LEA_MCPS_INTERFACE_H__ */