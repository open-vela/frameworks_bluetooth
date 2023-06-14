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
#define LOG_TAG "avrcp_control_api"

#include <stdint.h>

#include "avrcp_control_service.h"
#include "bt_avrcp_control.h"
#include "bt_profile.h"
#include "service_manager.h"
#include "utils/log.h"

static avrcp_control_interface_t *get_profile_service(void)
{
    return (avrcp_control_interface_t *)service_manager_get_profile(PROFILE_AVRCP_CT);
}

void *bt_avrcp_control_register_callbacks(bt_instance_t *ins, const avrcp_control_callbacks_t *callbacks)
{
    avrcp_control_interface_t *profile = get_profile_service();

    return profile->register_callbacks(NULL, callbacks);
}

bool bt_avrcp_control_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    avrcp_control_interface_t *profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_avrcp_control_send_pass_through_cmd(bt_address_t *bd_addr,
                                                   avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state)
{
    avrcp_control_interface_t *profile = get_profile_service();

    return profile->send_pass_through_cmd(bd_addr, key_code, key_state);
}

bt_status_t bt_avrcp_control_get_playback_state(bt_address_t *bd_addr)
{
    avrcp_control_interface_t *profile = get_profile_service();

    return profile->get_playback_state(bd_addr);
}

bt_status_t bt_avrcp_control_volume_changed_notify(bt_address_t *bd_addr, uint8_t volume)
{
    avrcp_control_interface_t *profile = get_profile_service();

    return profile->volume_changed_notify(bd_addr, volume);
}
