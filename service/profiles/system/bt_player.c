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

#include "bt_player.h"

typedef struct bt_media_controller {
    void *mediasession;
    void *holder;
    bt_media_notify_callback_t cb;
} bt_media_controller_t;

typedef struct {
    void *session;
} bt_media_player_t;

static void notify_media_event(bt_media_controller_t *controller,
                               bt_media_event_t event,
                               uint32_t value)
{
    if (controller->cb)
        controller->cb(controller, controller->holder, event, value);
}

static void media_session_event_cb(void *cookie, int event, int ret,
                                   const char *extra)
{
    bt_media_controller_t *controller = cookie;

    switch (event) {
    case MEDIA_EVENT_PREPARED:
        notify_media_event(controller, BT_MEDIA_EVT_PREPARED, 0);
        break;
    case MEDIA_EVENT_STARTED:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYBACK_STATUS_CHANGED, BT_MEDIA_PLAY_STATUS_PLAYING);
        break;
    case MEDIA_EVENT_PAUSED:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYBACK_STATUS_CHANGED, BT_MEDIA_PLAY_STATUS_PAUSED);
        break;
    case MEDIA_EVENT_STOPPED:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYBACK_STATUS_CHANGED, BT_MEDIA_PLAY_STATUS_STOPPED);
        break;
    case MEDIA_EVENT_PREVED:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYBACK_STATUS_CHANGED, BT_MEDIA_PLAY_STATUS_REV_SEEK);
        break;
    case MEDIA_EVENT_NEXTED:
        notify_media_event(controller, BT_MEDIA_EVT_PLAYBACK_STATUS_CHANGED, BT_MEDIA_PLAY_STATUS_FWD_SEEK);
        break;
    default:
        return;
    }
}

bt_media_controller_t *bt_media_controller_create(void *context, bt_media_notify_callback_t cb)
{
    bt_media_controller_t *controller = malloc(sizeof(*controller));
    int ret = 0;

    if (controller == NULL)
        return NULL;

    controller->mediasession = media_session_open(MEDIA_STREAM_MUSIC);
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

void bt_media_controller_set_context(bt_media_controller_t *controller, void *context)
{
    controller->holder = context;
}

void bt_media_controller_destory(bt_media_controller_t *controller)
{
    if (!controller)
        return;

    media_session_close(controller->mediasession);
    free(controller);
}

bt_status_t bt_media_player_play(bt_media_controller_t *controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_start(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_pause(bt_media_controller_t *controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_pause(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_stop(bt_media_controller_t *controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_stop(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_next(bt_media_controller_t *controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_next_song(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_prev(bt_media_controller_t *controller)
{
    if (!controller)
        return BT_STATUS_PARM_INVALID;

    if (media_session_prev_song(controller->mediasession) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_get_playback_status(bt_media_controller_t *controller,
                                                bt_media_status_t *status)
{
    int state = 0;

    if (!controller || !status)
        return BT_STATUS_PARM_INVALID;

    if (media_session_get_state(controller->mediasession, &state) != 0) {
        *status = BT_MEDIA_PLAY_STATUS_STOPPED;
        return BT_STATUS_NOT_SUPPORTED;
    }

    // media state to bt playback status
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_get_position(bt_media_controller_t *controller, uint32_t *positions)
{
    if (!controller || !positions)
        return BT_STATUS_PARM_INVALID;

    if (media_session_get_position(controller->mediasession, positions) != 0) {
        *positions = 0xFFFFFFFF;
        return BT_STATUS_NOT_SUPPORTED;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_player_get_durations(bt_media_controller_t *controller, uint32_t *durations)
{
    if (!controller || !durations)
        return BT_STATUS_PARM_INVALID;

    if (media_session_get_duration(controller->mediasession, durations) != 0) {
        *durations = 0xFFFFFFFF;
        return BT_STATUS_NOT_SUPPORTED;
    }

    return BT_STATUS_SUCCESS;
}

bt_media_player_t *bt_media_player_create(void)
{
    return NULL;
}

void bt_media_player_destory(bt_media_player_t *player)
{
}
