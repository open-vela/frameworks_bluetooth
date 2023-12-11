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
#define LOG_TAG "bt_media"
#include <stdint.h>

#include "bt_status.h"
#include <media_api.h>

#include "media_system.h"
#include "utils/log.h"

bt_status_t bt_media_set_a2dp_available(void)
{
    int is_available = 0;

    /* check A2DP device is available */
    if (media_policy_is_devices_available(MEDIA_DEVICE_A2DP, &is_available) != 0)
        return BT_STATUS_FAIL;

    if (is_available) {
        BT_LOGI("a2dp device had set available !");
        return BT_STATUS_SUCCESS;
    }

    /* set A2DP device available */
    if (media_policy_set_devices_available(MEDIA_DEVICE_A2DP) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_a2dp_unavailable(void)
{
    int is_available = 0;

    /* check A2DP device is unavailable */
    if (media_policy_is_devices_available(MEDIA_DEVICE_A2DP, &is_available) != 0)
        return BT_STATUS_FAIL;

    if (!is_available) {
        BT_LOGI("a2dp device had set unavailable !");
        return BT_STATUS_SUCCESS;
    }

    /* set A2DP device unavailable */
    if (media_policy_set_devices_unavailable(MEDIA_DEVICE_A2DP) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_hfp_samplerate(uint16_t samplerate)
{
    if (samplerate != 8000 && samplerate != 16000)
        return BT_STATUS_PARM_INVALID;

    /* set hfp samplerate, dev/pcm1c/p device ioctl */
    if (media_policy_set_hfp_samplerate(samplerate) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_listen_voice_call_volume_change(void)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_voice_call_volume(uint16_t volume)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_sco_available(void)
{
    /* set SCO device available */
    if (media_policy_set_devices_available(MEDIA_DEVICE_SCO) != 0)
        return BT_STATUS_FAIL;

    /* set audio mode as phone call mode */
    if (media_policy_set_audio_mode(MEDIA_AUDIO_MODE_PHONE) != 0)
        return BT_STATUS_FAIL;

    /* set SCO device in using state */
    if (media_policy_set_devices_use(MEDIA_DEVICE_SCO) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_sco_unavailable(void)
{
    int res = 0;

    /* check SCO device is in using state */
    if (media_policy_is_devices_use(MEDIA_DEVICE_SCO, &res) != 0)
        return BT_STATUS_FAIL;

    if (res) {
        /* if SCO in using, set SCO unuse */
        if (media_policy_set_devices_unuse(MEDIA_DEVICE_SCO) != 0)
            return BT_STATUS_FAIL;
    }

    /* switch audio mode to NORMAL mode */
    if (media_policy_set_audio_mode(MEDIA_AUDIO_MODE_NORMAL) != 0)
        return BT_STATUS_FAIL;

    /* set SCO device unavailable */
    if (media_policy_set_devices_unavailable(MEDIA_DEVICE_SCO) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}
