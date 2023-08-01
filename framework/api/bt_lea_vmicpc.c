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

#include "bt_lea_vmicpc.h"
#include "bt_profile.h"
#include "lea_vmicpc_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static lea_vmicpc_interface_t *get_profile_service(void)
{
    return (lea_vmicpc_interface_t *)service_manager_get_profile(PROFILE_LEAUDIO_VMICPC);
}

void *bt_lea_vmicpc_register_callbacks(bt_instance_t *ins, const lea_vmicpc_callbacks_t *callbacks)
{
    lea_vmicpc_interface_t *profile = get_profile_service();

    return profile->register_callbacks(NULL, (lea_vmicpc_callbacks_t *)callbacks);
}

bool bt_lea_vmicpc_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    lea_vmicpc_interface_t *profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_lea_vmicpc_get_volume_state(bt_instance_t *ins, bt_address_t *addr)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->vol_get(addr);
}

bt_status_t bt_lea_vmicpc_get_volume_flags(bt_instance_t *ins, bt_address_t *addr)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->flags_get(addr);
}

bt_status_t bt_lea_vmicpc_change_volume(bt_instance_t *ins, bt_address_t *addr, int dir)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->vol_change(addr, dir);
}

bt_status_t bt_lea_vmicpc_change_unmute_volume(bt_instance_t *ins, bt_address_t *addr, int dir)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->vol_unmute_change(addr, dir);
}

bt_status_t bt_lea_vmicpc_set_volume(bt_instance_t *ins, bt_address_t *addr, int vol)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->vol_set(addr, vol);
}

bt_status_t bt_lea_vmicpc_set_volume_mute(bt_instance_t *ins, bt_address_t *addr, int mute)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->mute_state_set(addr, mute);
}

bt_status_t bt_lea_vmicpc_get_mic_state(bt_instance_t *ins, bt_address_t *addr)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->mic_mute_get(addr);
}

bt_status_t bt_lea_vmicpc_set_mic_mute(bt_instance_t *ins, bt_address_t *addr, int mute)
{
    lea_vmicpc_interface_t *profile = get_profile_service();
    return profile->mic_mute_set(addr, mute);
}
