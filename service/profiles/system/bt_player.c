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
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <media_api.h>

#include <bt_player.h>
#include <bt_utils.h>

#include "utils/log.h"

typedef struct bt_media_controller {
    void* mediasession;
    void* holder;
    bt_media_notify_callback_t cb;
} bt_media_controller_t;

typedef struct bt_media_player {
    void* mediasession;
    void* context;
    bt_media_status_t play_status;
    bt_media_player_callback_t* cb;
} bt_media_player_t;

static void notify_media_event(bt_media_controller_t* controller,
    bt_media_event_t event,
    uint32_t value)
{
    if (controller->cb)
        controller->cb(controller, controller->holder, event, value);
}

static bt_media_status_t media_state_to_playback_status(int media_state)
{
    bt_media_status_t playback_status;

    if (media_state > 0) { /* Active */
        playback_status = BT_MEDIA_PLAY_STATUS_PLAYING;
    } else if (media_state == 0) { /* Inactive */
        playback_status = BT_MEDIA_PLAY_STATUS_PAUSED;
    } else { /* Error */
        playback_status = BT_MEDIA_PLAY_STATUS_ERROR;
    }
    BT_LOGD("%s, media_state:%d, playback_status:%d ", __func__, media_state, playback_status);

    return playback_status;
}

static void media_session_event_cb(void* cookie, int event, int ret,
    const char* extra)
{
    bt_media_controller_t* controller = cookie;
    int status;
    int media_state;

    switch (event) {
    case MEDIA_EVENT_START:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYSTATUS_CHANGED, BT_MEDIA_PLAY_STATUS_PLAYING);
        break;
    case MEDIA_EVENT_PAUSE:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYSTATUS_CHANGED, BT_MEDIA_PLAY_STATUS_PAUSED);
        break;
    case MEDIA_EVENT_STOP:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYSTATUS_CHANGED, BT_MEDIA_PLAY_STATUS_STOPPED);
        break;
    case MEDIA_EVENT_PREV_SONG:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYSTATUS_CHANGED, BT_MEDIA_PLAY_STATUS_REV_SEEK);
        break;
    case MEDIA_EVENT_NEXT_SONG:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYSTATUS_CHANGED, BT_MEDIA_PLAY_STATUS_FWD_SEEK);
        break;
    case MEDIA_EVENT_UPDATED:
    case MEDIA_EVENT_CHANGED:
        if (ret & MEDIA_METAFLAG_STATE) {
            /* playback status changed */
            status = media_session_get_state(controller->mediasession, &media_state);
            if (status != 0) {
                BT_LOGE("%s, faild to get media state", __func__);
                return;
            }

            notify_media_event(controller, BT_MEDIA_EVT_PLAYSTATUS_CHANGED,
                media_state_to_playback_status(media_state));
        }
        break;
    default:
        return;
    }
}

char* bt_media_evt_str(bt_media_event_t evt)
{
    switch (evt) {
        CASE_RETURN_STR(BT_MEDIA_EVT_PREPARED);
        CASE_RETURN_STR(BT_MEDIA_EVT_PLAYSTATUS_CHANGED);
        CASE_RETURN_STR(BT_MEDIA_EVT_POSITION_CHANGED);
        CASE_RETURN_STR(BT_MEDIA_EVT_TRACK_CHANGED);
    default:
        return "ERROR";
    }
}

char* bt_media_status_str(uint8_t status)
{
    switch (status) {
    case BT_MEDIA_PLAY_STATUS_STOPPED:
        return "STOPPED";
    case BT_MEDIA_PLAY_STATUS_PLAYING:
        return "PLAYING";
    case BT_MEDIA_PLAY_STATUS_PAUSED:
        return "PAUSED";
    case BT_MEDIA_PLAY_STATUS_FWD_SEEK:
        return "FWD_SEEK";
    case BT_MEDIA_PLAY_STATUS_REV_SEEK:
        return "PREV_SEEK";
    default:
        return "ERROR";
    }
}

bt_media_controller_t* bt_media_controller_create(void* context, bt_media_notify_callback_t cb)
{
    bt_media_controller_t* controller = malloc(sizeof(*controller));
    int ret = 0;

    if (controller == NULL)
        return NULL;

    controller->mediasession = media_session_open(NULL);
    if (!controller->mediasession) {
        free(controller);
        return NULL;
    }

    ret = media_session_set_event_callback(controller->mediasession,
        controller, media_session_event_cb);
    if (ret != 0) {
        media_session_close(controller->mediasession);
        free(controller);
        return NULL;
    }
    controller->holder = context;
    controller->cb = cb;

    return controller;
}

void bt_media_controller_set_context(bt_media_controller_t* controller, void* context)
{
    controller->holder = context;
}

void bt_media_controller_destory(bt_media_controller_t* controller)
{
    if (!controller)
        return;

    media_session_close(controller->mediasession);
    free(controller);
}

bt_status_t bt_media_player_play(bt_media_controller_t* controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_start(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_pause(bt_media_controller_t* controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_pause(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_stop(bt_media_controller_t* controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_stop(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_next(bt_media_controller_t* controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_next_song(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_prev(bt_media_controller_t* controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_prev_song(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_get_playback_status(bt_media_controller_t* controller,
    bt_media_status_t* status)
{
    int state = 0;

    if (!controller || !status)
        return BT_STATUS_PARM_INVALID;

    if (media_session_get_state(controller->mediasession, &state) != 0) {
        *status = BT_MEDIA_PLAY_STATUS_STOPPED;
        return BT_STATUS_NOT_SUPPORTED;
    }

    *status = media_state_to_playback_status(state);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_get_position(bt_media_controller_t* controller, uint32_t* positions)
{
    if (!controller || !positions)
        return BT_STATUS_PARM_INVALID;

    if (media_session_get_position(controller->mediasession, (unsigned int*)positions) != 0) {
        *positions = 0xFFFFFFFF;
        return BT_STATUS_NOT_SUPPORTED;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_get_durations(bt_media_controller_t* controller, uint32_t* durations)
{
    if (!controller || !durations)
        return BT_STATUS_PARM_INVALID;

    if (media_session_get_duration(controller->mediasession, (unsigned int*)durations) != 0) {
        *durations = 0xFFFFFFFF;
        return BT_STATUS_NOT_SUPPORTED;
    }

    return BT_STATUS_SUCCESS;
}

static void media_control_event_cb(void* cookie, int event,
    int ret, const char* extra)
{
    bt_media_player_t* player = cookie;

    switch (event) {
    case MEDIA_EVENT_START:
        player->cb->on_play(player, player->context);
        break;
    case MEDIA_EVENT_PAUSE:
        player->cb->on_pause(player, player->context);
        break;
    case MEDIA_EVENT_STOP:
        player->cb->on_stop(player, player->context);
        break;
    case MEDIA_EVENT_PREV_SONG:
        player->cb->on_prev_song(player, player->context);
        break;
    case MEDIA_EVENT_NEXT_SONG:
        player->cb->on_next_song(player, player->context);
        break;
    default:
        break;
    }
}

bt_media_player_t* bt_media_player_create(void* context, bt_media_player_callback_t* cb)
{
    if (context == NULL || cb == NULL)
        return NULL;

    bt_media_player_t* player = malloc(sizeof(*player));
    if (!player)
        return NULL;

    player->mediasession = media_session_register(player, media_control_event_cb);
    if (!player->mediasession) {
        free(player);
        return NULL;
    }
    player->cb = cb;
    player->context = context;
    player->play_status = BT_MEDIA_PLAY_STATUS_ERROR;

    return player;
}

void bt_media_player_destory(bt_media_player_t* player)
{
    if (!player)
        return;

    if (player->mediasession) {
        media_session_notify(player->mediasession, MEDIA_EVENT_STOPPED, 0, NULL);
        media_session_unregister(player->mediasession);
    }

    free(player);
}

bt_status_t bt_media_player_set_status(bt_media_player_t* player, bt_media_status_t status)
{
    int event;

    if (!player)
        return BT_STATUS_PARM_INVALID;

    if (player->play_status == status)
        return BT_STATUS_SUCCESS;

    switch (status) {
    case BT_MEDIA_PLAY_STATUS_STOPPED:
        event = MEDIA_EVENT_STOP;
        break;
    case BT_MEDIA_PLAY_STATUS_PLAYING:
        event = MEDIA_EVENT_START;
        break;
    case BT_MEDIA_PLAY_STATUS_PAUSED:
        event = MEDIA_EVENT_PAUSE;
        break;
    case BT_MEDIA_PLAY_STATUS_FWD_SEEK:
        event = MEDIA_EVENT_NEXT_SONG;
        break;
    case BT_MEDIA_PLAY_STATUS_REV_SEEK:
        event = MEDIA_EVENT_PREV_SONG;
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    media_session_notify(player->mediasession, event, 0, NULL);
    player->play_status = status;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_set_duration(bt_media_player_t* player, uint32_t duration)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_media_player_set_position(bt_media_player_t* player, uint32_t position)
{
    return BT_STATUS_NOT_SUPPORTED;
}
