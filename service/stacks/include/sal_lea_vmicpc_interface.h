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
#ifndef __SAL_LEA_VMICPC_INTERFACE_H__
#define __SAL_LEA_VMICPC_INTERFACE_H__

#include "bt_status.h"
#include "stack_adapter_lea_vmicp.h"
#include <stdint.h>

bt_status_t bt_sal_vmicpc_read_volume_state(bt_address_t *addr);
bt_status_t bt_sal_vmicpc_read_volume_flags(bt_address_t *addr);
bt_status_t bt_sal_vmicpc_change_volume(bt_address_t *addr, int dir);
bt_status_t bt_sal_vmicpc_change_unmute_volume(bt_address_t *addr, int dir);
bt_status_t bt_sal_vmicpc_set_absolute_volume(bt_address_t *addr, int vol);
bt_status_t bt_sal_vmicpc_set_mute(bt_address_t *addr, int mute);
bt_status_t bt_sal_vmicpc_read_mic_state(bt_address_t *addr);
bt_status_t bt_sal_vmicpc_set_mic_state(bt_address_t *addr, int mute);

#endif /* __SAL_LEA_VMICPC_INTERFACE_H__ */