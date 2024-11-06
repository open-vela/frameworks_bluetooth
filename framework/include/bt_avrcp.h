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
#ifndef __BT_AVRCP_H__
#define __BT_AVRCP_H__

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"

/**
 * @cond
 */
#define AVRCP_MAX_ATTR_COUNT 9
#define AVRCP_ATTR_MAX_TIELE_LEN 64
#define AVRCP_ATTR_MAX_ARTIST_LEN 64
#define AVRCP_ATTR_MAX_ALBUM_LEN 0
#define AVRCP_ATTR_MAX_TRACK_NUMBER_LEN 0
#define AVRCP_ATTR_MAX_TOTAL_TRACK_NUMBER_LEN 0
#define AVRCP_ATTR_MAX_GENER_LEN 0
#define AVRCP_ATTR_MAX_PLAYING_TIMES_LEN 0
#define AVRCP_ATTR_MAX_COVER_ART_HANDLE_LEN 0

typedef enum {
    PASSTHROUGH_CMD_ID_SELECT,
    PASSTHROUGH_CMD_ID_UP,
    PASSTHROUGH_CMD_ID_DOWN,
    PASSTHROUGH_CMD_ID_LEFT,
    PASSTHROUGH_CMD_ID_RIGHT,
    PASSTHROUGH_CMD_ID_RIGHT_UP,
    PASSTHROUGH_CMD_ID_RIGHT_DOWN,
    PASSTHROUGH_CMD_ID_LEFT_UP,
    PASSTHROUGH_CMD_ID_LEFT_DOWN,
    PASSTHROUGH_CMD_ID_ROOT_MENU,
    PASSTHROUGH_CMD_ID_SETUP_MENU,
    PASSTHROUGH_CMD_ID_CONTENTS_MENU,
    PASSTHROUGH_CMD_ID_FAVORITE_MENU,
    PASSTHROUGH_CMD_ID_EXIT,
    PASSTHROUGH_CMD_ID_0,
    PASSTHROUGH_CMD_ID_1,
    PASSTHROUGH_CMD_ID_2,
    PASSTHROUGH_CMD_ID_3,
    PASSTHROUGH_CMD_ID_4,
    PASSTHROUGH_CMD_ID_5,
    PASSTHROUGH_CMD_ID_6,
    PASSTHROUGH_CMD_ID_7,
    PASSTHROUGH_CMD_ID_8,
    PASSTHROUGH_CMD_ID_9,
    PASSTHROUGH_CMD_ID_DOT,
    PASSTHROUGH_CMD_ID_ENTER,
    PASSTHROUGH_CMD_ID_CLEAR,
    PASSTHROUGH_CMD_ID_CHANNEL_UP,
    PASSTHROUGH_CMD_ID_CHANNEL_DOWN,
    PASSTHROUGH_CMD_ID_PREVIOUS_CHANNEL,
    PASSTHROUGH_CMD_ID_SOUND_SELECT,
    PASSTHROUGH_CMD_ID_INPUT_SELECT,
    PASSTHROUGH_CMD_ID_DISPLAY_INFO,
    PASSTHROUGH_CMD_ID_HELP,
    PASSTHROUGH_CMD_ID_PAGE_UP,
    PASSTHROUGH_CMD_ID_PAGE_DOWN,
    PASSTHROUGH_CMD_ID_POWER,
    PASSTHROUGH_CMD_ID_VOLUME_UP,
    PASSTHROUGH_CMD_ID_VOLUME_DOWN,
    PASSTHROUGH_CMD_ID_MUTE,
    PASSTHROUGH_CMD_ID_PLAY,
    PASSTHROUGH_CMD_ID_STOP,
    PASSTHROUGH_CMD_ID_PAUSE,
    PASSTHROUGH_CMD_ID_RECORD,
    PASSTHROUGH_CMD_ID_REWIND,
    PASSTHROUGH_CMD_ID_FAST_FORWARD,
    PASSTHROUGH_CMD_ID_EJECT,
    PASSTHROUGH_CMD_ID_FORWARD,
    PASSTHROUGH_CMD_ID_BACKWARD,
    PASSTHROUGH_CMD_ID_ANGLE,
    PASSTHROUGH_CMD_ID_SUBPICTURE,
    PASSTHROUGH_CMD_ID_F1,
    PASSTHROUGH_CMD_ID_F2,
    PASSTHROUGH_CMD_ID_F3,
    PASSTHROUGH_CMD_ID_F4,
    PASSTHROUGH_CMD_ID_F5,
    PASSTHROUGH_CMD_ID_VENDOR_UNIQUE,
    PASSTHROUGH_CMD_ID_NEXT_GROUP,
    PASSTHROUGH_CMD_ID_PREV_GROUP,
    PASSTHROUGH_CMD_ID_RESERVED
} avrcp_passthr_cmd_t;

typedef enum {
    AVRCP_KEY_PRESSED,
    AVRCP_KEY_RELEASED
} avrcp_key_state_t;

typedef enum {
    PLAY_STATUS_STOPPED = 0,
    PLAY_STATUS_PLAYING,
    PLAY_STATUS_PAUSED,
    PLAY_STATUS_FWD_SEEK,
    PLAY_STATUS_REV_SEEK,
    PLAY_STATUS_ERROR,
} avrcp_play_status_t;

typedef enum {
    AVRCP_CAPABILITY_ID_COMPANY_ID = 2,
    AVRCP_CAPABILITY_ID_EVENTS_SUPPORTED,
} avrcp_capability_id_t;

typedef enum {
    NOTIFICATION_EVT_PALY_STATUS_CHANGED = 0x01,
    NOTIFICATION_EVT_TRACK_CHANGED,
    NOTIFICATION_EVT_TRACK_END,
    NOTIFICATION_EVT_TRACK_START,
    NOTIFICATION_EVT_PLAY_POS_CHANGED,
    NOTIFICATION_EVT_BATTERY_STATUS_CHANGED,
    NOTIFICATION_EVT_SYSTEM_STATUS_CHANGED,
    NOTIFICATION_EVT_APP_SETTING_CHANGED,
    NOTIFICATION_EVT_NOW_PLAYING_CONTENT_CHANGED,
    NOTIFICATION_EVT_AVAILABLE_PLAYERS_CHANGED,
    NOTIFICATION_EVT_ADDRESSED_PLAYER_CHANGED,
    NOTIFICATION_EVT_UIDS_CHANGED,
    NOTIFICATION_EVT_VOLUME_CHANGED,
    NOTIFICATION_EVT_FLAG_INTERIM
} avrcp_notification_event_t;

typedef enum {
    AVRCP_RESPONSE_NOT_IMPLEMENTED,
    AVRCP_RESPONSE_ACCEPTED,
    AVRCP_RESPONSE_REJECTED,
    AVRCP_RESPONSE_IN_TRANSITION,
    AVRCP_RESPONSE_IMPLEMENTED_STABLE,
    AVRCP_RESPONSE_CHANGED,
    AVRCP_RESPONSE_INTERIM,
    AVRCP_RESPONSE_BROWSING,
    AVRCP_RESPONSE_SKIPPED,
    AVRCP_RESPONSE_TIMEOUT
} avrcp_response_t;

typedef enum {
    AVRCP_ATTR_TITLE = 1,
    AVRCP_ATTR_ARTIST_NAME,
    AVRCP_ATTR_ALBUM_NAME,
    AVRCP_ATTR_TRACK_NUMBER,
    AVRCP_ATTR_TOTAL_NUMBER_OF_TRACKS,
    AVRCP_ATTR_GENRE,
    AVRCP_ATTR_PLAYING_TIME_MS,
    AVRCP_ATTR_COVER_ART_HANDLE
} avrcp_media_attr_type_t;
/**
 * @endcond
 */

/**
 * @brief Callback for AVRCP connection state changed.
 *
 * There are four states for an AVRCP connection, namely DISCONNECTED, CONNECTING,
 * CONNECTED, and DISCONNECTING. During the initialization phase of the AVRCP,
 * it is necessary to register callback functions. This callback is triggered
 * when there is a change in the state of the AVRCP connection.
 *
 * Stable States:
 *    DISCONNECTED: The initial state.
 *    CONNECTED: The AVRCP connection is established.
 * Transient states:
 *    CONNECTING: The AVRCP connection is being established.
 *    DISCONNECTING: The AVRCP connection is being terminated.
 *
 * @param cookie - Callback cookie.
 * @param addr - The Bluetooth address of the peer device.
 * @param state - AVRCP profile connection state.
 *
 * **Example:**
 * @code
static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    printf("AVRCP connection state is: %d", state);
}
 * @endcode
 */
typedef void (*avrcp_connection_state_callback)(void* cookie, bt_address_t* addr, profile_connection_state_t state);

#endif /* __BT_AVRCP_H__ */
