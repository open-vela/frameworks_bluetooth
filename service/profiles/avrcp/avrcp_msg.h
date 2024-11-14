/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#ifndef __AVRCP_MSG_H__
#define __AVRCP_MSG_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_avrcp.h"

typedef enum {
    AVRC_STARTUP,
    AVRC_SHUTDOWN,
    AVRC_CONNECTION_STATE_CHANGED,
    AVRC_GET_ELEMENT_ATTR_REQ,
    AVRC_GET_PLAY_STATUS_REQ,
    AVRC_PASSTHROUHT_CMD,
    AVRC_REGISTER_NOTIFICATION_REQ,
    AVRC_REGISTER_NOTIFICATION_ABSVOL_RSP,
    AVRC_PASSTHROUHT_CMD_RSP,
    AVRC_GET_CAPABILITY_RSP,
    AVRC_SET_ABSOLUTE_VOLUME,
    AVRC_REGISTER_NOTIFICATION_ABSVOL_REQ,
    AVRC_REGISTER_NOTIFICATION_RSP,
    AVRC_GET_ELEMENT_ATTRIBUTES_RSP,
    AVRC_GET_PLAY_STATUS_RSP,

    AVRC_GET_PLAYBACK_STATE,
    AVRC_VOLUME_CHANGED_NOTIFY,

    AVRC_PLAYSTATUS_NOTIFY,
} rc_msg_id_t;

typedef struct {
    profile_connection_state_t conn_state;
    profile_connection_reason_t reason;
} rc_profile_connection_state_t;

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
    uint8_t company_id;
    uint8_t cap_count;
    uint8_t capabilities[255];
} rc_capabilities_t;

typedef struct {
    avrcp_notification_event_t event;
    uint32_t value;
} rc_notification_rsp_t;

typedef struct {
    avrcp_passthr_cmd_t opcode;
    avrcp_key_state_t state;
} rc_passthr_cmd_t;

typedef struct {
    avrcp_notification_event_t event;
    uint32_t interval;
} rc_register_notification_t;

typedef struct {
    uint8_t volume;
} rc_absvol_t;

typedef struct {
    uint8_t count;
    uint32_t types[AVRCP_MAX_ATTR_COUNT];
    uint16_t chr_sets[AVRCP_MAX_ATTR_COUNT];
    char* attrs[AVRCP_MAX_ATTR_COUNT];
} rc_element_attrs_t;

typedef struct {
    bt_address_t addr;
    rc_msg_id_t id;
    uint8_t role;
    union {
        rc_profile_connection_state_t conn_state;
        rc_passthr_cmd_t passthr_cmd;
        rc_register_notification_t notify_req;
        rc_passthr_rsp_t passthr_rsp;
        rc_play_status_t playstatus;
        rc_capabilities_t cap;
        rc_notification_rsp_t notify_rsp;
        rc_absvol_t absvol;
        rc_element_attrs_t attrs;
        void* context;
    } data;
} avrcp_msg_t;

typedef void (*avrcp_msg_callback_t)(avrcp_msg_t* msg);

avrcp_msg_t* avrcp_msg_new(rc_msg_id_t msg, bt_address_t* bd_addr);
void avrcp_msg_destory(avrcp_msg_t* avrcp_msg);

#endif
