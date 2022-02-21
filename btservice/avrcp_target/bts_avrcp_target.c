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
#include <nuttx/input/keyboard.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "btm_manager.h"
#include "bts_a2dp_source.h"
#include "bts_avrcp_target.h"
#include "bts_service.h"
#include "stack_adapter_avrcp_target.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"
#include "utils/utils.h"
#define LOG_TAG "avrcp_tg"
#include "log.h"

#define AVRCP_INPUT_DEVICE "/dev/ukeyboard"

typedef struct avrcp_msg_ {
    bt_address addr;
    enum event_type {
        CONNECTION_STATE_CHANGED,
        GET_ELEMENT_ATTR_REQUEST,
        GET_PLAY_STATUS_REQUEST,
        PASSTHROUHT_CMD,
        REGISTER_NOTIFICATION,
        SET_ABSOLUTE_VOLUME
    } event;
    union {
        SERVICE_PROFILE_CONNECTION_STATE conn_state;
        struct passthrough_cmd {
            SERVICE_AVRCP_PANEL_OPERATION opcode;
            SERVICE_AVRCP_PANEL_STATE state;
        } cmd;
        struct register_notification {
            SERVICE_AVRCP_NOTIFICATION_EVENT event;
            uint32_t interval;
        } reg_notif;
        int volume;
    } data;
} avrcp_msg_t;

static const struct {
    const char* name;
    uint8_t avrcp;
    uint32_t mapped_id;
} rc_key_map[] = {
    { "PLAY", AVRCP_OPERATION_PLAY, XF86XK_AudioPlay },
    { "STOP", AVRCP_OPERATION_STOP, XF86XK_AudioStop },
    { "PAUSE", AVRCP_OPERATION_PAUSE, XF86XK_AudioPause },
    { "FORWARD", AVRCP_OPERATION_FORWARD, XF86XK_AudioNext },
    { "BACKWARD", AVRCP_OPERATION_BACKWARD, XF86XK_AudioPrev },
    //{ "VOLUME UP",    AVRCP_OPERATION_VOLUME_UP,   XF86XK_AudioRaiseVolume},
    //{ "VOLUME DOWN",  AVRCP_OPERATION_VOLUME_DOWN, XF86XK_AudioLowerVolume},
    { NULL, 0, 0 }
};

int g_keyboard_fd = -1;

static const char* avrcp_event_to_string(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(CONNECTION_STATE_CHANGED)
        CASE_RETURN_STR(GET_ELEMENT_ATTR_REQUEST)
        CASE_RETURN_STR(GET_PLAY_STATUS_REQUEST)
        CASE_RETURN_STR(PASSTHROUHT_CMD)
        CASE_RETURN_STR(REGISTER_NOTIFICATION)
        CASE_RETURN_STR(SET_ABSOLUTE_VOLUME)
    default:
        return "UNKNOWN";
    }
}

static const char* ket_state(SERVICE_AVRCP_PANEL_STATE state)
{
    switch (state) {
    case AVRCP_PANEL_PRESS:
        return "PRESSED";
    case AVRCP_PANEL_RELEASE:
        return "RELEASED";
    case AVRCP_PANEL_HOLD:
        return "HOLD";
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

    BT_LOGD("%s, fd: %d, keycode: 0x%08lx, pressed: %d", __func__, fd, keycode, pressed);
    key.code = keycode;
    key.type = pressed;

    return write(g_keyboard_fd, &key, sizeof(key));
}

static void avrcp_tg_connection_state_handler(BD_ADDR addr,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    BT_LOGD("%s, device: %s, connection state : %d", __func__, addr_str(addr), state);

    if (SERVICE_PROFILE_CONNECTED == state) {
        uinput_open();
    } else if (SERVICE_PROFILE_DISCONNECTED == state) {
        uinput_close();
    }
}

static void avrcp_passthrough_cmd_handler(BD_ADDR remote_addr,
    SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state)
{
    int i;

    if (!bts_a2dp_source_stream_ready()) {
        BT_LOGW("%s A2DP is not ready, Discarding passthrough cmd:%02x", __func__, op);
        return;
    }

    if (op == AVRCP_OPERATION_STOP && !bts_a2dp_source_stream_started()) {
        BT_LOGW("%s Stream suspended, Ignore STOP cmd", __func__);
        return;
    }

    for (i = 0; rc_key_map[i].name != NULL; i++) {
        if (op == rc_key_map[i].avrcp) {
            int pressed;
            BT_LOGD("%s: Key:[%s] %s", __func__, rc_key_map[i].name, ket_state(state));
            if (state == AVRCP_PANEL_RELEASE)
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

static void avrcp_service_msg_process(avrcp_msg_t* msg)
{
    BT_LOGD("%s addr: %s, event: %s", __func__, addr_str(msg->addr), avrcp_event_to_string(msg->event));

    switch (msg->event) {
    case CONNECTION_STATE_CHANGED:
        avrcp_tg_connection_state_handler(msg->addr, msg->data.conn_state);
        break;
    case PASSTHROUHT_CMD:
        avrcp_passthrough_cmd_handler(msg->addr, msg->data.cmd.opcode, msg->data.cmd.state);
        break;
    case REGISTER_NOTIFICATION:
        break;
    case SET_ABSOLUTE_VOLUME:
        break;
    case GET_ELEMENT_ATTR_REQUEST:
    case GET_PLAY_STATUS_REQUEST:
    default:
        BT_LOGW("%s Unsupport event", __func__);
        break;
    }
}

static void bts_avrcp_tg_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    if (!data)
        return;

    avrcp_service_msg_process((avrcp_msg_t*)data);
    free(data);
}

static void do_in_avrcp_tg_service(avrcp_msg_t* msg)
{
    avrcp_msg_t* tg_msg;

    tg_msg = (avrcp_msg_t*)malloc(sizeof(avrcp_msg_t));
    if (tg_msg == NULL)
        return;

    memcpy(tg_msg, msg, sizeof(avrcp_msg_t));
    bts_send_uv_msg(BT_PROFILE_AV_RC_TARGET_ID, tg_msg, sizeof(avrcp_msg_t));
}

static void adpt_connection_state_changed_cb(BD_ADDR remote_addr,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    avrcp_msg_t msg;
    memset(&msg, 0, sizeof(avrcp_msg_t));

    msg.event = CONNECTION_STATE_CHANGED;
    msg.data.conn_state = state;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));
    do_in_avrcp_tg_service(&msg);
}

static void adpt_register_notification_request_cb(BD_ADDR remote_addr,
    SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t interval)
{
    avrcp_msg_t msg;
    memset(&msg, 0, sizeof(avrcp_msg_t));

    msg.event = REGISTER_NOTIFICATION;
    msg.data.reg_notif.event = event;
    msg.data.reg_notif.interval = interval;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));
    do_in_avrcp_tg_service(&msg);
}

static void adpt_get_play_status_request_cb(BD_ADDR remote_addr)
{
    avrcp_msg_t msg = {0};

    msg.event = GET_PLAY_STATUS_REQUEST;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));
    do_in_avrcp_tg_service(&msg);
}

static void adpt_get_element_attr_request_cb(BD_ADDR remote_addr)
{
    avrcp_msg_t msg = {0};

    msg.event = GET_ELEMENT_ATTR_REQUEST;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));
    do_in_avrcp_tg_service(&msg);
}

static void adpt_set_volume_cb(BD_ADDR remote_addr, uint8_t volume)
{
    avrcp_msg_t msg;
    memset(&msg, 0, sizeof(avrcp_msg_t));

    msg.event = SET_ABSOLUTE_VOLUME;
    msg.data.volume = volume;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));
    do_in_avrcp_tg_service(&msg);
}

static void adpt_panel_operation_cb(BD_ADDR remote_addr,
    SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state)
{
    avrcp_msg_t msg;
    memset(&msg, 0, sizeof(avrcp_msg_t));

    msg.event = PASSTHROUHT_CMD;
    msg.data.cmd.opcode = op;
    msg.data.cmd.state = state;
    memcpy(msg.addr, remote_addr, sizeof(bt_address));
    do_in_avrcp_tg_service(&msg);
}

static AVRCP_TARGET_CALLBACKS_S g_avrcp_target = {
    .size = sizeof(g_avrcp_target),
    .avrcp_target_connection_state_changed_cb = adpt_connection_state_changed_cb,
    .avrcp_target_received_get_element_attr_request_cb = adpt_get_element_attr_request_cb,
    .avrcp_target_received_get_play_status_request_cb = adpt_get_play_status_request_cb,
    .avrcp_target_received_panel_operation_cb = adpt_panel_operation_cb,
    .avrcp_target_received_register_notification_request_cb = adpt_register_notification_request_cb,
    .avrcp_target_received_set_volume_cb = adpt_set_volume_cb
};

bt_result_code bts_avrcp_target_init(void)
{
    SERVICE_BT_STATUS ret = service_adapter_avrcp_target_init(&g_avrcp_target);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        return BT_RESULT_FAILED;
    }
    bts_register_profile_process(BT_PROFILE_AV_RC_TARGET_ID, bts_avrcp_tg_handle_service_msg);

    return BT_RESULT_SUCCESS;
}

void bts_avrcp_target_uninit(void)
{
    bts_unregister_profile_process(BT_PROFILE_AV_RC_TARGET_ID);
    service_adapter_avrcp_target_cleanup();
}