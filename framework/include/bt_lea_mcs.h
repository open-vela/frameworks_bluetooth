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

#ifndef __BT_LEA_MCS_H__
#define __BT_LEA_MCS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include "lea_audio_common.h"
#include <stddef.h>

#define OBJ_ID_SIZE 6

typedef uint8_t lea_object_id[OBJ_ID_SIZE];

typedef enum {
    ADPT_LEA_MCS_MEDIA_STATE_INACTIVE,
    ADPT_LEA_MCS_MEDIA_STATE_PLAYING,
    ADPT_LEA_MCS_MEDIA_STATE_PAUSED,
    ADPT_LEA_MCS_MEDIA_STATE_SEEKING,
    ADPT_LEA_MCS_MEDIA_STATE_LAST,
} lea_adpt_mcs_media_state_t;

typedef enum {
    ADPT_LEA_MCS_MEDIA_CONTROL_SUCCESS = 1,
    ADPT_LEA_MCS_MEDIA_CONTROL_NOT_SUPPORTED,
    ADPT_LEA_MCS_MEDIA_CONTROL_PLAYER_INACTIVE,
    ADPT_LEA_MCS_MEDIA_CONTROL_CANT_BE_COMPLETED,
} lea_adpt_mcs_media_control_result_t;

/** @brief Possible MCS object types. */
typedef enum {
    ADPT_LEA_MCS_OBJECT_TRACK, /**< Track object type. */
    ADPT_LEA_MCS_OBJECT_GROUP, /**< Group object type. */
} lea_adpt_mcs_object_type_t;

typedef struct {
    uint16_t length; /**< Length of the string, not including the ending 0. */
    char* string; /**< UTF-8 string, terminated by 0. Its life cycle shall not be shorter than its container. It may be recycled when its container is recycled. */
} lea_utf8_str_t;

/** @brief Date time structure. */
typedef struct {
    uint16_t year; /**< 1582 to 9999, 0 is unknown, all others are RFU */
    uint8_t month; /**< 1 to 12, 0 as unknown, all others are RFU */
    uint8_t day; /**< 1 to 31, 0 as unknown, all others are RFU */
    uint8_t hours; /**< 0 to 23, All others are RFU */
    uint8_t minutes; /**< 0 to 59, All others are RFU */
    uint8_t seconds; /**< 0 to 59, All others are RFU */
} lea_date_time_t;

/** @brief Media Object information. */
typedef struct {
    uint32_t mcs_id; /**< ID of the MCS instance the media player attached to. */
    void* obj_ref; /**< Application specified object identity. */
    uint8_t* name; /**< Initial Object Name, e.g. group name or track title. Zero terminated UTF-8 string. */
    uint32_t size; /**< Size of the object */
    uint8_t type; /**< Type of the object, one of #SERVICE_LEA_MCS_OBJECT_TYPE. */
    lea_date_time_t
        first_created; /**< The date and time when the object is first created. Set to all 0s if unknown. */
    lea_date_time_t
        last_modified; /**< The date and time when the object content is last modified. Set to all 0s if unknown. */
} lea_media_object_t;

typedef void (*lea_mcs_server_state_callback)(void* cookie, uint8_t event);

typedef struct
{
    size_t size;
    lea_mcs_server_state_callback mcs_state_cb;
} lea_mcs_callbacks_t;

void* bt_lea_mcs_register_callbacks(bt_instance_t* ins, const lea_mcs_callbacks_t* callbacks);
bool bt_lea_mcs_unregister_callbacks(bt_instance_t* ins, void* cookie);
bt_status_t bt_lea_mcs_service_add(bt_instance_t* ins);
bt_status_t bt_lea_mcs_service_remove(bt_instance_t* ins);
bt_status_t bt_lea_mcs_playing_order_changed(bt_instance_t* ins, uint8_t order);
bt_status_t bt_lea_mcs_media_state_changed(bt_instance_t* ins, uint8_t state);
bt_status_t bt_lea_mcs_playback_speed_changed(bt_instance_t* ins, int8_t speed);
bt_status_t bt_lea_mcs_seeking_speed_changed(bt_instance_t* ins, int8_t speed);
bt_status_t bt_lea_mcs_track_title_changed(void* handl, uint8_t* title);
bt_status_t bt_lea_mcs_track_duration_changed(bt_instance_t* ins, int32_t duration);
bt_status_t bt_lea_mcs_track_position_changed(bt_instance_t* ins, int32_t position);
bt_status_t bt_lea_mcs_current_track_change(bt_instance_t* ins, lea_object_id track_id);
bt_status_t bt_lea_mcs_next_track_changed(bt_instance_t* ins, lea_object_id track_id);
bt_status_t bt_lea_mcs_current_group_changed(bt_instance_t* ins, lea_object_id group_id);
bt_status_t bt_lea_mcs_parent_group_changed(bt_instance_t* ins, lea_object_id group_id);
bt_status_t bt_lea_mcs_set_media_player_info(bt_instance_t* ins);
bt_status_t bt_lea_mcs_media_control_point_response(bt_instance_t* ins, lea_adpt_mcs_media_control_result_t result);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LEA_MCS_H__ */