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
#ifndef __LEA_MCS_SERVICE_H__
#define __LEA_MCS_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_lea_mcs.h"
#include "sal_lea_mcs_interface.h"
#include "stddef.h"

/*
 * sal callback
 */
void lea_on_mcs_state(uint32_t mcs_id, uint8_t ccid, bool added);
void lea_on_mcs_player_info_set_result(uint32_t mcs_id, void* player_ref, bool result);
void lea_on_mcs_object_added_result(uint32_t mcs_id, void* obj_ref, lea_object_id obj_id);
void lea_on_mcs_set_position_result(uint32_t mcs_id, int32_t position);
void lea_on_mcs_set_playback_speed_result(uint32_t mcs_id, int8_t speed);
void lea_on_mcs_set_current_track_result(uint32_t mcs_id, lea_object_id track_id);
void lea_on_mcs_set_next_track_result(uint32_t mcs_id, lea_object_id track_id);
void lea_on_mcs_set_current_group_result(uint32_t mcs_id, lea_object_id group_id);
void lea_on_mcs_set_playing_order_result(uint32_t mcs_id, uint8_t order);
void lea_on_mcs_play_result(uint32_t mcs_id);
void lea_on_mcs_pause_result(uint32_t mcs_id);
void lea_on_mcs_fast_rewind_result(uint32_t mcs_id);
void lea_on_mcs_fast_forward_result(uint32_t mcs_id);
void lea_on_mcs_stop_result(uint32_t mcs_id);
void lea_on_mcs_move_result(uint32_t mcs_id, int32_t offset);
void lea_on_mcs_previous_segment_result(uint32_t mcs_id);
void lea_on_mcs_next_segment_result(uint32_t mcs_id);
void lea_on_mcs_first_segment_result(uint32_t mcs_id);
void lea_on_mcs_last_segment_result(uint32_t mcs_id);
void lea_on_mcs_goto_segment_result(uint32_t mcs_id, int32_t n_segment);
void lea_on_mcs_previous_track_result(uint32_t mcs_id);
void lea_on_mcs_next_track_result(uint32_t mcs_id);
void lea_on_mcs_first_track_result(uint32_t mcs_id);
void lea_on_mcs_last_track_result(uint32_t mcs_id);
void lea_on_mcs_goto_track_result(uint32_t mcs_id, int32_t n_track);
void lea_on_mcs_previous_group_result(uint32_t mcs_id);
void lea_on_mcs_next_group_result(uint32_t mcs_id);
void lea_on_mcs_first_group_result(uint32_t mcs_id);
void lea_on_mcs_last_group_result(uint32_t mcs_id);
void lea_on_mcs_goto_group_result(uint32_t mcs_id, int32_t n_group);
void lea_on_mcs_search_track_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition);
void lea_on_mcs_search_artist_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition);
void lea_on_mcs_search_album_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition);
void lea_on_mcs_search_group_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition);
void lea_on_mcs_search_earliest_year_result(uint32_t mcs_id, size_t size, char* year, bool last_condition);
void lea_on_mcs_search_latest_year_result(uint32_t mcs_id, size_t size, char* year, bool last_condition);
void lea_on_mcs_search_genre_result(uint32_t mcs_id, size_t size, char* name, bool last_condition);
void lea_on_mcs_search_tracks_result(uint32_t mcs_id, bool last_condition);
void lea_on_mcs_search_groups_result(uint32_t mcs_id, bool last_condition);

typedef struct {
    size_t size;
    bt_status_t (*mcs_add)();
    bt_status_t (*mcs_remove)();
    bt_status_t (*add_object)(uint32_t mcs_id, uint8_t type, uint8_t* name, void* obj_ref);
    bt_status_t (*playing_order_changed)(uint8_t order);
    bt_status_t (*media_state_changed)(lea_adpt_mcs_media_state_t state);
    bt_status_t (*playback_speed_changed)(int8_t speed);
    bt_status_t (*seeking_speed_changed)(int8_t speed);
    bt_status_t (*track_title_changed)(uint8_t* title);
    bt_status_t (*track_duration_changed)(int32_t duration);
    bt_status_t (*track_position_changed)(int32_t position);
    bt_status_t (*current_track_changed)(lea_object_id track_id);
    bt_status_t (*next_track_changed)(lea_object_id track_id);
    bt_status_t (*current_group_changed)(lea_object_id group_id);
    bt_status_t (*parent_group_changed)(lea_object_id group_id);
    bt_status_t (*set_media_player_info)();
    bt_status_t (*media_control_response)(lea_adpt_mcs_media_control_result_t result);
    void* (*set_callbacks)(void* handle, lea_mcs_callbacks_t* callbacks);
    bool (*reset_callbacks)(void** handle, void* cookie);
} lea_mcs_interface_t;

/*
 * register profile to service manager
 */
void register_lea_mcs_service(void);

#endif /* __LEA_MCS_SERVICE_H__ */