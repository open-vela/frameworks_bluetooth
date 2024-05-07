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

#define LOG_TAG "bts_lea_vmics_media"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>

#include "lea_vmics_media_control.h"
#include "media_api.h"
#include "media_session.h"
#include "utils.h"
#include "utils/log.h"

/****************************************************************************
 * Public function
 ****************************************************************************/

uint8_t btvol_convert2_mediavol(uint8_t vol)
{
    uint8_t volume;
    if (vol >= 225) {
        volume = 15;
    } else if (vol == 0) {
        volume = 0;
    } else {
        volume = vol / 17 + 1;
    }
    return volume;
}

uint8_t mediavol_convert2_btvol(uint8_t vol)
{
    uint8_t volume;
    if (vol >= 15) {
        volume = 255;
    } else if (vol == 0) {
        volume = 0;
    } else {
        volume = vol * 17;
    }
    return volume;
}

// server interface
void lea_vcs_vol_state_request(void* volume_session, uint8_t volume, uint8_t mute)
{
    BT_LOGD("%s, volume:%d, mute:%d", __func__, volume, mute);
    uint8_t vol = btvol_convert2_mediavol(volume);
    media_session_set_volume(volume_session, vol);
    media_policy_set_mute_mode(mute);
}
void lea_vcs_vol_flags_request(uint8_t flags)
{
    BT_LOGD("%s,flags:%d", __func__, flags);
}

void lea_mics_mic_mute_request(uint8_t mute)
{
    BT_LOGD("%s,mute:%d", __func__, mute);
    if (!mute) {
        media_policy_set_devices_use(MEDIA_DEVICE_MIC);
    } else {
        media_policy_set_devices_unuse(MEDIA_DEVICE_MIC);
    }
}

uint8_t lea_vcs_get_volume(void* volume_session)
{
    int volume = 0;
    media_session_get_volume(volume_session, &volume);
    BT_LOGD("%s, volume:%d", __func__, volume);
    return mediavol_convert2_btvol(volume);
}

uint8_t lea_vcs_get_mute(void)
{
    int mute = 0;
    media_policy_get_mute_mode(&mute);
    BT_LOGD("%s, mute:%d", __func__, mute);
    return mute;
}
