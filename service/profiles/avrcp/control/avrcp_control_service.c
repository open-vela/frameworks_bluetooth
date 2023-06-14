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
#define LOG_TAG "avrcp_control"

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
    struct list_node  list;
    bool              enable;
    pthread_mutex_t   mutex;
    callbacks_list_t  *callbacks;
} avrcp_control_global_t;

static avrcp_control_global_t g_avrcp_control = { 0 };

static bt_status_t avrcp_control_init(void)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&g_avrcp_control.mutex, &attr) < 0)
        return BT_STATUS_FAIL;

    g_avrcp_control.callbacks = bt_callbacks_list_new(2);

    return BT_STATUS_SUCCESS;
}

static void avrcp_control_cleanup(void)
{
    bt_callbacks_list_free(g_avrcp_control.callbacks);
    g_avrcp_control.callbacks = NULL;
    pthread_mutex_destroy(&g_avrcp_control.mutex);
}

static bt_status_t avrcp_control_startup(profile_on_startup_t cb)
{
    pthread_mutex_lock(&g_avrcp_control.mutex);
    if (g_avrcp_control.enable) {
        pthread_mutex_unlock(&g_avrcp_control.mutex);
        return BT_STATUS_NOT_ENABLED;
    }

    list_initialize(&g_avrcp_control.list);
    if (bt_sal_avrcp_control_init() != BT_STATUS_SUCCESS) {
        pthread_mutex_unlock(&g_avrcp_control.mutex);
        list_delete(&g_avrcp_control.list);
        return BT_STATUS_FAIL;
    }

    g_avrcp_control.enable = true;
    pthread_mutex_unlock(&g_avrcp_control.mutex);

    return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_control_shutdown(profile_on_shutdown_t cb)
{
    pthread_mutex_lock(&g_avrcp_control.mutex);
    if (!g_avrcp_control.enable) {
        pthread_mutex_unlock(&g_avrcp_control.mutex);
        return BT_STATUS_SUCCESS;
    }

    g_avrcp_control.enable = false;
    list_delete(&g_avrcp_control.list);
    bt_sal_avrcp_control_cleanup();
    pthread_mutex_unlock(&g_avrcp_control.mutex);

    return BT_STATUS_SUCCESS;
}

static void *avrcp_control_register_callbacks(void *remote, const avrcp_control_callbacks_t *callbacks)
{
    return bt_remote_callbacks_register(g_avrcp_control.callbacks, remote, (void *)callbacks);
}

static bool avrcp_control_unregister_callbacks(void **remote, void *cookie)
{
    return bt_remote_callbacks_unregister(g_avrcp_control.callbacks, remote, cookie);
}

static void avrcp_control_service_handle_event(void *data)
{
    avrcp_msg_t* msg = data;

    switch (msg->id) {
      case PASSTHROUHT_CMD:
        bt_sal_avrcp_control_send_pass_through_cmd(&msg->addr,
            msg->data.passthr_cmd.opcode, msg->data.passthr_cmd.state);
        break;
      case GET_PLAYBACK_STATE:
        bt_sal_avrcp_control_get_playback_state(&msg->addr);
        break;
      case VOLUME_CHANGED_NOTIFY:
        bt_sal_avrcp_control_volume_changed_notify(&msg->addr, msg->data.absvol.volume);
        break;
      default:
        BT_LOGW("%s Unsupport message", __func__);
        break;
    }

    avrcp_msg_destory(msg);
}

static void avrcp_control_service_handle_callback(void *data)
{
    avrcp_msg_t* msg = data;

    switch (msg->id) {
      case CONNECTION_STATE_CHANGED:
        AVRCP_CONTROL_CALLBACK_FOREACH(g_avrcp_control.callbacks,
            connection_state_cb, &msg->addr, msg->data.conn_state);
        break;
      case PASSTHROUHT_CMD_RSP:
        rc_passthr_rsp_t *rsp = &msg->data.passthr_rsp;
        AVRCP_CONTROL_CALLBACK_FOREACH(g_avrcp_control.callbacks,
            passthrough_rsp_cb, &msg->addr, rsp->cmd, rsp->state, rsp->rsp);
        break;
      case REGISTER_NOTIFICATION_RSP:
        rc_notification_rsp_t *notify = &msg->data.notify_rsp;
        if (notify->event == NOTIFICATION_EVT_PALY_STATUS_CHANGED)
          AVRCP_CONTROL_CALLBACK_FOREACH(g_avrcp_control.callbacks, play_status_changed_cb,
              &msg->addr, notify->value);
        else if (notify->event == NOTIFICATION_EVT_PLAY_POS_CHANGED)
          AVRCP_CONTROL_CALLBACK_FOREACH(g_avrcp_control.callbacks, play_position_changed_cb,
              &msg->addr, 0, notify->value);
        break;
      case GET_PLAY_STATUS_RSP:
        rc_play_status_t *status = &msg->data.playstatus;
        AVRCP_CONTROL_CALLBACK_FOREACH(g_avrcp_control.callbacks, play_status_changed_cb, &msg->addr, status->status);
        AVRCP_CONTROL_CALLBACK_FOREACH(g_avrcp_control.callbacks, play_position_changed_cb,
            &msg->addr, status->song_len, status->song_pos);
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

    do_in_service_loop(avrcp_control_service_handle_event, msg);
}

static bt_status_t avrcp_control_send_pass_through_cmd(
    bt_address_t *bd_addr, avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state)
{
  avrcp_msg_t *msg = avrcp_msg_new(PASSTHROUHT_CMD, bd_addr);

  if (msg == NULL)
    return BT_STATUS_NOMEM;

  msg->data.passthr_cmd.opcode = key_code;
  msg->data.passthr_cmd.state = key_state;

  do_in_avrcp_service(msg);

  return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_control_get_playback_state(bt_address_t *bd_addr)
{
  do_in_avrcp_service(avrcp_msg_new(GET_PLAYBACK_STATE, bd_addr));
  return BT_STATUS_SUCCESS;
}

static bt_status_t avrcp_control_volume_changed_notify(bt_address_t *bd_addr, uint8_t volume)
{
  avrcp_msg_t *msg = avrcp_msg_new(VOLUME_CHANGED_NOTIFY, bd_addr);

  if (msg == NULL)
    return BT_STATUS_NOMEM;

  msg->data.absvol.volume = volume;

  do_in_avrcp_service(msg);

  return BT_STATUS_SUCCESS;
}

static const avrcp_control_interface_t avrcp_controlInterface = {
    .size = sizeof(avrcp_controlInterface),
    .register_callbacks = avrcp_control_register_callbacks,
    .unregister_callbacks = avrcp_control_unregister_callbacks,
    .send_pass_through_cmd = avrcp_control_send_pass_through_cmd,
    .get_playback_state = avrcp_control_get_playback_state,
    .volume_changed_notify = avrcp_control_volume_changed_notify,
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
