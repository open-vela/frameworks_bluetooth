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
#include <stdlib.h>

#include "bt_status.h"
#include <media_api.h>

#include "media_system.h"
#include "utils/log.h"

#define MEDIA_POLICY_APPLY 1

typedef struct bt_media_listener {
    void *policy_handle;
    void *policy_cb;
    void *context;
} bt_media_listener_t;

void bt_media_remove_listener(void *handle)
{
    bt_media_listener_t *listener = (bt_media_listener_t *)handle;
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

static void bt_media_policy_volume_change_callback(void *cookie, int number, const char *literal)
{
    bt_media_listener_t *listener = cookie;
    if (listener && listener->policy_cb)
        ((bt_media_voice_volume_change_callback_t)(listener->policy_cb))(listener->context, number);
}

void *bt_media_listen_voice_call_volume_change(bt_media_voice_volume_change_callback_t cb, void *context)
{
    bt_media_listener_t *listener = malloc(sizeof(bt_media_listener_t));
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

bt_status_t bt_media_get_voice_call_volume(int *volume)
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
