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

#include "adapter_internel.h"
#include "avrcp_control_service.h"
#include "bt_addr.h"
#include "bt_list.h"
#include "bt_player.h"
#include "callbacks_list.h"
#include "sal_avrcp_control_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#define AVRCP_CT_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, avrcp_control_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    struct list_node list;
    bool enable;
    bt_list_t* devices;
    pthread_mutex_t mutex;
    callbacks_list_t* callbacks;
} avrcp_controller_service_t;

typedef struct {
    bt_address_t addr;
    bool initiator;
    bt_media_player_t* player;
    profile_connection_state_t state;
} avrcp_ct_device_t;

static void controller_startup(profile_on_startup_t startup);
static void controller_shutdown(profile_on_shutdown_t shutdown);
static void avrcp_ct_on_play(bt_media_player_t* player, void* context);
static void avrcp_ct_on_pause(bt_media_player_t* player, void* context);
static void avrcp_ct_on_stop(bt_media_player_t* player, void* context);
static void avrcp_ct_on_next(bt_media_player_t* player, void* context);
static void avrcp_ct_on_prev(bt_media_player_t* player, void* context);

static avrcp_controller_service_t g_avrc_controller = { 0 };
static bt_media_player_callback_t g_player_cb = {
    NULL,
    avrcp_ct_on_play,
    avrcp_ct_on_pause,
    avrcp_ct_on_stop,
    avrcp_ct_on_next,
    avrcp_ct_on_prev
};

static bool ct_device_cmp(void* device, void* addr)
{
    return bt_addr_compare(&((avrcp_ct_device_t*)device)->addr, addr) == 0;
}

static avrcp_ct_device_t* ct_device_find(bt_address_t* addr)
{
    if (!g_avrc_controller.devices || !addr)
        return NULL;

    return bt_list_find(g_avrc_controller.devices, ct_device_cmp, addr);
}

static avrcp_ct_device_t* ct_device_create(bt_address_t* addr, bool initiator)
{
    avrcp_ct_device_t* device;
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (!addr)
        return NULL;

    device = malloc(sizeof(avrcp_ct_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->initiator = initiator;
    device->player = NULL;
    device->state = PROFILE_STATE_DISCONNECTED;

    bt_list_add_tail(g_avrc_controller.devices, device);

    bt_addr_ba2str(addr, _addr_str);
    BT_LOGD("%s [%s] success", __func__, _addr_str);

    return device;
}

static void ct_device_destory(void* data)
{
    avrcp_ct_device_t* device = data;
    char _addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    assert(device);

    bt_addr_ba2str(&device->addr, _addr_str);
    BT_LOGD("%s [%s] success", __func__, _addr_str);

    if (device->player)
        bt_media_player_destory(device->player);

    if (device->state != PROFILE_STATE_DISCONNECTED)
        AVRCP_CT_CALLBACK_FOREACH(g_avrc_controller.callbacks, connection_state_cb, &device->addr, PROFILE_STATE_DISCONNECTED);

    free(device);
}

static void ct_device_remove(avrcp_ct_device_t* device)
{
    bt_list_remove(g_avrc_controller.devices, device);
}

static void avrcp_controller_service_handle_event(void* data)
{
    avrcp_msg_t* msg = data;

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

static void send_pass_through_cmd(avrcp_ct_device_t* device, avrcp_passthr_cmd_t cmd)
{
    bt_sal_avrcp_control_send_pass_through_cmd(&device->addr, cmd, AVRCP_KEY_PRESSED);
    bt_sal_avrcp_control_send_pass_through_cmd(&device->addr, cmd, AVRCP_KEY_RELEASED);
}

static void avrcp_ct_on_play(bt_media_player_t* player, void* context)
{
    avrcp_ct_device_t* device = context;

    BT_LOGD("%s", __func__);
    send_pass_through_cmd(device, PASSTHROUGH_CMD_ID_PLAY);
}

static void avrcp_ct_on_pause(bt_media_player_t* player, void* context)
{
    avrcp_ct_device_t* device = context;

    BT_LOGD("%s", __func__);
    send_pass_through_cmd(device, PASSTHROUGH_CMD_ID_PAUSE);
}

static void avrcp_ct_on_stop(bt_media_player_t* player, void* context)
{
    avrcp_ct_device_t* device = context;

    BT_LOGD("%s", __func__);
    send_pass_through_cmd(device, PASSTHROUGH_CMD_ID_STOP);
}

static void avrcp_ct_on_next(bt_media_player_t* player, void* context)
{
    avrcp_ct_device_t* device = context;

    BT_LOGD("%s", __func__);
    send_pass_through_cmd(device, PASSTHROUGH_CMD_ID_FORWARD);
}

static void avrcp_ct_on_prev(bt_media_player_t* player, void* context)
{
    avrcp_ct_device_t* device = context;

    BT_LOGD("%s", __func__);
    send_pass_through_cmd(device, PASSTHROUGH_CMD_ID_BACKWARD);
}

static void handle_avrcp_connection_state(avrcp_msg_t* msg)
{
    avrcp_ct_device_t* device = NULL;
    bt_address_t* addr = &msg->addr;
    profile_connection_state_t state = msg->data.conn_state.conn_state;

    pthread_mutex_lock(&g_avrc_controller.mutex);
    if (!g_avrc_controller.enable) {
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        return;
    }
    pthread_mutex_unlock(&g_avrc_controller.mutex);

    BT_LOGD("avrc ct connnection --> device:[%s], state: %d", bt_addr_str(addr), state);

    device = ct_device_find(addr);
    /* set device state */
    if (device)
        device->state = state;

    switch (state) {
    case PROFILE_STATE_DISCONNECTED:
        /* destory device and release resource if device is existed*/
        if (device)
            ct_device_remove(device);
        break;
    case PROFILE_STATE_CONNECTING:
        if (!device) {
            device = ct_device_create(addr, false);
            device->state = state;
        }
        break;
    case PROFILE_STATE_CONNECTED: {
        if (!device) {
            device = ct_device_create(addr, false);
            device->state = state;
        }
        bt_sal_avrcp_control_get_capabilities(addr, AVRCP_CAPABILITY_ID_EVENTS_SUPPORTED);
        device->player = bt_media_player_create(device, &g_player_cb);
    } break;
    case PROFILE_STATE_DISCONNECTING:
        assert(device);
        break;
    default:
        assert(0);
    }

    AVRCP_CT_CALLBACK_FOREACH(g_avrc_controller.callbacks, connection_state_cb, addr, state);
}

static void handle_avrcp_get_play_status_response(avrcp_msg_t* msg)
{
    avrcp_ct_device_t* device = NULL;
    bt_address_t* addr = &msg->addr;
    rc_play_status_t* playstatus = &msg->data.playstatus;

    device = ct_device_find(addr);
    if (!device)
        return;

    BT_LOGD("playback status rsp --> status: %s, songlen: %" PRIu32 ", position: %" PRIu32,
        bt_media_status_str(playstatus->status), playstatus->song_len, playstatus->song_pos);
    bt_media_player_set_status(device->player, playstatus->status);
    bt_media_player_set_duration(device->player, playstatus->song_len);
    bt_media_player_set_position(device->player, playstatus->song_pos);
}

static void handle_avrcp_get_capability_response(avrcp_msg_t* msg)
{
    avrcp_ct_device_t* device = NULL;
    bt_address_t* addr = &msg->addr;
    uint8_t* cap = msg->data.cap.capabilities;

    device = ct_device_find(addr);
    if (!device)
        return;

    while (msg->data.cap.cap_count) {
        BT_LOGD("capability support event: %d", *cap);
        switch (*cap) {
        case NOTIFICATION_EVT_PALY_STATUS_CHANGED:
            bt_sal_avrcp_control_register_notification(addr, *cap, 0);
            bt_sal_avrcp_control_get_playback_state(addr);
            break;
        case NOTIFICATION_EVT_PLAY_POS_CHANGED:
            bt_sal_avrcp_control_register_notification(addr, *cap, 2);
            break;
        case NOTIFICATION_EVT_VOLUME_CHANGED:
            /* don't work on controller role */
            break;
        default:
            break;
        }
        cap++;
        msg->data.cap.cap_count--;
    }
}

static void handle_avrcp_passthrough_cmd_response(avrcp_msg_t* msg)
{
    avrcp_ct_device_t* device = NULL;
    bt_address_t* addr = &msg->addr;

    device = ct_device_find(addr);
    if (!device)
        return;

    BT_LOGD("passthrough cmd rsp --> cmd: %d, state: %d, rsp: %d", msg->data.passthr_rsp.cmd,
        msg->data.passthr_rsp.state, msg->data.passthr_rsp.rsp);
}

static void handle_avrcp_register_notification_response(avrcp_msg_t* msg)
{
    avrcp_ct_device_t* device = NULL;
    bt_address_t* addr = &msg->addr;

    device = ct_device_find(addr);
    if (!device)
        return;

    BT_LOGD("register_notification evt: %d", msg->data.notify_rsp.event);
    switch (msg->data.notify_rsp.event) {
    case NOTIFICATION_EVT_PALY_STATUS_CHANGED: {
        bt_media_status_t status = msg->data.notify_rsp.value;
        BT_LOGD("playback status changed: %s, get status now...", bt_media_status_str(status));
        bt_media_player_set_status(device->player, status);
        bt_sal_avrcp_control_get_playback_state(addr);
        break;
    }
    case NOTIFICATION_EVT_PLAY_POS_CHANGED: {
        BT_LOGD("song position is: %" PRIu32, msg->data.notify_rsp.value);
        bt_media_player_set_position(device->player, msg->data.notify_rsp.value);
        break;
    }
    case NOTIFICATION_EVT_VOLUME_CHANGED: {
        /* don't work on controller role */
        break;
    }
    default:
        break;
    }
}

static void avrcp_control_service_handle_callback(void* data)
{
    avrcp_msg_t* msg = data;

    switch (msg->id) {
    case AVRC_CONNECTION_STATE_CHANGED:
        handle_avrcp_connection_state(msg);
        break;
    case AVRC_GET_PLAY_STATUS_RSP:
        handle_avrcp_get_play_status_response(msg);
        break;
    case AVRC_GET_CAPABILITY_RSP:
        handle_avrcp_get_capability_response(msg);
        break;
    case AVRC_PASSTHROUHT_CMD_RSP:
        handle_avrcp_passthrough_cmd_response(msg);
        break;
    case AVRC_REGISTER_NOTIFICATION_RSP:
        handle_avrcp_register_notification_response(msg);
        break;
    default:
        BT_LOGW("%s Unsupport message: %d", __func__, msg->id);
        break;
    }

    avrcp_msg_destory(msg);
}

static void do_in_avrcp_service(avrcp_msg_t* msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_controller_service_handle_event, msg);
}

static bt_status_t avrcp_control_init(void)
{
    pthread_mutexattr_t attr;

    memset(&g_avrc_controller, 0, sizeof(g_avrc_controller));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_avrc_controller.mutex, &attr) < 0)
        return BT_STATUS_FAIL;

    g_avrc_controller.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

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
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        startup(PROFILE_AVRCP_CT, true);
        return;
    }

    g_avrc_controller.devices = bt_list_new(ct_device_destory);
    if (!g_avrc_controller.devices) {
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        startup(PROFILE_AVRCP_CT, false);
        return;
    }

    if (bt_sal_avrcp_control_init() != BT_STATUS_SUCCESS) {
        list_delete(&g_avrc_controller.list);
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        startup(PROFILE_AVRCP_CT, false);
        return;
    }

    g_avrc_controller.enable = true;
    pthread_mutex_unlock(&g_avrc_controller.mutex);
    startup(PROFILE_AVRCP_CT, true);
}

static void controller_shutdown(profile_on_shutdown_t shutdown)
{
    pthread_mutex_lock(&g_avrc_controller.mutex);
    if (!g_avrc_controller.enable) {
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        shutdown(PROFILE_AVRCP_CT, true);
        return;
    }

    g_avrc_controller.enable = false;
    bt_list_free(g_avrc_controller.devices);
    g_avrc_controller.devices = NULL;
    bt_sal_avrcp_control_cleanup();
    pthread_mutex_unlock(&g_avrc_controller.mutex);
    shutdown(PROFILE_AVRCP_CT, true);
}

static bt_status_t avrcp_control_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_avrc_controller.mutex);
    if (g_avrc_controller.enable) {
        pthread_mutex_unlock(&g_avrc_controller.mutex);
        return BT_STATUS_NOT_ENABLED;
    }
    pthread_mutex_unlock(&g_avrc_controller.mutex);
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_STARTUP, NULL);
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
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_SHUTDOWN, NULL);
    msg->data.context = cb;
    do_in_avrcp_service(msg);

    return BT_STATUS_SUCCESS;
}

static void* avrcp_control_register_callbacks(void* remote, const avrcp_control_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_avrc_controller.callbacks, remote, (void*)callbacks);
}

static bool avrcp_control_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_avrc_controller.callbacks, remote, cookie);
}

static const avrcp_control_interface_t avrcp_controlInterface = {
    .size = sizeof(avrcp_controlInterface),
    .register_callbacks = avrcp_control_register_callbacks,
    .unregister_callbacks = avrcp_control_unregister_callbacks
};

static const void* get_avrcp_control_profile_interface(void)
{
    return (void*)&avrcp_controlInterface;
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
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = avrcp_control_init,
    .startup = avrcp_control_startup,
    .shutdown = avrcp_control_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_avrcp_control_profile_interface,
    .cleanup = avrcp_control_cleanup,
    .dump = avrcp_control_dump,
};

void bt_sal_avrcp_control_event_callback(avrcp_msg_t* msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(avrcp_control_service_handle_callback, msg);
}

void register_avrcp_control_service(void)
{
    register_service(&avrcp_control_service);
}
