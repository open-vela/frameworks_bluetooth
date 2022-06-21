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
#include "bts_avrcp_target.h"

static bt_result_code avrc_get_play_status_rsp(bt_address addr,
                                        play_status_t status,
                                        uint32_t song_len, uint32_t song_pos)
{
    return bts_avrcp_get_play_status_response(addr, status, song_len, song_pos);
}

static bt_result_code avrc_play_status_notify(bt_address addr, play_status_t status)
{
    return bts_avrcp_notify_play_state_changed(addr, status);
}

static bt_result_code avrc_volume_changed_notify(bt_address addr, uint8_t volume)
{
    return bts_avrcp_notify_volume_changed(addr, volume);
}

static bt_result_code avrc_set_absolute_volume(bt_address addr, uint8_t volume)
{
    return BT_RESULT_UNSUPPORTED;
}

static bt_result_code avrc_set_callbacks(avrcp_tg_callbacks_t* callbacks)
{
    bts_avrcp_set_callbacks(callbacks);

    return BT_RESULT_SUCCESS;
}

static void avrc_reset_callbacks(void)
{
    bts_avrcp_set_callbacks(NULL);
}

static const avrcp_tg_interface_t avrcpTgInterface = {
    sizeof(avrcpTgInterface),
    avrc_get_play_status_rsp,
    avrc_play_status_notify,
    avrc_volume_changed_notify,
    avrc_set_absolute_volume,
    avrc_set_callbacks,
    avrc_reset_callbacks
};

bt_result_code avrcp_target_service_start(void)
{
    return bts_avrcp_target_init();
}

void avrcp_target_service_stop(void)
{
    bts_avrcp_target_cleanup();
}

const avrcp_tg_interface_t *get_avrcp_tg_service_interface(void)
{
    return &avrcpTgInterface;
}