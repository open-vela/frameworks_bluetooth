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
#define LOG_TAG "avrcp_controller"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/list.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "callbacks_list.h"
#include "sal_avrcp_control_interface.h"
#include "service_loop.h"
#include "service_manager.h"

#include "avrcp_control_service.h"

#include "utils/log.h"

#define AVRCP_CONTROL_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, avrcp_control_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node list;
    bool enable;
    pthread_mutex_t mutex;
    callbacks_list_t *callbacks;
} avrcp_controller_service_t;

static void controller_startup(profile_on_startup_t startup);
static void controller_shutdown(profile_on_shutdown_t shutdown);

static avrcp_controller_service_t g_avrc_controller = { 0 };

static void avrcp_controller_service_handle_event(void *data)
{
    avrcp_msg_t *msg = data;

    switch (msg->id) {
    case AVRC_STARTUP:
        controller_startup((profile_on_startup_t)msg->data.context);
        break;
    case AVRC_SHUTDOWN:
        controller_shutdown((profile_on_shutdown_t)msg->data.context);
        break;
    default:
        BT_LOGW("%s Unsupport message", __func__);
        break;
    }

    avrcp_msg_destory(msg);
}

static void avrcp_control_service_handle_callback(void *data)
{
    avrcp_msg_t *msg = data;

    switch (msg->id) {
    default:
        BT_LOGW("%s Unsupport message", __func__);
        break;
    }

    avrcp_msg_destory(msg);
}

static void do_in_avrcp_service(avrcp_msg_t *msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_controller_service_handle_event, msg);
}

static bt_status_t avrcp_control_init(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_avrc_controller.mutex, &attr) < 0)
        return BT_STATUS_FAIL;

    g_avrc_controller.callbacks = bt_callbacks_list_new(2);

    return BT_STATUS_SUCCESS;
}

static void avrcp_control_cleanup(void)
{
    bt_callbacks_list_free(g_avrc_controller.callbacks);
    g_avrc_controller.callbacks = NULL;
    pthread_mutex_destroy(&g_avrc_controller.mutex);
}

static void controller_startup(profile_on_startup_t startup)
{
    pthread_mutex_lock(&g_avrc_controller.mutex);
    if (g_avrc_controller.enable) {
        startup(PROFILE_AVRCP_CT, true);
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        return;
    }

    list_initialize(&g_avrc_controller.list);
    if (bt_sal_avrcp_control_init() != BT_STATUS_SUCCESS) {
        list_delete(&g_avrc_controller.list);
        startup(PROFILE_AVRCP_CT, false);
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        return;
    }

    g_avrc_controller.enable = true;
    startup(PROFILE_AVRCP_CT, true);
    pthread_mutex_unlock(&g_avrc_controller.mutex);
}

static void controller_shutdown(profile_on_shutdown_t shutdown)
{
    pthread_mutex_lock(&g_avrc_controller.mutex);
    if (!g_avrc_controller.enable) {
        shutdown(PROFILE_AVRCP_CT, true);
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        return;
    }

    g_avrc_controller.enable = false;
    list_delete(&g_avrc_controller.list);
    bt_sal_avrcp_control_cleanup();
    shutdown(PROFILE_AVRCP_CT, true);
    pthread_mutex_unlock(&g_avrc_controller.mutex);
}

static bt_status_t avrcp_control_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_avrc_controller.mutex);
    if (g_avrc_controller.enable) {
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        return BT_STATUS_NOT_ENABLED;
    }
    pthread_mutex_unlock(&g_avrc_controller.mutex);
    avrcp_msg_t *msg = avrcp_msg_new(AVRC_STARTUP, NULL);
    msg->data.context = cb;
    do_in_avrcp_service(msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_control_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_avrc_controller.mutex);
    if (!g_avrc_controller.enable) {
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        return BT_STATUS_SUCCESS;
    }
    pthread_mutex_unlock(&g_avrc_controller.mutex);
    avrcp_msg_t *msg = avrcp_msg_new(AVRC_SHUTDOWN, NULL);
    msg->data.context = cb;
    do_in_avrcp_service(msg);

    return BT_STATUS_SUCCESS;
}

static void *avrcp_control_register_callbacks(void *remote, const avrcp_control_callbacks_t *callbacks)
{
    return bt_remote_callbacks_register(g_avrc_controller.callbacks, remote, (void *)callbacks);
}

static bool avrcp_control_unregister_callbacks(void **remote, void *cookie)
{
    return bt_remote_callbacks_unregister(g_avrc_controller.callbacks, remote, cookie);
}

static const avrcp_control_interface_t avrcp_controlInterface = {
    .size = sizeof(avrcp_controlInterface),
    .register_callbacks = avrcp_control_register_callbacks,
    .unregister_callbacks = avrcp_control_unregister_callbacks
};

static const void *get_avrcp_control_profile_interface(void)
{
    return (void *)&avrcp_controlInterface;
}

static int avrcp_control_dump(void)
{
    return 0;
}

static const profile_service_t avrcp_control_service = {
    .auto_start = true,
    .name = PROFILE_AVRCP_CT_NAME,
    .id = PROFILE_AVRCP_CT,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = avrcp_control_init,
    .startup = avrcp_control_startup,
    .shutdown = avrcp_control_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_avrcp_control_profile_interface,
    .cleanup = avrcp_control_cleanup,
    .dump = avrcp_control_dump,
};

void bt_sal_avrcp_control_event_callback(avrcp_msg_t *msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_control_service_handle_callback, msg);
}

void register_avrcp_control_service(void)
{
    register_service(&avrcp_control_service);
}
