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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_lea_mcp.h"

#include "bluetooth.h"
#include "lea_mcs_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_mcs_interface.h"

#define UNKNOWN_INFO "unknown"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCS

static void adpt_lea_mcs_state_callback(uint32_t mcs_id, uint8_t ccid, bool added);
static void adpt_lea_mcs_player_set_callback(uint32_t mcs_id, void *player_ref, bool result);
static void adpt_lea_mcs_object_added_callback(uint32_t mcs_id, void *obj_ref, LEA_OBJ_ID obj_id);
static void adpt_lea_mcs_set_position_callback(uint32_t mcs_id, int32_t position);
static void adpt_lea_mcs_set_playback_speed_callback(uint32_t mcs_id, int8_t speed);
static void adpt_lea_mcs_set_current_track_callback(uint32_t mcs_id, LEA_OBJ_ID track_id);
static void adpt_lea_mcs_set_next_track_callback(uint32_t mcs_id, LEA_OBJ_ID track_id);
static void adpt_lea_mcs_set_current_group_callback(uint32_t mcs_id, LEA_OBJ_ID group_id);
static void adpt_lea_mcs_set_playing_order_callback(uint32_t mcs_id, uint8_t order);
static void adpt_lea_mcs_play_callback(uint32_t mcs_id);
static void adpt_lea_mcs_pause_callback(uint32_t mcs_id);
static void adpt_lea_mcs_fast_rewind_callback(uint32_t mcs_id);
static void adpt_lea_mcs_fast_forward_callback(uint32_t mcs_id);
static void adpt_lea_mcs_stop_callback(uint32_t mcs_id);
static void adpt_lea_mcs_move_callback(uint32_t mcs_id, int32_t offset);
static void adpt_lea_mcs_previous_segment_callback(uint32_t mcs_id);
static void adpt_lea_mcs_next_segment_callback(uint32_t mcs_id);
static void adpt_lea_mcs_first_segment_callback(uint32_t mcs_id);
static void adpt_lea_mcs_last_segment_callback(uint32_t mcs_id);
static void adpt_lea_mcs_goto_segment_callback(uint32_t mcs_id, int32_t n_segment);
static void adpt_lea_mcs_previous_track_callback(uint32_t mcs_id);
static void adpt_lea_mcs_next_track_callback(uint32_t mcs_id);
static void adpt_lea_mcs_first_track_callback(uint32_t mcs_id);
static void adpt_lea_mcs_last_track_callback(uint32_t mcs_id);
static void adpt_lea_mcs_goto_track_callback(uint32_t mcs_id, int32_t n_track);
static void adpt_lea_mcs_previous_group_callback(uint32_t mcs_id);
static void adpt_lea_mcs_next_group_callback(uint32_t mcs_id);
static void adpt_lea_mcs_first_group_callback(uint32_t mcs_id);
static void adpt_lea_mcs_last_group_callback(uint32_t mcs_id);
static void adpt_lea_mcs_goto_group_callback(uint32_t mcs_id, int32_t n_group);
static void adpt_lea_mcs_search_track_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition);
static void adpt_lea_mcs_search_artist_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition);
static void adpt_lea_mcs_search_album_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition);
static void adpt_lea_mcs_search_group_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition);
static void adpt_lea_mcs_search_earliest_year_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *year, bool last_condition);
static void adpt_lea_mcs_search_latest_year_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *year, bool last_condition);
static void adpt_lea_mcs_search_genre_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition);
static void adpt_lea_mcs_search_tracks_callback(uint32_t mcs_id, bool last_condition);
static void adpt_lea_mcs_search_groups_callback(uint32_t mcs_id, bool last_condition);

static uint32_t tmp_player_ref = 1;
static char *MEDIA_PLAYER_NAME = "MiPlayer";
static char *MEDIA_PLAYER_ICON_URL = "www.xiaomi.com";

const LEA_MCS_CALLBACK_S adpt_lea_mcp_server_callbacks = {
    .lea_mcs_state_cb = adpt_lea_mcs_state_callback,
    .lea_mcs_player_set_cb = adpt_lea_mcs_player_set_callback,
    .lea_mcs_object_added_cb = adpt_lea_mcs_object_added_callback,

    .lea_mcs_set_position_cb = adpt_lea_mcs_set_position_callback,
    .lea_mcs_set_playback_speed_cb = adpt_lea_mcs_set_playback_speed_callback,
    .lea_mcs_set_current_track_cb = adpt_lea_mcs_set_current_track_callback,
    .lea_mcs_set_next_track_cb = adpt_lea_mcs_set_next_track_callback,
    .lea_mcs_set_current_group_cb = adpt_lea_mcs_set_current_group_callback,
    .lea_mcs_set_playing_order_cb = adpt_lea_mcs_set_playing_order_callback,

    .lea_mcs_play_cb = adpt_lea_mcs_play_callback,
    .lea_mcs_pause_cb = adpt_lea_mcs_pause_callback,
    .lea_mcs_fast_rewind_cb = adpt_lea_mcs_fast_rewind_callback,
    .lea_mcs_fast_forward_cb = adpt_lea_mcs_fast_forward_callback,
    .lea_mcs_stop_cb = adpt_lea_mcs_stop_callback,
    .lea_mcs_move_cb = adpt_lea_mcs_move_callback,
    .lea_mcs_previous_segment_cb = adpt_lea_mcs_previous_segment_callback,
    .lea_mcs_next_segment_cb = adpt_lea_mcs_next_segment_callback,
    .lea_mcs_first_segment_cb = adpt_lea_mcs_first_segment_callback,
    .lea_mcs_last_segment_cb = adpt_lea_mcs_last_segment_callback,
    .lea_mcs_goto_segment_cb = adpt_lea_mcs_goto_segment_callback,
    .lea_mcs_previous_track_cb = adpt_lea_mcs_previous_track_callback,
    .lea_mcs_next_track_cb = adpt_lea_mcs_next_track_callback,
    .lea_mcs_first_track_cb = adpt_lea_mcs_first_track_callback,
    .lea_mcs_last_track_cb = adpt_lea_mcs_last_track_callback,
    .lea_mcs_goto_track_cb = adpt_lea_mcs_goto_track_callback,
    .lea_mcs_previous_group_cb = adpt_lea_mcs_previous_group_callback,
    .lea_mcs_next_group_cb = adpt_lea_mcs_next_group_callback,
    .lea_mcs_first_group_cb = adpt_lea_mcs_first_group_callback,
    .lea_mcs_last_group_cb = adpt_lea_mcs_last_group_callback,
    .lea_mcs_goto_group_cb = adpt_lea_mcs_goto_group_callback,

    .lea_mcs_search_track_name_cb = adpt_lea_mcs_search_track_name_callback,
    .lea_mcs_search_artist_name_cb = adpt_lea_mcs_search_artist_name_callback,
    .lea_mcs_search_album_name_cb = adpt_lea_mcs_search_album_name_callback,
    .lea_mcs_search_group_name_cb = adpt_lea_mcs_search_group_name_callback,
    .lea_mcs_search_earliest_year_cb = adpt_lea_mcs_search_earliest_year_callback,
    .lea_mcs_search_latest_year_cb = adpt_lea_mcs_search_latest_year_callback,
    .lea_mcs_search_genre_cb = adpt_lea_mcs_search_genre_callback,
    .lea_mcs_search_tracks_cb = adpt_lea_mcs_search_tracks_callback,
    .lea_mcs_search_groups_cb = adpt_lea_mcs_search_groups_callback,
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_mcs_state_callback(uint32_t mcs_id, uint8_t ccid, bool added)
{
    lea_on_mcs_state(mcs_id, ccid, added);
}

static void adpt_lea_mcs_player_set_callback(uint32_t mcs_id, void *player_ref, bool result)
{
    lea_on_mcs_player_info_set_result(mcs_id, player_ref, result);
}

static void adpt_lea_mcs_object_added_callback(uint32_t mcs_id, void *obj_ref, LEA_OBJ_ID obj_id)
{
    lea_on_mcs_object_added_result(mcs_id, obj_ref, obj_id); // todo
}

static void adpt_lea_mcs_set_position_callback(uint32_t mcs_id, int32_t position)
{
    lea_on_mcs_set_position_result(mcs_id, position);
}

static void adpt_lea_mcs_set_playback_speed_callback(uint32_t mcs_id, int8_t speed)
{
    lea_on_mcs_set_playback_speed_result(mcs_id, speed);
}

static void adpt_lea_mcs_set_current_track_callback(uint32_t mcs_id, LEA_OBJ_ID track_id)
{
    lea_on_mcs_set_current_track_result(mcs_id, track_id);
}

static void adpt_lea_mcs_set_next_track_callback(uint32_t mcs_id, LEA_OBJ_ID track_id)
{
    lea_on_mcs_set_next_track_result(mcs_id, track_id);
}

static void adpt_lea_mcs_set_current_group_callback(uint32_t mcs_id, LEA_OBJ_ID group_id)
{
    lea_on_mcs_set_current_group_result(mcs_id, group_id);
}

static void adpt_lea_mcs_set_playing_order_callback(uint32_t mcs_id, uint8_t order)
{
    lea_on_mcs_set_playing_order_result(mcs_id, order);
}

static void adpt_lea_mcs_play_callback(uint32_t mcs_id)
{
    lea_on_mcs_play_result(mcs_id);
}

static void adpt_lea_mcs_pause_callback(uint32_t mcs_id)
{
    lea_on_mcs_pause_result(mcs_id);
}

static void adpt_lea_mcs_fast_rewind_callback(uint32_t mcs_id)
{
    lea_on_mcs_fast_rewind_result(mcs_id);
}

static void adpt_lea_mcs_fast_forward_callback(uint32_t mcs_id)
{
    lea_on_mcs_fast_forward_result(mcs_id);
}

static void adpt_lea_mcs_stop_callback(uint32_t mcs_id)
{
    lea_on_mcs_stop_result(mcs_id);
}

static void adpt_lea_mcs_move_callback(uint32_t mcs_id, int32_t offset)
{
    lea_on_mcs_move_result(mcs_id, offset);
}

static void adpt_lea_mcs_previous_segment_callback(uint32_t mcs_id)
{
    lea_on_mcs_previous_segment_result(mcs_id);
}

static void adpt_lea_mcs_next_segment_callback(uint32_t mcs_id)
{
    lea_on_mcs_next_segment_result(mcs_id);
}

static void adpt_lea_mcs_first_segment_callback(uint32_t mcs_id)
{
    lea_on_mcs_first_segment_result(mcs_id);
}

static void adpt_lea_mcs_last_segment_callback(uint32_t mcs_id)
{
    lea_on_mcs_last_segment_result(mcs_id);
}

static void adpt_lea_mcs_goto_segment_callback(uint32_t mcs_id, int32_t n_segment)
{
    lea_on_mcs_goto_segment_result(mcs_id, n_segment);
}

static void adpt_lea_mcs_previous_track_callback(uint32_t mcs_id)
{
    lea_on_mcs_previous_track_result(mcs_id);
}

static void adpt_lea_mcs_next_track_callback(uint32_t mcs_id)
{
    lea_on_mcs_next_track_result(mcs_id);
}

static void adpt_lea_mcs_first_track_callback(uint32_t mcs_id)
{
    lea_on_mcs_first_track_result(mcs_id);
}

static void adpt_lea_mcs_last_track_callback(uint32_t mcs_id)
{
    lea_on_mcs_last_track_result(mcs_id);
}

static void adpt_lea_mcs_goto_track_callback(uint32_t mcs_id, int32_t n_track)
{
    lea_on_mcs_goto_track_result(mcs_id, n_track);
}

static void adpt_lea_mcs_previous_group_callback(uint32_t mcs_id)
{
    lea_on_mcs_previous_group_result(mcs_id);
}

static void adpt_lea_mcs_next_group_callback(uint32_t mcs_id)
{
    lea_on_mcs_next_group_result(mcs_id);
}

static void adpt_lea_mcs_first_group_callback(uint32_t mcs_id)
{
    lea_on_mcs_first_group_result(mcs_id);
}

static void adpt_lea_mcs_last_group_callback(uint32_t mcs_id)
{
    lea_on_mcs_last_group_result(mcs_id);
}

static void adpt_lea_mcs_goto_group_callback(uint32_t mcs_id, int32_t n_group)
{
    lea_on_mcs_goto_group_result(mcs_id, n_group);
}

static void adpt_lea_mcs_search_track_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition)
{
    char *nullname = UNKNOWN_INFO;
    if (name == NULL) {
        lea_on_mcs_search_track_name_result(mcs_id, strlen(nullname) + 1, nullname, last_condition);
    } else {
        lea_on_mcs_search_track_name_result(mcs_id, name->length + 1, (char *)name->string, last_condition);
    }
}

static void adpt_lea_mcs_search_artist_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition)
{
    char *nullname = UNKNOWN_INFO;
    if (name == NULL) {
        lea_on_mcs_search_artist_name_result(mcs_id, strlen(nullname) + 1, nullname, last_condition);
    } else {
        lea_on_mcs_search_artist_name_result(mcs_id, name->length + 1, (char *)name->string, last_condition);
    }
}

static void adpt_lea_mcs_search_album_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition)
{
    char *nullname = UNKNOWN_INFO;
    if (name == NULL) {
        lea_on_mcs_search_album_name_result(mcs_id, strlen(nullname) + 1, nullname, last_condition);
    } else {
        lea_on_mcs_search_album_name_result(mcs_id, name->length + 1, (char *)name->string, last_condition);
    }
}

static void adpt_lea_mcs_search_group_name_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition)
{
    char *nullname = UNKNOWN_INFO;
    if (name == NULL) {
        lea_on_mcs_search_group_name_result(mcs_id, strlen(nullname) + 1, nullname, last_condition);
    } else {
        lea_on_mcs_search_group_name_result(mcs_id, name->length + 1, (char *)name->string, last_condition);
    }
}

static void adpt_lea_mcs_search_earliest_year_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *year, bool last_condition)
{
    char *nullyear = UNKNOWN_INFO;
    if (year == NULL) {
        lea_on_mcs_search_earliest_year_result(mcs_id, strlen(nullyear) + 1, nullyear, last_condition);
    } else {
        lea_on_mcs_search_earliest_year_result(mcs_id, year->length + 1, (char *)year->string, last_condition);
    }
}

static void adpt_lea_mcs_search_latest_year_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *year, bool last_condition)
{
    char *nullyear = UNKNOWN_INFO;
    if (year == NULL) {
        lea_on_mcs_search_latest_year_result(mcs_id, strlen(nullyear) + 1, nullyear, last_condition);
    } else {
        lea_on_mcs_search_latest_year_result(mcs_id, year->length + 1, (char *)year->string, last_condition);
    }
}

static void adpt_lea_mcs_search_genre_callback(uint32_t mcs_id, SERVICE_LEA_UTF8_STR *name, bool last_condition)
{
    char *nullname = UNKNOWN_INFO;
    if (name == NULL) {
        lea_on_mcs_search_genre_result(mcs_id, strlen(nullname) + 1, nullname, last_condition);
    } else {
        lea_on_mcs_search_genre_result(mcs_id, name->length + 1, (char *)name->string, last_condition);
    }
}

static void adpt_lea_mcs_search_tracks_callback(uint32_t mcs_id, bool last_condition)
{
    lea_on_mcs_search_tracks_result(mcs_id, last_condition);
}

static void adpt_lea_mcs_search_groups_callback(uint32_t mcs_id, bool last_condition)
{
    lea_on_mcs_search_groups_result(mcs_id, last_condition);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bt_status_t bt_sal_lea_mcs_add(uint32_t mcs_id)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_add(mcs_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_remove(uint32_t mcs_id)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_remove(mcs_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_set_media_player_info(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);

    SERVICE_LEA_MEDIA_PLAYER_S *mediaplayerinfo;
    mediaplayerinfo = (SERVICE_LEA_MEDIA_PLAYER_S *)malloc(sizeof(SERVICE_LEA_MEDIA_PLAYER_S));

    mediaplayerinfo->mcs_id = mcs_id; // lea_mcs_id;
    mediaplayerinfo->player_ref = (void *)tmp_player_ref;
    mediaplayerinfo->player_name = (uint8_t *)MEDIA_PLAYER_NAME;
    mediaplayerinfo->player_icon_url = (uint8_t *)MEDIA_PLAYER_ICON_URL;
    mediaplayerinfo->icon_object_size = 0; // 0 if it has no icon
    mediaplayerinfo->playback_speed = 1; // playback speed(s) = 2^(p/64)
    mediaplayerinfo->seeking_speed = 0; // not seeking
    mediaplayerinfo->playing_order = 0x01; // Single once
    mediaplayerinfo->playing_orders_supported = 0x03FF; // supported ALL
    mediaplayerinfo->control_opcodes_supported = 0x00001833; // PLAY/PAUSE/STOP/MOVE/PREVTRACK/NEXTTRACK

    SAL_CHECK_RET(stack_adapter_lea_mcs_set_media_player(mediaplayerinfo), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_add_object(uint32_t mcs_id, uint8_t type, uint8_t *name, void *obj_ref)
{
    SERVICE_LEA_MEDIA_OBJECT_S obj;
    memset(&obj, 0, sizeof(SERVICE_LEA_MEDIA_OBJECT_S));
    obj.mcs_id = mcs_id;

    obj.type = type;
    obj.name = name;
    obj.obj_ref = obj_ref; /* Type & Index as context */
    SAL_CHECK_RET(stack_adapter_lea_mcs_add_object(&obj), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_playing_order_changed(uint32_t mcs_id, uint8_t order)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_playing_order_changed(mcs_id, order), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_media_state_changed(uint32_t mcs_id, SERVICE_LEA_MCS_MEDIA_STATE state)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_media_state_changed(mcs_id, state), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_playback_speed_changed(uint32_t mcs_id, int8_t speed)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_playback_speed_changed(mcs_id, speed), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_seeking_speed_changed(uint32_t mcs_id, int8_t speed)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_seeking_speed_changed(mcs_id, speed), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_track_title_changed(uint32_t mcs_id, uint8_t *title)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_track_title_changed(mcs_id, title), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_track_duration_changed(uint32_t mcs_id, int32_t duration)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_track_duration_changed(mcs_id, duration), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_track_position_changed(uint32_t mcs_id, int32_t position)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_track_position_changed(mcs_id, position), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_current_track_changed(uint32_t mcs_id, LEA_OBJ_ID track_id)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_current_track_changed(mcs_id, track_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_next_track_changed(uint32_t mcs_id, LEA_OBJ_ID track_id)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_next_track_changed(mcs_id, track_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_current_group_changed(uint32_t mcs_id, LEA_OBJ_ID group_id)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_current_group_changed(mcs_id, group_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_parent_group_changed(uint32_t mcs_id, LEA_OBJ_ID group_id)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_parent_group_changed(mcs_id, group_id), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_mcs_media_control_response(uint32_t mcs_id, SERVICE_LEA_MEDIA_CONTROL_RESULT result)
{
    SAL_CHECK_RET(stack_adapter_lea_mcs_media_control_response(mcs_id, result), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

#endif
