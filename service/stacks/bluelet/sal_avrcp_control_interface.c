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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_avrcp.h"
#include "stack_adapter_avrcp_target.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_avrcp_control_interface.h"
#include "sal_bluelet.h"

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL

static void ctrl_connection_state_changed_cb(BD_ADDR addr,
                                             SERVICE_PROFILE_CONNECTION_STATE state,
                                             SERVICE_PROFILE_CONNECTION_REASON reason);
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

static AVRCP_CALLBACKS_S avrcp_ctrl_cbks = {
    .size = sizeof(avrcp_ctrl_cbks),
    .avrcp_connection_state_changed_cb = ctrl_connection_state_changed_cb,
    .avrcp_received_panel_rsp_cb = panel_rsp_cb,
    .avrcp_received_notification_cb = register_notification_event_cb,
    .avrcp_received_remote_capabilities_cb = remote_capabilities_rsp_cb,
    .avrcp_received_element_attributes_cb = NULL,
    .avrcp_received_play_status_cb = get_play_status_rsp_cb
};

static void ctrl_connection_state_changed_cb(BD_ADDR addr,
                                             SERVICE_PROFILE_CONNECTION_STATE state,
                                             SERVICE_PROFILE_CONNECTION_REASON reason)
{
    avrcp_msg_t *msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, (void *)addr);

    if (msg == NULL)
        return;

    switch (state) {
    case SERVICE_PROFILE_CONNECTING:
        msg->data.conn_state.conn_state = PROFILE_STATE_CONNECTING;
        break;
    case SERVICE_PROFILE_CONNECTED:
        msg->data.conn_state.conn_state = PROFILE_STATE_CONNECTED;
        break;
    case SERVICE_PROFILE_DISCONNECTING:
        msg->data.conn_state.conn_state = PROFILE_STATE_DISCONNECTING;
        break;
    default:
        msg->data.conn_state.conn_state = PROFILE_STATE_DISCONNECTED;
        break;
    }

    msg->data.conn_state.reason = bluelet_profile_connection_reason(reason);

    bt_sal_avrcp_control_event_callback(msg);
}

static void panel_rsp_cb(BD_ADDR addr,
                         SERVICE_AVRCP_RESPONSE response,
                         SERVICE_AVRCP_PANEL_OPERATION op, SERVICE_AVRCP_PANEL_STATE state)
{
    avrcp_msg_t *msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD_RSP, (void *)addr);

    if (msg == NULL)
        return;

    msg->data.passthr_rsp.cmd = op;
    msg->data.passthr_rsp.state = (state == AVRCP_PANEL_RELEASE) ? AVRCP_KEY_RELEASED : AVRCP_KEY_PRESSED;
    msg->data.passthr_rsp.rsp = response;

    bt_sal_avrcp_control_event_callback(msg);
}

static void register_notification_event_cb(BD_ADDR addr,
                                           SERVICE_AVRCP_NOTIFICATION_EVENT event,
                                           void *value)
{
    avrcp_msg_t *msg;

    if (event == AVRCP_NOTIFICATION_VOLUME_CHANGED) {
        /* only target register volume changed notification */
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_ABSVOL_RSP, (void *)addr);
        if (msg == NULL)
            return;

        msg->data.absvol.volume = *(uint8_t *)value;
    } else {
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_RSP, (void *)addr);
        if (msg == NULL)
            return;

        msg->data.notify_rsp.event = event + 1;
        if (event == AVRCP_NOTIFICATION_PLAY_POS_CHANGED)
            msg->data.notify_rsp.value = *(uint32_t *)value;
        else if (event == AVRCP_NOTIFICATION_UIDS_CHANGED ||
                 event == AVRCP_NOTIFICATION_ADDRESSED_PLAYER_CHANGED)
            msg->data.notify_rsp.value = *(uint16_t *)value;
        else
            msg->data.notify_rsp.value = *(uint8_t *)value;
    }

    bt_sal_avrcp_control_event_callback(msg);
}

static void remote_capabilities_rsp_cb(BD_ADDR addr,
                                       SERVICE_AVRCP_CAPABILITY_RSP_S *capabilities)
{
    avrcp_msg_t *msg;
    uint8_t *caps = capabilities->capability;

    msg = avrcp_msg_new(AVRC_GET_CAPABILITY_RSP, (void *)addr);
    if (msg == NULL)
        return;

    msg->data.cap.company_id = capabilities->id;
    msg->data.cap.cap_count = capabilities->count;
    msg->data.cap.capabilities[capabilities->count] = 0;
    memcpy(msg->data.cap.capabilities, caps, capabilities->count);
    bt_sal_avrcp_control_event_callback(msg);
}

static void get_play_status_rsp_cb(BD_ADDR addr,
                                   SERVICE_AVRCP_MEDIA_STATUS media_status,
                                   uint32_t song_length, uint32_t position)
{
    avrcp_msg_t *msg = avrcp_msg_new(AVRC_GET_PLAY_STATUS_RSP, (void *)addr);

    if (msg == NULL)
        return;

    msg->data.playstatus.status = media_status;
    msg->data.playstatus.song_len = song_length;
    msg->data.playstatus.song_pos = position;

    bt_sal_avrcp_control_event_callback(msg);
}
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */

bt_status_t bt_sal_avrcp_control_init(void)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    SAL_CHECK_RET(service_adapter_avrcp_init(&avrcp_ctrl_cbks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_avrcp_control_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    service_adapter_avrcp_cleanup();
#endif
}

bt_status_t bt_sal_avrcp_control_send_pass_through_cmd(bt_address_t *bd_addr,
                                                       avrcp_passthr_cmd_t key_code,
                                                       avrcp_key_state_t key_state)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    SAL_CHECK_RET(service_adapter_avrcp_send_panel_operation((void *)bd_addr, key_code,
                                                             key_state == AVRCP_KEY_RELEASED ? AVRCP_PANEL_RELEASE : AVRCP_PANEL_PRESS),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_playback_state(bt_address_t *bd_addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    SAL_CHECK_RET(service_adapter_avrcp_get_play_status((void *)bd_addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_volume_changed_notify(bt_address_t *bd_addr, uint8_t volume)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    SAL_CHECK_RET(service_adapter_avrcp_target_notify_volume_changed((void *)bd_addr, volume),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_connect(bt_address_t *bd_addr)
{
#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_TARGET)
    SAL_CHECK_RET(service_adapter_avrcp_connect((void *)bd_addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_disconnect(bt_address_t *bd_addr)
{
#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_TARGET)
    SAL_CHECK_RET(service_adapter_avrcp_disconnect((void *)bd_addr),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_capabilities(bt_address_t *bd_addr, uint8_t cap_id)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    SAL_CHECK_RET(service_adapter_avrcp_get_remote_capabilities((void *)bd_addr, cap_id),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_register_notification(bt_address_t *bd_addr,
                                                       avrcp_notification_event_t event,
                                                       uint32_t interval)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    SAL_CHECK_RET(service_adapter_avrcp_register_notification((void *)bd_addr, event - 1, interval),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}
