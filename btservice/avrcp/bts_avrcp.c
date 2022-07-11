/*
 * Copyright (C) 2020 Xiaomi Corporation
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
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "btm_manager.h"
#include "btm_avrcp.h"
#include "bts_avrc.h"
#include "bts_service.h"
#include "stack_adapter_avrcp.h"
#include "stack_adapter_avrcp_target.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"
#include "utils/utils.h"
#define LOG_TAG "avrc"
#include "log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct avrcp_service {
    bool enabled;
    bool absolute_support;
    avrcp_msg_callback_t cb;
} avrcp_service_t;

typedef struct {
    uint8_t refs;
    avrcp_service_t service[2];
} avrcp_scb_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void target_connection_state_changed_cb(BD_ADDR addr,
    SERVICE_PROFILE_CONNECTION_STATE state);
static void register_notification_request_cb(BD_ADDR addr,
    SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t interval);
static void get_play_status_request_cb(BD_ADDR addr);
static void panel_operation_cb(BD_ADDR addr,
                               SERVICE_AVRCP_PANEL_OPERATION op,
                               SERVICE_AVRCP_PANEL_STATE state);
static void absolute_volume_cb(BD_ADDR addr, uint8_t volume);
static void ctrl_connection_state_changed_cb(BD_ADDR addr,
                                      SERVICE_PROFILE_CONNECTION_STATE state);
static void panel_rsp_cb(BD_ADDR addr,
                         SERVICE_AVRCP_RESPONSE response,
                         SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state);
static void register_notification_event_cb(BD_ADDR addr,
                                          SERVICE_AVRCP_NOTIFICATION_EVENT event,
                                          void *value);
static void remote_capabilities_rsp_cb(BD_ADDR addr,
                                       SERVICE_AVRCP_CAPABILITY_RSP_S *capabilities);
static void get_play_status_rsp_cb(BD_ADDR addr,
                                   SERVICE_AVRCP_MEDIA_STATUS media_status,
                                   uint32_t song_length, uint32_t position);


/****************************************************************************
 * Private Data
 ****************************************************************************/

static AVRCP_TARGET_CALLBACKS_S g_avrcp_target_cbks = {
    .size = sizeof(g_avrcp_target_cbks),
    .avrcp_target_connection_state_changed_cb = target_connection_state_changed_cb,
    .avrcp_target_received_get_element_attr_request_cb = NULL,
    .avrcp_target_received_get_play_status_request_cb = get_play_status_request_cb,
    .avrcp_target_received_panel_operation_cb = panel_operation_cb,
    .avrcp_target_received_register_notification_request_cb = register_notification_request_cb,
    .avrcp_target_received_set_volume_cb = absolute_volume_cb,
};

static AVRCP_CALLBACKS_S g_avrcp_ctrl_cbks = {
    .size = sizeof(g_avrcp_ctrl_cbks),
    .avrcp_connection_state_changed_cb = ctrl_connection_state_changed_cb,
    .avrcp_received_panel_rsp_cb = panel_rsp_cb,
    .avrcp_received_notification_cb = register_notification_event_cb,
    .avrcp_received_remote_capabilities_cb = remote_capabilities_rsp_cb,
    .avrcp_received_element_attributes_cb = NULL,
    .avrcp_received_play_status_cb = get_play_status_rsp_cb
};

static avrcp_scb_t g_avrc_scb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void send_to_avrcp_service(uint8_t role, avrcp_msg_t *msg)
{
    if (role > AVRC_ROLE_CTRL)
        return;

    if (g_avrc_scb.service[role].enabled && g_avrc_scb.service[role].cb)
        g_avrc_scb.service[role].cb(msg);
}

static void avrcp_connection_state_changed(uint8_t role, BD_ADDR addr,
                                      SERVICE_PROFILE_CONNECTION_STATE state)
{
    avrcp_msg_t msg;

    switch (state) {
        case SERVICE_PROFILE_CONNECTING:
            msg.data.conn_state = AVRC_CONNECTION_STATE_CONNECTING;
        break;
            case SERVICE_PROFILE_CONNECTED:
            msg.data.conn_state = AVRC_CONNECTION_STATE_CONNECTED;
        break;
            case SERVICE_PROFILE_DISCONNECTING:
            msg.data.conn_state = AVRC_CONNECTION_STATE_DISCONNECTING;
        break;
        default:
            msg.data.conn_state = AVRC_CONNECTION_STATE_DISCONNECTED;
            break;
    }
    msg.id = CONNECTION_STATE_CHANGED;
    memcpy(msg.addr, addr, sizeof(bt_address));
    send_to_avrcp_service(role, &msg);
}

static void target_connection_state_changed_cb(BD_ADDR addr,
    SERVICE_PROFILE_CONNECTION_STATE state)
{
    avrcp_connection_state_changed(AVRC_ROLE_TARGET, addr, state);
}

static void register_notification_request_cb(BD_ADDR addr,
    SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t interval)
{
    avrcp_msg_t msg = {0};

    memcpy(msg.addr, addr, sizeof(bt_address));

    if (event == AVRCP_NOTIFICATION_VOLUME_CHANGED) {
        if (g_avrc_scb.service[AVRC_ROLE_CTRL].enabled) {
            msg.id = REGISTER_NOTIFICATION_ABSVOL_REQ;
            send_to_avrcp_service(AVRC_ROLE_CTRL, &msg);
        } else
            BT_LOGD("Controller role not support");
    } else {
        msg.id = REGISTER_NOTIFICATION_REQ;
        msg.data.notify_req.event = event + 1;
        msg.data.notify_req.interval = interval;
        send_to_avrcp_service(AVRC_ROLE_TARGET, &msg);
    }
}

static void get_play_status_request_cb(BD_ADDR addr)
{
    avrcp_msg_t msg = {0};

    msg.id = GET_PLAY_STATUS_REQ;
    memcpy(msg.addr, addr, sizeof(bt_address));
    send_to_avrcp_service(AVRC_ROLE_TARGET, &msg);
}

static void panel_operation_cb(BD_ADDR addr,
    SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state)
{
    avrcp_msg_t msg = {0};

    msg.id = PASSTHROUHT_CMD;
    msg.data.passthr_cmd.opcode = op;
    msg.data.passthr_cmd.state = state == AVRCP_PANEL_RELEASE ?
                                 AVRCP_KEY_RELEASED : AVRCP_KEY_PRESSED;
    memcpy(msg.addr, addr, sizeof(bt_address));
    send_to_avrcp_service(AVRC_ROLE_TARGET, &msg);
}

static void absolute_volume_cb(BD_ADDR addr, uint8_t volume)
{
    avrcp_msg_t msg = {0};

    msg.id = SET_ABSOLUTE_VOLUME;
    msg.data.absvol.volume = volume;
    memcpy(msg.addr, addr, sizeof(bt_address));
    /* target service don't care this event, send to controller service */
    send_to_avrcp_service(AVRC_ROLE_CTRL, &msg);
}

static void ctrl_connection_state_changed_cb(BD_ADDR addr,
                                      SERVICE_PROFILE_CONNECTION_STATE state)
{
    avrcp_connection_state_changed(AVRC_ROLE_CTRL, addr, state);
}

static void panel_rsp_cb(BD_ADDR addr,
                                SERVICE_AVRCP_RESPONSE response,
                                SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state)
{
    avrcp_msg_t msg = {0};

    memcpy(msg.addr, addr, sizeof(bt_address));
    msg.id = PASSTHROUHT_CMD_RSP;
    msg.data.passthr_rsp.cmd = op;
    msg.data.passthr_rsp.state = (state == AVRCP_PANEL_RELEASE) ?
                            AVRCP_KEY_RELEASED : AVRCP_KEY_PRESSED;
    msg.data.passthr_rsp.rsp = response;
    send_to_avrcp_service(AVRC_ROLE_CTRL, &msg);
}

static void register_notification_event_cb(BD_ADDR addr,
                                          SERVICE_AVRCP_NOTIFICATION_EVENT event,
                                          void *value)
{
    avrcp_msg_t msg = {0};

    memcpy(msg.addr, addr, 6);
    if (event == AVRCP_NOTIFICATION_VOLUME_CHANGED) {
        /* only target register volume changed notification */
        msg.id = REGISTER_NOTIFICATION_ABSVOL_RSP;
        msg.data.absvol.volume = *(uint8_t *)value;
        send_to_avrcp_service(AVRC_ROLE_TARGET, &msg);
    } else {
        msg.id = REGISTER_NOTIFICATION_RSP;
        msg.data.notify_rsp.event = event + 1;
        if (event == AVRCP_NOTIFICATION_PLAY_POS_CHANGED)
            msg.data.notify_rsp.value = *(uint32_t *)value;
        else if (event == AVRCP_NOTIFICATION_UIDS_CHANGED ||
                event == AVRCP_NOTIFICATION_ADDRESSED_PLAYER_CHANGED)
            msg.data.notify_rsp.value = *(uint16_t *)value;
        else
            msg.data.notify_rsp.value = *(uint8_t *)value;
        send_to_avrcp_service(AVRC_ROLE_CTRL, &msg);
    }
}

static void remote_capabilities_rsp_cb(BD_ADDR addr,
                                       SERVICE_AVRCP_CAPABILITY_RSP_S *capabilities)
{
    uint8_t *caps = (void *)capabilities->capability;

    while (capabilities->count) {
        switch (*caps-1) {
            case AVRCP_NOTIFICATION_MEDIA_STATUS_CHANGED:
                service_adapter_avrcp_register_notification(addr, *caps-1, 0);
                break;
            case AVRCP_NOTIFICATION_PLAY_POS_CHANGED:
                service_adapter_avrcp_register_notification(addr, *caps-1, 2);
                break;
        }
        capabilities->count--;
        caps++;
    }
}

static void get_play_status_rsp_cb(BD_ADDR addr,
                                   SERVICE_AVRCP_MEDIA_STATUS media_status,
                                   uint32_t song_length, uint32_t position)
{
    avrcp_msg_t msg = {0};

    memcpy(msg.addr, addr, 6);
    msg.id = GET_PLAY_STATUS_RSP;
    msg.data.playstatus.status = media_status;
    msg.data.playstatus.song_len = song_length;
    msg.data.playstatus.song_pos = position;

    send_to_avrcp_service(AVRC_ROLE_CTRL, &msg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bt_result_code bts_avrcp_get_play_status_response(bt_address addr, avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos)
{
    SERVICE_BT_STATUS ret;

    ret = service_adapter_avrcp_target_get_play_status_response(addr, status, song_len, song_pos);
    if (ret != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_notify_play_state_changed(bt_address addr, avrcp_play_status_t status)
{
    SERVICE_BT_STATUS ret;

    BT_LOGD("%s addr:%s, status:%d", __func__, addr_str(addr), status);
    ret = service_adapter_avrcp_target_notify_play_status_changed(addr, status);
    if (ret != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_register_volume_changed(bt_address addr)
{
    SERVICE_BT_STATUS status;

    status = service_adapter_avrcp_register_notification(addr, AVRCP_NOTIFICATION_VOLUME_CHANGED, 0);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("register volume change notification failed");
        return BT_RESULT_FAILED;
    }

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_notify_volume_changed(bt_address addr, uint8_t volume)
{
    SERVICE_BT_STATUS ret;

    BT_LOGD("%s addr:%s, new volume:%d", __func__, addr_str(addr), volume);
    ret = service_adapter_avrcp_target_notify_volume_changed(addr, volume);
    if (ret != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_set_absolute_volume(bt_address addr, uint8_t volume)
{
    SERVICE_BT_STATUS ret;

    BT_LOGD("%s addr:%s, new volume:%d", __func__, addr_str(addr), volume);
    ret = service_adapter_avrcp_set_absolute_volume(addr, volume);
    if (ret != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_notify_track_changed(bt_address addr, bool selected)
{
    SERVICE_BT_STATUS ret;

    ret = service_adapter_avrcp_target_notify_track_changed(addr, selected);
    if (ret != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_notify_play_position_changed(bt_address addr, uint32_t position)
{
    SERVICE_BT_STATUS ret;

    ret = service_adapter_avrcp_target_notify_play_position_changed(addr, position);
    if (ret != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_send_passthrough_cmd(bt_address addr, avrcp_passthr_cmd_t cmd, avrcp_key_state_t state)
{
    SERVICE_BT_STATUS status;

    status = service_adapter_avrcp_send_panel_operation(addr, cmd,
                            state == AVRCP_KEY_RELEASED ? AVRCP_PANEL_RELEASE : AVRCP_PANEL_PRESS);
    if (status != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_get_play_status(bt_address addr)
{
    SERVICE_BT_STATUS status;

    status = service_adapter_avrcp_get_play_status(addr);
    if (status != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

bt_result_code bts_avrcp_get_remote_capabilities(bt_address addr)
{
    SERVICE_BT_STATUS status;

    status = service_adapter_avrcp_get_remote_capabilities(addr, SERVICE_AVRCP_CAPABILITY_ID_EVENTS_SUPPORTED);
    if (status != SERVICE_BT_STATUS_SUCCESS)
        return BT_RESULT_FAILED;

    return BT_RESULT_SUCCESS;
}

void bts_avrcp_init(uint8_t role, bool absolute_support, avrcp_msg_callback_t callback)
{
    if (role > AVRC_ROLE_CTRL || g_avrc_scb.refs == 2)
        return;

    if (g_avrc_scb.refs == 0) {
#if defined(CONFIG_BLUETOOTH_AVRCP_TG) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
        service_adapter_avrcp_target_init(&g_avrcp_target_cbks);
#endif
#if defined(CONFIG_BLUETOOTH_AVRCP_CT) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
        service_adapter_avrcp_init(&g_avrcp_ctrl_cbks);
#endif
    }

    if (!g_avrc_scb.service[role].enabled) {
        g_avrc_scb.service[role].cb = callback;
        g_avrc_scb.service[role].enabled = true;
        g_avrc_scb.service[role].absolute_support = absolute_support;
        g_avrc_scb.refs++;
    }
}

void bts_avrcp_cleanup(uint8_t role)
{
    if (role > AVRC_ROLE_CTRL || g_avrc_scb.refs == 0)
        return;

    if (g_avrc_scb.refs > 1) {
#if defined(CONFIG_BLUETOOTH_AVRCP_TG) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
        service_adapter_avrcp_target_cleanup();
#endif
#if defined(CONFIG_BLUETOOTH_AVRCP_CT) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
        service_adapter_avrcp_cleanup();
#endif
    }

    if (g_avrc_scb.service[role].enabled) {
        memset(&g_avrc_scb.service[role], 0, sizeof(avrcp_service_t));
        g_avrc_scb.refs--;
    }
}

