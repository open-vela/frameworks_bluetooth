/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "sal_avrcp"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "bluetooth.h"
#include "bt_addr.h"
#include "sal_a2dp_sink_interface.h"
#include "sal_a2dp_source_interface.h"
#include "sal_avrcp_control_interface.h"
#include "sal_avrcp_target_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"

#include <zephyr/bluetooth/zephyr3/avrcp_cttg.h>

#include "bt_utils.h"
#include "utils/log.h"

#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_TARGET)

extern bt_status_t bt_sal_a2dp_get_role(struct bt_conn* conn, uint8_t* a2dp_role);

static void zblue_on_connected(struct bt_conn* conn);
static void zblue_on_disconnected(struct bt_conn* conn);
static void zblue_on_notify(struct bt_conn* conn, uint8_t event_id, uint8_t status);
static void zblue_on_pass_ctrl(struct bt_conn* conn, uint8_t op_id, uint8_t state);
static void zblue_on_get_play_status(struct bt_conn* conn, uint8_t cmd, uint32_t* song_len, uint32_t* song_pos, uint8_t* play_state);
static void zblue_on_get_volume(struct bt_conn* conn, uint8_t* volume);
static void zblue_on_update_id3_info(struct bt_conn* conn, struct id3_info* info);
static void zblue_on_playback_pos(struct bt_conn* conn, uint32_t pos);

static struct bt_avrcp_app_cb avrcp_cbks = {
    .connected = zblue_on_connected,
    .disconnected = zblue_on_disconnected,
    .notify = zblue_on_notify,
    .pass_ctrl = zblue_on_pass_ctrl,
    .get_play_status = zblue_on_get_play_status,
    .get_volume = zblue_on_get_volume,
    .update_id3_info = zblue_on_update_id3_info,
    .playback_pos = zblue_on_playback_pos,
};

static avrcp_passthr_cmd_t zephyr_op_2_sal_op(uint8_t op)
{
    switch (op) {
    case AVRCP_OPERATION_ID_SKIP:
        return PASSTHROUGH_CMD_ID_RESERVED;
    case AVRCP_OPERATION_ID_VOLUME_UP:
        return PASSTHROUGH_CMD_ID_VOLUME_UP;
    case AVRCP_OPERATION_ID_VOLUME_DOWN:
        return PASSTHROUGH_CMD_ID_VOLUME_DOWN;
    case AVRCP_OPERATION_ID_MUTE:
        return PASSTHROUGH_CMD_ID_MUTE;
    case AVRCP_OPERATION_ID_PLAY:
        return PASSTHROUGH_CMD_ID_PLAY;
    case AVRCP_OPERATION_ID_STOP:
        return PASSTHROUGH_CMD_ID_STOP;
    case AVRCP_OPERATION_ID_PAUSE:
        return PASSTHROUGH_CMD_ID_PAUSE;
    case AVRCP_OPERATION_ID_REWIND:
        return PASSTHROUGH_CMD_ID_REWIND;
    case AVRCP_OPERATION_ID_FAST_FORWARD:
        return PASSTHROUGH_CMD_ID_FAST_FORWARD;
    case AVRCP_OPERATION_ID_FORWARD:
        return PASSTHROUGH_CMD_ID_FORWARD;
    case AVRCP_OPERATION_ID_BACKWARD:
        return PASSTHROUGH_CMD_ID_BACKWARD;
    case AVRCP_OPERATION_ID_UNDEFINED:
        return PASSTHROUGH_CMD_ID_RESERVED;
    default:
        BT_LOGW("%s, unrecognized operation: 0x%x", __func__, op);
        return PASSTHROUGH_CMD_ID_RESERVED;
    }
}

static uint8_t sal_op_2_zephyr_op(avrcp_passthr_cmd_t op)
{
    switch (op) {
    case PASSTHROUGH_CMD_ID_VOLUME_UP:
        return AVRCP_OPERATION_ID_VOLUME_UP;
    case PASSTHROUGH_CMD_ID_VOLUME_DOWN:
        return AVRCP_OPERATION_ID_VOLUME_DOWN;
    case PASSTHROUGH_CMD_ID_MUTE:
        return AVRCP_OPERATION_ID_MUTE;
    case PASSTHROUGH_CMD_ID_PLAY:
        return AVRCP_OPERATION_ID_PLAY;
    case PASSTHROUGH_CMD_ID_STOP:
        return AVRCP_OPERATION_ID_STOP;
    case PASSTHROUGH_CMD_ID_PAUSE:
        return AVRCP_OPERATION_ID_PAUSE;
    case PASSTHROUGH_CMD_ID_REWIND:
        return AVRCP_OPERATION_ID_REWIND;
    case PASSTHROUGH_CMD_ID_FAST_FORWARD:
        return AVRCP_OPERATION_ID_FAST_FORWARD;
    case PASSTHROUGH_CMD_ID_FORWARD:
        return AVRCP_OPERATION_ID_FORWARD;
    case PASSTHROUGH_CMD_ID_BACKWARD:
        return AVRCP_OPERATION_ID_BACKWARD;
    case PASSTHROUGH_CMD_ID_RESERVED:
        return AVRCP_OPERATION_ID_UNDEFINED;
    default:
        BT_LOGW("%s, unsupported operation: 0x%x", __func__, op);
        return AVRCP_OPERATION_ID_UNDEFINED;
    }
}

static void zblue_on_connected(struct bt_conn* conn)
{
    bt_address_t bd_addr;
    avrcp_msg_t* msg;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, &bd_addr);
    msg->data.conn_state.conn_state = PROFILE_STATE_CONNECTED;
    msg->data.conn_state.reason = PROFILE_REASON_UNSPECIFIED;
    bt_sal_avrcp_control_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, &bd_addr);
    msg->data.conn_state.conn_state = PROFILE_STATE_CONNECTED;
    msg->data.conn_state.reason = PROFILE_REASON_UNSPECIFIED;
    bt_sal_avrcp_target_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */
}

static void zblue_on_disconnected(struct bt_conn* conn)
{
    bt_address_t bd_addr;
    avrcp_msg_t* msg;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, &bd_addr);
    msg->data.conn_state.conn_state = PROFILE_STATE_DISCONNECTED;
    msg->data.conn_state.reason = PROFILE_REASON_UNSPECIFIED;
    bt_sal_avrcp_control_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, &bd_addr);
    msg->data.conn_state.conn_state = PROFILE_STATE_DISCONNECTED;
    msg->data.conn_state.reason = PROFILE_REASON_UNSPECIFIED;
    bt_sal_avrcp_target_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */
}

static void zblue_on_notify(struct bt_conn* conn, uint8_t event_id, uint8_t status)
{
    bt_address_t bd_addr;
    avrcp_msg_t* msg;
    uint8_t role;
    bt_status_t get_role_status;

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    get_role_status = bt_sal_a2dp_get_role(conn, &role);
#endif

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    switch (event_id) {
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    case BT_AVRCP_EVENT_PLAYBACK_STATUS_CHANGED:
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_RSP, &bd_addr);
        msg->data.notify_rsp.event = NOTIFICATION_EVT_PALY_STATUS_CHANGED;
        msg->data.notify_rsp.value = status;
        bt_sal_avrcp_control_event_callback(msg);
        break;
    case BT_AVRCP_EVENT_TRACK_CHANGED:
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_RSP, &bd_addr);
        msg->data.notify_rsp.event = NOTIFICATION_EVT_TRACK_CHANGED;
        bt_sal_avrcp_control_event_callback(msg);
        break;
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    case BT_AVRCP_EVENT_VOLUME_CHANGED:
#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET)
        if (get_role_status != 0 || role == 0 /* SEP_SRC */) {
            /* Note: This callback can be triggered when a set absolute volume response is received */
            msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_ABSVOL_RSP, &bd_addr);
            msg->data.absvol.volume = status;
            bt_sal_avrcp_control_event_callback(msg);
        }
#elif defined(CONFIG_BLUETOOTH_AVRCP_CONTROL)
        if (get_role_status != 0 || role == 1 /* SEP_SNK */) {
            /* Note: This callback can be triggered when a set absolute volume command is received */
            msg = avrcp_msg_new(AVRC_SET_ABSOLUTE_VOLUME, &bd_addr);
            msg->data.absvol.volume = status;
            bt_sal_avrcp_control_event_callback(msg);
        }
#else
        break;
#endif
        break;
#endif /* CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */
    default:
        BT_LOGE("%s, event 0x%x not supported", __func__, event_id);
        break;
    }
}

static void zblue_on_pass_ctrl(struct bt_conn* conn, uint8_t op_id, uint8_t state)
{
    avrcp_passthr_cmd_t cmd = zephyr_op_2_sal_op(op_id);
    bt_address_t bd_addr;
    avrcp_msg_t* msg;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    if (cmd == PASSTHROUGH_CMD_ID_RESERVED) {
        BT_LOGW("%s, operation 0x%x not recognized", __func__, op_id);
        return;
    }

    switch (state) {
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    case BT_AVRCP_RSP_STATE_PASS_THROUGH_PUSHED:
        msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD_RSP, &bd_addr);
        msg->data.passthr_rsp.cmd = cmd;
        msg->data.passthr_rsp.state = AVRCP_KEY_PRESSED;
        msg->data.passthr_rsp.rsp = AVRCP_RESPONSE_ACCEPTED;
        bt_sal_avrcp_control_event_callback(msg);
        break;
    case BT_AVRCP_RSP_STATE_PASS_THROUGH_RELEASED:
        msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD_RSP, &bd_addr);
        msg->data.passthr_rsp.cmd = cmd;
        msg->data.passthr_rsp.state = AVRCP_KEY_RELEASED;
        msg->data.passthr_rsp.rsp = AVRCP_RESPONSE_ACCEPTED;
        bt_sal_avrcp_control_event_callback(msg);
        break;
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    case BT_AVRCP_CMD_STATE_PASS_THROUGH_PUSHED:
        msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD, &bd_addr);
        msg->data.passthr_cmd.opcode = cmd;
        msg->data.passthr_cmd.state = AVRCP_KEY_PRESSED;
        bt_sal_avrcp_target_event_callback(msg);
        break;
    case BT_AVRCP_CMD_STATE_PASS_THROUGH_RELEASED:
        msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD, &bd_addr);
        msg->data.passthr_cmd.opcode = cmd;
        msg->data.passthr_cmd.state = AVRCP_KEY_RELEASED;
        bt_sal_avrcp_target_event_callback(msg);
        break;
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */
    default:
        BT_LOGW("%s, operation 0x%x not handled", __func__, op_id);
        return;
    }
}

static void zblue_on_get_play_status(struct bt_conn* conn, uint8_t cmd, uint32_t* song_len, uint32_t* song_pos, uint8_t* play_state)
{
    bt_address_t bd_addr;
    avrcp_msg_t* msg;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    if (cmd) { /* Command received */
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
        msg = avrcp_msg_new(AVRC_GET_PLAY_STATUS_REQ, &bd_addr);
        bt_sal_avrcp_target_event_callback(msg);
        /* TODO: fetch values from AVRCP service */
        *song_len = 0xFFFFFFFF;
        *song_pos = 0xFFFFFFFF;
        *play_state = 0xFF;
#else
        *song_len = 0xFFFFFFFF;
        *song_pos = 0xFFFFFFFF;
        *play_state = 0xFF;
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */
    } else { /* Response received */
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
        msg = avrcp_msg_new(AVRC_GET_PLAY_STATUS_RSP, &bd_addr);
        msg->data.playstatus.status = *play_state;
        msg->data.playstatus.song_len = *song_len;
        msg->data.playstatus.song_pos = *song_pos;
        bt_sal_avrcp_control_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
    }
}

static void zblue_on_get_volume(struct bt_conn* conn, uint8_t* volume)
{
    /*
     * NOTE: This callback is triggered when the register notification command is received.
     * *volume shall be assigned with a value for the interim response.
     */
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    BT_LOGW("%s, Assuming the EVENT_VOLUME_CHANGED (0x0D) is registered", __func__);
    /* TODO: fetch value from AVRCP service */
    *volume = 0x7F;
#endif /* CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
}

static void zblue_on_update_id3_info(struct bt_conn* conn, struct id3_info* info)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    /* ToDo */
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
}

static void zblue_on_playback_pos(struct bt_conn* conn, uint32_t pos)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    bt_address_t bd_addr;
    avrcp_msg_t* msg;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_RSP, &bd_addr);
    msg->data.notify_rsp.event = NOTIFICATION_EVT_PLAY_POS_CHANGED;
    msg->data.notify_rsp.value = pos;
    bt_sal_avrcp_control_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
}

bt_status_t bt_sal_avrcp_control_init(void)
{
#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
    SAL_CHECK_RET(bt_avrcp_cttg_register_cb(&avrcp_cbks), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_init(void)
{
#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
    SAL_CHECK_RET(bt_avrcp_cttg_register_cb(&avrcp_cbks), 0);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_avrcp_control_cleanup(void)
{
}

void bt_sal_avrcp_target_cleanup(void)
{
}

bt_status_t bt_sal_avrcp_control_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_avrcp_cttg_connect(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    SAL_CHECK_RET_WITH_CONN(bt_avrcp_cttg_disconnect(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_send_pass_through_cmd(bt_controller_id_t id,
    bt_address_t* bd_addr, avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)bd_addr);
    uint8_t op_id = sal_op_2_zephyr_op(key_code);
    bool push = key_state == AVRCP_KEY_PRESSED ? true : false;

    if (op_id == AVRCP_OPERATION_ID_UNDEFINED) {
        bt_conn_unref(conn);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET_WITH_CONN(bt_avrcp_ct_pass_through_cmd(conn, op_id, push), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_register_notification(bt_controller_id_t id,
    bt_address_t* bd_addr, avrcp_notification_event_t event, uint32_t interval)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    /* No need to do */

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_set_absolute_volume(bt_controller_id_t id, bt_address_t* addr,
    uint8_t volume)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    union {
        uint8_t c_param[4]; /* 0: dev_type (meaningless for this command)
                             * 1: length of data
                             * 2: data(LSB)
                             * 3: data(MSB) */
        int32_t i_param;
    } value;

    value.c_param[0] = 0; /* Not used */
    value.c_param[1] = 1; /* length of data */
    value.c_param[2] = volume; /* data */
    value.c_param[3] = 0; /* Not used */

    SAL_CHECK_RET_WITH_CONN(bt_avrcp_ct_set_absolute_volume(conn, value.i_param), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_capabilities(bt_controller_id_t id, bt_address_t* bd_addr,
    uint8_t cap_id)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)bd_addr);

    SAL_CHECK_RET_WITH_CONN(bt_pts_avrcp_ct_get_capabilities(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_playback_state(bt_controller_id_t id, bt_address_t* bd_addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)bd_addr);

    SAL_CHECK_RET_WITH_CONN(bt_avrcp_ct_get_play_status(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_get_play_status_rsp(bt_controller_id_t id, bt_address_t* addr,
    avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    BT_LOGW("%s, not supported", __func__);
    return BT_STATUS_NOT_SUPPORTED;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_play_status_notify(bt_controller_id_t id, bt_address_t* addr,
    avrcp_play_status_t status)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    BT_LOGW("%s, NOTIFICATION_EVT_PALY_STATUS_CHANGED not supported", __func__);
    return BT_STATUS_NOT_SUPPORTED;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_notify_track_changed(bt_controller_id_t id, bt_address_t* addr,
    bool selected)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    BT_LOGW("%s, NOTIFICATION_EVT_TRACK_CHANGED not supported", __func__);
    return BT_STATUS_NOT_SUPPORTED;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_notify_play_position_changed(bt_controller_id_t id,
    bt_address_t* addr, uint32_t position)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    BT_LOGW("%s, NOTIFICATION_EVT_PLAY_POS_CHANGED not supported", __func__);
    return BT_STATUS_NOT_SUPPORTED;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_volume_changed_notify(bt_controller_id_t id,
    bt_address_t* bd_addr, uint8_t volume)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)bd_addr);

    SAL_CHECK_RET_WITH_CONN(bt_avrcp_tg_notify_change(conn, volume), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_element_attributes(bt_controller_id_t id,
    bt_address_t* bd_addr, uint8_t attrs_count, avrcp_media_attr_type_t* types)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)bd_addr);

    SAL_CHECK_RET_WITH_CONN(bt_avrcp_ct_get_id3_info(conn), 0, conn);

    bt_conn_unref(conn);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL || CONFIG_BLUETOOTH_AVRCP_TARGET */
