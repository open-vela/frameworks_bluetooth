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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bluetooth.h"
#include "lea_vmicpc_service.h"
#include "sal.h"

#include "sal_lea_vmicpc_interface.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPC
void adpt_lea_vcc_volume_state_cbk(BD_ADDR vcs_addr, SERVICE_LEA_VCS_VOLUME_STATE_S *vol_state)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, vcs_addr, BD_ADDR_SIZE);
    lea_vmicpc_on_volume_state_changed(&addr, vol_state->volume, vol_state->mute);
}
void adpt_lea_vcc_volume_flags_cbk(BD_ADDR vcs_addr, uint8_t vol_flags)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, vcs_addr, BD_ADDR_SIZE);
    lea_vmicpc_on_volume_flags_changed(&addr, vol_flags);
}

void adpt_lea_micc_mute_cbk(BD_ADDR mics_addr, uint8_t mute)
{
    bt_address_t addr = { 0 };

    memcpy(addr.addr, mics_addr, BD_ADDR_SIZE);
    lea_vmicpc_on_mic_state_changed(&addr, mute);
}

/****************************************************************************
 *
 ****************************************************************************/
bt_status_t bt_sal_vmicpc_read_volume_state(bt_address_t *addr)
{
    SAL_CHECK_RET(stack_adapter_lea_vcc_read_volume_state(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicpc_read_volume_flags(bt_address_t *addr)
{
    SAL_CHECK_RET(stack_adapter_lea_vcc_read_volume_flags(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicpc_change_volume(bt_address_t *addr, int dir)
{
    if (dir) {
        SAL_CHECK_RET(stack_adapter_lea_vcc_volume_up(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    } else {
        SAL_CHECK_RET(stack_adapter_lea_vcc_volume_down(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicpc_change_unmute_volume(bt_address_t *addr, int dir)
{
    if (dir) {
        SAL_CHECK_RET(stack_adapter_lea_vcc_unmute_volume_up(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    } else {
        SAL_CHECK_RET(stack_adapter_lea_vcc_unmute_volume_down(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicpc_set_absolute_volume(bt_address_t *addr, int vol)
{
    SAL_CHECK_RET(stack_adapter_lea_vcc_set_absolute_volume(addr->addr, vol), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicpc_set_mute(bt_address_t *addr, int mute)
{
    if (mute) {
        SAL_CHECK_RET(stack_adapter_lea_vcc_mute(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    } else {
        SAL_CHECK_RET(stack_adapter_lea_vcc_unmute(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    }
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicpc_read_mic_state(bt_address_t *addr)
{
    SAL_CHECK_RET(stack_adapter_lea_micc_read_mute(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicpc_set_mic_state(bt_address_t *addr, int mute)
{
    if (mute) {
        SAL_CHECK_RET(stack_adapter_lea_micc_set_muted(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    } else {
        SAL_CHECK_RET(stack_adapter_lea_micc_set_unmuted(addr->addr), SERVICE_BT_STATUS_SUCCESS);
    }
    return BT_STATUS_SUCCESS;
}

#endif
