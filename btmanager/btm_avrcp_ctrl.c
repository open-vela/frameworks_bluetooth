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
#include "bts_service.h"
#include "btm_avrcp.h"
#include "bts_avrcp.h"
#include "bts_service_interface.h"


static avrc_ctrl_interface_t* get_service(void)
{
    return (avrc_ctrl_interface_t*)get_bluetooth_service_interface()->get_profile_interface(BT_PROFILE_AV_RC_CTRL);
}

static bt_result_code avrcp_send_pass_through_cmd(bt_address addr, avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state)
{
    avrc_ctrl_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->send_pass_through_cmd(addr, key_code, key_state);
}

static bt_result_code avrcp_get_playback_state(bt_address addr)
{
    avrc_ctrl_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->get_playback_state(addr);
}

static bt_result_code avrcp_set_callbacks(const avrc_ctrl_callbacks_t* callbacks)
{
    avrc_ctrl_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->set_callbacks(callbacks);
}

static void avrcp_reset_callbacks(void)
{
    avrc_ctrl_interface_t* service = get_service();
    if (!service)
        return;

    return service->reset_callbacks();
}

static const avrc_ctrl_interface_t avrcpCtrlInterface = {
    sizeof(avrc_ctrl_interface_t),
    avrcp_send_pass_through_cmd,
    avrcp_get_playback_state,
    avrcp_set_callbacks,
    avrcp_reset_callbacks
};

const avrc_ctrl_interface_t* get_avrcp_ctrl_interface(void)
{
    return &avrcpCtrlInterface;
}