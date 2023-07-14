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
#define LOG_TAG "lea_vmicps_service"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <string.h>
#include <sys/types.h>


#include "bt_lea_vmicps.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_vmicps_event.h"
#include "lea_vmicps_media_control.h"
#include "lea_vmicps_service.h"
#include "sal_lea_vmicps_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#include "media_session.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS
#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_vmicps_service.started)    \
            return BT_STATUS_NOT_ENABLED; \
    }

#define VMICPS_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_vmicps_callbacks_t, _cback, ##__VA_ARGS__)
#define LEA_VMICPS_MEDIA_SESSION_NAME "Music"

typedef struct
{
    bool started;
    callbacks_list_t *callbacks;
    pthread_mutex_t vmicps_lock;
    void *volume_session;
} vmicps_service_t;

static vmicps_service_t g_vmicps_service = {
    .started = false,
    .callbacks = NULL,

    .volume_session = NULL,
};

/****************************************************************************
 * messange handle
 ****************************************************************************/
static void lea_vmicps_process_message(void *data)
{
    lea_vmicps_msg_t *msg = (lea_vmicps_msg_t *)data;
    switch (msg->event) {
    case STACK_EVENT_VCS_VOLUME_STATE: {
        lea_vcs_vol_state_request(g_vmicps_service.volume_session,
                                  msg->data.vol_state.volume, msg->data.vol_state.mute);
        VMICPS_CALLBACK_FOREACH(g_vmicps_service.callbacks, test_cb, 0);
        break;
    }
    case STACK_EVENT_VCS_VOLUME_FLAGS: {
        lea_vcs_vol_flags_request(msg->data.vol_flags);
        VMICPS_CALLBACK_FOREACH(g_vmicps_service.callbacks, test_cb, 0);
        break;
    }
    case STACK_EVENT_MICS_MUTE_STATE: {
        lea_mics_mic_mute_request(msg->data.mic_mute_state);
        VMICPS_CALLBACK_FOREACH(g_vmicps_service.callbacks, test_cb, 0);
        break;
    }
    default: {
        BT_LOGE("Idle: Unexpected stack event");
        break;
    }
    }
    lea_vmicps_msg_destory(msg);
}

static bt_status_t lea_vmicps_send_msg(lea_vmicps_msg_t *msg)
{
    assert(msg);
    do_in_service_loop(lea_vmicps_process_message, msg);
    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_vmicps_on_vcs_volume_state_changed(service_lea_vcs_volume_state_s *vol_state)
{
    lea_vmicps_msg_t *msg = lea_vmicps_msg_new(STACK_EVENT_VCS_VOLUME_STATE);
    msg->data.vol_state.volume = vol_state->volume;
    msg->data.vol_state.mute = vol_state->mute;
    lea_vmicps_send_msg(msg);
}

void lea_vmicps_on_vcs_volume_flags_changed(uint8_t flags)
{
    lea_vmicps_msg_t *msg = lea_vmicps_msg_new(STACK_EVENT_VCS_VOLUME_FLAGS);
    msg->data.vol_flags = flags;
    lea_vmicps_send_msg(msg);
}

void lea_vmicps_on_mics_mute_state_changed(uint8_t mute)
{
    lea_vmicps_msg_t *msg = lea_vmicps_msg_new(STACK_EVENT_MICS_MUTE_STATE);
    msg->data.mic_mute_state = mute;
    lea_vmicps_send_msg(msg);
}

static bt_status_t lea_vmicps_vol_notify(void *handle, int vol)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicps_service.vmicps_lock);
    ret = bt_sal_vmicps_notify_vcs_volume(vol);
    pthread_mutex_unlock(&g_vmicps_service.vmicps_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_vcs_volume_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vmicps_mute_notify(void *handle, int mute)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicps_service.vmicps_lock);
    ret = bt_sal_vmicps_notify_vcs_mute(mute);
    pthread_mutex_unlock(&g_vmicps_service.vmicps_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_vcs_mute_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vmicps_vol_flags_notify(void *handle, int flags)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicps_service.vmicps_lock);
    ret = bt_sal_vmicps_notify_vcs_volume_flags(flags);
    pthread_mutex_unlock(&g_vmicps_service.vmicps_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_vcs_volume_flags_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vmicps_mic_mute_notify(void *handle, int mute)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicps_service.vmicps_lock);
    ret = bt_sal_vmicps_notify_mics_mute(mute);
    pthread_mutex_unlock(&g_vmicps_service.vmicps_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_mics_mute_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void *lea_vmicps_register_callbacks(void *handle, lea_vmicps_callbacks_t *callbacks)
{
    if (!g_vmicps_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_vmicps_service.callbacks, handle, (void *)callbacks);
}

static bool lea_vmicps_unregister_callbacks(void **handle, void *cookie)
{
    if (!g_vmicps_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_vmicps_service.callbacks, handle, cookie);
}

static const lea_vmicps_interface_t leaVmicpsInterface = {
    .size = sizeof(leaVmicpsInterface),
    .vcs_volume_notify = lea_vmicps_vol_notify,
    .vcs_mute_notify = lea_vmicps_mute_notify,
    .vcs_volume_flags_notify = lea_vmicps_vol_flags_notify,
    .mics_mute_notify = lea_vmicps_mic_mute_notify,
    .register_callbacks = lea_vmicps_register_callbacks,
    .unregister_callbacks = lea_vmicps_unregister_callbacks,
};

static const void *get_lea_vmicps_profile_interface(void)
{
    return &leaVmicpsInterface;
}

/****************************************************************************
 * Public function
 ****************************************************************************/
static bt_status_t lea_vmicps_init(void)
{
    BT_LOGD("%s", __func__);
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vmicps_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    pthread_mutexattr_t attr;
    vmicps_service_t *service = &g_vmicps_service;

    BT_LOGD("%s", __func__);
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->callbacks = bt_callbacks_list_new(2);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->vmicps_lock, &attr);

    service->started = true;

    service->volume_session = media_session_open(LEA_VMICPS_MEDIA_SESSION_NAME);
    return BT_STATUS_SUCCESS;

fail:
    bt_callbacks_list_free(service->callbacks);
    pthread_mutex_destroy(&service->vmicps_lock);
    return status;
}

static bt_status_t lea_vmicps_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);

    pthread_mutex_lock(&g_vmicps_service.vmicps_lock);
    g_vmicps_service.started = false;

    bt_callbacks_list_free(g_vmicps_service.callbacks);
    g_vmicps_service.callbacks = NULL;
    pthread_mutex_unlock(&g_vmicps_service.vmicps_lock);
    pthread_mutex_destroy(&g_vmicps_service.vmicps_lock);

    media_session_close(g_vmicps_service.volume_session);
    return BT_STATUS_SUCCESS;
}

static void lea_vmicps_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_vmicps_dump(void)
{
    printf("impl leaudio vmicps dump");
    return 0;
}

static const profile_service_t lea_vmicps_service = {
    .auto_start = true,
    .name = PROFILE_VMICPS_NAME,
    .id = PROFILE_LEAUDIO_VMICPS,
    .transport = BT_TRANSPORT_BLE,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = lea_vmicps_init,
    .startup = lea_vmicps_startup,
    .shutdown = lea_vmicps_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_vmicps_profile_interface,
    .cleanup = lea_vmicps_cleanup,
    .dump = lea_vmicps_dump,
};

void register_lea_vmicps_service(void)
{
    register_service(&lea_vmicps_service);
}

#endif
