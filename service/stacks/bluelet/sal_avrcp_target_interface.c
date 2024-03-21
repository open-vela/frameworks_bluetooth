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

#include "bluetooth.h"
#include "stack_adapter_avrcp.h"
#include "stack_adapter_avrcp_target.h"
#include "stack_adapter_common.h"

#include "sal.h"
#include "sal_avrcp_control_interface.h"
#include "sal_avrcp_target_interface.h"
#include "sal_bluelet.h"

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)

static void register_notification_request_cb(BD_ADDR addr,
    SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t interval);

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
static void target_connection_state_changed_cb(BD_ADDR addr,
    SERVICE_PROFILE_CONNECTION_STATE state,
    SERVICE_PROFILE_CONNECTION_REASON reason);
static void register_notification_request_cb(BD_ADDR addr,
    SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t interval);
static void get_play_status_request_cb(BD_ADDR addr);
static void panel_operation_cb(BD_ADDR addr,
    SERVICE_AVRCP_PANEL_OPERATION op,
    SERVICE_AVRCP_PANEL_STATE state);
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
static void absolute_volume_cb(BD_ADDR addr, uint8_t volume);
#endif /* CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */

static AVRCP_TARGET_CALLBACKS_S avrcp_target_cbks = {
    .size = sizeof(avrcp_target_cbks),
    .avrcp_target_received_register_notification_request_cb = register_notification_request_cb,
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    .avrcp_target_connection_state_changed_cb = target_connection_state_changed_cb,
    .avrcp_target_received_get_element_attr_request_cb = NULL,
    .avrcp_target_received_get_play_status_request_cb = get_play_status_request_cb,
    .avrcp_target_received_panel_operation_cb = panel_operation_cb,
#else
    .avrcp_target_connection_state_changed_cb = NULL,
    .avrcp_target_received_get_element_attr_request_cb = NULL,
    .avrcp_target_received_get_play_status_request_cb = NULL,
    .avrcp_target_received_panel_operation_cb = NULL,
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    .avrcp_target_received_set_volume_cb = absolute_volume_cb,
#else
    .avrcp_target_received_set_volume_cb = NULL,
#endif
};

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
static void target_connection_state_changed_cb(BD_ADDR addr,
    SERVICE_PROFILE_CONNECTION_STATE state,
    SERVICE_PROFILE_CONNECTION_REASON reason)
{
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, (void*)addr);

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

    bt_sal_avrcp_target_event_callback(msg);
}

static void get_play_status_request_cb(BD_ADDR addr)
{
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_GET_PLAY_STATUS_REQ, (void*)addr);

    if (msg == NULL)
        return;

    bt_sal_avrcp_target_event_callback(msg);
}

static void panel_operation_cb(BD_ADDR addr,
    SERVICE_AVRCP_PANEL_OPERATION op,
    SERVICE_AVRCP_PANEL_STATE state)
{
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD, (void*)addr);

    if (msg == NULL)
        return;

    msg->data.passthr_cmd.opcode = op;
    msg->data.passthr_cmd.state = state == AVRCP_PANEL_RELEASE ? AVRCP_KEY_RELEASED : AVRCP_KEY_PRESSED;

    bt_sal_avrcp_target_event_callback(msg);
}

#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */

static void register_notification_request_cb(BD_ADDR addr,
    SERVICE_AVRCP_NOTIFICATION_EVENT event, uint32_t interval)
{
    avrcp_msg_t* msg;

    msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_REQ, (void*)addr);

    if (msg == NULL)
        return;

    msg->data.notify_req.event = event + 1;
    msg->data.notify_req.interval = interval;

    if (event == AVRCP_NOTIFICATION_VOLUME_CHANGED) {
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
        bt_sal_avrcp_control_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */
    } else {
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
        bt_sal_avrcp_target_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */
    }
}

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME

static void absolute_volume_cb(BD_ADDR addr, uint8_t volume)
{
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_SET_ABSOLUTE_VOLUME, (void*)addr);

    if (msg == NULL)
        return;

    msg->data.absvol.volume = volume;
    bt_sal_avrcp_control_event_callback(msg);
}
#endif /* CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */

#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET || CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */

bt_status_t bt_sal_avrcp_target_init(void)
{
#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
    SAL_CHECK_RET(service_adapter_avrcp_target_init(&avrcp_target_cbks),
        SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_avrcp_target_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    service_adapter_avrcp_target_cleanup();
#endif
}

bt_status_t bt_sal_avrcp_target_get_play_status_rsp(bt_address_t* addr,
    avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    SAL_CHECK_RET(service_adapter_avrcp_target_get_play_status_response(
                      (void*)addr, status, song_len, song_pos),
        SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_play_status_notify(bt_address_t* addr, avrcp_play_status_t status)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    SAL_CHECK_RET(service_adapter_avrcp_target_notify_play_status_changed(
                      (void*)addr, status),
        SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_set_absolute_volume(bt_address_t* addr, uint8_t volume)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    SAL_CHECK_RET(service_adapter_avrcp_set_absolute_volume(
                      (void*)addr, volume),
        SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_notify_track_changed(bt_address_t* addr, bool selected)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    SAL_CHECK_RET(service_adapter_avrcp_target_notify_track_changed(
                      (void*)addr, selected),
        SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_notify_play_position_changed(bt_address_t* addr, uint32_t position)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    SAL_CHECK_RET(service_adapter_avrcp_target_notify_play_position_changed(
                      (void*)addr, position),
        SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_register_volume_changed(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    SAL_CHECK_RET(service_adapter_avrcp_register_notification(
                      (void*)addr, AVRCP_NOTIFICATION_VOLUME_CHANGED, 0),
        SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}
