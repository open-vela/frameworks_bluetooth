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
#ifndef __SAL_LEA_MCP_INTERFACE_H__
#define __SAL_LEA_MCP_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "stack_adapter_lea_mcp.h"

typedef enum {
    MCP_MEDIA_CONTROL_PLAY = 0x01,
    MCP_MEDIA_CONTROL_PAUSE = 0x02,
    MCP_MEDIA_CONTROL_FAST_REWIND = 0x03,
    MCP_MEDIA_CONTROL_FAST_FORWARD = 0x04,
    MCP_MEDIA_CONTROL_STOP = 0x05,

    MCP_MEDIA_CONTROL_MOVE_RELATIVE = 0x10,

    MCP_MEDIA_CONTROL_PREVIOUS_SEGMENT = 0x20,
    MCP_MEDIA_CONTROL_NEXT_SEGMENT = 0x21,
    MCP_MEDIA_CONTROL_FIRST_SEGMENT = 0x22,
    MCP_MEDIA_CONTROL_LAST_SEGMENT = 0x23,
    MCP_MEDIA_CONTROL_GOTO_SEGMENT = 0x24,

    MCP_MEDIA_CONTROL_PREVIOUS_TRACK = 0x30,
    MCP_MEDIA_CONTROL_NEXT_TRACK = 0x31,
    MCP_MEDIA_CONTROL_FIRST_TRACK = 0x32,
    MCP_MEDIA_CONTROL_LAST_TRACK = 0x33,
    MCP_MEDIA_CONTROL_GOTO_TRACK = 0x34,

    MCP_MEDIA_CONTROL_PREVIOUS_GROUP = 0x40,
    MCP_MEDIA_CONTROL_NEXT_GROUP = 0x41,
    MCP_MEDIA_CONTROL_FIRST_GROUP = 0x42,
    MCP_MEDIA_CONTROL_LAST_GROUP = 0x43,
    MCP_MEDIA_CONTROL_GOTO_GROUP = 0x44,
} LEA_MCP_MEDIA_CONTROL_OPCODE;

/** @brief Type of search control item. */
typedef enum {
    MCP_SEARCH_TRACK_NAME = 0x01, /**< Search track name, UTF-8 string */
    MCP_SEARCH_ARTIST_NAME, /**< Search artist name, UTF-8 string */
    MCP_SEARCH_ALBUM_NAME, /**< Search album name, UTF-8 string */
    MCP_SEARCH_GROUP_NAME, /**< Search group name, UTF-8 string */
    MCP_SEARCH_EARLIEST_YEAR, /**< Search items to the specified year , UTF-8 string */
    MCP_SEARCH_LATEST_YEAR, /**< Search items from the specified year, UTF-8 string */
    MCP_SEARCH_GENRE, /**< Search by any UTF-8 string */
    MCP_SEARCH_ONLY_TRACKS, /**< Search only tracks, no additional parameter */
    MCP_SEARCH_ONLY_GROUPS, /**< Search only groups, no additional parameter */
} LEA_MCP_SEARCH_CONTROL_ITEM_TYPE;

bt_status_t bt_sal_lea_mcp_read_media_player_name(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_media_player_icon_object_id(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_media_player_icon_url(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_playback_speed(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_seeking_speed(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_playing_order(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_playing_orders_supported(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_media_control_opcodes_supported(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_track_title(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_track_duration(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_track_position(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_media_state(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_media_control_request(bt_address_t *addr, uint32_t mcs_id,
                                                 LEA_MCP_MEDIA_CONTROL_OPCODE opcode, int32_t n);
bt_status_t bt_sal_lea_mcp_search_control_request(bt_address_t *addr, uint32_t mcs_id,
                                                  uint8_t number, LEA_MCP_SEARCH_CONTROL_ITEM_TYPE type, uint8_t *parameter);
bt_status_t bt_sal_lea_mcp_read_current_track_object_id(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_next_track_object_id(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_parent_group_object_id(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_current_group_object_id(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_search_results_object_id(bt_address_t *addr, uint32_t mcs_id);
bt_status_t bt_sal_lea_mcp_read_content_control_id(bt_address_t *addr, uint32_t mcs_id);

#endif /* __SAL_LEA_MCP_INTERFACE_H__ */
