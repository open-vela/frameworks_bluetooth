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

#ifndef __SAL_LEA_VMICPS_INTERFACE_H__
#define __SAL_LEA_VMICPS_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_lea_vmicps.h"
#include "bt_status.h"
#include "stack_adapter_common.h"
#include "stack_adapter_lea_vmicp.h"

typedef struct {
    uint8_t volume;
    uint8_t mute;
    uint8_t change_counter;
} service_lea_vcs_volume_state_s;

bt_status_t bt_sal_vmicps_notify_vcs_volume(int volume);
bt_status_t bt_sal_vmicps_notify_vcs_mute(int mute);
bt_status_t bt_sal_vmicps_notify_vcs_volume_flags(int flags);
bt_status_t bt_sal_vmicps_notify_mics_mute(int mute);

// leaudio vcs callbacks from barrot stack
void adpt_lea_vcs_set_volume_state_callback(service_lea_vcs_volume_state_s *vol_state);
void adpt_lea_vcs_set_volume_flags_callback(uint8_t vol_flags);

// leaudio mics callbacks from barrot stack
void adpt_lea_mics_set_mute_callback(uint8_t mute);

#endif /* __SAL_LEA_VMICPS_INTERFACE_H__ */