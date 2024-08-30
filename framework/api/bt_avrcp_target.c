/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "avrcp_target_api"

#include <stdint.h>

#include "avrcp_target_service.h"
#include "bt_avrcp_target.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "service_manager.h"
#include "utils/log.h"

static avrcp_target_interface_t* get_profile_service(void)
{
    return (avrcp_target_interface_t*)service_manager_get_profile(PROFILE_AVRCP_TG);
}

void* BTSYMBOLS(bt_avrcp_target_register_callbacks)(bt_instance_t* ins, const avrcp_target_callbacks_t* callbacks)
{
    avrcp_target_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool BTSYMBOLS(bt_avrcp_target_unregister_callbacks)(bt_instance_t* ins, void* cookie)
{
    avrcp_target_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t BTSYMBOLS(bt_avrcp_target_get_play_status_response)(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t play_status,
    uint32_t song_len, uint32_t song_pos)
{
    avrcp_target_interface_t* profile = get_profile_service();

    return profile->get_play_status_rsp(addr, play_status, song_len, song_pos);
}

bt_status_t BTSYMBOLS(bt_avrcp_target_play_status_notify)(bt_instance_t* ins, bt_address_t* addr, avrcp_play_status_t play_status)
{
    avrcp_target_interface_t* profile = get_profile_service();

    return profile->play_status_notify(addr, play_status);
}
