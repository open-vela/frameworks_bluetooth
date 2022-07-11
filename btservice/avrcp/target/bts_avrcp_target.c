/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_avrcp.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <nuttx/input/keyboard.h>

#include "stack_adapter_avrcp.h"
#include "stack_adapter_avrcp_target.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"
#include "btm_manager.h"
#include "btm_avrcp.h"
#include "bts_a2dp_source.h"
#include "bts_avrcp_target.h"
#include "bts_avrc.h"
#include "bts_service.h"
#include "utils/utils.h"
#define LOG_TAG "avrcp_tg"
#include "log.h"

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
static avrcp_tg_callbacks_t *g_avrcp_cbs = NULL;

static const char* avrcp_event_to_string(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(CONNECTION_STATE_CHANGED)
        CASE_RETURN_STR(GET_ELEMENT_ATTR_REQ)
        CASE_RETURN_STR(GET_PLAY_STATUS_REQ)
        CASE_RETURN_STR(PASSTHROUHT_CMD)
        CASE_RETURN_STR(REGISTER_NOTIFICATION_REQ)
        CASE_RETURN_STR(REGISTER_NOTIFICATION_ABSVOL_RSP)
    default:
        return "UNKNOWN";
    }
}

static const char* ket_state(avrcp_key_state_t state)
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

static void handle_avrcp_tg_connection_state(bt_address addr, avrcp_connection_state_t state)
{
    BT_LOGD("%s, device: %s, connection state : %d", __func__, addr_str(addr), state);

    if (AVRC_CONNECTION_STATE_CONNECTED == state) {
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
        bts_avrcp_register_volume_changed(addr);
#endif
        uinput_open();
    } else if (AVRC_CONNECTION_STATE_DISCONNECTED == state) {
        uinput_close();
    }

    if (g_avrcp_cbs && g_avrcp_cbs->connection_state_cb)
        g_avrcp_cbs->connection_state_cb(addr, state);
}

static void handle_avrcp_passthrough_cmd(bt_address addr,
                                          avrcp_passthr_cmd_t op,
                                          avrcp_key_state_t state)
{
    int i;

    if (op == PASSTHROUGH_CMD_ID_STOP && !bts_a2dp_source_stream_started()) {
        BT_LOGW("%s Stream suspended, Ignore STOP cmd", __func__);
        return;
    }

    for (i = 0; rc_key_map[i].name != NULL; i++) {
        if (op == rc_key_map[i].avrcp) {
            int pressed;
            BT_LOGD("%s: Key:[%s] %s", __func__, rc_key_map[i].name, ket_state(state));
            if (state == AVRCP_KEY_RELEASED)
                pressed = KEYBOARD_RELEASE;
            else
                pressed = KEYBOARD_PRESS;

            send_key(g_keyboard_fd, rc_key_map[i].mapped_id, pressed);
            break;
        }
    }

    if (rc_key_map[i].name == NULL)
        BT_LOGW("%s AVRCP: unknown Key 0x%02X %s", __func__, op, ket_state(state));
}

static avrcp_play_status_t current_playback_status(void)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (bts_a2dp_source_stream_started())
        return PLAY_STATUS_PLAYING;
    else if (bts_a2dp_source_stream_ready())
        return PLAY_STATUS_PAUSED;
    else
        return PLAY_STATUS_STOPPED;
#else
    return PLAY_STATUS_ERROR;
#endif
}

static void handle_avrcp_register_notification(bt_address addr,
                                               avrcp_notification_event_t event,
                                               uint32_t interval)
{
    switch (event) {
        case NOTIFICATION_EVT_PALY_STATUS_CHANGED:{
            if (g_avrcp_cbs && g_avrcp_cbs->playback_register_notification_cb)
                g_avrcp_cbs->playback_register_notification_cb(addr);
            else
                bts_avrcp_notify_play_state_changed(addr, current_playback_status());
            break;
        }
        case NOTIFICATION_EVT_TRACK_CHANGED:
            bts_avrcp_notify_track_changed(addr, FALSE);
            break;
        case NOTIFICATION_EVT_PLAY_POS_CHANGED:
            bts_avrcp_notify_play_position_changed(addr, 0);
            break;
        default:
            break;
    }
}

static void handle_avrcp_play_status_request(bt_address addr)
{
    if (g_avrcp_cbs && g_avrcp_cbs->get_play_status_cb)
        g_avrcp_cbs->get_play_status_cb(addr);
    else
        bts_avrcp_get_play_status_response(addr, current_playback_status(), 0, 0);
}

static void handle_avrcp_volume_changed(bt_address addr, uint8_t volume)
{
    if (g_avrcp_cbs && g_avrcp_cbs->volume_changed_cb)
        g_avrcp_cbs->volume_changed_cb(addr, volume);
    else if (!volume)
        bts_avrcp_set_absolute_volume(addr, 0x3F);
}

static void bts_avrcp_tg_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    if (!data)
        return;

    avrcp_msg_t* msg = (avrcp_msg_t*)data;
    BT_LOGD("%s addr: %s, id: %s", __func__, addr_str(msg->addr), avrcp_event_to_string(msg->id));

    switch (msg->id) {
        case CONNECTION_STATE_CHANGED:
            handle_avrcp_tg_connection_state(msg->addr, msg->data.conn_state);
            break;
        case PASSTHROUHT_CMD:
            handle_avrcp_passthrough_cmd(msg->addr, msg->data.passthr_cmd.opcode, msg->data.passthr_cmd.state);
            break;
        case REGISTER_NOTIFICATION_REQ:
            handle_avrcp_register_notification(msg->addr, msg->data.notify_req.event,
                                               msg->data.notify_req.interval);
            break;
        case GET_PLAY_STATUS_REQ:
            handle_avrcp_play_status_request(msg->addr);
            break;
        case REGISTER_NOTIFICATION_ABSVOL_RSP:
            handle_avrcp_volume_changed(msg->addr, msg->data.absvol.volume);
            break;
        default:
            BT_LOGW("%s Unsupport event", __func__);
            break;
    }

    free(data);
}

static void avrcp_target_msg_callback(avrcp_msg_t *msg)
{
    avrcp_msg_t* copy;

    copy = (avrcp_msg_t*)malloc(sizeof(avrcp_msg_t));
    if (copy == NULL)
        return;

    memcpy(copy, msg, sizeof(avrcp_msg_t));
    bts_send_uv_msg(BT_PROFILE_AV_RC_TARGET_ID, copy, sizeof(avrcp_msg_t));
}

static bt_result_code avrc_get_play_status_rsp(bt_address addr,
                                        avrcp_play_status_t status,
                                        uint32_t song_len, uint32_t song_pos)
{
    return bts_avrcp_get_play_status_response(addr, status, song_len, song_pos);
}

static bt_result_code avrc_play_status_notify(bt_address addr, avrcp_play_status_t status)
{
    return bts_avrcp_notify_play_state_changed(addr, status);
}

static bt_result_code avrc_set_absolute_volume(bt_address addr, uint8_t volume)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    return bts_avrcp_set_absolute_volume(addr, volume);
#else
    return BT_RESULT_UNSUPPORTED;
#endif
}

static bt_result_code avrc_set_callbacks(avrcp_tg_callbacks_t* callbacks)
{
    g_avrcp_cbs = callbacks;

    return BT_RESULT_SUCCESS;
}

static void avrc_reset_callbacks(void)
{
    g_avrcp_cbs = NULL;
}

static const avrcp_tg_interface_t avrcpTgInterface = {
    sizeof(avrcpTgInterface),
    avrc_get_play_status_rsp,
    avrc_play_status_notify,
    avrc_set_absolute_volume,
    avrc_set_callbacks,
    avrc_reset_callbacks
};

bt_result_code avrcp_target_service_start(void)
{
    bts_avrcp_init(AVRC_ROLE_TARGET, false, avrcp_target_msg_callback);
    bts_register_profile_process(BT_PROFILE_AV_RC_TARGET_ID, bts_avrcp_tg_handle_service_msg);

    return BT_RESULT_SUCCESS;
}

void avrcp_target_service_stop(void)
{
    bts_unregister_profile_process(BT_PROFILE_AV_RC_TARGET_ID);
    bts_avrcp_cleanup(AVRC_ROLE_TARGET);
}

const avrcp_tg_interface_t *get_avrcp_tg_service_interface(void)
{
    return &avrcpTgInterface;
}