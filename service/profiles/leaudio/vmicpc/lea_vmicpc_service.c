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
#define LOG_TAG "lea_vmicpc_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_vmicpc.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_audio_common.h"
#include "lea_vmicpc_event.h"
#include "lea_vmicpc_service.h"
#include "sal_lea_vmicpc_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "tapi.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPC
/****************************************************************************
 * Private Data
 ****************************************************************************/

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_vmicpc_service.started)    \
            return BT_STATUS_NOT_ENABLED; \
    }

#define VMICPC_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_vmicpc_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct
{
    bool started;
    callbacks_list_t *callbacks;
    pthread_mutex_t vmicpc_lock;
} lea_vmicpc_service_t;

static lea_vmicpc_service_t g_vmicpc_service = {
    .started = false,
    .callbacks = NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void lea_vmicpc_process_message(void *data)
{
    lea_vmicpc_msg_t *msg = (lea_vmicpc_msg_t *)data;
    switch (msg->event) {
    case STACK_EVENT_VCC_VOLUME_STATE: {
        VMICPC_CALLBACK_FOREACH(g_vmicpc_service.callbacks, volume_state_cb, &msg->remote_addr,
                                msg->data.vol_state.volume, msg->data.vol_state.mute);
        break;
    }
    case STACK_EVENT_VCC_VOLUME_FLAGS: {
        VMICPC_CALLBACK_FOREACH(g_vmicpc_service.callbacks, volume_flags_cb, &msg->remote_addr, msg->data.vol_flags);
        break;
    }
    case STACK_EVENT_MICC_MUTE_STATE: {
        VMICPC_CALLBACK_FOREACH(g_vmicpc_service.callbacks, mic_state_cb, &msg->remote_addr, msg->data.mic_mute_state);
        break;
    }
    default: {
        BT_LOGE("Idle: Unexpected stack event");
        break;
    }
    }
    lea_vmicpc_msg_destory(msg);
}

static bt_status_t lea_vmicpc_send_msg(lea_vmicpc_msg_t *msg)
{
    assert(msg);
    do_in_service_loop(lea_vmicpc_process_message, msg);
    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_vmicpc_on_volume_state_changed(bt_address_t *addr, uint8_t volume, uint8_t mute)
{
    lea_vmicpc_msg_t *msg = lea_vmicpc_msg_new(STACK_EVENT_VCC_VOLUME_STATE, addr);
    msg->data.vol_state.volume = volume;
    msg->data.vol_state.mute = mute;
    lea_vmicpc_send_msg(msg);
}
void lea_vmicpc_on_volume_flags_changed(bt_address_t *addr, uint8_t flags)
{
    lea_vmicpc_msg_t *msg = lea_vmicpc_msg_new(STACK_EVENT_VCC_VOLUME_FLAGS, addr);
    msg->data.vol_flags = flags;
    lea_vmicpc_send_msg(msg);
}
void lea_vmicpc_on_mic_state_changed(bt_address_t *addr, uint8_t mute)
{
    lea_vmicpc_msg_t *msg = lea_vmicpc_msg_new(STACK_EVENT_MICC_MUTE_STATE, addr);
    msg->data.mic_mute_state = mute;
    lea_vmicpc_send_msg(msg);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bt_status_t lea_vcc_vol_get(bt_address_t *remote_addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_read_volume_state(remote_addr);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicpc_read_volume_state err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_flags_get(bt_address_t *remote_addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_read_volume_flags(remote_addr);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicpc_read_volume_flags err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_vol_change(bt_address_t *remote_addr, int dir)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_change_volume(remote_addr, dir);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicpc_change_volume err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_vol_unmute_change(bt_address_t *remote_addr, int dir)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_change_unmute_volume(remote_addr, dir);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicpc_change_unmute_volume err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_vol_set(bt_address_t *remote_addr, int vol)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_set_absolute_volume(remote_addr, vol);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_vcs_volume_flags_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vcc_mute_state_set(bt_address_t *remote_addr, int state)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_set_mute(remote_addr, state);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, lea_vcs_volume_flags_changed err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_micc_mute_state_get(bt_address_t *remote_addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_read_mic_state(remote_addr);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicpc_get_mic_state err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_micc_mute_state_set(bt_address_t *remote_addr, int state)
{
    CHECK_ENABLED();
    bt_status_t ret;
    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    ret = bt_sal_vmicpc_set_mic_state(remote_addr, state);
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("fail, bt_sal_vmicpc_set_mic_state err:%d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static void *lea_vmicpc_register_callbacks(void *handle, lea_vmicpc_callbacks_t *callbacks)
{
    if (!g_vmicpc_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_vmicpc_service.callbacks, handle, (void *)callbacks);
}

static bool lea_vmicpc_unregister_callbacks(void **handle, void *cookie)
{
    if (!g_vmicpc_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_vmicpc_service.callbacks, handle, cookie);
}

static const lea_vmicpc_interface_t leaVmicpcInterface = {
    .size = sizeof(leaVmicpcInterface),
    .vol_get = lea_vcc_vol_get,
    .flags_get = lea_vcc_flags_get,
    .vol_change = lea_vcc_vol_change,
    .vol_unmute_change = lea_vcc_vol_unmute_change,
    .vol_set = lea_vcc_vol_set,
    .mute_state_set = lea_vcc_mute_state_set,
    .mic_mute_get = lea_micc_mute_state_get,
    .mic_mute_set = lea_micc_mute_state_set,

    .register_callbacks = lea_vmicpc_register_callbacks,
    .unregister_callbacks = lea_vmicpc_unregister_callbacks,
};

/****************************************************************************
 * Public function
 ****************************************************************************/
static const void *get_lea_vmicpc_profile_interface(void)
{
    return &leaVmicpcInterface;
}

static bt_status_t lea_vmicpc_init(void)
{
    BT_LOGD("%s", __func__);
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_vmicpc_startup(profile_on_startup_t cb)
{
    BT_LOGD("%s", __func__);
    bt_status_t status;
    pthread_mutexattr_t attr;
    lea_vmicpc_service_t *service = &g_vmicpc_service;
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->callbacks = bt_callbacks_list_new(3);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->vmicpc_lock, &attr);
    service->started = true;

    return BT_STATUS_SUCCESS;

fail:
    bt_callbacks_list_free(service->callbacks);
    pthread_mutex_destroy(&service->vmicpc_lock);
    return status;
}

static bt_status_t lea_vmicpc_shutdown(profile_on_shutdown_t cb)
{
    if (!g_vmicpc_service.started)
        return BT_STATUS_SUCCESS;

    pthread_mutex_lock(&g_vmicpc_service.vmicpc_lock);
    g_vmicpc_service.started = false;

    bt_callbacks_list_free(g_vmicpc_service.callbacks);
    g_vmicpc_service.callbacks = NULL;
    pthread_mutex_unlock(&g_vmicpc_service.vmicpc_lock);
    pthread_mutex_destroy(&g_vmicpc_service.vmicpc_lock);
    return BT_STATUS_SUCCESS;
}

static void lea_vmicpc_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_vmicpc_dump(void)
{
    printf("impl leaudio tbs dump");
    return 0;
}

static const profile_service_t lea_vmicpc_service = {
    .auto_start = true,
    .name = PROFILE_VMICPC_NAME,
    .id = PROFILE_LEAUDIO_VMICPC,
    .transport = BT_TRANSPORT_BLE,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = lea_vmicpc_init,
    .startup = lea_vmicpc_startup,
    .shutdown = lea_vmicpc_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_vmicpc_profile_interface,
    .cleanup = lea_vmicpc_cleanup,
    .dump = lea_vmicpc_dump,
};

void register_lea_vmicpc_service(void)
{
    register_service(&lea_vmicpc_service);
}

#endif