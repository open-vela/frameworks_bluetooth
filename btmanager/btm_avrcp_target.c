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
#include "bts_service.h"
#include "bts_service_interface.h"


static avrcp_tg_interface_t* get_service(void)
{
    return (avrcp_tg_interface_t*)get_bluetooth_service_interface()->get_profile_interface(BT_PROFILE_AV_RC_TARGET);
}

static bt_result_code avrc_get_play_status_rsp(bt_address addr,
                                        avrcp_play_status_t status,
                                        uint32_t song_len, uint32_t song_pos)
{
    avrcp_tg_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->get_play_status_rsp(addr, status, song_len, song_pos);
}

static bt_result_code avrc_play_status_notify(bt_address addr, avrcp_play_status_t status)
{
    avrcp_tg_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->play_status_notify(addr, status);
}

static bt_result_code avrc_set_absolute_volume(bt_address addr, uint8_t volume)
{
    avrcp_tg_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->set_absolute_volume(addr, volume);
}

static bt_result_code avrc_set_callbacks(avrcp_tg_callbacks_t* callbacks)
{
    avrcp_tg_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->set_callbacks(callbacks);
}

static void avrc_reset_callbacks(void)
{
    avrcp_tg_interface_t* service = get_service();
    if (!service)
        return;

    service->reset_callbacks();
}

static const avrcp_tg_interface_t avrcpTgInterface = {
    sizeof(avrcpTgInterface),
    avrc_get_play_status_rsp,
    avrc_play_status_notify,
    avrc_set_absolute_volume,
    avrc_set_callbacks,
    avrc_reset_callbacks
};

const avrcp_tg_interface_t *get_avrcp_tg_interface(void)
{
    return &avrcpTgInterface;
}