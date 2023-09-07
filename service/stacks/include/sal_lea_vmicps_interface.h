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
} service_lea_vcs_volume_state_s;

bool adpt_req_vcs_info_callback(SERVICE_LEA_VCS_INFO_S *info);
bool adpt_req_mics_info_callback(SERVICE_LEA_MICS_INFO_S *info);

bt_status_t bt_sal_vmicps_notify_vcs_volume(int volume);
bt_status_t bt_sal_vmicps_notify_vcs_mute(int mute);
bt_status_t bt_sal_vmicps_notify_vcs_volume_flags(int flags);
bt_status_t bt_sal_vmicps_notify_mics_mute(int mute);

#endif /* __SAL_LEA_VMICPS_INTERFACE_H__ */