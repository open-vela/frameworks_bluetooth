/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_avrcp.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "btm_manager.h"
#include "btm_avrcp.h"
#include "bts_avrcp_ctrl.h"
#include "bts_avrc.h"
#include "bts_service.h"
#include "utils/utils.h"
#define LOG_TAG "avrcp_ctrl"
#include "log.h"

static const avrc_ctrl_callbacks_t *g_ctrl_callbacks = NULL;
#if 0
static const char* avrcp_event_to_string(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(CONNECTION_STATE_CHANGED)
        CASE_RETURN_STR(PASSTHROUHT_CMD_RSP)
        CASE_RETURN_STR(SET_ABSOLUTE_VOLUME)
        CASE_RETURN_STR(REGISTER_NOTIFICATION_ABSVOL_REQ)
        CASE_RETURN_STR(REGISTER_NOTIFICATION_RSP)
        CASE_RETURN_STR(GET_PLAY_STATUS_RSP)
    default:
        return "UNKNOWN";
    }
}
#endif
static void handle_avrcp_ctrl_connection_state(bt_address addr,
                                               avrcp_connection_state_t state)
{
    BT_LOGD("%s, device: %s, connection state : %d", __func__, addr_str(addr), state);
    if (state == AVRC_CONNECTION_STATE_CONNECTED)
        bts_avrcp_get_remote_capabilities(addr);

    BT_CBACK(g_ctrl_callbacks, connection_state_cb, addr, state);
}

static void handle_avrcp_ctrl_register_notification_rsp(bt_address addr, avrcp_notification_event_t event, uint32_t value)
{
    switch (event) {
        case NOTIFICATION_EVT_PALY_STATUS_CHANGED:
            BT_CBACK(g_ctrl_callbacks, play_status_changed_cb, addr, value);
            break;
        case NOTIFICATION_EVT_PLAY_POS_CHANGED:
            BT_CBACK(g_ctrl_callbacks, play_position_changed_cb, addr, 0, value);
            break;
        default:
            break;
    }
}

static void bts_avrcp_ctrl_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    if (!data)
        return;

    avrcp_msg_t *msg = (avrcp_msg_t*)data;
    //BT_LOGD("%s, addr: %s, msgid: %s", __func__, addr_str(msg->addr), avrcp_event_to_string(msg->id));
    switch (msg->id) {
        case CONNECTION_STATE_CHANGED:
            handle_avrcp_ctrl_connection_state(msg->addr, msg->data.conn_state);
            break;
        case PASSTHROUHT_CMD_RSP: {
            rc_passthr_rsp_t *rsp = &msg->data.passthr_rsp;
            BT_CBACK(g_ctrl_callbacks, passthrough_rsp_cb, msg->addr, rsp->cmd, rsp->state, rsp->rsp);
            break;
        }
        case REGISTER_NOTIFICATION_ABSVOL_REQ:
            BT_CBACK(g_ctrl_callbacks, register_notification_absvol_cb, msg->addr);
            break;
        case REGISTER_NOTIFICATION_RSP: {
            rc_notification_rsp_t *notify = &msg->data.notify_rsp;
            handle_avrcp_ctrl_register_notification_rsp(msg->addr, notify->event, notify->value);
            break;
        }
        case GET_PLAY_STATUS_RSP: {
            rc_play_status_t *status = &msg->data.playstatus;
            BT_CBACK(g_ctrl_callbacks, play_status_changed_cb, msg->addr, status->status);
            BT_CBACK(g_ctrl_callbacks, play_position_changed_cb, msg->addr, status->song_len, status->song_pos);
            break;
        }
        case SET_ABSOLUTE_VOLUME:
            BT_CBACK(g_ctrl_callbacks, set_volume_cb, msg->addr, msg->data.absvol.volume);
            break;
        default:
        break;
    }
    free(data);
}

static void avrcp_ctrl_msg_callback(avrcp_msg_t *msg)
{
    avrcp_msg_t* copy;

    copy = (avrcp_msg_t*)malloc(sizeof(avrcp_msg_t));
    if (copy == NULL)
        return;

    memcpy(copy, msg, sizeof(avrcp_msg_t));
    bts_send_uv_msg(BT_PROFILE_AV_RC_CTRL_ID, copy, sizeof(avrcp_msg_t));
}

static bt_result_code avrcp_send_pass_through_cmd(bt_address addr, avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state)
{
    return bts_avrcp_send_passthrough_cmd(addr, key_code, key_state);
}

static bt_result_code avrcp_get_playback_state(bt_address addr)
{
    return bts_avrcp_get_play_status(addr);
}

static bt_result_code avrcp_volume_changed_notify(bt_address addr, uint8_t volume)
{
    return bts_avrcp_notify_volume_changed(addr, volume);
}

static bt_result_code avrcp_set_callbacks(const avrc_ctrl_callbacks_t* callbacks)
{
    g_ctrl_callbacks = callbacks;
    return BT_RESULT_SUCCESS;
}

static void avrcp_reset_callbacks(void)
{
    g_ctrl_callbacks = NULL;
}

static const avrc_ctrl_interface_t avrcpCtrlSvrInterface = {
    sizeof(avrc_ctrl_interface_t),
    avrcp_send_pass_through_cmd,
    avrcp_get_playback_state,
    avrcp_volume_changed_notify,
    avrcp_set_callbacks,
    avrcp_reset_callbacks
};

bt_result_code avrcp_ctrl_service_start(void)
{
    bts_avrcp_init(AVRC_ROLE_CTRL, false, avrcp_ctrl_msg_callback);
    bts_register_profile_process(BT_PROFILE_AV_RC_CTRL_ID, bts_avrcp_ctrl_handle_service_msg);

    return BT_RESULT_SUCCESS;
}

void avrcp_ctrl_service_stop(void)
{
    bts_unregister_profile_process(BT_PROFILE_AV_RC_CTRL_ID);
    bts_avrcp_cleanup(AVRC_ROLE_CTRL);
}

const avrc_ctrl_interface_t* get_avrcp_ctrl_service_interface(void)
{
    return &avrcpCtrlSvrInterface;
}