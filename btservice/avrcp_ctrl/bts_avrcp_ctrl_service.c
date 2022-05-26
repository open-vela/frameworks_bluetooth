/****************************************************************************
 * frameworks/bluetooth/src/btmanager/avrcp_target.c
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
#include <stdio.h>
#include <stdint.h>

#include "btm_manager.h"
#include "btm_avrcp.h"
#include "bts_avrcp.h"

static const avrc_ctrl_callbacks_t* avrcpCtrlCallbacks = NULL;

static void avrcp_connection_state_cb(bt_address addr, avrcp_connection_state_t state)
{
    if (avrcpCtrlCallbacks && avrcpCtrlCallbacks->connection_state_cb)
        avrcpCtrlCallbacks->connection_state_cb(addr, state);
}

static void avrcp_passthrough_rsp_cb(bt_address addr, avrcp_passthr_cmd_t key_code,
                                    avrcp_key_state_t key_state, uint8_t response)
{
    if (avrcpCtrlCallbacks && avrcpCtrlCallbacks->passthrough_rsp_cb)
        avrcpCtrlCallbacks->passthrough_rsp_cb(addr, key_code, key_state, response);
}

static void avrcp_play_position_changed_cb(bt_address addr, uint32_t song_len, uint32_t song_pos)
{
    if (avrcpCtrlCallbacks && avrcpCtrlCallbacks->play_position_changed_cb)
        avrcpCtrlCallbacks->play_position_changed_cb(addr, song_len, song_pos);
}

static void avrcp_play_status_changed_cb(bt_address addr, play_status_t play_status)
{
    if (avrcpCtrlCallbacks && avrcpCtrlCallbacks->play_status_changed_cb)
        avrcpCtrlCallbacks->play_status_changed_cb(addr, play_status);
}

static const avrc_ctrl_callbacks_t avrcp_ctrl_cbs = {
    sizeof(avrcp_ctrl_cbs),
    avrcp_connection_state_cb,
    avrcp_passthrough_rsp_cb,
    avrcp_play_position_changed_cb,
    avrcp_play_status_changed_cb
};

static bt_result_code avrcp_send_pass_through_cmd(bt_address addr, avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state)
{
    return bts_avrcp_send_passthrough_cmd(addr, key_code, key_state);
}

static bt_result_code avrcp_get_playback_state(bt_address addr)
{
    return bts_avrcp_get_play_status(addr);
}

static bt_result_code avrcp_set_callbacks(const avrc_ctrl_callbacks_t* callbacks)
{
    avrcpCtrlCallbacks = callbacks;
    return BT_RESULT_SUCCESS;
}

static void avrcp_reset_callbacks(void)
{
    avrcpCtrlCallbacks = NULL;
}

static const avrc_ctrl_interface_t avrcpCtrlSvrInterface = {
    sizeof(avrc_ctrl_interface_t),
    avrcp_send_pass_through_cmd,
    avrcp_get_playback_state,
    avrcp_set_callbacks,
    avrcp_reset_callbacks
};

bt_result_code avrcp_ctrl_service_start(void)
{
    return bts_avrcp_ctrl_init(&avrcp_ctrl_cbs);
}

void avrcp_ctrl_service_stop(void)
{
    bts_avrcp_ctrl_cleanup();
}

const avrc_ctrl_interface_t* get_avrcp_ctrl_service_interface(void)
{
    return &avrcpCtrlSvrInterface;
}