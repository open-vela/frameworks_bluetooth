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
#ifndef __BT_PLAYER_H__
#define __BT_PLAYER_H__

#include <bt_status.h>
typedef enum {
    BT_MEDIA_PLAY_STATUS_STOPPED = 0,
    BT_MEDIA_PLAY_STATUS_PLAYING,
    BT_MEDIA_PLAY_STATUS_PAUSED,
    BT_MEDIA_PLAY_STATUS_FWD_SEEK,
    BT_MEDIA_PLAY_STATUS_REV_SEEK,
    BT_MEDIA_PLAY_STATUS_ERROR,
} bt_media_status_t;

typedef enum {
    BT_MEDIA_EVT_PREPARED = 0,
    BT_MEDIA_EVT_PLAYSTATUS_CHANGED,
    BT_MEDIA_EVT_POSITION_CHANGED,
    BT_MEDIA_EVT_TRACK_CHANGED,
    BT_MEDIA_EVT_UNSUPPORT,
} bt_media_event_t;

typedef struct bt_media_controller bt_media_controller_t;
typedef struct bt_media_player bt_media_player_t;

typedef struct {
    void (*on_prepare)(bt_media_player_t* player, void* context);
    void (*on_play)(bt_media_player_t* player, void* context);
    void (*on_pause)(bt_media_player_t* player, void* context);
    void (*on_stop)(bt_media_player_t* player, void* context);
    void (*on_next_song)(bt_media_player_t* player, void* context);
    void (*on_prev_song)(bt_media_player_t* player, void* context);
} bt_media_player_callback_t;

typedef void (*bt_media_notify_callback_t)(bt_media_controller_t* controller, void* context,
    bt_media_event_t event, uint32_t value);

char* bt_media_evt_str(bt_media_event_t evt);
char* bt_media_status_str(uint8_t status);
bt_media_controller_t* bt_media_controller_create(void* context, bt_media_notify_callback_t cb);
void bt_media_controller_set_context(bt_media_controller_t* controller, void* context);
void bt_media_controller_destory(bt_media_controller_t* controller);
bt_status_t bt_media_player_play(bt_media_controller_t* controller);
bt_status_t bt_media_player_pause(bt_media_controller_t* controller);
bt_status_t bt_media_player_stop(bt_media_controller_t* controller);
bt_status_t bt_media_player_next(bt_media_controller_t* controller);
bt_status_t bt_media_player_prev(bt_media_controller_t* controller);
bt_status_t bt_media_player_get_playback_status(bt_media_controller_t* controller,
    bt_media_status_t* status);
bt_status_t bt_media_player_get_position(bt_media_controller_t* controller, uint32_t* positions);
bt_status_t bt_media_player_get_durations(bt_media_controller_t* controller, uint32_t* durations);

bt_media_player_t* bt_media_player_create(void* context, bt_media_player_callback_t* cb);
void bt_media_player_destory(bt_media_player_t* player);
bt_status_t bt_media_player_set_status(bt_media_player_t* player, bt_media_status_t status);
bt_status_t bt_media_player_set_duration(bt_media_player_t* player, uint32_t duration);
bt_status_t bt_media_player_set_position(bt_media_player_t* player, uint32_t position);
#endif /* __BT_PLAYER_H__ */