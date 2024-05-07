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
#ifndef __LEA_MCP_EVENT_H__
#define __LEA_MCP_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"
#include "bt_lea_mcp.h"
#include <stdint.h>

typedef enum {
    MCP_MEDIA_PLAYER_NAME_CHANGED,
    MCP_MEDIA_PLAYER_ICON_OBJ_ID,
    MCP_MEDIA_PLAYER_ICON_URL,
    MCP_READ_PLAYBACK_SPEED,
    MCP_READ_SEEKING_SPEED,
    MCP_READ_PLAYING_ORDER,
    MCP_READ_PLAYING_ORDER_SUPPORTED,
    MCP_READ_MEDIA_CONTROL_OPCODES_SUPPORTED,
    MCP_TRACK_CHANGED,
    MCP_READ_TRACK_TITLE,
    MCP_READ_TRACK_DURATION,
    MCP_READ_TRACK_POSITION,
    MCP_READ_MEDIA_STATE,
    MCP_MEDIA_CONTROL_REQ,
    MCP_SEARCH_CONTROL_RESULT_REQ,
    MCP_CURRENT_TRACK_SEGMENTS_OBJ_ID,
    MCP_CURRENT_TRACK_OBJ_ID,
    MCP_NEXT_TRACK_OBJ_ID,
    MCP_PARENT_GROUP_OBJ_ID,
    MCP_CURRENT_GROUP_OBJ_ID,
    MCP_SEARCH_RESULTS_OBJ_ID,
    MCP_READ_CCID
} mcp_event_type_t;

typedef struct {
    uint32_t mcs_id;
    int8_t valueint8;
    uint8_t valueuint8_0;
    uint8_t valueuint8_1;
    uint16_t valueuint16;
    int32_t valueint32;
    uint32_t valueuint32;
    lea_mcp_object_id obj_id;
    char string1[0];
} mcp_event_data_t;

typedef struct {
    bt_address_t remote_addr;
    mcp_event_type_t event;
    mcp_event_data_t event_data;
} mcp_event_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
mcp_event_t* mcp_event_new(mcp_event_type_t event, bt_address_t* remote_addr, uint32_t mcs_id);
mcp_event_t* mcp_event_new_ext(mcp_event_type_t event, bt_address_t* remote_addr, uint32_t mcs_id, size_t size);
void mcp_event_destory(mcp_event_t* mcp_event);

#endif /* __LEA_MCP_EVENT_H__ */
