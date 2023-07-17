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
#ifndef __LEA_MCPC_SERVICE_H__
#define __LEA_MCPC_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "stddef.h"
#include "bt_lea_mcpc.h"
#include "sal_lea_mcpc_interface.h"

typedef struct {
    uint8_t num;
    uint32_t sid;
} bts_mcs_info_s;

typedef enum {
    MCPC_MEDIA_PLAYER_NAME = 1,
    MCPC_MEIDA_PLAYER_ICON_OBJECT_ID,
    MCPC_MEIDA_PLAYER_ICON_URL,
    MCPC_PLAYBACK_SPEED,
    MCPC_SEEKING_SPEED,
    MCPC_PLAYING_ORDER,
    MCPC_PLAYING_ORDERS_SUPPORTED,
    MCPC_MEIDA_CONTROL_OPCODES_SUPPORTED,
    MCPC_TRACK_TITLE,
    MCPC_TRACK_DURATION,
    MCPC_TRACK_POSITION,
    MCPC_MEDIA_STATE,
    MCPC_CURRENT_TRACK_OBJECT_ID,
    MCPC_NEXT_TRACK_OBJECT_ID,
    MCPC_PARENT_GROUP_OBJECT_ID,
    MCPC_CURRENT_GROUP_OBJECT_ID,
    MCPC_SEARCH_RESULTS_OBJECT_ID,
    MCPC_CONTENT_CONTROL_ID,
} lea_mcpc_opcode_t;

/*
 * sal callback
 */
void lea_mcpc_on_media_player_name(bt_address_t *addr, uint32_t mcs_id, size_t size, char *name);
void lea_mcpc_on_media_player_icon_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id);
void lea_mcpc_on_media_player_icon_url(bt_address_t *addr, uint32_t mcs_id, size_t size, char *url);
void lea_mcpc_on_playback_speed(bt_address_t *addr, uint32_t mcs_id, int8_t speed);
void lea_mcpc_on_seeking_speed(bt_address_t *addr, uint32_t mcs_id, int8_t speed);
void lea_mcpc_on_playing_order(bt_address_t *addr, uint32_t mcs_id, int8_t order);
void lea_mcpc_on_playing_orders_supported(bt_address_t *addr, uint32_t mcs_id, uint16_t orders);
void lea_mcpc_on_media_control_opcodes_supported(bt_address_t *addr, uint32_t mcs_id, uint32_t opcodes);
void lea_mcpc_on_track_changed(bt_address_t *addr, uint32_t mcs_id);
void lea_mcpc_on_track_title(bt_address_t *addr, uint32_t mcs_id, size_t size, char *title);
void lea_mcpc_on_track_duration(bt_address_t *addr, uint32_t mcs_id, int32_t duration);
void lea_mcpc_on_track_position(bt_address_t *addr, uint32_t mcs_id, int32_t position);
void lea_mcpc_on_media_state(bt_address_t *addr, uint32_t mcs_id, uint8_t state);
void lea_mcpc_on_media_control_result(bt_address_t *addr, uint32_t mcs_id, uint8_t opcode, uint8_t result);
void lea_mcpc_on_search_control_result(bt_address_t *addr, uint32_t mcs_id, uint8_t result);
void lea_mcpc_on_current_track_segments_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id);
void lea_mcpc_on_current_track_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id);
void lea_mcpc_on_next_track_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id);
void lea_mcpc_on_parent_group_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id);
void lea_mcpc_on_current_group_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id);
void lea_mcpc_on_search_results_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id);
void lea_mcpc_on_content_control_id(bt_address_t *addr, uint32_t mcs_id, uint8_t ccid);

typedef struct {
    size_t size;
    bt_status_t (*read_remote_mcs_info)(bt_address_t *addr, uint8_t opcode);
    bt_status_t (*media_control_request)(bt_address_t *addr,
        LEA_MCC_MEDIA_CONTROL_OPCODE opcode, int32_t n);
    bt_status_t (*search_control_request)(bt_address_t *addr,
        uint8_t number, LEA_MCC_SEARCH_CONTROL_ITEM_TYPE type, uint8_t *parameter);
    void *(*set_callbacks)(void *handle, lea_mcpc_callbacks_t* callbacks);
    bool (*reset_callbacks)(void **handle, void *cookie);
} lea_mcpc_interface_t;

/*
 * register profile to service manager
 */
void register_lea_mcpc_service(void);

/*
 * set mcs id infomation
 */
void adapt_mcs_sid_changed(uint32_t sid);

#endif /* __LEA_MCPC_SERVICE_H__ */
