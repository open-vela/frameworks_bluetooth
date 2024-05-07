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

#include <stdint.h>

#include "bt_lea_vmics.h"
#include "bt_profile.h"
#include "lea_vmics_service.h"
#include "service_manager.h"

static lea_vmics_interface_t* get_profile_service(void)
{
    return (lea_vmics_interface_t*)service_manager_get_profile(PROFILE_LEAUDIO_VMICS);
}

void* bt_lea_vmics_register_callbacks(bt_instance_t* ins, const lea_vmics_callbacks_t* callbacks)
{
    lea_vmics_interface_t* profile = get_profile_service();

    return profile->register_callbacks(NULL, (lea_vmics_callbacks_t*)callbacks);
}

bool bt_lea_vmics_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    lea_vmics_interface_t* profile = get_profile_service();

    return profile->unregister_callbacks(NULL, cookie);
}

bt_status_t bt_lea_vcs_volume_set(bt_instance_t* ins, int vol)
{
    lea_vmics_interface_t* profile = get_profile_service();
    return profile->vcs_volume_notify(ins, vol);
}

bt_status_t bt_lea_vcs_mute_set(bt_instance_t* ins, int mute)
{
    lea_vmics_interface_t* profile = get_profile_service();
    return profile->vcs_mute_notify(ins, mute);
}

bt_status_t bt_lea_vcs_volume_flags_set(bt_instance_t* ins, int flags)
{
    lea_vmics_interface_t* profile = get_profile_service();
    return profile->vcs_volume_flags_notify(ins, flags);
}

bt_status_t bt_lea_mics_mute_set(bt_instance_t* ins, int mute)
{
    lea_vmics_interface_t* profile = get_profile_service();
    return profile->mics_mute_notify(ins, mute);
}
