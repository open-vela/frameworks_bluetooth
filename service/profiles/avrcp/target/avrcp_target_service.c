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

#include "adapter_internel.h"
#include "avrcp_msg.h"
#include "avrcp_target_service.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "bt_player.h"
#include "callbacks_list.h"
#include "power_manager.h"
#include "sal_avrcp_control_interface.h"
#include "sal_avrcp_target_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "time.h"
#include "utils/log.h"

#define AVCTP_RETRY_MAX 1

#define AVRCP_TG_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, avrcp_target_callbacks_t, _cback, ##__VA_ARGS__)

#define POS_NOT_SUPPORT 0xFFFFFFFF
#define VOL_NOT_SUPPORT -1
typedef struct {
    bool enable;
    bool support_absvol;
    uint32_t features;
    uint32_t capabilities;
    pthread_mutex_t mutex;
    bt_list_t* devices;
    bt_media_controller_t* controller;
    callbacks_list_t* callbacks;
} avrcp_target_servie_t;

typedef struct {
    bool initiator;
    bt_address_t addr;
    bool absvol_support;
    uint8_t retry_cnt;
    uint32_t interval;
    uint16_t registered_events;
    bt_media_status_t play_status;
    service_timer_t* pos_update;
    service_timer_t* retry_timer;
    profile_connection_state_t state;
    bt_media_controller_t* controller;
} avrcp_tg_device_t;

static avrcp_target_servie_t g_avrc_target = { 0 };

static void target_startup(profile_on_startup_t startup);
static void target_shutdown(profile_on_shutdown_t shutdown);

static bool tg_device_cmp(void* device, void* addr)
{
    return bt_addr_compare(&((avrcp_tg_device_t*)device)->addr, addr) == 0;
}

static avrcp_tg_device_t* tg_device_find(bt_address_t* addr)
{
    avrcp_tg_device_t* device;

    if (!addr)
        return NULL;

    pthread_mutex_lock(&g_avrc_target.mutex);
    if (!g_avrc_target.devices) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        return NULL;
    }

    device = bt_list_find(g_avrc_target.devices, tg_device_cmp, addr);
    pthread_mutex_unlock(&g_avrc_target.mutex);

    return device;
}

static avrcp_tg_device_t* tg_device_create(bt_address_t* addr, bool initiator)
{
    avrcp_tg_device_t* device;
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (!addr)
        return NULL;

    device = malloc(sizeof(avrcp_tg_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->initiator = initiator;
    device->play_status = BT_MEDIA_PLAY_STATUS_STOPPED;
    device->interval = 0;
    device->retry_cnt = 0;
    device->pos_update = NULL;
    device->retry_timer = NULL;
    device->state = PROFILE_STATE_DISCONNECTED;

    pthread_mutex_lock(&g_avrc_target.mutex);
    bt_list_add_tail(g_avrc_target.devices, device);
    pthread_mutex_unlock(&g_avrc_target.mutex);

    bt_addr_ba2str(addr, _addr_str);
    BT_LOGD("%s [%s] success", __func__, _addr_str);

    return device;
}

static void tg_device_destory(void* data)
{
    avrcp_tg_device_t* device = data;
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    assert(device);

    bt_addr_ba2str(&device->addr, _addr_str);
    BT_LOGD("%s [%s] success", __func__, _addr_str);

    if (device->pos_update)
        service_loop_cancel_timer(device->pos_update);

    if (device->retry_timer)
        service_loop_cancel_timer(device->retry_timer);

    if (device->state != PROFILE_STATE_DISCONNECTED)
        AVRCP_TG_CALLBACK_FOREACH(g_avrc_target.callbacks, connection_state_cb, &device->addr, PROFILE_STATE_DISCONNECTED);

    bt_pm_conn_close(PROFILE_AVRCP_TG, &device->addr);
    free(device);
}

static void tg_device_remove(avrcp_tg_device_t* device)
{
    pthread_mutex_lock(&g_avrc_target.mutex);
    bt_list_remove(g_avrc_target.devices, device);
    pthread_mutex_unlock(&g_avrc_target.mutex);
}
static avrcp_tg_device_t* get_active_device(void)
{
    bt_list_node_t* node;

    /* get a2dp active device */
    pthread_mutex_lock(&g_avrc_target.mutex);
    node = bt_list_head(g_avrc_target.devices);
    pthread_mutex_unlock(&g_avrc_target.mutex);

    if (node == NULL)
        return NULL;

    return bt_list_node(node);
}

static void media_player_notify_cb(bt_media_controller_t* controller, void* context,
    bt_media_event_t event, uint32_t value)
{
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    avrcp_tg_device_t* device = get_active_device();
    if (!device)
        return;

    bt_addr_ba2str(&device->addr, _addr_str);
    BT_LOGD("%s: device=[%s], evt=%s, value=%" PRIu32, __func__, _addr_str, bt_media_evt_str(event), value);
    switch (event) {
    case BT_MEDIA_EVT_PREPARED:
        break;
    case BT_MEDIA_EVT_PLAYSTATUS_CHANGED:
        BT_LOGD("send playstatus notification --> %s", bt_media_status_str(value));
        device->play_status = value;
        bt_sal_avrcp_target_play_status_notify(PRIMARY_ADAPTER, &device->addr, value);
        break;
    case BT_MEDIA_EVT_POSITION_CHANGED:
        BT_LOGD("send position notification --> position: %" PRIu32, value);
        bt_sal_avrcp_target_notify_play_position_changed(PRIMARY_ADAPTER, &device->addr, value);
        break;
    case BT_MEDIA_EVT_TRACK_CHANGED:
        break;
    default:
        break;
    }
}

static void tg_retry_callback(service_timer_t* timer, void* data)
{
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    avrcp_tg_device_t* device = (avrcp_tg_device_t*)data;

    if (!device)
        return;

    bt_addr_ba2str(&device->addr, _addr_str);
    BT_LOGD("%s: device=[%s], state=%d, retry_cnt=%d", __func__, _addr_str, device->state, device->retry_cnt);
    if (device->state == PROFILE_STATE_DISCONNECTED)
        bt_sal_avrcp_control_connect(PRIMARY_ADAPTER, &device->addr);

    device->retry_timer = NULL;
}

static void handle_avrcp_connection_state(avrcp_msg_t* msg)
{
    avrcp_tg_device_t* device = NULL;
    bt_address_t* addr = &msg->addr;
    profile_connection_state_t state = msg->data.conn_state.conn_state;
    profile_connection_reason_t reason = msg->data.conn_state.reason;
    uint32_t random_timeout;

    pthread_mutex_lock(&g_avrc_target.mutex);
    if (!g_avrc_target.enable) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        return;
    }
    pthread_mutex_unlock(&g_avrc_target.mutex);

    BT_LOGD("avrc tg connnection --> device:[%s], state: %d", bt_addr_str(addr), state);

    device = tg_device_find(addr);

    switch (state) {
    case PROFILE_STATE_DISCONNECTED:
        assert(device);
        if ((device->state == PROFILE_STATE_CONNECTING) && (reason == PROFILE_REASON_COLLISION) && (device->retry_cnt < AVCTP_RETRY_MAX)) {
            /* failed to establish an AVRCP connection, retry for up to AVCTP_RETRY_MAX times */
            if (device->retry_timer == NULL) {
                /* AVRCP requires a random waiting time between 100ms and 1 seconds.
                   To compensate for transmission delays, the random delay is set to no more than 900ms */
                srand(time(NULL)); /* set random seed */
                random_timeout = 100 + (rand() % 800);
                BT_LOGD("retry AVRCP connection with device:[%s], delay=%" PRIu32 "ms",
                    bt_addr_str(addr), random_timeout);
                device->retry_timer = service_loop_timer(random_timeout, 0, tg_retry_callback, device);
                device->retry_cnt++;
            }
            break;
        }
        if (device->retry_timer) {
            service_loop_cancel_timer(device->retry_timer);
            device->retry_timer = NULL;
        }
        device->retry_cnt = 0;
        bt_pm_conn_close(PROFILE_AVRCP_TG, &device->addr);

        /* destory device and release resource if device is existed*/
        tg_device_remove(device);
        device = NULL;

        break;
    case PROFILE_STATE_CONNECTING:
        if (!device) {
            /* target as acceptor */
            device = tg_device_create(addr, false);
        }
        break;
    case PROFILE_STATE_CONNECTED:
        if (!device) {
            /* target as acceptor */
            device = tg_device_create(addr, false);
        }

        bt_pm_conn_open(PROFILE_AVRCP_TG, &device->addr);
        if (!g_avrc_target.controller) {
            g_avrc_target.controller = bt_media_controller_create(device, media_player_notify_cb);
            assert(g_avrc_target.controller);
        }

        device->controller = g_avrc_target.controller;
        device->retry_cnt = 0;
        break;
    case PROFILE_STATE_DISCONNECTING:
        assert(device);
        break;
    default:
        assert(0);
    }

    /* set device state */
    if (device)
        device->state = state;

    AVRCP_TG_CALLBACK_FOREACH(g_avrc_target.callbacks, connection_state_cb, addr, state);
}

static void handle_avrcp_passthrough_cmd(bt_address_t* addr,
    avrcp_passthr_cmd_t op,
    avrcp_key_state_t state)
{
    avrcp_tg_device_t* device = NULL;
    bt_status_t status = BT_STATUS_NOT_SUPPORTED;

    device = tg_device_find(addr);
    if (!device) {
        BT_LOGE("%s device not found", __func__);
        return;
    }

    BT_LOGD("passthrough cmd: %d, state: %d", op, state);
    AVRCP_TG_CALLBACK_FOREACH(g_avrc_target.callbacks, received_panel_operation_cb, addr, op, state);
    if (state != AVRCP_KEY_PRESSED)
        return;

    switch (op) {
    case PASSTHROUGH_CMD_ID_PLAY:
        status = bt_media_player_play(device->controller);
        break;
    case PASSTHROUGH_CMD_ID_STOP:
        status = bt_media_player_stop(device->controller);
        break;
    case PASSTHROUGH_CMD_ID_PAUSE:
        status = bt_media_player_pause(device->controller);
        break;
    case PASSTHROUGH_CMD_ID_FORWARD:
        status = bt_media_player_next(device->controller);
        break;
    case PASSTHROUGH_CMD_ID_BACKWARD:
        status = bt_media_player_prev(device->controller);
        break;
    default:
        break;
    }

    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, handle op:%d fail", __func__, op);
    }
}

static void handle_avrcp_play_status_request(avrcp_msg_t* msg)
{
    bt_address_t* addr = &msg->addr;
    avrcp_tg_device_t* device = NULL;
    bt_media_controller_t* controller = NULL;
    bt_media_status_t playback;
    uint32_t durations = POS_NOT_SUPPORT;
    uint32_t position = POS_NOT_SUPPORT;

    device = tg_device_find(addr);
    if (!device) {
        BT_LOGE("%s device not found", __func__);
        return;
    }

    BT_LOGD("handle get playback status request:");
    controller = device->controller;
    if (bt_media_player_get_playback_status(controller, &playback) != BT_STATUS_SUCCESS)
        playback = device->play_status;

    bt_media_player_get_position(controller, &position);
    bt_media_player_get_durations(controller, &durations);

    BT_LOGD("playback status: %s, duration: 0x%08" PRIx32 ", position: 0x%08" PRIx32, bt_media_status_str(playback), durations, position);
    bt_sal_avrcp_target_get_play_status_rsp(PRIMARY_ADAPTER, addr, playback, durations, position);

    AVRCP_TG_CALLBACK_FOREACH(g_avrc_target.callbacks, received_get_play_status_request_cb, addr);
}

static void handle_avrcp_register_notification(avrcp_msg_t* msg)
{
    bt_address_t* addr = &msg->addr;
    avrcp_tg_device_t* device = NULL;
    bt_media_controller_t* controller = NULL;
    avrcp_notification_event_t event = msg->data.notify_req.event;

    device = tg_device_find(addr);
    if (!device) {
        BT_LOGE("%s device not found", __func__);
        return;
    }

    controller = device->controller;
    device->registered_events |= (1 << event);
    BT_LOGD("register notification event: %d", event);
    switch (event) {
    case NOTIFICATION_EVT_PALY_STATUS_CHANGED: {
        bt_media_status_t playback;

        /* TODO:
            1. get mediaplayer playback status
            2. check A2DP stream state
            3. notify playback status
            4. listen mediaplayer status changed
        */

        if (bt_media_player_get_playback_status(controller, &playback) != BT_STATUS_SUCCESS)
            playback = device->play_status;

        BT_LOGD("send playstatus notification --> %s", bt_media_status_str(playback));
        bt_sal_avrcp_target_play_status_notify(PRIMARY_ADAPTER, addr, playback);
        break;
    }
    case NOTIFICATION_EVT_TRACK_CHANGED: {
        /*
         *   not support track changed notification
         */
        bt_sal_avrcp_target_notify_track_changed(PRIMARY_ADAPTER, addr, false);
        break;
    }
    case NOTIFICATION_EVT_PLAY_POS_CHANGED: {
        uint32_t position = POS_NOT_SUPPORT;

        device->interval = msg->data.notify_req.interval;
        bt_media_player_get_position(controller, &position);
        // if (position != POS_NOT_SUPPORT)
        //     device->pos_update = service_loop_timer();
        bt_sal_avrcp_target_notify_play_position_changed(PRIMARY_ADAPTER, addr, position);
        break;
    }
    case NOTIFICATION_EVT_VOLUME_CHANGED: {
        break;
    }
    default:
        break;
    }

    AVRCP_TG_CALLBACK_FOREACH(g_avrc_target.callbacks, received_register_notification_request_cb, addr, event, msg->data.notify_req.interval);
}

static void avrcp_target_service_handle_callback(void* data)
{
    avrcp_msg_t* msg = data;

    BT_LOGD("%s, %d", __func__, msg->id);
    switch (msg->id) {
    case AVRC_CONNECTION_STATE_CHANGED:
        handle_avrcp_connection_state(msg);
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

static void avrcp_target_service_handle_event(void* data)
{
    avrcp_msg_t* msg = (avrcp_msg_t*)data;

    switch (msg->id) {
    case AVRC_STARTUP:
        target_startup((profile_on_startup_t)msg->data.context);
        break;
    case AVRC_SHUTDOWN:
        target_shutdown((profile_on_shutdown_t)msg->data.context);
        break;
    default:
        break;
    }

    avrcp_msg_destory(msg);
}

static void do_in_avrcp_service(avrcp_msg_t* msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_target_service_handle_event, msg);
}

static bt_status_t avrcp_target_init(void)
{
    pthread_mutexattr_t attr;

    memset(&g_avrc_target, 0, sizeof(g_avrc_target));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_avrc_target.mutex, &attr) < 0)
        return BT_STATUS_FAIL;

    g_avrc_target.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    return BT_STATUS_SUCCESS;
}

static void avrcp_target_cleanup(void)
{
    bt_callbacks_list_free(g_avrc_target.callbacks);
    g_avrc_target.callbacks = NULL;
    pthread_mutex_destroy(&g_avrc_target.mutex);
}

static void target_startup(profile_on_startup_t startup)
{
    pthread_mutex_lock(&g_avrc_target.mutex);

    g_avrc_target.devices = bt_list_new(tg_device_destory);
    if (!g_avrc_target.devices) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        startup(PROFILE_AVRCP_TG, false);
        return;
    }

    if (bt_sal_avrcp_target_init() != BT_STATUS_SUCCESS) {
        bt_list_free(g_avrc_target.devices);
        g_avrc_target.devices = NULL;
        pthread_mutex_unlock(&g_avrc_target.mutex);
        startup(PROFILE_AVRCP_TG, false);
        return;
    }

    g_avrc_target.controller = NULL;
    g_avrc_target.enable = true;
    pthread_mutex_unlock(&g_avrc_target.mutex);
    startup(PROFILE_AVRCP_TG, true);
}

static void target_shutdown(profile_on_shutdown_t shutdown)
{
    pthread_mutex_lock(&g_avrc_target.mutex);
    if (!g_avrc_target.enable) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        shutdown(PROFILE_AVRCP_TG, true);
        return;
    }

    g_avrc_target.enable = false;
    bt_media_controller_destory(g_avrc_target.controller);
    g_avrc_target.controller = NULL;
    bt_list_free(g_avrc_target.devices);
    g_avrc_target.devices = NULL;
    bt_sal_avrcp_target_cleanup();
    pthread_mutex_unlock(&g_avrc_target.mutex);
    shutdown(PROFILE_AVRCP_TG, true);
}

static bt_status_t avrcp_target_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_avrc_target.mutex);
    if (g_avrc_target.enable) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        return BT_STATUS_NOT_ENABLED;
    }
    pthread_mutex_unlock(&g_avrc_target.mutex);

    avrcp_msg_t* msg = avrcp_msg_new(AVRC_STARTUP, NULL);
    msg->data.context = cb;
    do_in_avrcp_service(msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_target_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_avrc_target.mutex);
    if (!g_avrc_target.enable) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        return BT_STATUS_SUCCESS;
    }
    pthread_mutex_unlock(&g_avrc_target.mutex);

    avrcp_msg_t* msg = avrcp_msg_new(AVRC_SHUTDOWN, NULL);
    msg->data.context = cb;
    do_in_avrcp_service(msg);

    return BT_STATUS_SUCCESS;
}

static void* avrcp_target_register_callbacks(void* remote, const avrcp_target_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_avrc_target.callbacks, remote, (void*)callbacks);
}

static bool avrcp_target_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_avrc_target.callbacks, remote, cookie);
}

static bt_status_t avrcp_target_get_play_status_response(bt_address_t* addr, avrcp_play_status_t status,
    uint32_t song_len, uint32_t song_pos)
{
    avrcp_tg_device_t* device = NULL;

    pthread_mutex_lock(&g_avrc_target.mutex);
    if (g_avrc_target.enable) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        return BT_STATUS_NOT_ENABLED;
    }

    pthread_mutex_unlock(&g_avrc_target.mutex);

    device = tg_device_find(addr);
    if (!device) {
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    return bt_sal_avrcp_target_get_play_status_rsp(PRIMARY_ADAPTER, addr, status, song_len, song_pos);
}

static bt_status_t avrcp_target_play_status_notify(bt_address_t* addr, avrcp_play_status_t status)
{
    avrcp_tg_device_t* device = NULL;

    pthread_mutex_lock(&g_avrc_target.mutex);
    if (g_avrc_target.enable) {
        pthread_mutex_unlock(&g_avrc_target.mutex);
        return BT_STATUS_NOT_ENABLED;
    }

    pthread_mutex_unlock(&g_avrc_target.mutex);

    device = tg_device_find(addr);
    if (!device) {
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    return bt_sal_avrcp_target_play_status_notify(PRIMARY_ADAPTER, addr, status);
}

static const avrcp_target_interface_t avrcp_targetInterface = {
    .size = sizeof(avrcp_targetInterface),
    .register_callbacks = avrcp_target_register_callbacks,
    .unregister_callbacks = avrcp_target_unregister_callbacks,
    .get_play_status_rsp = avrcp_target_get_play_status_response,
    .play_status_notify = avrcp_target_play_status_notify,
};

static const void* get_avrcp_target_profile_interface(void)
{
    return (void*)&avrcp_targetInterface;
}

static int avrcp_target_dump(void)
{
    return 0;
}

static void avrcp_target_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_REMOTE_DETACH: {
        bt_instance_t* ins = msg->data.data;

        if (ins->avrcp_target_cookie) {
            BT_LOGD("%s PROFILE_EVT_REMOTE_DETACH", __func__);
            avrcp_target_unregister_callbacks((void**)&ins, ins->avrcp_target_cookie);
            ins->avrcp_target_cookie = NULL;
        }
        break;
    }
    default:
        break;
    }
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
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = avrcp_target_init,
    .startup = avrcp_target_startup,
    .shutdown = avrcp_target_shutdown,
    .process_msg = avrcp_target_process_msg,
    .get_state = avrcp_target_get_state,
    .get_profile_interface = get_avrcp_target_profile_interface,
    .cleanup = avrcp_target_cleanup,
    .dump = avrcp_target_dump,
};

void bt_sal_avrcp_target_event_callback(avrcp_msg_t* msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_target_service_handle_callback, msg);
}

void register_avrcp_target_service(void)
{
    register_service(&avrcp_target_service);
}
