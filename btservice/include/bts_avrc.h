/*
 * Copyright (C) 2020 Xiaomi Corporation
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
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "btm_manager.h"
#include "btm_avrcp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AVRC_ROLE_TARGET    0
#define AVRC_ROLE_CTRL      1

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum {
    CONNECTION_STATE_CHANGED,
    GET_ELEMENT_ATTR_REQ,
    GET_PLAY_STATUS_REQ,
    PASSTHROUHT_CMD,
    REGISTER_NOTIFICATION_REQ,
    REGISTER_NOTIFICATION_ABSVOL_RSP,
    PASSTHROUHT_CMD_RSP,
    GET_CAPABILITY_RSP,
    SET_ABSOLUTE_VOLUME,
    REGISTER_NOTIFICATION_ABSVOL_REQ,
    REGISTER_NOTIFICATION_RSP,
    GET_ELEMENT_ATTRIBUTES_RSP,
    GET_PLAY_STATUS_RSP
} rc_msg_id_t;

typedef struct {
    avrcp_passthr_cmd_t cmd;
    avrcp_key_state_t state;
    uint8_t rsp;
} rc_passthr_rsp_t;

typedef struct {
    avrcp_play_status_t status;
    uint32_t song_len;
    uint32_t song_pos;
} rc_play_status_t;

typedef struct {
    uint8_t cap_count;
    uint8_t *capabilities;
} rc_capabilities_t;

typedef struct {
    avrcp_notification_event_t event;
    uint32_t value;
} rc_notification_rsp_t;

typedef struct {
    avrcp_passthr_cmd_t opcode;
    avrcp_key_state_t   state;
} rc_passthr_cmd_t;

typedef struct {
    avrcp_notification_event_t event;
    uint32_t interval;
} rc_register_notification_t;

typedef struct {
    uint8_t volume;
} rc_absvol_t;

typedef struct {
    bt_address addr;
    rc_msg_id_t id;
    uint8_t role;
    union {
        avrcp_connection_state_t conn_state;
        rc_passthr_cmd_t passthr_cmd;
        rc_register_notification_t notify_req;
        rc_passthr_rsp_t passthr_rsp;
        rc_play_status_t playstatus;
        rc_capabilities_t cap;
        rc_notification_rsp_t notify_rsp;
        rc_absvol_t absvol;
    } data;
} avrcp_msg_t;

typedef void (*avrcp_msg_callback_t)(avrcp_msg_t *msg);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
bt_result_code bts_avrcp_get_play_status_response(bt_address addr, avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos);
bt_result_code bts_avrcp_notify_play_state_changed(bt_address addr, avrcp_play_status_t status);
bt_result_code bts_avrcp_register_volume_changed(bt_address addr);
bt_result_code bts_avrcp_notify_volume_changed(bt_address addr, uint8_t volume);
bt_result_code bts_avrcp_set_absolute_volume(bt_address addr, uint8_t volume);
bt_result_code bts_avrcp_notify_track_changed(bt_address addr, bool selected);
bt_result_code bts_avrcp_notify_play_position_changed(bt_address addr, uint32_t position);
bt_result_code bts_avrcp_send_passthrough_cmd(bt_address addr, avrcp_passthr_cmd_t cmd, avrcp_key_state_t state);
bt_result_code bts_avrcp_get_play_status(bt_address addr);
bt_result_code bts_avrcp_get_remote_capabilities(bt_address addr);
void bts_avrcp_init(uint8_t role, bool absolute_support, avrcp_msg_callback_t callback);
void bts_avrcp_cleanup(uint8_t role);