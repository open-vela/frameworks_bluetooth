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
#define LOG_TAG "avrcp_target"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/list.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "callbacks_list.h"
#include "sal_avrcp_target_interface.h"
#include "service_loop.h"
#include "service_manager.h"

#include "avrcp_target_service.h"

#include "utils/log.h"

#define AVRCP_TARGET_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, avrcp_target_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node list;
    bool enable;
    pthread_mutex_t mutex;
    callbacks_list_t *callbacks;
} avrcp_target_servie_t;

static avrcp_target_servie_t g_avrc_tg_service = { 0 };

static void target_startup(profile_on_startup_t startup);
static void target_cleanup(profile_on_shutdown_t shutdown);

static void handle_avrcp_target_connection_state(avrcp_msg_t *msg)
{
    bt_address_t *addr = &msg->addr;
    profile_connection_state_t state = msg->data.conn_state;
    BT_LOGD("%s, device: %s, connection state : %d", __func__, bt_addr_str(addr), state);

    if (PROFILE_STATE_CONNECTED == state) {
        AVRCP_TARGET_CALLBACK_FOREACH(g_avrc_tg_service.callbacks,
                                      connection_state_cb, &msg->addr, msg->data.conn_state);
    }
}

static void handle_avrcp_passthrough_cmd(bt_address_t *addr,
                                         avrcp_passthr_cmd_t op,
                                         avrcp_key_state_t state)
{
    if (op == PASSTHROUGH_CMD_ID_STOP
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        && !a2dp_source_stream_started()
#endif
    ) {
        BT_LOGW("%s Stream suspended, Ignore STOP cmd", __func__);
        return;
    }
}

static avrcp_play_status_t current_playback_status(void)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (a2dp_source_stream_started())
        return PLAY_STATUS_PLAYING;
    else if (a2dp_source_stream_ready())
        return PLAY_STATUS_PAUSED;
    else
        return PLAY_STATUS_STOPPED;
#else
    return PLAY_STATUS_ERROR;
#endif
}

static void handle_avrcp_play_status_request(avrcp_msg_t *msg)
{
    AVRCP_TARGET_CALLBACK_FOREACH(g_avrc_tg_service.callbacks, get_play_status_cb, &msg->addr);
    bt_sal_avrcp_target_get_play_status_rsp(&msg->addr, current_playback_status(), 0, 0);
}

static void handle_avrcp_register_notification(avrcp_msg_t *msg)
{
    bt_address_t *addr = &msg->addr;
    avrcp_notification_event_t event = msg->data.notify_req.event;

    switch (event) {
    case NOTIFICATION_EVT_PALY_STATUS_CHANGED: {
        AVRCP_TARGET_CALLBACK_FOREACH(g_avrc_tg_service.callbacks, playback_register_notification_cb, &msg->addr);
        bt_sal_avrcp_target_play_status_notify(addr, current_playback_status());
        break;
    }
    case NOTIFICATION_EVT_TRACK_CHANGED:
        bt_sal_avrcp_target_notify_track_changed(addr, false);
        break;
    case NOTIFICATION_EVT_PLAY_POS_CHANGED:
        bt_sal_avrcp_target_notify_play_position_changed(addr, 0);
        break;
    default:
        break;
    }
}

static void avrcp_target_service_handle_callback(void *data)
{
    avrcp_msg_t *msg = data;

    switch (msg->id) {
    case AVRC_CONNECTION_STATE_CHANGED:
        handle_avrcp_target_connection_state(msg);
        break;
    case AVRC_PASSTHROUHT_CMD:
        handle_avrcp_passthrough_cmd(&msg->addr, msg->data.passthr_cmd.opcode, msg->data.passthr_cmd.state);
        break;
    case AVRC_REGISTER_NOTIFICATION_REQ:
        handle_avrcp_register_notification(msg);
        break;
    case AVRC_GET_PLAY_STATUS_REQ:
        handle_avrcp_play_status_request(msg);
        break;
    default:
        BT_LOGW("%s Unsupport message", __func__);
        break;
    }

    avrcp_msg_destory(msg);
}

static void avrcp_target_service_handle_event(void *data)
{
    avrcp_msg_t *msg = (avrcp_msg_t *)data;

    switch (msg->id) {
    case AVRC_STARTUP:
        target_startup((profile_on_startup_t)msg->data.context);
        break;
    case AVRC_SHUTDOWN:
        target_cleanup((profile_on_shutdown_t)msg->data.context);
        break;
    default:
        break;
    }

    avrcp_msg_destory(msg);
}

static void do_in_avrcp_service(avrcp_msg_t *msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_target_service_handle_event, msg);
}

static bt_status_t avrcp_target_init(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_avrc_tg_service.mutex, &attr) < 0)
        return BT_STATUS_FAIL;

    g_avrc_tg_service.callbacks = bt_callbacks_list_new(2);

    return BT_STATUS_SUCCESS;
}

static void avrcp_target_cleanup(void)
{
    bt_callbacks_list_free(g_avrc_tg_service.callbacks);
    g_avrc_tg_service.callbacks = NULL;
    pthread_mutex_destroy(&g_avrc_tg_service.mutex);
}

static void target_startup(profile_on_startup_t startup)
{
    pthread_mutex_lock(&g_avrc_tg_service.mutex);

    list_initialize(&g_avrc_tg_service.list);
    if (bt_sal_avrcp_target_init() != BT_STATUS_SUCCESS) {
        list_delete(&g_avrc_tg_service.list);
        startup(PROFILE_AVRCP_TG, false);
        pthread_mutex_unlock(&g_avrc_tg_service.mutex);
        return;
    }

    g_avrc_tg_service.enable = true;
    startup(PROFILE_AVRCP_TG, true);
    pthread_mutex_unlock(&g_avrc_tg_service.mutex);
}

static void target_cleanup(profile_on_shutdown_t shutdown)
{
    pthread_mutex_lock(&g_avrc_tg_service.mutex);
    if (!g_avrc_tg_service.enable) {
        pthread_mutex_unlock(&g_avrc_tg_service.mutex);
        shutdown(PROFILE_AVRCP_TG, true);
        return;
    }

    g_avrc_tg_service.enable = false;
    list_delete(&g_avrc_tg_service.list);
    bt_sal_avrcp_target_cleanup();
    shutdown(PROFILE_AVRCP_TG, true);
    pthread_mutex_unlock(&g_avrc_tg_service.mutex);
}

static bt_status_t avrcp_target_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_avrc_tg_service.mutex);
    if (g_avrc_tg_service.enable) {
        pthread_mutex_unlock(&g_avrc_tg_service.mutex);
        return BT_STATUS_NOT_ENABLED;
    }
    pthread_mutex_unlock(&g_avrc_tg_service.mutex);

    avrcp_msg_t *msg = avrcp_msg_new(AVRC_STARTUP, NULL);
    msg->data.context = cb;
    do_in_avrcp_service(msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_target_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_avrc_tg_service.mutex);
    if (!g_avrc_tg_service.enable) {
        pthread_mutex_unlock(&g_avrc_tg_service.mutex);
        return BT_STATUS_SUCCESS;
    }
    pthread_mutex_unlock(&g_avrc_tg_service.mutex);

    avrcp_msg_t *msg = avrcp_msg_new(AVRC_SHUTDOWN, NULL);
    msg->data.context = cb;
    do_in_avrcp_service(msg);

    return BT_STATUS_SUCCESS;
}

static void *avrcp_target_register_callbacks(void *remote, const avrcp_target_callbacks_t *callbacks)
{
    return bt_remote_callbacks_register(g_avrc_tg_service.callbacks, remote, (void *)callbacks);
}

static bool avrcp_target_unregister_callbacks(void **remote, void *cookie)
{
    return bt_remote_callbacks_unregister(g_avrc_tg_service.callbacks, remote, cookie);
}

static const avrcp_target_interface_t avrcp_targetInterface = {
    .size = sizeof(avrcp_targetInterface),
    .register_callbacks = avrcp_target_register_callbacks,
    .unregister_callbacks = avrcp_target_unregister_callbacks,
};

static const void *get_avrcp_target_profile_interface(void)
{
    return (void *)&avrcp_targetInterface;
}

static int avrcp_target_dump(void)
{
    return 0;
}

static int avrcp_target_get_state(void)
{
    return 1;
}

static const profile_service_t avrcp_target_service = {
    .auto_start = true,
    .name = PROFILE_AVRCP_TG_NAME,
    .id = PROFILE_AVRCP_TG,
    .transport = BT_TRANSPORT_BREDR,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = avrcp_target_init,
    .startup = avrcp_target_startup,
    .shutdown = avrcp_target_shutdown,
    .process_msg = NULL,
    .get_state = avrcp_target_get_state,
    .get_profile_interface = get_avrcp_target_profile_interface,
    .cleanup = avrcp_target_cleanup,
    .dump = avrcp_target_dump,
};

void bt_sal_avrcp_target_event_callback(avrcp_msg_t *msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_target_service_handle_callback, msg);
}

void register_avrcp_target_service(void)
{
    register_service(&avrcp_target_service);
}
