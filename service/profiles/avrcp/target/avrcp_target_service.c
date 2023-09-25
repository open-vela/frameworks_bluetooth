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
#include <nuttx/input/keyboard.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "callbacks_list.h"
#include "sal_avrcp_target_interface.h"
#include "service_loop.h"
#include "service_manager.h"

#include "avrcp_target_service.h"

#include "utils/log.h"

#define AVRCP_TARGET_CALLBACK_FOREACH(_list, _cback, ...) \
  BT_CALLBACK_FOREACH(_list, avrcp_target_callbacks_t, _cback, ##__VA_ARGS__)

#define AVRCP_INPUT_DEVICE "/dev/ukeyboard"

static const struct {
    const char* name;
    uint8_t avrcp;
    uint32_t mapped_id;
} rc_key_map[] = {
    { "PLAY", PASSTHROUGH_CMD_ID_PLAY, XF86XK_AudioPlay },
    { "STOP", PASSTHROUGH_CMD_ID_STOP, XF86XK_AudioStop },
    { "PAUSE", PASSTHROUGH_CMD_ID_PAUSE, XF86XK_AudioPause },
    { "FORWARD", PASSTHROUGH_CMD_ID_FORWARD, XF86XK_AudioNext },
    { "BACKWARD", PASSTHROUGH_CMD_ID_BACKWARD, XF86XK_AudioPrev },
    /*
    { "VOLUME UP",    PASSTHROUGH_CMD_ID_VOLUME_UP,   XF86XK_AudioRaiseVolume},
    { "VOLUME DOWN",  PASSTHROUGH_CMD_ID_VOLUME_DOWN, XF86XK_AudioLowerVolume},
    */
    { NULL, 0, 0 }
};

static int g_keyboard_fd = -1;

typedef struct {
    struct list_node  list;
    bool              enable;
    pthread_mutex_t   mutex;
    callbacks_list_t  *callbacks;
} avrcp_target_global_t;

static avrcp_target_global_t g_avrcp_target = { 0 };

static bt_status_t avrcp_target_init(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_avrcp_target.mutex, &attr) < 0)
        return BT_STATUS_FAIL;

    g_avrcp_target.callbacks = bt_callbacks_list_new(2);

    return BT_STATUS_SUCCESS;
}

static void avrcp_target_cleanup(void)
{
    bt_callbacks_list_free(g_avrcp_target.callbacks);
    g_avrcp_target.callbacks = NULL;
    pthread_mutex_destroy(&g_avrcp_target.mutex);
}

static bt_status_t avrcp_target_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_avrcp_target.mutex);
    if (g_avrcp_target.enable) {
        pthread_mutex_unlock(&g_avrcp_target.mutex);
        return BT_STATUS_NOT_ENABLED;
    }

    list_initialize(&g_avrcp_target.list);
    if (bt_sal_avrcp_target_init() != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_avrcp_target.mutex);
        list_delete(&g_avrcp_target.list);
        return BT_STATUS_FAIL;
    }

    g_avrcp_target.enable = true;
    pthread_mutex_unlock(&g_avrcp_target.mutex);

    return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_target_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_avrcp_target.mutex);
    if (!g_avrcp_target.enable) {
        pthread_mutex_unlock(&g_avrcp_target.mutex);
        return BT_STATUS_SUCCESS;
    }

    g_avrcp_target.enable = false;
    list_delete(&g_avrcp_target.list);
    bt_sal_avrcp_target_cleanup();
    pthread_mutex_unlock(&g_avrcp_target.mutex);

    return BT_STATUS_SUCCESS;
}

static void *avrcp_target_register_callbacks(void *remote, const avrcp_target_callbacks_t *callbacks)
{
    return bt_remote_callbacks_register(g_avrcp_target.callbacks, remote, (void *)callbacks);
}

static bool avrcp_target_unregister_callbacks(void **remote, void *cookie)
{
    return bt_remote_callbacks_unregister(g_avrcp_target.callbacks, remote, cookie);
}

static void avrcp_target_service_handle_event(void *data)
{
    avrcp_msg_t* msg = data;

    switch (msg->id) {
      case GET_PLAY_STATUS_RSP:
        bt_sal_avrcp_target_get_play_status_rsp(&msg->addr,
            msg->data.playstatus.status, msg->data.playstatus.song_len,
            msg->data.playstatus.song_pos);
        break;
      case PLAYSTATUS_NOTIFY:
        bt_sal_avrcp_target_play_status_notify(&msg->addr, msg->data.playstatus.status);
        break;
      case SET_ABSOLUTE_VOLUME:
        bt_sal_avrcp_target_set_absolute_volume(&msg->addr, msg->data.absvol.volume);
        break;
      default:
        BT_LOGW("%s Unsupport message", __func__);
        break;
    }

    avrcp_msg_destory(msg);
}

static const char* key_state(avrcp_key_state_t state)
{
    switch (state) {
        case AVRCP_KEY_PRESSED:
            return "PRESSED";
        case AVRCP_KEY_RELEASED:
            return "RELEASED";
    }

    return "?";
}

static int uinput_open(void)
{
    if (access(AVRCP_INPUT_DEVICE, O_RDWR) != 0) {
        BT_LOGW("%s, driver %s not register", __func__, AVRCP_INPUT_DEVICE);
        return -1;
    }

    g_keyboard_fd = open(AVRCP_INPUT_DEVICE, O_WRONLY);
    if (g_keyboard_fd < 0)
        BT_LOGE("%s %s failed, errno : %d", __func__, AVRCP_INPUT_DEVICE, errno);

    return g_keyboard_fd;
}

static void uinput_close(void)
{
    if (g_keyboard_fd > 0) {
        close(g_keyboard_fd);
        g_keyboard_fd = -1;
    }
}

static int send_key(int fd, uint32_t keycode, int pressed)
{
    struct keyboard_event_s key;
    if (g_keyboard_fd < 0)
        return -1;

    BT_LOGD("%s, fd: %d, keycode: 0x%08" PRIx32", pressed: %d", __func__, fd, keycode, pressed);
    key.code = keycode;
    key.type = pressed;

    return write(g_keyboard_fd, &key, sizeof(key));
}

static void handle_avrcp_target_connection_state(avrcp_msg_t* msg)
{
    bt_address_t *addr = &msg->addr;
    avrcp_connection_state_t state = msg->data.conn_state;
    BT_LOGD("%s, device: %s, connection state : %d", __func__, bt_addr_str(addr), state);

    if (AVRC_CONNECTION_STATE_CONNECTED == state) {
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
        bt_sal_avrcp_target_register_volume_changed(addr);
#endif
        uinput_open();
    } else if (AVRC_CONNECTION_STATE_DISCONNECTED == state) {
        uinput_close();
    }

    AVRCP_TARGET_CALLBACK_FOREACH(g_avrcp_target.callbacks,
        connection_state_cb, &msg->addr, msg->data.conn_state);
}

static void handle_avrcp_passthrough_cmd(bt_address_t *addr,
                                          avrcp_passthr_cmd_t op,
                                          avrcp_key_state_t state)
{
    int i;

    if (op == PASSTHROUGH_CMD_ID_STOP
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
        && !a2dp_source_stream_started()
#endif
       ) {
        BT_LOGW("%s Stream suspended, Ignore STOP cmd", __func__);
        return;
    }

    for (i = 0; rc_key_map[i].name != NULL; i++) {
        if (op == rc_key_map[i].avrcp) {
            int pressed;
            BT_LOGD("%s: Key:[%s] %s", __func__, rc_key_map[i].name, key_state(state));
            if (state == AVRCP_KEY_RELEASED)
                pressed = KEYBOARD_RELEASE;
            else
                pressed = KEYBOARD_PRESS;

            send_key(g_keyboard_fd, rc_key_map[i].mapped_id, pressed);
            break;
        }
    }

    if (rc_key_map[i].name == NULL)
        BT_LOGW("%s AVRCP: unknown Key 0x%02X %s", __func__, op, key_state(state));
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

static void handle_avrcp_play_status_request(avrcp_msg_t* msg)
{
  AVRCP_TARGET_CALLBACK_FOREACH(g_avrcp_target.callbacks, get_play_status_cb, &msg->addr);
  bt_sal_avrcp_target_get_play_status_rsp(&msg->addr, current_playback_status(), 0, 0);
}

static void handle_avrcp_register_notification(avrcp_msg_t* msg)
{
  bt_address_t *addr = &msg->addr;
  avrcp_notification_event_t event = msg->data.notify_req.event;

    switch (event) {
        case NOTIFICATION_EVT_PALY_STATUS_CHANGED:{
            AVRCP_TARGET_CALLBACK_FOREACH(g_avrcp_target.callbacks, playback_register_notification_cb, &msg->addr);
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
    avrcp_msg_t* msg = data;

    switch (msg->id) {
      case CONNECTION_STATE_CHANGED:
        handle_avrcp_target_connection_state(msg);
        break;
      case PASSTHROUHT_CMD:
        handle_avrcp_passthrough_cmd(&msg->addr, msg->data.passthr_cmd.opcode, msg->data.passthr_cmd.state);
        break;
      case REGISTER_NOTIFICATION_REQ:
        handle_avrcp_register_notification(msg);
        break;
      case GET_PLAY_STATUS_REQ:
        handle_avrcp_play_status_request(msg);
        break;
      case SET_ABSOLUTE_VOLUME:
        AVRCP_TARGET_CALLBACK_FOREACH(g_avrcp_target.callbacks,
            volume_changed_cb, &msg->addr, msg->data.absvol.volume);
        break;
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

    do_in_service_loop(avrcp_target_service_handle_event, msg);
}

static bt_status_t avrcp_target_get_play_status_rsp(
    bt_address_t *addr, avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos)
{
  avrcp_msg_t *msg = avrcp_msg_new(GET_PLAY_STATUS_RSP, addr);

  if (msg == NULL)
    return BT_STATUS_NOMEM;

  msg->data.playstatus.status = status;
  msg->data.playstatus.song_len = song_len;
  msg->data.playstatus.song_pos = song_pos;

  do_in_avrcp_service(msg);

  return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_target_play_status_notify(bt_address_t *addr, avrcp_play_status_t status)
{
  avrcp_msg_t *msg = avrcp_msg_new(PLAYSTATUS_NOTIFY, addr);

  if (msg == NULL)
    return BT_STATUS_NOMEM;

  msg->data.playstatus.status = status;

  do_in_avrcp_service(msg);

  return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_target_set_absolute_volume(bt_address_t *addr, uint8_t volume)
{
  avrcp_msg_t *msg = avrcp_msg_new(SET_ABSOLUTE_VOLUME, addr);

  if (msg == NULL)
    return BT_STATUS_NOMEM;

  msg->data.absvol.volume = volume;

  do_in_avrcp_service(msg);

  return BT_STATUS_SUCCESS;
}

static const avrcp_target_interface_t avrcp_targetInterface = {
    .size = sizeof(avrcp_targetInterface),
    .register_callbacks = avrcp_target_register_callbacks,
    .unregister_callbacks = avrcp_target_unregister_callbacks,
    .get_play_status_rsp = avrcp_target_get_play_status_rsp,
    .play_status_notify = avrcp_target_play_status_notify,
    .set_absolute_volume = avrcp_target_set_absolute_volume,
};

static const void *get_avrcp_target_profile_interface(void)
{
    return (void *)&avrcp_targetInterface;
}

static int avrcp_target_dump(void)
{
    return 0;
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
    .get_state = NULL,
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
