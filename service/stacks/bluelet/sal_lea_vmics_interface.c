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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_lea_gaf.h"
#include "stack_adapter_lea_vmicp.h"

#include "bluetooth.h"
#include "bt_lea_vmicps.h"
#include "lea_vmicps_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_vmicps_interface.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS

// leaudio vcs callbacks from barrot stack
static void adpt_lea_vcs_set_volume_state_callback(SERVICE_LEA_VCS_VOLUME_STATE_S *vol_state);
static void adpt_lea_vcs_set_volume_flags_callback(uint8_t vol_flags);
static void adpt_lea_mics_set_mute_callback(uint8_t mute);

const LEA_VCS_CALLBACK_S adpt_lea_vcs_server_callbacks = {
    .lea_vcs_set_volume_state_cb = adpt_lea_vcs_set_volume_state_callback,
    .lea_vcs_set_volume_flags_cb = adpt_lea_vcs_set_volume_flags_callback,
};

const LEA_MICS_CALLBACK_S adpt_lea_mics_server_callbacks = {
    .lea_mics_set_mute_cb = adpt_lea_mics_set_mute_callback,
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_vcs_set_volume_state_callback(SERVICE_LEA_VCS_VOLUME_STATE_S *vol_state)
{
    BT_LOGD("%s volume:%d, mute:%d", __func__, vol_state->volume, vol_state->mute);
    service_lea_vcs_volume_state_s volume_state;
    volume_state.volume = vol_state->volume;
    volume_state.mute = vol_state->mute;
    lea_vmicps_on_vcs_volume_state_changed(&volume_state);
}

static void adpt_lea_vcs_set_volume_flags_callback(uint8_t vol_flags)
{
    BT_LOGD("%s flags:%d", __func__, vol_flags);
    lea_vmicps_on_vcs_volume_flags_changed(vol_flags);
}

static void adpt_lea_mics_set_mute_callback(uint8_t mute)
{
    BT_LOGD("%s mute:%d", __func__, mute);
    lea_vmicps_on_mics_mute_state_changed(mute);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bool adpt_req_vcs_info_callback(SERVICE_LEA_VCS_INFO_S *info)
{
    info->step_size = CONFIG_BLUETOOTH_LEAUDIO_VCS_VOLUME_STEP;
    info->volume = CONFIG_BLUETOOTH_LEAUDIO_VCS_VOLUME_INITIAL;
    info->mute = CONFIG_BLUETOOTH_LEAUDIO_VCS_UNMUTED;
    info->vsps_flag = CONFIG_BLUETOOTH_LEAUDIO_VCS_VOLUME_DEFAULT_SETTING;
    info->vocs_number = CONFIG_BLUETOOTH_LEAUDIO_VOCS_NUMBER;
    info->aics_number = CONFIG_BLUETOOTH_LEAUDIO_AICS_NUMBER;
    info->vocs_list = NULL;
    info->aics_list = NULL;
    info->vocs_recycle_func = NULL;
    info->aics_recycle_func = NULL;
    return true;
}

bool adpt_req_mics_info_callback(SERVICE_LEA_MICS_INFO_S *info)
{
    info->mute = CONFIG_BLUETOOTH_LEAUDIO_VCS_UNMUTED;
    info->aics_number = CONFIG_BLUETOOTH_LEAUDIO_MICS_NUMBER;
    info->aics_list = NULL;
    info->recycle_func = NULL;
    return true;
}

bt_status_t bt_sal_vmicps_notify_vcs_volume(int volume)
{
    SAL_CHECK_RET(stack_adapter_lea_vcs_volume_changed(volume), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicps_notify_vcs_mute(int mute)
{
    SAL_CHECK_RET(stack_adapter_lea_vcs_mute_changed(mute), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicps_notify_vcs_volume_flags(int flags)
{
    SAL_CHECK_RET(stack_adapter_lea_vcs_volume_flags_changed(flags), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_vmicps_notify_mics_mute(int mute)
{
    SAL_CHECK_RET(stack_adapter_lea_mics_mute_changed(mute), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

#endif
