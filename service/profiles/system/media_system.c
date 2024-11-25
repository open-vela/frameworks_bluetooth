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
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "bt_status.h"
#include <media_api.h>

#ifdef CONFIG_MICO_MEDIA_MAIN_PLAYER
#include "audio_manager_c.h"
#endif /* CONFIG_MICO_MEDIA_MAIN_PLAYER */
#include "media_system.h"
#include "utils/log.h"

#define MEDIA_POLICY_APPLY 1

#define AVRCP_MAX_ABSOLUTE_VOLUME 0x7F
#define MAX_HFP_SCO_VOICE_CALL_VOLUME 15
#define MIN_HFP_SCO_VOICE_CALL_VOLUME 1
#define UI_MAX_VOLUME 100

static int g_media_max_volume;
static int g_vc_max_volume = 15; // TODO: read via media_policy_get_range()
static int g_vc_min_volume = 1; // TODO: read via media_policy_get_range()

typedef struct bt_media_listener {
    void* policy_handle;
    void* policy_cb;
    void* context;
} bt_media_listener_t;

#ifdef CONFIG_MICO_MEDIA_MAIN_PLAYER
static int media_volume_to_ui_volume(int volume)
{
    return (volume * UI_MAX_VOLUME) / g_media_max_volume;
}
#endif /* CONFIG_MICO_MEDIA_MAIN_PLAYER */

int bt_media_get_music_volume_range()
{
    int media_min_volume = 0; /* min volume of AVRCP must be 0. */
    int status;

    status = media_policy_get_range(MEDIA_SCENARIO_MUSIC MEDIA_POLICY_VOLUME, &media_min_volume, &g_media_max_volume);

    assert(!media_min_volume);
    return status;
}

int bt_media_volume_avrcp_to_media(uint8_t volume)
{
    if (volume >= AVRCP_MAX_ABSOLUTE_VOLUME) {
        return g_media_max_volume;
    }

    int media_volume = (volume * g_media_max_volume + (AVRCP_MAX_ABSOLUTE_VOLUME >> 1)) / AVRCP_MAX_ABSOLUTE_VOLUME;

    return media_volume;
}

uint8_t bt_media_volume_media_to_avrcp(int volume)
{
    if (volume <= 0) {
        return 0;
    }

    if (volume >= g_media_max_volume) {
        return AVRCP_MAX_ABSOLUTE_VOLUME;
    }

    int avrcp_volume = (volume * AVRCP_MAX_ABSOLUTE_VOLUME + (g_media_max_volume >> 1)) / g_media_max_volume;

    return avrcp_volume;
}

int bt_media_volume_hfp_to_media(uint8_t hfp_volume)
{
    int media_range, hfp_range, media_offset, media_volume;

    if (hfp_volume <= MIN_HFP_SCO_VOICE_CALL_VOLUME) {
        return g_vc_min_volume;
    }

    if (hfp_volume >= MAX_HFP_SCO_VOICE_CALL_VOLUME) {
        return g_vc_max_volume;
    }

    media_range = g_vc_max_volume - g_vc_min_volume;
    hfp_range = MAX_HFP_SCO_VOICE_CALL_VOLUME - MIN_HFP_SCO_VOICE_CALL_VOLUME;
    media_offset = (media_range * (hfp_volume - MIN_HFP_SCO_VOICE_CALL_VOLUME)) / hfp_range;
    media_volume = g_vc_min_volume + media_offset;

    return media_volume;
}

uint8_t bt_media_volume_media_to_hfp(int media_volume)
{
    int media_range, hfp_range, hfp_offset;
    uint8_t hfp_volume;

    if (media_volume <= g_vc_min_volume) {
        return MIN_HFP_SCO_VOICE_CALL_VOLUME;
    }

    if (media_volume >= g_vc_max_volume) {
        return MAX_HFP_SCO_VOICE_CALL_VOLUME;
    }

    media_range = (g_vc_max_volume > g_vc_min_volume) ? (g_vc_max_volume - g_vc_min_volume) : 1;
    hfp_range = MAX_HFP_SCO_VOICE_CALL_VOLUME - MIN_HFP_SCO_VOICE_CALL_VOLUME;
    hfp_offset = (hfp_range * (media_volume - g_vc_min_volume)) / media_range;
    hfp_volume = MIN_HFP_SCO_VOICE_CALL_VOLUME + hfp_offset;

    return hfp_volume;
}

void bt_media_remove_listener(void* handle)
{
    bt_media_listener_t* listener = (bt_media_listener_t*)handle;
    if (!listener)
        return;

    if (listener->policy_handle) {
        media_policy_unsubscribe(listener->policy_handle);
        listener->policy_handle = NULL;
    }

    free(listener);
}

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

static void bt_media_policy_volume_change_callback(void* cookie, int number, const char* literal)
{
    bt_media_listener_t* listener = cookie;
    if (listener && listener->policy_cb)
        ((bt_media_voice_volume_change_callback_t)(listener->policy_cb))(listener->context, number);
}

void* bt_media_listen_voice_call_volume_change(bt_media_voice_volume_change_callback_t cb, void* context)
{
    bt_media_listener_t* listener = malloc(sizeof(bt_media_listener_t));
    if (!listener)
        return NULL;

    listener->context = context;
    listener->policy_cb = cb;
    listener->policy_handle = media_policy_subscribe(MEDIA_SCENARIO_INCALL MEDIA_POLICY_VOLUME, bt_media_policy_volume_change_callback, listener);
    if (!listener->policy_handle) {
        BT_LOGI("media policy subscribe(%s-%s) failed!", MEDIA_SCENARIO_INCALL, MEDIA_POLICY_VOLUME);
        free(listener);
        listener = NULL;
    }

    return listener;
}

bt_status_t bt_media_get_voice_call_volume(int* volume)
{
    if (media_policy_get_stream_volume(MEDIA_SCENARIO_INCALL, volume) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_voice_call_volume(int volume)
{
    if (media_policy_set_stream_volume(MEDIA_SCENARIO_INCALL, volume) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_music_volume(int volume)
{
    bt_status_t status;

    status = media_policy_set_stream_volume(MEDIA_STREAM_MUSIC, volume);

    if (status) {
        BT_LOGE("set music stream volume fail: %d, status: %d", volume, status);
        return status;
    }

#ifdef CONFIG_MICO_MEDIA_MAIN_PLAYER
    void* mAm = am_get_audio_manager("UIVOLUME");

    if (mAm == 0) {
        BT_LOGE("am_get_audio_manager err");
        return BT_STATUS_NO_RESOURCES;
    }

    status = am_set_volume(mAm, AM_STREAM_TYPE_MEDIA, media_volume_to_ui_volume(volume));

    if (status != 0) {
        BT_LOGE("am_set_volume err, status: %d", status);
    }

    if ((status = am_audio_manager_release(mAm)) != 0) {
        BT_LOGE("am_audio_manager_release err, status: %d", status);
    }
#endif /* CONFIG_MICO_MEDIA_MAIN_PLAYER */

    return status;
}

bt_status_t bt_media_get_music_volume(int* volume)
{
    if (media_policy_get_stream_volume(MEDIA_STREAM_MUSIC, volume) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

void* bt_media_listen_music_volume_change(bt_media_voice_volume_change_callback_t cb, void* context)
{
    bt_media_listener_t* listener;

    listener = malloc(sizeof(bt_media_listener_t));
    if (!listener)
        return NULL;

    listener->context = context;
    listener->policy_cb = cb;
    listener->policy_handle = media_policy_subscribe(MEDIA_SCENARIO_MUSIC MEDIA_POLICY_VOLUME, bt_media_policy_volume_change_callback, listener);

    return listener;
}

bt_status_t bt_media_set_sco_available(void)
{
    /* set SCO device available */
    if (media_policy_set_devices_available(MEDIA_DEVICE_SCO) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_sco_unavailable(void)
{
    if (media_policy_set_devices_unavailable(MEDIA_DEVICE_SCO) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_a2dp_offloading(bool enable)
{
    // todo set a2dp offload async

    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_media_set_hfp_offloading(bool enable)
{
    // todo set hfp offload?

    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_media_set_lea_available(void)
{
    int is_available = 0;

    /* check LEA device is available */
    if (media_policy_is_devices_available(MEDIA_DEVICE_BLE, &is_available) != 0)
        return BT_STATUS_FAIL;

    if (is_available) {
        BT_LOGI("lea device had set available !");
        return BT_STATUS_SUCCESS;
    }

    /* set LEA device available */
    if (media_policy_set_devices_available(MEDIA_DEVICE_BLE) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_lea_unavailable(void)
{
    int is_available = 0;

    /* check LEA device is unavailable */
    if (media_policy_is_devices_available(MEDIA_DEVICE_BLE, &is_available) != 0)
        return BT_STATUS_FAIL;

    if (!is_available) {
        BT_LOGI("a2dp device had set unavailable !");
        return BT_STATUS_SUCCESS;
    }

    /* set LEA device unavailable */
    if (media_policy_set_devices_unavailable(MEDIA_DEVICE_BLE) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_media_set_lea_offloading(bool enable)
{
    // todo set le audio offload?

    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_media_set_anc_enable(bool enable)
{
    if (media_policy_set_int(MEDIA_POLICY_ANC_OFFLOAD_MODE, (int)enable, MEDIA_POLICY_APPLY) != 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}
