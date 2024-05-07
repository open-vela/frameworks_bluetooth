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
#include "bt_lea_mcs.h"
#include "bt_profile.h"
#include "lea_mcs_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static lea_mcs_interface_t* get_profile_service(void)
{
    return (lea_mcs_interface_t*)service_manager_get_profile(PROFILE_LEAUDIO_MCS);
}

void* bt_lea_mcs_register_callbacks(bt_instance_t* ins, const lea_mcs_callbacks_t* callbacks)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->set_callbacks(NULL, (lea_mcs_callbacks_t*)callbacks);
}

bool bt_lea_mcs_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->reset_callbacks(NULL, cookie);
}

bt_status_t bt_lea_mcs_service_add(bt_instance_t* ins)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->mcs_add();
}

bt_status_t bt_lea_mcs_service_remove(bt_instance_t* ins)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->mcs_remove();
}

bt_status_t bt_lea_mcs_playing_order_changed(bt_instance_t* ins, uint8_t order)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->playing_order_changed(order);
}

bt_status_t bt_lea_mcs_media_state_changed(bt_instance_t* ins, uint8_t state)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->media_state_changed(state);
}

bt_status_t bt_lea_mcs_playback_speed_changed(bt_instance_t* ins, int8_t speed)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->playback_speed_changed(speed);
}

bt_status_t bt_lea_mcs_seeking_speed_changed(bt_instance_t* ins, int8_t speed)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->seeking_speed_changed(speed);
}

bt_status_t bt_lea_mcs_track_title_changed(void* handl, uint8_t* title)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->track_title_changed(title);
}

bt_status_t bt_lea_mcs_track_duration_changed(bt_instance_t* ins, int32_t duration)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->track_duration_changed(duration);
}

bt_status_t bt_lea_mcs_track_position_changed(bt_instance_t* ins, int32_t position)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->track_position_changed(position);
}

bt_status_t bt_lea_mcs_current_track_change(bt_instance_t* ins, lea_object_id track_id)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->current_track_changed(track_id);
}

bt_status_t bt_lea_mcs_next_track_changed(bt_instance_t* ins, lea_object_id track_id)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->next_track_changed(track_id);
}

bt_status_t bt_lea_mcs_current_group_changed(bt_instance_t* ins, lea_object_id group_id)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->current_group_changed(group_id);
}

bt_status_t bt_lea_mcs_parent_group_changed(bt_instance_t* ins, lea_object_id group_id)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->parent_group_changed(group_id);
}

bt_status_t bt_lea_mcs_set_media_player_info(bt_instance_t* ins)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->set_media_player_info();
}

bt_status_t bt_lea_mcs_media_control_point_response(bt_instance_t* ins, lea_adpt_mcs_media_control_result_t result)
{
    lea_mcs_interface_t* profile = get_profile_service();

    return profile->media_control_response(result);
}
