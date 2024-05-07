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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "lea_vmicp_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_vmicp.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_audio_common.h"
#include "lea_vmicp_event.h"
#include "lea_vmicp_service.h"
#include "sal_lea_vmicp_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "tapi.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICP
/****************************************************************************
 * Private Data
 ****************************************************************************/

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_vmicp_service.started)     \
            return BT_STATUS_NOT_ENABLED; \
    }

#define VMICP_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_vmicp_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct
{
    bool started;
    callbacks_list_t* callbacks;
    pthread_mutex_t vmicp_lock;
} lea_vmicp_service_t;

static lea_vmicp_service_t g_vmicp_service = {
    .started = false,
    .callbacks = NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void lea_vmicp_process_message(void* data)
{
    lea_vmicp_msg_t* msg = (lea_vmicp_msg_t*)data;
    switch (msg->event) {
    case STACK_EVENT_VCC_VOLUME_STATE: {
        VMICP_CALLBACK_FOREACH(g_vmicp_service.callbacks, volume_state_cb, &msg->remote_addr,
            msg->data.vol_state.volume, msg->data.vol_state.mute);
        break;
    }
    case STACK_EVENT_VCC_VOLUME_FLAGS: {
        VMICP_CALLBACK_FOREACH(g_vmicp_service.callbacks, volume_flags_cb, &msg->remote_addr, msg->data.vol_flags);
        break;
    }
    case STACK_EVENT_MICC_MUTE_STATE: {
        VMICP_CALLBACK_FOREACH(g_vmicp_service.callbacks, mic_state_cb, &msg->remote_addr, msg->data.mic_mute_state);
        break;
    }
    default: {
        BT_LOGE("Idle: Unexpected stack event");
        break;
    }
    }
    lea_vmicp_msg_destory(msg);
}

static bt_status_t lea_vmicp_send_msg(lea_vmicp_msg_t* msg)
{
    assert(msg);
    do_in_service_loop(lea_vmicp_process_message, msg);
    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_vmicp_on_volume_state_changed(bt_address_t* addr, uint8_t volume, uint8_t mute)
{
    lea_vmicp_msg_t* msg = lea_vmicp_msg_new(STACK_EVENT_VCC_VOLUME_STATE, addr);
    msg->data.vol_state.volume = volume;
    msg->data.vol_state.mute = mute;
    lea_vmicp_send_msg(msg);
}
void lea_vmicp_on_volume_flags_changed(bt_address_t* addr, uint8_t flags)
{
    lea_vmicp_msg_t* msg = lea_vmicp_msg_new(STACK_EVENT_VCC_VOLUME_FLAGS, addr);
    msg->data.vol_flags = flags;
    lea_vmicp_send_msg(msg);
}
void lea_vmicp_on_mic_state_changed(bt_address_t* addr, uint8_t mute)
{
    lea_vmicp_msg_t* msg = lea_vmicp_msg_new(STACK_EVENT_MICC_MUTE_STATE, addr);
    msg->data.mic_mute_state = mute;
    lea_vmicp_send_msg(msg);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bt_status_t lea_vcc_vol_get(bt_address_t* remote_addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_read_volume_state(remote_addr);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicp_read_volume_state err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_flags_get(bt_address_t* remote_addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_read_volume_flags(remote_addr);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicp_read_volume_flags err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_vol_change(bt_address_t* remote_addr, int dir)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_change_volume(remote_addr, dir);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicp_change_volume err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_vol_unmute_change(bt_address_t* remote_addr, int dir)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_change_unmute_volume(remote_addr, dir);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicp_change_unmute_volume err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_vol_set(bt_address_t* remote_addr, int vol)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_set_absolute_volume(remote_addr, vol);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_vcs_volume_flags_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_mute_state_set(bt_address_t* remote_addr, int state)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_set_mute(remote_addr, state);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_vcs_volume_flags_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_micc_mute_state_get(bt_address_t* remote_addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_read_mic_state(remote_addr);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicp_get_mic_state err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_micc_mute_state_set(bt_address_t* remote_addr, int state)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    ret = bt_sal_vmicp_set_mic_state(remote_addr, state);
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicp_set_mic_state err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void* lea_vmicp_register_callbacks(void* handle, lea_vmicp_callbacks_t* callbacks)
{
    if (!g_vmicp_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_vmicp_service.callbacks, handle, (void*)callbacks);
}

static bool lea_vmicp_unregister_callbacks(void** handle, void* cookie)
{
    if (!g_vmicp_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_vmicp_service.callbacks, handle, cookie);
}

static const lea_vmicp_interface_t leaVmicpInterface = {
    .size = sizeof(leaVmicpInterface),
    .vol_get = lea_vcc_vol_get,
    .flags_get = lea_vcc_flags_get,
    .vol_change = lea_vcc_vol_change,
    .vol_unmute_change = lea_vcc_vol_unmute_change,
    .vol_set = lea_vcc_vol_set,
    .mute_state_set = lea_vcc_mute_state_set,
    .mic_mute_get = lea_micc_mute_state_get,
    .mic_mute_set = lea_micc_mute_state_set,

    .register_callbacks = lea_vmicp_register_callbacks,
    .unregister_callbacks = lea_vmicp_unregister_callbacks,
};

/****************************************************************************
 * Public function
 ****************************************************************************/
static const void* get_lea_vmicp_profile_interface(void)
{
    return &leaVmicpInterface;
}

static bt_status_t lea_vmicp_init(void)
{
    BT_LOGD("%s", __func__);
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vmicp_startup(profile_on_startup_t cb)
{
    BT_LOGD("%s", __func__);
    bt_status_t status;
    pthread_mutexattr_t attr;
    lea_vmicp_service_t* service = &g_vmicp_service;
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->callbacks = bt_callbacks_list_new(3);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->vmicp_lock, &attr);
    service->started = true;

    return BT_STATUS_SUCCESS;

fail:
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    pthread_mutex_destroy(&service->vmicp_lock);
    return status;
}

static bt_status_t lea_vmicp_shutdown(profile_on_shutdown_t cb)
{
    if (!g_vmicp_service.started)
        return BT_STATUS_SUCCESS;

    pthread_mutex_lock(&g_vmicp_service.vmicp_lock);
    g_vmicp_service.started = false;

    bt_callbacks_list_free(g_vmicp_service.callbacks);
    g_vmicp_service.callbacks = NULL;
    pthread_mutex_unlock(&g_vmicp_service.vmicp_lock);
    pthread_mutex_destroy(&g_vmicp_service.vmicp_lock);
    return BT_STATUS_SUCCESS;
}

static void lea_vmicp_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_vmicp_dump(void)
{
    printf("impl leaudio tbs dump");
    return 0;
}

static const profile_service_t lea_vmicp_service = {
    .auto_start = true,
    .name = PROFILE_VMICP_NAME,
    .id = PROFILE_LEAUDIO_VMICP,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = lea_vmicp_init,
    .startup = lea_vmicp_startup,
    .shutdown = lea_vmicp_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_vmicp_profile_interface,
    .cleanup = lea_vmicp_cleanup,
    .dump = lea_vmicp_dump,
};

void register_lea_vmicp_service(void)
{
    register_service(&lea_vmicp_service);
}

#endif