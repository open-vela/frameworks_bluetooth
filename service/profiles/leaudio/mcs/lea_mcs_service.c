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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "lea_mcs_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_mcs.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_mcs_event.h"
#include "lea_mcs_service.h"
#include "media_session.h"
#include "sal_lea_mcs_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCS

#define MCS_GROUPS_MAX 2
#define MCS_TRACKS_MAX 2
#define MCS_PLAYER_INACTIVE -2

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_mcs_service.started)       \
            return BT_STATUS_NOT_ENABLED; \
    }

#define MCS_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_mcs_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct
{
    bool started;
    callbacks_list_t* callbacks;
    pthread_mutex_t device_lock;
} mcs_service_t;

static mcs_service_t g_mcs_service = {
    .started = false,
    .callbacks = NULL,
};

static uint32_t lea_mcs_id = ADPT_LEA_GMCS_ID;
static char* MCS_TRACK_TITLE = "MiFire";
static char* MCS_OBJECT_GROUP0 = "Group0";
static char* MCS_OBJECT_TRACK0 = "Track0";
static char* MCS_OBJECT_TRACK1 = "Track1";
static void* control_session = NULL;
static bool isRemoteControl = false;
static int mcs_cur_group_oid = MCS_GROUPS_MAX;
static int mcs_cur_track_oid = MCS_TRACKS_MAX;
static int mcs_next_track_oid = MCS_TRACKS_MAX;
static int mcs_player_inactive = MCS_PLAYER_INACTIVE;
static lea_adpt_mcs_media_state_t current_state;
static lea_object_id mcs_group_oids[MCS_GROUPS_MAX];
static lea_object_id mcs_track_oids[MCS_TRACKS_MAX];

static bt_status_t lea_mcs_add();
static bt_status_t lea_mcs_remove();
static bt_status_t lea_mcs_set_media_player_info();
static bt_status_t lea_mcs_add_object(uint32_t mcs_id, uint8_t type, uint8_t* name, void* obj_ref);
static bt_status_t lea_mcs_playing_order_changed(uint8_t order);
static bt_status_t lea_mcs_media_state_changed(lea_adpt_mcs_media_state_t state);
static bt_status_t lea_mcs_playback_speed_changed(int8_t speed);
static bt_status_t lea_mcs_seeking_speed_changed(int8_t speed);
static bt_status_t lea_mcs_track_title_changed(uint8_t* title);
static bt_status_t lea_mcs_track_duration_changed(int32_t duration);
static bt_status_t lea_mcs_track_position_changed(int32_t position);
static bt_status_t lea_mcs_current_track_changed(lea_object_id track_id);
static bt_status_t lea_mcs_next_track_changed(lea_object_id track_id);
static bt_status_t lea_mcs_current_group_changed(lea_object_id group_id);
static bt_status_t lea_mcs_parent_group_changed(lea_object_id group_id);
static bt_status_t lea_mcs_media_control_response(lea_adpt_mcs_media_control_result_t result);
static void* lea_mcs_set_callbacks(void* handle, lea_mcs_callbacks_t* callbacks);
static bool lea_mcs_reset_callbacks(void** handle, void* cookie);

static void lea_mcs_process_message(void* data)
{
    mcs_event_t* msg = (mcs_event_t*)data;
    BT_LOGD("%s, msg->event:%d ", __func__, msg->event);
    switch (msg->event) {
    case MCS_STATE: {
        MCS_CALLBACK_FOREACH(g_mcs_service.callbacks, mcs_state_cb, msg->event);
        break;
    }
    case MCS_PLAYER_SET: {
        lea_mcs_add_object(msg->event_data.mcs_id, ADPT_LEA_MCS_OBJECT_GROUP,
            (uint8_t*)MCS_OBJECT_GROUP0, (void*)(ADPT_LEA_MCS_OBJECT_GROUP << 16));
        lea_mcs_add_object(msg->event_data.mcs_id, ADPT_LEA_MCS_OBJECT_TRACK,
            (uint8_t*)MCS_OBJECT_TRACK0, (void*)0);
        lea_mcs_add_object(msg->event_data.mcs_id, ADPT_LEA_MCS_OBJECT_TRACK,
            (uint8_t*)MCS_OBJECT_TRACK1, (void*)1);
        break;
    }
    case MCS_OBJECT_ADDAD:
    case MCS_OBJECT_REMOVED:
    case MCS_SEGMENTS_STATE:
        break;
    case MCS_SET_PLAYBACK_SPEED:
    case MCS_SET_CURRENT_TRACK:
    case MCS_SET_NEXT_TRACK:
    case MCS_SET_CURRENT_GROUP:
    case MCS_SET_PLAYING_ORDER: {
        MCS_CALLBACK_FOREACH(g_mcs_service.callbacks, mcs_state_cb, msg->event);
        break;
    }
    case MCS_CONTROL_POINT_PLAY: {
        if (current_state == ADPT_LEA_MCS_MEDIA_STATE_PLAYING) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            break;
        }

        if (media_session_start(control_session) < 0) {
            BT_LOGE("%s, Session: Play control failed", __func__);
            if (media_session_start(control_session) == mcs_player_inactive) {
                lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_PLAYER_INACTIVE);
                current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
                lea_mcs_media_state_changed(current_state);
                break;
            }
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
            break;
        }
        isRemoteControl = true;
        break;
    }
    case MCS_CONTROL_POINT_PAUSE: {
        if (current_state == ADPT_LEA_MCS_MEDIA_STATE_PAUSED) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            break;
        }

        if (media_session_pause(control_session) < 0) {
            BT_LOGE("%s, Session: Pause control failed", __func__);
            if (media_session_pause(control_session) == mcs_player_inactive) {
                lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_PLAYER_INACTIVE);
                current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
                lea_mcs_media_state_changed(current_state);
                break;
            }
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
            break;
        }
        isRemoteControl = true;
        break;
    }
    case MCS_CONTROL_POINT_STOP: {
        if (current_state == ADPT_LEA_MCS_MEDIA_STATE_PAUSED) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            break;
        }

        if (media_session_pause(control_session) < 0) {
            BT_LOGE("%s, Session: Stop-Pause control failed", __func__);
            if (media_session_pause(control_session) == mcs_player_inactive) {
                lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_PLAYER_INACTIVE);
                current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
                lea_mcs_media_state_changed(current_state);
                break;
            }
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
            break;
        }

        if (media_session_seek(control_session, 0) < 0) {
            BT_LOGE("%s, Session: pause-seek(0) control failed", __func__);
            // todo: update position
        }
        isRemoteControl = true;
        break;
    }
    case MCS_CONTROL_POINT_PREVIOUS_TRACK: {
        if (media_session_prev_song(control_session) < 0) {
            BT_LOGE("%s, Session: PREVIOUS_TRACK control failed", __func__);
            if (media_session_prev_song(control_session) == mcs_player_inactive) {
                lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_PLAYER_INACTIVE);
                current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
                lea_mcs_media_state_changed(current_state);
                break;
            }
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
            break;
        }
        isRemoteControl = true;
        break;
    }
    case MCS_CONTROL_POINT_NEXT_TRACK: {
        if (media_session_next_song(control_session) < 0) {
            BT_LOGE("%s, Session: NEXT_TRACK control failed", __func__);
            if (media_session_next_song(control_session) == mcs_player_inactive) {
                lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_PLAYER_INACTIVE);
                current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
                lea_mcs_media_state_changed(current_state);
                break;
            }
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
            break;
        }
        isRemoteControl = true;
        break;
    }
    case MCS_CONTROL_POINT_MOVE: {
        uint32_t position, duration;
        if (media_session_get_position(control_session, &position) < 0 || media_session_get_duration(control_session, &duration) < 0) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
            break;
        }

        int32_t offset = msg->event_data.valueint32;
        if (offset > (int32_t)(duration - position)) {
            position = duration;
        } else if (offset < -(int32_t)position) {
            position = 0;
        } else {
            position += offset;
        }

        if (media_session_seek(control_session, position) < 0) {
            BT_LOGE("%s, Session: MOVE_RELATIVE control failed", __func__);
            if (media_session_seek(control_session, position) == mcs_player_inactive) {
                lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_PLAYER_INACTIVE);
                current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
                lea_mcs_media_state_changed(current_state);
                break;
            }
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
            break;
        }
        isRemoteControl = true;
        break;
    }
    case MCS_CONTROL_POINT_FAST_REWIND:
    case MCS_CONTROL_POINT_FAST_FORWARD:
    case MCS_CONTROL_POINT_PREVIOUS_SEGMENT:
    case MCS_CONTROL_POINT_NEXT_SEGMENT:
    case MCS_CONTROL_POINT_FIRST_SEGMENT:
    case MCS_CONTROL_POINT_LAST_SEGMENT:
    case MCS_CONTROL_POINT_FIRST_TRACK:
    case MCS_CONTROL_POINT_LAST_TRACK:
    case MCS_CONTROL_POINT_PREVIOUS_GROUP:
    case MCS_CONTROL_POINT_NEXT_GROUP:
    case MCS_CONTROL_POINT_FIRST_GROUP:
    case MCS_CONTROL_POINT_LAST_GROUP:
    case MCS_SET_POSITION:
    case MCS_CONTROL_POINT_GOTO_SEGMENT:
    case MCS_CONTROL_POINT_GOTO_TRACK:
    case MCS_CONTROL_POINT_GOTO_GROUP:
    case MCS_SEARCH_TRACK_NAME:
    case MCS_SEARCH_ARTIST_NAME:
    case MCS_SEARCH_ALBUM_NAME:
    case MCS_SEARCH_GROUP_NAME:
    case MCS_SEARCH_EARLIEST_YEAR:
    case MCS_SEARCH_LATEST_YEAR:
    case MCS_SEARCH_GENRE:
    case MCS_SEARCH_TRACKS:
    case MCS_SEARCH_GROUPS: {
        MCS_CALLBACK_FOREACH(g_mcs_service.callbacks, mcs_state_cb, msg->event);
        break;
    }
    default:
        BT_LOGW("%s, Unknown event!", __func__);
        break;
    }
    mcs_event_destory(msg);
}

static bt_status_t lea_mcs_send_msg(mcs_event_t* msg)
{
    assert(msg);

    do_in_service_loop(lea_mcs_process_message, msg);

    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_on_mcs_state(uint32_t mcs_id, uint8_t ccid, bool added)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_STATE, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint8 = ccid;
    event->event_data.valuebool = added;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_player_info_set_result(uint32_t mcs_id, void* player_ref, bool result)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_PLAYER_SET, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.ref = player_ref;
    event->event_data.valuebool = result;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_object_added_result(uint32_t mcs_id, void* obj_ref, lea_object_id obj_id)
{
    BT_LOGD("%s, [MCS][obj_id:%02x%02x%02x%02x%02x%02x]", __func__, obj_id[5], obj_id[4],
        obj_id[3], obj_id[2], obj_id[1], obj_id[0]);

    int idx = (uint32_t)obj_ref & 0xFF;
    int type = (uint32_t)obj_ref >> 16;
    if (type == ADPT_LEA_MCS_OBJECT_GROUP) {
        memcpy(mcs_group_oids + idx, obj_id, sizeof(lea_object_id));
        if (mcs_cur_group_oid == MCS_GROUPS_MAX) {
            mcs_cur_group_oid = idx;
            lea_mcs_current_group_changed(obj_id);
        }
    } else {
        memcpy(mcs_track_oids + idx, obj_id, sizeof(lea_object_id));
        if (mcs_cur_track_oid == MCS_TRACKS_MAX) {
            mcs_cur_track_oid = idx;
            lea_mcs_current_track_changed(obj_id);
        } else if (idx == (mcs_cur_track_oid + 1)) {
            mcs_next_track_oid = idx;
            lea_mcs_next_track_changed(obj_id);
        }
    }
}

void lea_on_mcs_set_position_result(uint32_t mcs_id, int32_t position)
{
    BT_LOGD("%s, position:%d ", __func__, position);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SET_POSITION, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueint32 = position;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_set_playback_speed_result(uint32_t mcs_id, int8_t speed)
{
    BT_LOGD("%s, speed:%d ", __func__, speed);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SET_PLAYBACK_SPEED, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueint8 = speed;
    lea_mcs_send_msg(event);
}

void lea_on_mcs_set_current_track_result(uint32_t mcs_id, lea_object_id track_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SET_CURRENT_TRACK, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (track_id != NULL)
        memcpy(&event->event_data.obj_id, track_id, sizeof(lea_object_id));

    lea_mcs_send_msg(event);
}

void lea_on_mcs_set_next_track_result(uint32_t mcs_id, lea_object_id track_id)
{
    BT_LOGD("%s", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SET_NEXT_TRACK, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (track_id != NULL)
        memcpy(&event->event_data.obj_id, track_id, sizeof(lea_object_id));

    lea_mcs_send_msg(event);
}

void lea_on_mcs_set_current_group_result(uint32_t mcs_id, lea_object_id group_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SET_CURRENT_GROUP, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (group_id != NULL)
        memcpy(&event->event_data.obj_id, group_id, sizeof(lea_object_id));

    lea_mcs_send_msg(event);
}

void lea_on_mcs_set_playing_order_result(uint32_t mcs_id, uint8_t order)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SET_PLAYING_ORDER, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint8 = order;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_play_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_PLAY, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_pause_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_PAUSE, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_fast_rewind_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_FAST_REWIND, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_fast_forward_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_FAST_FORWARD, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_stop_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_STOP, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_move_result(uint32_t mcs_id, int32_t offset)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_MOVE, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueint32 = offset;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_previous_segment_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_PREVIOUS_SEGMENT, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_next_segment_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_NEXT_SEGMENT, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_first_segment_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_FIRST_SEGMENT, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_last_segment_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_LAST_SEGMENT, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_goto_segment_result(uint32_t mcs_id, int32_t n_segment)
{
    BT_LOGD("%s, segment num:%d ", __func__, n_segment);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_GOTO_SEGMENT, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueint32 = n_segment;
    lea_mcs_send_msg(event);
}

void lea_on_mcs_previous_track_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_PREVIOUS_TRACK, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_next_track_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_NEXT_TRACK, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_first_track_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_FIRST_TRACK, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_last_track_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_LAST_TRACK, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_goto_track_result(uint32_t mcs_id, int32_t n_track)
{
    BT_LOGD("%s, track num:%d", __func__, n_track);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_GOTO_TRACK, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueint32 = n_track;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_previous_group_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_PREVIOUS_GROUP, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_next_group_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_NEXT_GROUP, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_first_group_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_FIRST_GROUP, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_last_group_result(uint32_t mcs_id)
{
    BT_LOGD("%s ", __func__);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_LAST_GROUP, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcs_send_msg(event);
}

void lea_on_mcs_goto_group_result(uint32_t mcs_id, int32_t n_group)
{
    BT_LOGD("%s, group num:%d ", __func__, n_group);
    mcs_event_t* event;

    event = mcs_event_new(MCS_CONTROL_POINT_GOTO_GROUP, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueint32 = n_group;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_track_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new_ext(MCS_SEARCH_TRACK_NAME, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint16 = size;
    event->event_data.valuebool = last_condition;
    strcpy((char*)event->event_data.dataarry, name);

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_artist_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new_ext(MCS_SEARCH_ARTIST_NAME, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint16 = size;
    event->event_data.valuebool = last_condition;
    strcpy((char*)event->event_data.dataarry, name);

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_album_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new_ext(MCS_SEARCH_ALBUM_NAME, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint16 = size;
    event->event_data.valuebool = last_condition;
    strcpy((char*)event->event_data.dataarry, name);

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_group_name_result(uint32_t mcs_id, size_t size, char* name, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new_ext(MCS_SEARCH_GROUP_NAME, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint16 = size;
    event->event_data.valuebool = last_condition;
    strcpy((char*)event->event_data.dataarry, name);

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_earliest_year_result(uint32_t mcs_id, size_t size, char* year, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new_ext(MCS_SEARCH_EARLIEST_YEAR, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint16 = size;
    event->event_data.valuebool = last_condition;
    strcpy((char*)event->event_data.dataarry, year);

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_latest_year_result(uint32_t mcs_id, size_t size, char* year, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new_ext(MCS_SEARCH_LATEST_YEAR, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint16 = size;
    event->event_data.valuebool = last_condition;
    strcpy((char*)event->event_data.dataarry, year);

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_genre_result(uint32_t mcs_id, size_t size, char* name, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new_ext(MCS_SEARCH_GENRE, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valueuint16 = size;
    event->event_data.valuebool = last_condition;
    strcpy((char*)event->event_data.dataarry, name);

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_tracks_result(uint32_t mcs_id, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SEARCH_TRACKS, mcs_id);

    event->event_data.valuebool = last_condition;

    lea_mcs_send_msg(event);
}

void lea_on_mcs_search_groups_result(uint32_t mcs_id, bool last_condition)
{
    BT_LOGD("%s, last_condition:%d ", __func__, last_condition);
    mcs_event_t* event;

    event = mcs_event_new(MCS_SEARCH_GROUPS, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    event->event_data.valuebool = last_condition;

    lea_mcs_send_msg(event);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

void inactive_state_command_handler(int event)
{
    BT_LOGD("%s, event:%d ", __func__, event);
    switch (event) {
    case MEDIA_EVENT_START: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PLAYING;
        lea_mcs_media_state_changed(current_state);
        break;
    }
    default:
        BT_LOGW("%s, Unexpect event:%d ", __func__, event);
        break;
    }
}

void playing_state_command_handler(int event)
{
    BT_LOGD("%s, event:%d ", __func__, event);
    switch (event) {
    case MEDIA_EVENT_STOP: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
        lea_mcs_media_state_changed(current_state);
        break;
    }
    case MEDIA_EVENT_START: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PLAYING;
        break;
    }
    case MEDIA_EVENT_PAUSE: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PAUSED;
        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        lea_mcs_media_state_changed(current_state);
        break;
    }
#if 0
    case MEDIA_EVENT_SEEK: {
        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        break;
    }
#endif
    case MEDIA_EVENT_PREV_SONG:
    case MEDIA_EVENT_NEXT_SONG: {
        int temp_id = mcs_cur_track_oid;
        mcs_cur_track_oid = mcs_next_track_oid;
        mcs_next_track_oid = temp_id;

        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        lea_mcs_track_title_changed((uint8_t*)MCS_TRACK_TITLE);
        lea_mcs_current_track_changed(mcs_track_oids[mcs_cur_track_oid]);
        lea_mcs_next_track_changed(mcs_track_oids[mcs_next_track_oid]);
        break;
    }
    default:
        BT_LOGW("%s, Unexpect event:%d ", __func__, event);
        break;
    }
}

void paused_state_command_handler(int event)
{
    BT_LOGD("%s, event:%d ", __func__, event);
    switch (event) {
    case MEDIA_EVENT_STOP: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
        lea_mcs_media_state_changed(current_state);
        break;
    }
    case MEDIA_EVENT_START: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PLAYING;
        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        lea_mcs_media_state_changed(current_state);
        break;
    }
    case MEDIA_EVENT_PAUSE: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PAUSED;
        break;
    }
#if 0
    case MEDIA_EVENT_SEEK: {
        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        break;
    }
#endif
    case MEDIA_EVENT_PREV_SONG:
    case MEDIA_EVENT_NEXT_SONG: {
        int temp_id = mcs_cur_track_oid;
        mcs_cur_track_oid = mcs_next_track_oid;
        mcs_next_track_oid = temp_id;

        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        lea_mcs_track_title_changed((uint8_t*)MCS_TRACK_TITLE);
        lea_mcs_current_track_changed(mcs_track_oids[mcs_cur_track_oid]);
        lea_mcs_next_track_changed(mcs_track_oids[mcs_next_track_oid]);
        break;
    }
    default:
        BT_LOGW("%s, Unexpect event:%d ", __func__, event);
        break;
    }
}

void seeking_state_command_handler(int event)
{
    BT_LOGD("%s, event:%d ", __func__, event);
    switch (event) {
    case MEDIA_EVENT_STOP: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
        lea_mcs_media_state_changed(current_state);
        break;
    }
    case MEDIA_EVENT_START: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PLAYING;
        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        lea_mcs_seeking_speed_changed(0);
        lea_mcs_media_state_changed(current_state);
        break;
    }
    case MEDIA_EVENT_PAUSE: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PAUSED;
        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        lea_mcs_seeking_speed_changed(0);
        lea_mcs_media_state_changed(current_state);
        break;
    }
#if 0
    case MEDIA_EVENT_SEEK: {
        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        break;
    }
#endif
    case MEDIA_EVENT_PREV_SONG:
    case MEDIA_EVENT_NEXT_SONG: {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_PAUSED;
        int temp_id = mcs_cur_track_oid;
        mcs_cur_track_oid = mcs_next_track_oid;
        mcs_next_track_oid = temp_id;

        if (isRemoteControl) {
            lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS);
            isRemoteControl = false;
        }
        lea_mcs_media_state_changed(current_state);
        lea_mcs_track_title_changed((uint8_t*)MCS_TRACK_TITLE);
        lea_mcs_current_track_changed(mcs_track_oids[mcs_cur_track_oid]);
        lea_mcs_next_track_changed(mcs_track_oids[mcs_next_track_oid]);
        break;
    }
    default:
        BT_LOGW("%s, Unexpect event:%d ", __func__, event);
        break;
    }
}

void (*command_handlers[ADPT_LEA_MCS_MEDIA_STATE_LAST])(int event) = {
    inactive_state_command_handler,
    playing_state_command_handler,
    paused_state_command_handler,
    seeking_state_command_handler
};

static lea_adpt_mcs_media_state_t playerState2McsState(int playerState)
{
    lea_adpt_mcs_media_state_t McsState;

    if (playerState == MEDIA_EVENT_START) {
        McsState = ADPT_LEA_MCS_MEDIA_STATE_PLAYING;
    } else if (playerState == MEDIA_EVENT_PAUSE) {
        McsState = ADPT_LEA_MCS_MEDIA_STATE_PAUSED;
    } else if (playerState == MEDIA_EVENT_STOP) {
        McsState = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
    } else {
        McsState = ADPT_LEA_MCS_MEDIA_STATE_LAST;
    }
    BT_LOGD("%s, event_cb:%d, McsState:%d ", __func__, playerState, McsState);
    return McsState;
}

static void mcs_session_event_callback(void* cookie, int event,
    int ret, const char* data)
{
    uint32_t position, duration;

    if (isRemoteControl && ret < 0) {
        BT_LOGE("%s, Session event cb, ret:%d ", __func__, ret);
        lea_mcs_media_control_response(ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED);
        isRemoteControl = false;
        return;
    }

    command_handlers[current_state](event);

    if (media_session_get_position(control_session, &position) >= 0) {
        lea_mcs_track_position_changed(position);
    }
    if (media_session_get_duration(control_session, &duration) >= 0) {
        lea_mcs_track_duration_changed(duration);
    }
}

static bt_status_t lea_mcs_media_init()
{
    int ret, playerState;
    uint32_t position, duration;

    lea_mcs_add();
    lea_mcs_set_media_player_info();

    control_session = media_session_open("Music");
    if (!control_session) {
        BT_LOGE("%s media session open failed.", __func__);
        return BT_STATUS_FAIL;
    }
    ret = media_session_set_event_callback(control_session, control_session, mcs_session_event_callback);
    assert(!ret);

    ret = media_session_get_state(control_session, &playerState);
    if (ret < 0) {
        BT_LOGE("%s get player state failed.", __func__);
        current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
    } else {
        current_state = playerState2McsState(playerState);
    }

    if (current_state >= ADPT_LEA_MCS_MEDIA_STATE_LAST) {
        current_state = ADPT_LEA_MCS_MEDIA_STATE_INACTIVE;
        lea_mcs_media_state_changed(ADPT_LEA_MCS_MEDIA_STATE_INACTIVE);
    } else {
        lea_mcs_media_state_changed(current_state);
    }

    if (media_session_get_position(control_session, &position) >= 0) {
        lea_mcs_track_position_changed(position);
    }

    if (media_session_get_duration(control_session, &duration) >= 0) {
        lea_mcs_track_duration_changed(duration);
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_media_cleanup()
{
    lea_mcs_remove();

    media_session_close(control_session);
    isRemoteControl = false;

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_add()
{
    BT_LOGD("%s", __func__);
    bt_status_t ret;

    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_add(lea_mcs_id);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_remove()
{
    BT_LOGD("%s", __func__);
    bt_status_t ret;

    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_remove(lea_mcs_id);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_set_media_player_info()
{
    BT_LOGD("%s ", __func__);
    bt_status_t ret;

    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_set_media_player_info(lea_mcs_id);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_add_object(uint32_t mcs_id, uint8_t type, uint8_t* name, void* obj_ref)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, type:%d, name:%s", __func__, type, name);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_add_object(mcs_id, type, name, obj_ref);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_playing_order_changed(uint8_t order)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, order:%d", __func__, order);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_playing_order_changed(lea_mcs_id, order);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_media_state_changed(lea_adpt_mcs_media_state_t state)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, state:%d", __func__, state);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_media_state_changed(lea_mcs_id, state);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_playback_speed_changed(int8_t speed)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, speed:%d", __func__, speed);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_playback_speed_changed(lea_mcs_id, speed);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_seeking_speed_changed(int8_t speed)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, speed:%d", __func__, speed);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_seeking_speed_changed(lea_mcs_id, speed);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_track_title_changed(uint8_t* title)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, title:%s", __func__, title);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_track_title_changed(lea_mcs_id, title);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_track_duration_changed(int32_t duration)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, duration:%d", __func__, duration);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_track_duration_changed(lea_mcs_id, duration);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_track_position_changed(int32_t position)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, position:%d", __func__, position);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_track_position_changed(lea_mcs_id, position);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_current_track_changed(lea_object_id track_id)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s, track_id:%s", __func__, track_id);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_current_track_changed(lea_mcs_id, track_id);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_next_track_changed(lea_object_id track_id)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s ", __func__);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_next_track_changed(lea_mcs_id, track_id);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_current_group_changed(lea_object_id group_id)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s ", __func__);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_current_group_changed(lea_mcs_id, group_id);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_parent_group_changed(lea_object_id group_id)
{
    CHECK_ENABLED();
    bt_status_t ret;

    BT_LOGD("%s ", __func__);
    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_parent_group_changed(lea_mcs_id, group_id);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_media_control_response(lea_adpt_mcs_media_control_result_t result)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcs_service.device_lock);
    ret = bt_sal_lea_mcs_media_control_response(lea_mcs_id, result);
    if (ret != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_mcs_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcs_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static void* lea_mcs_set_callbacks(void* handle, lea_mcs_callbacks_t* callbacks)
{
    if (!g_mcs_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_mcs_service.callbacks, handle, (void*)callbacks);
}

static bool lea_mcs_reset_callbacks(void** handle, void* cookie)
{
    if (!g_mcs_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_mcs_service.callbacks, handle, cookie);
}

static const lea_mcs_interface_t leMcsInterface = {
    .size = sizeof(leMcsInterface),
    .mcs_add = lea_mcs_add,
    .mcs_remove = lea_mcs_remove,
    .add_object = lea_mcs_add_object,
    .playing_order_changed = lea_mcs_playing_order_changed,
    .media_state_changed = lea_mcs_media_state_changed,
    .playback_speed_changed = lea_mcs_playback_speed_changed,
    .seeking_speed_changed = lea_mcs_seeking_speed_changed,
    .track_title_changed = lea_mcs_track_title_changed,
    .track_duration_changed = lea_mcs_track_duration_changed,
    .track_position_changed = lea_mcs_track_position_changed,
    .current_track_changed = lea_mcs_current_track_changed,
    .next_track_changed = lea_mcs_next_track_changed,
    .current_group_changed = lea_mcs_current_group_changed,
    .parent_group_changed = lea_mcs_parent_group_changed,
    .set_media_player_info = lea_mcs_set_media_player_info,
    .media_control_response = lea_mcs_media_control_response,
    .set_callbacks = lea_mcs_set_callbacks,
    .reset_callbacks = lea_mcs_reset_callbacks,
};

/****************************************************************************
 * Public function
 ****************************************************************************/
static const void* get_lea_mcs_profile_interface(void)
{
    return &leMcsInterface;
}

static bt_status_t lea_mcs_init(void)
{
    BT_LOGD("%s", __func__);
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcs_startup(profile_on_startup_t cb)
{
    BT_LOGD("%s", __func__);
    bt_status_t status;
    pthread_mutexattr_t attr;
    mcs_service_t* service = &g_mcs_service;
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->callbacks = bt_callbacks_list_new(2);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->device_lock, &attr);

    service->started = true;

    lea_mcs_media_init();

    return BT_STATUS_SUCCESS;

fail:
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    pthread_mutex_destroy(&service->device_lock);
    return status;
}

static bt_status_t lea_mcs_shutdown(profile_on_shutdown_t cb)
{
    if (!g_mcs_service.started)
        return BT_STATUS_SUCCESS;

    pthread_mutex_lock(&g_mcs_service.device_lock);
    lea_mcs_media_cleanup();

    g_mcs_service.started = false;

    bt_callbacks_list_free(g_mcs_service.callbacks);
    g_mcs_service.callbacks = NULL;
    pthread_mutex_unlock(&g_mcs_service.device_lock);
    pthread_mutex_destroy(&g_mcs_service.device_lock);
    return BT_STATUS_SUCCESS;
}

static void lea_mcs_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_mcs_dump(void)
{
    printf("impl leaudio mcs dump");
    return 0;
}

static const profile_service_t lea_mcs_service = {
    .auto_start = true,
    .name = PROFILE_MCS_NAME,
    .id = PROFILE_LEAUDIO_MCS,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = lea_mcs_init,
    .startup = lea_mcs_startup,
    .shutdown = lea_mcs_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_mcs_profile_interface,
    .cleanup = lea_mcs_cleanup,
    .dump = lea_mcs_dump,
};

void register_lea_mcs_service(void)
{
    register_service(&lea_mcs_service);
}

#endif