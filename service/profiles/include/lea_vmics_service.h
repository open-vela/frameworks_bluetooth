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
#ifndef __LEA_VMICS_SERVICE_H__
#define __LEA_VMICS_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_lea_vmics.h"
#include "sal_lea_vmics_interface.h"

void lea_vmics_on_vcs_volume_state_changed(service_lea_vcs_volume_state_s* vol_state);
void lea_vmics_on_vcs_volume_flags_changed(uint8_t flags);
void lea_vmics_on_mics_mute_state_changed(uint8_t mute);

typedef struct {
    size_t size;
    bt_status_t (*vcs_volume_notify)(void* handle, int vol);
    bt_status_t (*vcs_mute_notify)(void* handle, int mute);
    bt_status_t (*vcs_volume_flags_notify)(void* handle, int flags);
    bt_status_t (*mics_mute_notify)(void* handle, int mute);
    void* (*register_callbacks)(void* handle, lea_vmics_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** handle, void* cookie);
} lea_vmics_interface_t;

/*
 * register profile to service manager
 */
void register_lea_vmics_service(void);

#endif /* __LEA_VMICS_SERVICE_H__ */
