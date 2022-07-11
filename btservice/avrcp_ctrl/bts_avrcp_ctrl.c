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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "btm_manager.h"
#include "btm_avrcp.h"
#include "bts_avrcp.h"
#include "bts_service.h"
#include "stack_adapter_avrcp.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"
#include "utils/utils.h"
#define LOG_TAG "avrcp_ctrl"
#include "log.h"

static const avrc_ctrl_callbacks_t *g_ctrl_callbacks = NULL;

static void avrcp_ctrl_connection_state_handler(bt_address addr,
                                                avrcp_connection_state_t state)
{
    BT_LOGD("%s, device: %s, connection state : %d", __func__, addr_str(addr), state);
    if (state == AVRC_CONNECTION_STATE_CONNECTED)
        service_adapter_avrcp_get_remote_capabilities(addr, SERVICE_AVRCP_CAPABILITY_ID_EVENTS_SUPPORTED);

    BT_CBACK(g_ctrl_callbacks, connection_state_cb, addr, state);
}

static void avrcp_ctrl_register_notification_rsp_handler(bt_address addr, SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t value)
{
    switch (event) {
        case AVRCP_NOTIFICATION_MEDIA_STATUS_CHANGED:
            BT_LOGD("%s, device:%s, playback:%d", __func__, addr_str(addr), value);
            BT_CBACK(g_ctrl_callbacks, play_status_changed_cb, addr, value);
            break;
        case AVRCP_NOTIFICATION_PLAY_POS_CHANGED:
            BT_CBACK(g_ctrl_callbacks, play_position_changed_cb, addr, 0, value);
            break;
        case AVRCP_NOTIFICATION_VOLUME_CHANGED:
        break;
        default:
            break;
    }
}

static void avrcp_ctrl_service_msg_process(avrcp_ctrl_msg_t* msg)
{
    switch (msg->msg_id) {
        case CONNECTION_STATE_CHANGED:
            avrcp_ctrl_connection_state_handler(msg->addr, msg->data.conn_state);
            break;
        case PASSTHROUHT_CMD_RSP: {
            avrcp_passthr_rsp_t *rsp = &msg->data.passthr_rsp;
            BT_CBACK(g_ctrl_callbacks, passthrough_rsp_cb, msg->addr, rsp->cmd, rsp->state, rsp->rsp);
            break;
        }
        case REGISTER_NOTIFICATION_RSP: {
            avrcp_notification_t *notify = &msg->data.notification;
            avrcp_ctrl_register_notification_rsp_handler(msg->addr, notify->event, notify->value);
            break;
        }
        case GET_ELEMENT_ATTRIBUTES_RSP:
            break;
        case GET_PLAY_STATUS_RSP: {
            avrcp_play_status_t *status = &msg->data.play_status;
            BT_CBACK(g_ctrl_callbacks, play_status_changed_cb, msg->addr, status->status);
            BT_CBACK(g_ctrl_callbacks, play_position_changed_cb, msg->addr, status->song_len, status->song_pos);
            break;
        }
        default:
        break;
    }
}

static void bts_avrcp_ctrl_handle_service_msg(bt_profile_id id, void* data, size_t size)
{
    if (!data)
        return;

    avrcp_ctrl_service_msg_process((avrcp_ctrl_msg_t*)data);
    free(data);
}

static void do_in_avrcp_ct_service(avrcp_ctrl_msg_t* msg)
{
    avrcp_ctrl_msg_t* ct_msg;

    ct_msg = (avrcp_ctrl_msg_t*)malloc(sizeof(avrcp_ctrl_msg_t));
    if (ct_msg == NULL)
        return;

    memcpy(ct_msg, msg, sizeof(avrcp_ctrl_msg_t));
    bts_send_uv_msg(BT_PROFILE_AV_RC_CTRL_ID, ct_msg, sizeof(avrcp_ctrl_msg_t));
}

static void adpt_connection_state_changed_cb(BD_ADDR addr,
                                      SERVICE_PROFILE_CONNECTION_STATE state)
{
    avrcp_ctrl_msg_t msg;

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
    msg.msg_id = CONNECTION_STATE_CHANGED;
    memcpy(msg.addr, addr, sizeof(bt_address));
    do_in_avrcp_ct_service(&msg);
}

static void adpt_received_panel_rsp_cb(BD_ADDR addr,
                                SERVICE_AVRCP_RESPONSE response,
                                SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state)
{
    avrcp_ctrl_msg_t msg;

    memcpy(msg.addr, addr, sizeof(bt_address));
    msg.msg_id = PASSTHROUHT_CMD_RSP;
    msg.data.passthr_rsp.cmd = op;
    msg.data.passthr_rsp.state = (state == AVRCP_PANEL_RELEASE) ?
                            AVRCP_KEY_RELEASED : AVRCP_KEY_PRESSED;
    msg.data.passthr_rsp.rsp = response;
    do_in_avrcp_ct_service(&msg);
}

static void adpt_received_notification_cb(BD_ADDR addr,
                                          SERVICE_AVRCP_NOTIFICATION_EVENT event,
                                          void *value)
{
    avrcp_ctrl_msg_t msg;

    memcpy(msg.addr, addr, 6);
    msg.msg_id = REGISTER_NOTIFICATION_RSP;
    msg.data.notification.event = event;
    if (event == AVRCP_NOTIFICATION_PLAY_POS_CHANGED)
        msg.data.notification.value = *(uint32_t *)value;
    else if (event == AVRCP_NOTIFICATION_UIDS_CHANGED || event == AVRCP_NOTIFICATION_ADDRESSED_PLAYER_CHANGED)
        msg.data.notification.value = *(uint16_t *)value;
    else
        msg.data.notification.value = *(uint8_t *)value;

    do_in_avrcp_ct_service(&msg);
}

static void adpt_received_remote_capabilities_cb(BD_ADDR addr,
                                                 SERVICE_AVRCP_CAPABILITY_RSP_S *capabilities)
{
    uint8_t *caps = (void *)capabilities->capability;

    while (capabilities->count) {
        switch (*caps-1) {
            case AVRCP_NOTIFICATION_MEDIA_STATUS_CHANGED:
            case AVRCP_NOTIFICATION_VOLUME_CHANGED:
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

static void adpt_received_play_status_cb(BD_ADDR addr,
                                         SERVICE_AVRCP_MEDIA_STATUS media_status,
                                         uint32_t song_length, uint32_t position)
{
    avrcp_ctrl_msg_t msg;

    memcpy(msg.addr, addr, 6);
    msg.msg_id = GET_PLAY_STATUS_RSP;
    msg.data.play_status.status = media_status;
    msg.data.play_status.song_len = song_length;
    msg.data.play_status.song_pos = position;

    do_in_avrcp_ct_service(&msg);
}

static AVRCP_CALLBACKS_S g_adpt_avrcp_cbks = {
    .size = sizeof(g_adpt_avrcp_cbks),
    .avrcp_connection_state_changed_cb = adpt_connection_state_changed_cb,
    .avrcp_received_panel_rsp_cb = adpt_received_panel_rsp_cb,
    .avrcp_received_notification_cb = adpt_received_notification_cb,
    .avrcp_received_remote_capabilities_cb = adpt_received_remote_capabilities_cb,
    .avrcp_received_element_attributes_cb = NULL,
    .avrcp_received_play_status_cb = adpt_received_play_status_cb
};

bt_result_code bts_avrcp_ctrl_init(const avrc_ctrl_callbacks_t *cbs)
{
    SERVICE_BT_STATUS ret = service_adapter_avrcp_init(&g_adpt_avrcp_cbks);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        return BT_RESULT_FAILED;
    }

    bts_register_profile_process(BT_PROFILE_AV_RC_CTRL_ID, bts_avrcp_ctrl_handle_service_msg);
    g_ctrl_callbacks = cbs;

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

void bts_avrcp_ctrl_cleanup(void)
{
    g_ctrl_callbacks = NULL;
    bts_unregister_profile_process(BT_PROFILE_AV_RC_CTRL_ID);
    service_adapter_avrcp_cleanup();
}