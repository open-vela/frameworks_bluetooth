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
#ifndef __LEA_MCP_SERVICE_H__
#define __LEA_MCP_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_lea_mcp.h"
#include "sal_lea_mcp_interface.h"
#include "stddef.h"

typedef struct {
    uint8_t num;
    uint32_t sid;
} bts_mcs_info_s;

typedef enum {
    MCP_MEDIA_PLAYER_NAME = 1,
    MCP_MEIDA_PLAYER_ICON_OBJECT_ID,
    MCP_MEIDA_PLAYER_ICON_URL,
    MCP_PLAYBACK_SPEED,
    MCP_SEEKING_SPEED,
    MCP_PLAYING_ORDER,
    MCP_PLAYING_ORDERS_SUPPORTED,
    MCP_MEIDA_CONTROL_OPCODES_SUPPORTED,
    MCP_TRACK_TITLE,
    MCP_TRACK_DURATION,
    MCP_TRACK_POSITION,
    MCP_MEDIA_STATE,
    MCP_CURRENT_TRACK_OBJECT_ID,
    MCP_NEXT_TRACK_OBJECT_ID,
    MCP_PARENT_GROUP_OBJECT_ID,
    MCP_CURRENT_GROUP_OBJECT_ID,
    MCP_SEARCH_RESULTS_OBJECT_ID,
    MCP_CONTENT_CONTROL_ID,
} lea_mcp_opcode_t;

/*
 * sal callback
 */
void lea_mcp_on_media_player_name(bt_address_t* addr, uint32_t mcs_id, size_t size, char* name);
void lea_mcp_on_media_player_icon_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id);
void lea_mcp_on_media_player_icon_url(bt_address_t* addr, uint32_t mcs_id, size_t size, char* url);
void lea_mcp_on_playback_speed(bt_address_t* addr, uint32_t mcs_id, int8_t speed);
void lea_mcp_on_seeking_speed(bt_address_t* addr, uint32_t mcs_id, int8_t speed);
void lea_mcp_on_playing_order(bt_address_t* addr, uint32_t mcs_id, int8_t order);
void lea_mcp_on_playing_orders_supported(bt_address_t* addr, uint32_t mcs_id, uint16_t orders);
void lea_mcp_on_media_control_opcodes_supported(bt_address_t* addr, uint32_t mcs_id, uint32_t opcodes);
void lea_mcp_on_track_changed(bt_address_t* addr, uint32_t mcs_id);
void lea_mcp_on_track_title(bt_address_t* addr, uint32_t mcs_id, size_t size, char* title);
void lea_mcp_on_track_duration(bt_address_t* addr, uint32_t mcs_id, int32_t duration);
void lea_mcp_on_track_position(bt_address_t* addr, uint32_t mcs_id, int32_t position);
void lea_mcp_on_media_state(bt_address_t* addr, uint32_t mcs_id, uint8_t state);
void lea_mcp_on_media_control_result(bt_address_t* addr, uint32_t mcs_id, uint8_t opcode, uint8_t result);
void lea_mcp_on_search_control_result(bt_address_t* addr, uint32_t mcs_id, uint8_t result);
void lea_mcp_on_current_track_segments_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id);
void lea_mcp_on_current_track_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id);
void lea_mcp_on_next_track_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id);
void lea_mcp_on_parent_group_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id);
void lea_mcp_on_current_group_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id);
void lea_mcp_on_search_results_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id);
void lea_mcp_on_content_control_id(bt_address_t* addr, uint32_t mcs_id, uint8_t ccid);

typedef struct {
    size_t size;
    bt_status_t (*read_remote_mcs_info)(bt_address_t* addr, uint8_t opcode);
    bt_status_t (*media_control_request)(bt_address_t* addr,
        LEA_MCP_MEDIA_CONTROL_OPCODE opcode, int32_t n);
    bt_status_t (*search_control_request)(bt_address_t* addr,
        uint8_t number, LEA_MCP_SEARCH_CONTROL_ITEM_TYPE type, uint8_t* parameter);
    void* (*set_callbacks)(void* handle, lea_mcp_callbacks_t* callbacks);
    bool (*reset_callbacks)(void** handle, void* cookie);
} lea_mcp_interface_t;

/*
 * register profile to service manager
 */
void register_lea_mcp_service(void);

/*
 * set mcs id infomation
 */
void adapt_mcs_sid_changed(uint32_t sid);

#endif /* __LEA_MCP_SERVICE_H__ */
