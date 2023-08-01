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
#ifndef __LEA_MCPS_EVENT_H__
#define __LEA_MCPS_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include "bt_lea_mcps.h"

typedef enum {
    MCS_STATE = 0,
    MCS_PLAYER_SET,
    MCS_OBJECT_ADDAD,
    MCS_OBJECT_REMOVED,
    MCS_SEGMENTS_STATE,
    MCS_SET_POSITION,
    MCS_SET_PLAYBACK_SPEED,
    MCS_SET_CURRENT_TRACK,
    MCS_SET_NEXT_TRACK,
    MCS_SET_CURRENT_GROUP,
    MCS_SET_PLAYING_ORDER,
    MCS_CONTROL_POINT_PLAY,
    MCS_CONTROL_POINT_PAUSE,
    MCS_CONTROL_POINT_FAST_REWIND,
    MCS_CONTROL_POINT_FAST_FORWARD,
    MCS_CONTROL_POINT_STOP,
    MCS_CONTROL_POINT_MOVE,
    MCS_CONTROL_POINT_PREVIOUS_SEGMENT,
    MCS_CONTROL_POINT_NEXT_SEGMENT,
    MCS_CONTROL_POINT_FIRST_SEGMENT,
    MCS_CONTROL_POINT_LAST_SEGMENT,
    MCS_CONTROL_POINT_GOTO_SEGMENT,
    MCS_CONTROL_POINT_PREVIOUS_TRACK,
    MCS_CONTROL_POINT_NEXT_TRACK,
    MCS_CONTROL_POINT_FIRST_TRACK,
    MCS_CONTROL_POINT_LAST_TRACK,
    MCS_CONTROL_POINT_GOTO_TRACK,
    MCS_CONTROL_POINT_PREVIOUS_GROUP,
    MCS_CONTROL_POINT_NEXT_GROUP,
    MCS_CONTROL_POINT_FIRST_GROUP,
    MCS_CONTROL_POINT_LAST_GROUP,
    MCS_CONTROL_POINT_GOTO_GROUP,
    MCS_SEARCH_TRACK_NAME,
    MCS_SEARCH_ARTIST_NAME,
    MCS_SEARCH_ALBUM_NAME,
    MCS_SEARCH_GROUP_NAME,
    MCS_SEARCH_EARLIEST_YEAR,
    MCS_SEARCH_LATEST_YEAR,
    MCS_SEARCH_GENRE,
    MCS_SEARCH_TRACKS,
    MCS_SEARCH_GROUPS
} mcps_event_type_t;

typedef struct {
    uint32_t mcs_id;
    uint32_t valueuint32;
    int32_t valueint32;
    uint16_t valueuint16;
    uint8_t valueuint8;
    int8_t valueint8;
    bool valuebool;
    void *ref;
    lea_object_id obj_id;
    uint8_t dataarry[1];
} mcps_event_data_t;

typedef struct {
    mcps_event_type_t event;
    mcps_event_data_t event_data;
} mcps_event_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
mcps_event_t *mcps_event_new(mcps_event_type_t event, uint32_t mcs_id);
mcps_event_t *mcps_event_new_ext(mcps_event_type_t event, uint32_t mcs_id, size_t size);
void mcps_event_destory(mcps_event_t* mcps_event);

#endif /* __LEA_MCPS_EVENT_H__ */