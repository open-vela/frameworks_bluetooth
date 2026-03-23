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
#include "bt_avrcp.h"
#include "sal_a2dp_sink_interface.h"
#include "sal_a2dp_source_interface.h"
#include "sal_avrcp_control_interface.h"
#include "sal_avrcp_target_interface.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"

#include "bt_uuid.h"
#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/sys/byteorder.h>

#include "bt_utils.h"
#include "utils/log.h"

#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_TARGET)
#define AVCTP_VER_1_4 (0x0104u)
#define AVRCP_VER_1_6 (0x0106u)

#define AVRCP_CAT_1 BIT(0) /* Player/Recorder */
#define AVRCP_CAT_2 BIT(1) /* Monitor/Amplifier */
#define AVRCP_CAT_3 BIT(2) /* Tuner */
#define AVRCP_CAT_4 BIT(3) /* Menu */

typedef enum {
    SAL_AVRCP_GET_PLAY_STATUS,
    SAL_AVRCP_REG_NTF_PLAYBACK_STATUS_CHANGED,
    SAL_AVRCP_REG_NTF_TRACK_CHANGED,
    SAL_AVRCP_REG_NTF_PLAYBACK_POS_CHANGED,
    SAL_AVRCP_REG_NTF_VOLUME_CHANGED,
} zblue_tg_msg_id;

typedef struct {
    uint8_t tid;
    zblue_tg_msg_id msg_id;
    uint8_t next_rsp;
} zblue_tg_tid_t;

typedef struct {
    uint8_t ct_tid;
    bt_list_t* tg_tid;
    bool is_cleanup; // cleanup flag，if true, free bt_a2dp_conn
    bt_address_t bd_addr;
    struct bt_conn* conn;
    struct bt_avrcp_tg* tg;
    struct bt_avrcp_ct* ct;
} zblue_avrcp_info_t;

extern bt_status_t bt_sal_a2dp_get_role(struct bt_conn* conn, uint8_t* a2dp_role);

#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
static void zblue_on_ct_connected(struct bt_conn* conn, struct bt_avrcp_ct* ct);
static void zblue_on_ct_disconnected(struct bt_avrcp_ct* ct);
static void zblue_on_ct_get_caps_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, struct net_buf* buf);
static void zblue_on_ct_unit_info_rsp(struct bt_avrcp_ct* ct, uint8_t tid, struct bt_avrcp_unit_info_rsp* rsp);
static void zblue_on_ct_subunit_info_rsp(struct bt_avrcp_ct* ct, uint8_t tid, struct bt_avrcp_subunit_info_rsp* rsp);
static void zblue_on_ct_passthrough_rsp(struct bt_avrcp_ct* ct, uint8_t tid, bt_avrcp_rsp_t result, const struct bt_avrcp_passthrough_rsp* rsp);
static void zblue_on_ct_notification_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, uint8_t event_id, struct bt_avrcp_event_data* data);
static void zblue_on_ct_get_element_attrs_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, struct net_buf* buf);
static void zblue_on_ct_get_play_status_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, struct net_buf* buf);
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
static void zblue_on_ct_set_absolute_volume_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, uint8_t absolute_volume);
#endif

static struct bt_avrcp_ct_cb avrcp_ct_cbks = {
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    .connected = zblue_on_ct_connected,
    .disconnected = zblue_on_ct_disconnected,
    .get_caps = zblue_on_ct_get_caps_rsp,
    .unit_info_rsp = zblue_on_ct_unit_info_rsp,
    .subunit_info_rsp = zblue_on_ct_subunit_info_rsp,
    .passthrough_rsp = zblue_on_ct_passthrough_rsp,
    .notification = zblue_on_ct_notification_rsp,
    .get_element_attrs = zblue_on_ct_get_element_attrs_rsp,
    .get_play_status = zblue_on_ct_get_play_status_rsp,
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    .set_absolute_volume = zblue_on_ct_set_absolute_volume_rsp,
#endif
};
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL || CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
static void zblue_on_tg_unit_info_req(struct bt_avrcp_tg* tg, uint8_t tid);
static void zblue_on_tg_subunit_info_req(struct bt_avrcp_tg* tg, uint8_t tid);
static void zblue_on_tg_passthrough_req(struct bt_avrcp_tg* tg, uint8_t tid, struct net_buf* buf);
static void zblue_on_tg_get_play_status_req(struct bt_avrcp_tg* tg, uint8_t tid);
#endif

static void zblue_on_tg_connected(struct bt_conn* conn, struct bt_avrcp_tg* tg);
static void zblue_on_tg_disconnected(struct bt_avrcp_tg* tg);
static void zblue_on_tg_get_caps_req(struct bt_avrcp_tg* tg, uint8_t tid, uint8_t cap_id);
static void zblue_on_tg_register_notification_req(struct bt_avrcp_tg* tg, uint8_t tid, uint8_t event_id, uint32_t interval);

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
static void zblue_on_tg_set_absolute_volume_req(struct bt_avrcp_tg* tg, uint8_t tid, uint8_t absolute_volume);
#endif

static struct bt_avrcp_tg_cb avrcp_tg_cbks = {
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    .unit_info_req = zblue_on_tg_unit_info_req,
    .subunit_info_req = zblue_on_tg_subunit_info_req,
    .passthrough_req = zblue_on_tg_passthrough_req,
    .get_play_status = zblue_on_tg_get_play_status_req,
#endif
    .connected = zblue_on_tg_connected,
    .disconnected = zblue_on_tg_disconnected,
    .get_caps = zblue_on_tg_get_caps_req,
    .register_notification = zblue_on_tg_register_notification_req,
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    .set_absolute_volume = zblue_on_tg_set_absolute_volume_req,
#endif
};
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET || CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */

static bt_status_t bt_sal_avrcp_disconnect(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data);

#ifdef AVRCP_SDP_BY_APP
#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static struct bt_sdp_attribute avrcp_ct_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS) },
            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_CONTROLLER_SVCLASS) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL) }, ) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(AVCTP_VER_1_4) }, ) }, )),
    /* C1: Browsing not supported */
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(AVRCP_VER_1_6) }, ) }, )),
    BT_SDP_SUPPORTED_FEATURES(AVRCP_CAT_1 | AVRCP_CAT_2),
    /* O: Provider Name not presented */
    BT_SDP_SERVICE_NAME("AVRCP Controller"),
};

static struct bt_sdp_record avrcp_ct_rec = BT_SDP_RECORD(avrcp_ct_attrs);
#endif

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static struct bt_sdp_attribute avrcp_tg_attrs[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_TARGET_SVCLASS) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST({ BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                                          BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL) }, ) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_UUID_AVCTP_VAL) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(AVCTP_VER_1_4) }, ) }, )),
    /* C2: Cover Art not supported */
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_AV_REMOTE_SVCLASS) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(AVRCP_VER_1_6) }, ) }, )),
    BT_SDP_SUPPORTED_FEATURES(AVRCP_CAT_1 | AVRCP_CAT_2),
    /* O: Provider Name not presented */
    BT_SDP_SERVICE_NAME("AVRCP Target"),
};

static struct bt_sdp_record avrcp_tg_rec = BT_SDP_RECORD(avrcp_tg_attrs);
#endif
#endif

static bt_list_t* bt_avrcp_conn = NULL;
#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static bool avrcp_ct_registered = false;
#endif

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static bool avrcp_tg_registered = false;
#endif

NET_BUF_POOL_DEFINE(bt_avrcp_tx_pool, CONFIG_BT_MAX_CONN,
    BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static const uint8_t bt_supported_avrcp_events[] = {
    BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED,
    BT_AVRCP_EVT_TRACK_CHANGED,
    BT_AVRCP_EVT_PLAYBACK_POS_CHANGED,
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    BT_AVRCP_EVT_VOLUME_CHANGED,
#endif
};
#endif

static avrcp_passthr_cmd_t zephyr_op_2_sal_op(uint8_t op)
{
    switch (op) {
    case BT_AVRCP_OPID_SELECT:
        return PASSTHROUGH_CMD_ID_SELECT;
    case BT_AVRCP_OPID_UP:
        return PASSTHROUGH_CMD_ID_UP;
    case BT_AVRCP_OPID_DOWN:
        return PASSTHROUGH_CMD_ID_DOWN;
    case BT_AVRCP_OPID_LEFT:
        return PASSTHROUGH_CMD_ID_LEFT;
    case BT_AVRCP_OPID_RIGHT:
        return PASSTHROUGH_CMD_ID_RIGHT;
    case BT_AVRCP_OPID_RIGHT_UP:
        return PASSTHROUGH_CMD_ID_RIGHT_UP;
    case BT_AVRCP_OPID_RIGHT_DOWN:
        return PASSTHROUGH_CMD_ID_RIGHT_DOWN;
    case BT_AVRCP_OPID_LEFT_UP:
        return PASSTHROUGH_CMD_ID_LEFT_UP;
    case BT_AVRCP_OPID_LEFT_DOWN:
        return PASSTHROUGH_CMD_ID_LEFT_DOWN;
    case BT_AVRCP_OPID_ROOT_MENU:
        return PASSTHROUGH_CMD_ID_ROOT_MENU;
    case BT_AVRCP_OPID_SETUP_MENU:
        return PASSTHROUGH_CMD_ID_SETUP_MENU;
    case BT_AVRCP_OPID_CONTENTS_MENU:
        return PASSTHROUGH_CMD_ID_CONTENTS_MENU;
    case BT_AVRCP_OPID_FAVORITE_MENU:
        return PASSTHROUGH_CMD_ID_FAVORITE_MENU;
    case BT_AVRCP_OPID_EXIT:
        return PASSTHROUGH_CMD_ID_EXIT;
    case BT_AVRCP_OPID_0:
        return PASSTHROUGH_CMD_ID_0;
    case BT_AVRCP_OPID_1:
        return PASSTHROUGH_CMD_ID_1;
    case BT_AVRCP_OPID_2:
        return PASSTHROUGH_CMD_ID_2;
    case BT_AVRCP_OPID_3:
        return PASSTHROUGH_CMD_ID_3;
    case BT_AVRCP_OPID_4:
        return PASSTHROUGH_CMD_ID_4;
    case BT_AVRCP_OPID_5:
        return PASSTHROUGH_CMD_ID_5;
    case BT_AVRCP_OPID_6:
        return PASSTHROUGH_CMD_ID_6;
    case BT_AVRCP_OPID_7:
        return PASSTHROUGH_CMD_ID_7;
    case BT_AVRCP_OPID_8:
        return PASSTHROUGH_CMD_ID_8;
    case BT_AVRCP_OPID_9:
        return PASSTHROUGH_CMD_ID_9;
    case BT_AVRCP_OPID_DOT:
        return PASSTHROUGH_CMD_ID_DOT;
    case BT_AVRCP_OPID_ENTER:
        return PASSTHROUGH_CMD_ID_ENTER;
    case BT_AVRCP_OPID_CLEAR:
        return PASSTHROUGH_CMD_ID_CLEAR;
    case BT_AVRCP_OPID_CHANNEL_UP:
        return PASSTHROUGH_CMD_ID_CHANNEL_UP;
    case BT_AVRCP_OPID_CHANNEL_DOWN:
        return PASSTHROUGH_CMD_ID_CHANNEL_DOWN;
    case BT_AVRCP_OPID_PREVIOUS_CHANNEL:
        return PASSTHROUGH_CMD_ID_PREVIOUS_CHANNEL;
    case BT_AVRCP_OPID_SOUND_SELECT:
        return PASSTHROUGH_CMD_ID_SOUND_SELECT;
    case BT_AVRCP_OPID_INPUT_SELECT:
        return PASSTHROUGH_CMD_ID_INPUT_SELECT;
    case BT_AVRCP_OPID_DISPLAY_INFORMATION:
        return PASSTHROUGH_CMD_ID_DISPLAY_INFO;
    case BT_AVRCP_OPID_HELP:
        return PASSTHROUGH_CMD_ID_HELP;
    case BT_AVRCP_OPID_PAGE_UP:
        return PASSTHROUGH_CMD_ID_PAGE_UP;
    case BT_AVRCP_OPID_PAGE_DOWN:
        return PASSTHROUGH_CMD_ID_PAGE_DOWN;
    case BT_AVRCP_OPID_POWER:
        return PASSTHROUGH_CMD_ID_POWER;
    case BT_AVRCP_OPID_VOLUME_UP:
        return PASSTHROUGH_CMD_ID_VOLUME_UP;
    case BT_AVRCP_OPID_VOLUME_DOWN:
        return PASSTHROUGH_CMD_ID_VOLUME_DOWN;
    case BT_AVRCP_OPID_MUTE:
        return PASSTHROUGH_CMD_ID_MUTE;
    case BT_AVRCP_OPID_PLAY:
        return PASSTHROUGH_CMD_ID_PLAY;
    case BT_AVRCP_OPID_STOP:
        return PASSTHROUGH_CMD_ID_STOP;
    case BT_AVRCP_OPID_PAUSE:
        return PASSTHROUGH_CMD_ID_PAUSE;
    case BT_AVRCP_OPID_RECORD:
        return PASSTHROUGH_CMD_ID_RECORD;
    case BT_AVRCP_OPID_REWIND:
        return PASSTHROUGH_CMD_ID_REWIND;
    case BT_AVRCP_OPID_FAST_FORWARD:
        return PASSTHROUGH_CMD_ID_FAST_FORWARD;
    case BT_AVRCP_OPID_EJECT:
        return PASSTHROUGH_CMD_ID_EJECT;
    case BT_AVRCP_OPID_FORWARD:
        return PASSTHROUGH_CMD_ID_FORWARD;
    case BT_AVRCP_OPID_BACKWARD:
        return PASSTHROUGH_CMD_ID_BACKWARD;
    case BT_AVRCP_OPID_ANGLE:
        return PASSTHROUGH_CMD_ID_ANGLE;
    case BT_AVRCP_OPID_SUBPICTURE:
        return PASSTHROUGH_CMD_ID_SUBPICTURE;
    case BT_AVRCP_OPID_F1:
        return PASSTHROUGH_CMD_ID_F1;
    case BT_AVRCP_OPID_F2:
        return PASSTHROUGH_CMD_ID_F2;
    case BT_AVRCP_OPID_F3:
        return PASSTHROUGH_CMD_ID_F3;
    case BT_AVRCP_OPID_F4:
        return PASSTHROUGH_CMD_ID_F4;
    case BT_AVRCP_OPID_F5:
        return PASSTHROUGH_CMD_ID_F5;
    case BT_AVRCP_OPID_VENDOR_UNIQUE:
        return PASSTHROUGH_CMD_ID_VENDOR_UNIQUE;
    default:
        BT_LOGW("%s, unrecognized operation: 0x%x", __func__, op);
        return PASSTHROUGH_CMD_ID_RESERVED;
    }
}

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
static uint8_t sal_op_2_zephyr_op(avrcp_passthr_cmd_t op)
{
    switch (op) {
    case PASSTHROUGH_CMD_ID_SELECT:
        return BT_AVRCP_OPID_SELECT;
    case PASSTHROUGH_CMD_ID_UP:
        return BT_AVRCP_OPID_UP;
    case PASSTHROUGH_CMD_ID_DOWN:
        return BT_AVRCP_OPID_DOWN;
    case PASSTHROUGH_CMD_ID_LEFT:
        return BT_AVRCP_OPID_LEFT;
    case PASSTHROUGH_CMD_ID_RIGHT:
        return BT_AVRCP_OPID_RIGHT;
    case PASSTHROUGH_CMD_ID_RIGHT_UP:
        return BT_AVRCP_OPID_RIGHT_UP;
    case PASSTHROUGH_CMD_ID_RIGHT_DOWN:
        return BT_AVRCP_OPID_RIGHT_DOWN;
    case PASSTHROUGH_CMD_ID_LEFT_UP:
        return BT_AVRCP_OPID_LEFT_UP;
    case PASSTHROUGH_CMD_ID_LEFT_DOWN:
        return BT_AVRCP_OPID_LEFT_DOWN;
    case PASSTHROUGH_CMD_ID_ROOT_MENU:
        return BT_AVRCP_OPID_ROOT_MENU;
    case PASSTHROUGH_CMD_ID_SETUP_MENU:
        return BT_AVRCP_OPID_SETUP_MENU;
    case PASSTHROUGH_CMD_ID_CONTENTS_MENU:
        return BT_AVRCP_OPID_CONTENTS_MENU;
    case PASSTHROUGH_CMD_ID_FAVORITE_MENU:
        return BT_AVRCP_OPID_FAVORITE_MENU;
    case PASSTHROUGH_CMD_ID_EXIT:
        return BT_AVRCP_OPID_EXIT;
    case PASSTHROUGH_CMD_ID_0:
        return BT_AVRCP_OPID_0;
    case PASSTHROUGH_CMD_ID_1:
        return BT_AVRCP_OPID_1;
    case PASSTHROUGH_CMD_ID_2:
        return BT_AVRCP_OPID_2;
    case PASSTHROUGH_CMD_ID_3:
        return BT_AVRCP_OPID_3;
    case PASSTHROUGH_CMD_ID_4:
        return BT_AVRCP_OPID_4;
    case PASSTHROUGH_CMD_ID_5:
        return BT_AVRCP_OPID_5;
    case PASSTHROUGH_CMD_ID_6:
        return BT_AVRCP_OPID_6;
    case PASSTHROUGH_CMD_ID_7:
        return BT_AVRCP_OPID_7;
    case PASSTHROUGH_CMD_ID_8:
        return BT_AVRCP_OPID_8;
    case PASSTHROUGH_CMD_ID_9:
        return BT_AVRCP_OPID_9;
    case PASSTHROUGH_CMD_ID_DOT:
        return BT_AVRCP_OPID_DOT;
    case PASSTHROUGH_CMD_ID_ENTER:
        return BT_AVRCP_OPID_ENTER;
    case PASSTHROUGH_CMD_ID_CLEAR:
        return BT_AVRCP_OPID_CLEAR;
    case PASSTHROUGH_CMD_ID_CHANNEL_UP:
        return BT_AVRCP_OPID_CHANNEL_UP;
    case PASSTHROUGH_CMD_ID_CHANNEL_DOWN:
        return BT_AVRCP_OPID_CHANNEL_DOWN;
    case PASSTHROUGH_CMD_ID_PREVIOUS_CHANNEL:
        return BT_AVRCP_OPID_PREVIOUS_CHANNEL;
    case PASSTHROUGH_CMD_ID_SOUND_SELECT:
        return BT_AVRCP_OPID_SOUND_SELECT;
    case PASSTHROUGH_CMD_ID_INPUT_SELECT:
        return BT_AVRCP_OPID_INPUT_SELECT;
    case PASSTHROUGH_CMD_ID_DISPLAY_INFO:
        return BT_AVRCP_OPID_DISPLAY_INFORMATION;
    case PASSTHROUGH_CMD_ID_HELP:
        return BT_AVRCP_OPID_HELP;
    case PASSTHROUGH_CMD_ID_PAGE_UP:
        return BT_AVRCP_OPID_PAGE_UP;
    case PASSTHROUGH_CMD_ID_PAGE_DOWN:
        return BT_AVRCP_OPID_PAGE_DOWN;
    case PASSTHROUGH_CMD_ID_POWER:
        return BT_AVRCP_OPID_POWER;
    case PASSTHROUGH_CMD_ID_VOLUME_UP:
        return BT_AVRCP_OPID_VOLUME_UP;
    case PASSTHROUGH_CMD_ID_VOLUME_DOWN:
        return BT_AVRCP_OPID_VOLUME_DOWN;
    case PASSTHROUGH_CMD_ID_MUTE:
        return BT_AVRCP_OPID_MUTE;
    case PASSTHROUGH_CMD_ID_PLAY:
        return BT_AVRCP_OPID_PLAY;
    case PASSTHROUGH_CMD_ID_STOP:
        return BT_AVRCP_OPID_STOP;
    case PASSTHROUGH_CMD_ID_PAUSE:
        return BT_AVRCP_OPID_PAUSE;
    case PASSTHROUGH_CMD_ID_RECORD:
        return BT_AVRCP_OPID_RECORD;
    case PASSTHROUGH_CMD_ID_REWIND:
        return BT_AVRCP_OPID_REWIND;
    case PASSTHROUGH_CMD_ID_FAST_FORWARD:
        return BT_AVRCP_OPID_FAST_FORWARD;
    case PASSTHROUGH_CMD_ID_EJECT:
        return BT_AVRCP_OPID_EJECT;
    case PASSTHROUGH_CMD_ID_FORWARD:
        return BT_AVRCP_OPID_FORWARD;
    case PASSTHROUGH_CMD_ID_BACKWARD:
        return BT_AVRCP_OPID_BACKWARD;
    case PASSTHROUGH_CMD_ID_ANGLE:
        return BT_AVRCP_OPID_ANGLE;
    case PASSTHROUGH_CMD_ID_SUBPICTURE:
        return BT_AVRCP_OPID_SUBPICTURE;
    case PASSTHROUGH_CMD_ID_F1:
        return BT_AVRCP_OPID_F1;
    case PASSTHROUGH_CMD_ID_F2:
        return BT_AVRCP_OPID_F2;
    case PASSTHROUGH_CMD_ID_F3:
        return BT_AVRCP_OPID_F3;
    case PASSTHROUGH_CMD_ID_F4:
        return BT_AVRCP_OPID_F4;
    case PASSTHROUGH_CMD_ID_F5:
        return BT_AVRCP_OPID_F5;
    case PASSTHROUGH_CMD_ID_VENDOR_UNIQUE:
        return BT_AVRCP_OPID_VENDOR_UNIQUE;
    default:
        BT_LOGW("%s, unsupported operation: 0x%x", __func__, op);
        return PASSTHROUGH_CMD_ID_RESERVED;
    }
}

static bt_status_t sal_event_2_zephyr_event(bt_avrcp_evt_t* out, avrcp_notification_event_t in)
{
    bt_status_t status = BT_STATUS_SUCCESS;

    switch (in) {
    case NOTIFICATION_EVT_PALY_STATUS_CHANGED:
        *out = BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED;
        break;
    case NOTIFICATION_EVT_TRACK_CHANGED:
        *out = BT_AVRCP_EVT_TRACK_CHANGED;
        break;
    case NOTIFICATION_EVT_PLAY_POS_CHANGED:
        *out = BT_AVRCP_EVT_PLAYBACK_POS_CHANGED;
        break;
    case NOTIFICATION_EVT_VOLUME_CHANGED:
        *out = BT_AVRCP_EVT_VOLUME_CHANGED;
        break;
    default:
        BT_LOGW("%s, unsupported notification event: 0x%x", __func__, in);
        return BT_STATUS_PARM_INVALID;
    }

    return status;
}
#endif

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static bt_status_t zephyr_event_2_sal_event(avrcp_notification_event_t* out, uint8_t in)
{
    bt_status_t status = BT_STATUS_SUCCESS;

    switch (in) {
    case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
        *out = NOTIFICATION_EVT_PALY_STATUS_CHANGED;
        break;
    case BT_AVRCP_EVT_TRACK_CHANGED:
        *out = NOTIFICATION_EVT_TRACK_CHANGED;
        break;
    case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
        *out = NOTIFICATION_EVT_PLAY_POS_CHANGED;
        break;
    case BT_AVRCP_EVT_VOLUME_CHANGED:
        *out = NOTIFICATION_EVT_VOLUME_CHANGED;
        break;
    default:
        BT_LOGW("%s, unsupported notification event: 0x%x", __func__, in);
        return BT_STATUS_PARM_INVALID;
    }

    return status;
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
static bt_avrcp_playback_status_t sal_playback_state_2_zephyr_state(avrcp_play_status_t state)
{
    switch (state) {
    case PLAY_STATUS_STOPPED:
        return BT_AVRCP_PLAYBACK_STATUS_STOPPED;
    case PLAY_STATUS_PLAYING:
        return BT_AVRCP_PLAYBACK_STATUS_PLAYING;
    case PLAY_STATUS_PAUSED:
        return BT_AVRCP_PLAYBACK_STATUS_PAUSED;
    case PLAY_STATUS_FWD_SEEK:
        return BT_AVRCP_PLAYBACK_STATUS_FWD_SEEK;
    case PLAY_STATUS_REV_SEEK:
        return BT_AVRCP_PLAYBACK_STATUS_REV_SEEK;
    default:
        return BT_AVRCP_PLAYBACK_STATUS_ERROR;
    }
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
static avrcp_play_status_t zblue_playback_state_2_sal_state(bt_avrcp_playback_status_t state)
{
    switch (state) {
    case BT_AVRCP_PLAYBACK_STATUS_STOPPED:
        return PLAY_STATUS_STOPPED;
    case BT_AVRCP_PLAYBACK_STATUS_PLAYING:
        return PLAY_STATUS_PLAYING;
    case BT_AVRCP_PLAYBACK_STATUS_PAUSED:
        return PLAY_STATUS_PAUSED;
    case BT_AVRCP_PLAYBACK_STATUS_FWD_SEEK:
        return PLAY_STATUS_FWD_SEEK;
    case BT_AVRCP_PLAYBACK_STATUS_REV_SEEK:
        return PLAY_STATUS_REV_SEEK;
    default:
        return PLAY_STATUS_ERROR;
    }
}
#endif

static bool bt_avrcp_info_find_by_conn(void* data, void* context)
{
    zblue_avrcp_info_t* avrcp_info = (zblue_avrcp_info_t*)data;
    if (!avrcp_info)
        return false;

    return avrcp_info->conn == context;
}

static bool bt_avrcp_info_find_addr(void* data, void* context)
{
    zblue_avrcp_info_t* avrcp_info = (zblue_avrcp_info_t*)data;
    if (!avrcp_info || !context)
        return false;

    return memcmp(&avrcp_info->bd_addr, context, sizeof(bt_address_t)) == 0;
}

#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static bool bt_avrcp_info_find_by_ct(void* data, void* context)
{
    zblue_avrcp_info_t* avrcp_info = (zblue_avrcp_info_t*)data;
    if (!avrcp_info)
        return false;

    return avrcp_info->ct == context;
}
#endif

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static bool bt_avrcp_info_find_by_tg(void* data, void* context)
{
    zblue_avrcp_info_t* avrcp_info = (zblue_avrcp_info_t*)data;
    if (!avrcp_info)
        return false;

    return avrcp_info->tg == context;
}
#endif

static bool bt_avrcp_tg_tid_find_by_msg_id(void* data, void* context)
{
    zblue_tg_tid_t* tid_info = (zblue_tg_tid_t*)data;
    if (!tid_info)
        return false;

    return tid_info->msg_id == *(zblue_tg_msg_id*)context;
}

static int tg_get_and_remove_tid(zblue_avrcp_info_t* avrcp_info, zblue_tg_msg_id msg_id)
{
    zblue_tg_tid_t* tid_info = bt_list_find(avrcp_info->tg_tid, bt_avrcp_tg_tid_find_by_msg_id, &msg_id);
    int tid = -1;
    if (!tid_info)
        return tid;

    tid = tid_info->tid;

    if (tid_info->next_rsp == BT_AVRCP_RSP_INTERIM) {
        tid_info->next_rsp = BT_AVRCP_RSP_CHANGED;
        return tid;
    }

    bt_list_remove(avrcp_info->tg_tid, tid_info);
    return tid;
}

static uint8_t get_next_ct_tid(zblue_avrcp_info_t* avrcp_info)
{
    uint8_t ret = avrcp_info->ct_tid;

    avrcp_info->ct_tid++;
    avrcp_info->ct_tid &= 0x0F;

    return ret;
}

static void bt_list_remove_avrcp_info(zblue_avrcp_info_t* avrcp_info)
{
    bool is_cleanup = avrcp_info->is_cleanup;

    if (!avrcp_info->ct && !avrcp_info->tg && bt_avrcp_conn) {
        bt_list_free(avrcp_info->tg_tid);
        bt_list_remove(bt_avrcp_conn, avrcp_info);
    }

    if (is_cleanup && bt_list_length(bt_avrcp_conn) == 0) {
        bt_list_free(bt_avrcp_conn);
        bt_avrcp_conn = NULL;
    }
}

static zblue_avrcp_info_t* bt_avrcp_create_avrcp_info(struct bt_conn* conn)
{
    zblue_avrcp_info_t* avrcp_info;

    avrcp_info = calloc(1, sizeof(zblue_avrcp_info_t));
    avrcp_info->tg_tid = bt_list_new(free);
    avrcp_info->conn = conn;

    if (bt_sal_get_remote_address(conn, &avrcp_info->bd_addr) != BT_STATUS_SUCCESS) {
        bt_list_free(avrcp_info->tg_tid);
        free(avrcp_info);
        return NULL;
    }

    bt_list_add_tail(bt_avrcp_conn, avrcp_info);

    return avrcp_info;
}

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
static void avrcp_on_connection_state_changed(bt_address_t* bd_addr,
    profile_connection_state_t state);
static void zblue_on_ct_connected(struct bt_conn* conn, struct bt_avrcp_ct* ct)
{
    zblue_avrcp_info_t* avrcp_info;

    if (!bt_avrcp_conn) {
        BT_LOGW("%s, bt_avrcp_conn not initialized", __func__);
        return;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_conn, conn);
    if (!avrcp_info)
        avrcp_info = bt_avrcp_create_avrcp_info(conn);

    if (!avrcp_info)
        return;

    avrcp_info->ct = ct;

    avrcp_on_connection_state_changed(&avrcp_info->bd_addr, PROFILE_STATE_CONNECTED);
    bt_sal_cm_profile_connected_callback(&avrcp_info->bd_addr, PROFILE_AVRCP_CT, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&avrcp_info->bd_addr, PROFILE_AVRCP_CT, CONN_ID_DEFAULT, PRIMARY_ADAPTER, bt_sal_avrcp_disconnect, NULL);
}

static void zblue_on_ct_disconnected(struct bt_avrcp_ct* ct)
{
    zblue_avrcp_info_t* avrcp_info;

    if (!bt_avrcp_conn) {
        BT_LOGW("%s, bt_avrcp_conn not initialized", __func__);
        return;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_ct, ct);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    avrcp_info->ct = NULL;

    avrcp_on_connection_state_changed(&avrcp_info->bd_addr, PROFILE_STATE_DISCONNECTED);
    bt_sal_cm_profile_disconnected_callback(&avrcp_info->bd_addr, PROFILE_AVRCP_CT, CONN_ID_DEFAULT);

    bt_list_remove_avrcp_info(avrcp_info);
}

static void zblue_on_ct_get_caps_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, struct net_buf* buf)
{
    struct bt_avrcp_get_caps_rsp* rsp;
    zblue_avrcp_info_t* avrcp_info;
    avrcp_msg_t* msg;

    if (status != BT_AVRCP_STATUS_SUCCESS)
        return;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_ct, ct);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    if (buf == NULL)
        return;

    if (buf->len < sizeof(*rsp)) {
        BT_LOGW("Invalid response data length");
        return;
    }

    rsp = net_buf_pull_mem(buf, sizeof(*rsp));
    if (buf->len < rsp->cap_cnt) {
        BT_LOGW("incompleted message for supported EventID ");
        return;
    }

    if (rsp->cap_id == BT_AVRCP_CAP_COMPANY_ID)
        return;

    net_buf_pull_mem(buf, rsp->cap_cnt);

    msg = avrcp_msg_new(AVRC_GET_CAPABILITY_RSP, &avrcp_info->bd_addr);
    if (msg == NULL)
        return;

    if (rsp->cap_cnt > sizeof(msg->data.cap.capabilities)) {
        avrcp_msg_destory(msg);
        return;
    }

    msg->data.cap.company_id = 0;
    msg->data.cap.cap_count = rsp->cap_cnt;
    msg->data.cap.capabilities[rsp->cap_cnt] = 0;

    memcpy(msg->data.cap.capabilities, rsp->cap, rsp->cap_cnt);
    bt_sal_avrcp_control_event_callback(msg);
}

static void zblue_on_ct_unit_info_rsp(struct bt_avrcp_ct* ct, uint8_t tid, struct bt_avrcp_unit_info_rsp* rsp)
{
    BT_LOGW("%s, not supported", __func__);
}

static void zblue_on_ct_subunit_info_rsp(struct bt_avrcp_ct* ct, uint8_t tid, struct bt_avrcp_subunit_info_rsp* rsp)
{
    BT_LOGW("%s, not supported", __func__);
}

static void zblue_on_ct_passthrough_rsp(struct bt_avrcp_ct* ct, uint8_t tid, bt_avrcp_rsp_t result, const struct bt_avrcp_passthrough_rsp* rsp)
{
    zblue_avrcp_info_t* avrcp_info;
    avrcp_passthr_cmd_t cmd;
    avrcp_msg_t* msg;
    uint8_t op_id;
    uint8_t state;

    state = BT_AVRCP_PASSTHROUGH_GET_STATE(rsp);
    op_id = BT_AVRCP_PASSTHROUGH_GET_OPID(rsp);
    cmd = zephyr_op_2_sal_op(op_id);

    if (cmd == PASSTHROUGH_CMD_ID_RESERVED) {
        BT_LOGW("%s, operation 0x%x not recognized", __func__, op_id);
        return;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_ct, ct);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD_RSP, &avrcp_info->bd_addr);
    if (msg == NULL)
        return;

    msg->data.passthr_rsp.cmd = cmd;
    msg->data.passthr_rsp.state = (state == BT_AVRCP_BUTTON_PRESSED) ? AVRCP_KEY_PRESSED : AVRCP_KEY_RELEASED;
    msg->data.passthr_rsp.rsp = result;

    bt_sal_avrcp_control_event_callback(msg);
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
static void zblue_on_ct_set_absolute_volume_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, uint8_t absolute_volume)
{
    if (status == BT_AVRCP_STATUS_SUCCESS) {
        BT_LOGD("AVRCP set absolute volume rsp: volume=0x%02x", absolute_volume);
    } else {
        BT_LOGW("AVRCP set absolute volume failed");
    }
}
#endif

#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static void zblue_on_ct_notification_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, uint8_t event_id, struct bt_avrcp_event_data* data)
{
    zblue_avrcp_info_t* avrcp_info;
    avrcp_msg_t* msg;

    if (status != BT_AVRCP_STATUS_SUCCESS)
        return;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_ct, ct);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    switch (event_id) {
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_RSP, &avrcp_info->bd_addr);
        msg->data.notify_rsp.event = NOTIFICATION_EVT_PALY_STATUS_CHANGED;
        msg->data.notify_rsp.value = data->play_status;
        bt_sal_avrcp_control_event_callback(msg);
        break;
    case BT_AVRCP_EVT_TRACK_CHANGED:
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_RSP, &avrcp_info->bd_addr);
        msg->data.notify_rsp.event = NOTIFICATION_EVT_TRACK_CHANGED;
        bt_sal_avrcp_control_event_callback(msg);
        break;
    case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_RSP, &avrcp_info->bd_addr);
        msg->data.notify_rsp.event = NOTIFICATION_EVT_PLAY_POS_CHANGED;
        msg->data.notify_rsp.value = data->playback_pos;
        bt_sal_avrcp_control_event_callback(msg);
        break;
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    case BT_AVRCP_EVT_VOLUME_CHANGED: {
        uint8_t role;
        bt_status_t get_role_status;

        get_role_status = bt_sal_a2dp_get_role(avrcp_info->conn, &role);
        if (get_role_status != BT_STATUS_SUCCESS || role == 0 /* SEP_SRC */) {
            msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_ABSVOL_RSP, &avrcp_info->bd_addr);
            msg->data.absvol.volume = data->absolute_volume;
            bt_sal_avrcp_target_event_callback(msg);
        }
        break;
    }
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */
#endif /* CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */
    default:
        BT_LOGE("%s, event 0x%x not supported", __func__, event_id);
        break;
    }
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
static void zblue_on_ct_get_element_attrs_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, struct net_buf* buf)
{
    const struct bt_avrcp_get_element_attrs_rsp* rsp;
    struct bt_avrcp_media_attr* attr;
    zblue_avrcp_info_t* avrcp_info;

    if (status != BT_AVRCP_STATUS_SUCCESS)
        return;

    if (buf == NULL)
        return;

    if (buf->len < sizeof(*rsp)) {
        BT_LOGW("Invalid GetElementAttributes response length: %d", buf->len);
        return;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_ct, ct);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    rsp = net_buf_pull_mem(buf, sizeof(*rsp));

    avrcp_msg_t* msg = avrcp_msg_new(AVRC_GET_ELEMENT_ATTRIBUTES_RSP, &avrcp_info->bd_addr);
    if (msg == NULL)
        return;

    memset(&msg->data.attrs, 0, sizeof(rc_element_attrs_t));
    msg->data.attrs.count = rsp->num_attrs;

    for (int i = 0; i < rsp->num_attrs && i < AVRCP_MAX_ATTR_COUNT; i++) {
        if (buf->len < sizeof(struct bt_avrcp_media_attr)) {
            BT_LOGW("incompleted message");
            break;
        }

        attr = net_buf_pull_mem(buf, sizeof(struct bt_avrcp_media_attr));

        msg->data.attrs.types[i] = sys_be32_to_cpu(attr->attr_id);
        msg->data.attrs.chr_sets[i] = sys_be16_to_cpu(attr->charset_id);
        uint16_t attr_len = sys_be16_to_cpu(attr->attr_len);
        if (buf->len < attr_len)
            break;

        if (attr_len == 0)
            continue;

        msg->data.attrs.attrs[i] = (char*)malloc(attr_len + 1);
        net_buf_pull_mem(buf, attr_len);
        memcpy(msg->data.attrs.attrs[i], attr->attr_val, attr_len);
        msg->data.attrs.attrs[i][attr_len] = '\0';
    }

    bt_sal_avrcp_control_event_callback(msg);
}

static void zblue_on_ct_get_play_status_rsp(struct bt_avrcp_ct* ct, uint8_t tid, uint8_t status, struct net_buf* buf)
{
    struct bt_avrcp_get_play_status_rsp* rsp;
    zblue_avrcp_info_t* avrcp_info;
    avrcp_msg_t* msg;

    if (status != BT_AVRCP_STATUS_SUCCESS)
        return;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_ct, ct);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    if (buf == NULL)
        return;

    if (buf->len < sizeof(*rsp)) {
        BT_LOGW("Invalid response data length");
        return;
    }
    rsp = net_buf_pull_mem(buf, sizeof(*rsp));

    msg = avrcp_msg_new(AVRC_GET_PLAY_STATUS_RSP, &avrcp_info->bd_addr);
    msg->data.playstatus.status = zblue_playback_state_2_sal_state(rsp->play_status);
    msg->data.playstatus.song_len = rsp->song_length;
    msg->data.playstatus.song_pos = rsp->song_position;

    bt_sal_avrcp_control_event_callback(msg);
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
static void bt_avrcp_target_send_unit_info_rsp(struct bt_avrcp_tg* tg, uint8_t tid)
{
    struct bt_avrcp_unit_info_rsp rsp;

    rsp.unit_type = BT_AVRCP_SUBUNIT_TYPE_PANEL;
    rsp.company_id = BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG;

    bt_avrcp_tg_send_unit_info_rsp(tg, tid, &rsp);
}

static void zblue_on_tg_unit_info_req(struct bt_avrcp_tg* tg, uint8_t tid)
{
    bt_avrcp_target_send_unit_info_rsp(tg, tid);
}
#endif

#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static void zblue_on_tg_connected(struct bt_conn* conn, struct bt_avrcp_tg* tg)
{
    zblue_avrcp_info_t* avrcp_info;

    if (!bt_avrcp_conn) {
        BT_LOGW("%s, bt_avrcp_conn not initialized", __func__);
        return;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_conn, conn);
    if (!avrcp_info)
        avrcp_info = bt_avrcp_create_avrcp_info(conn);

    if (!avrcp_info)
        return;

    avrcp_info->tg = tg;

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    avrcp_msg_t* msg;
    msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, &avrcp_info->bd_addr);
    msg->data.conn_state.conn_state = PROFILE_STATE_CONNECTED;
    msg->data.conn_state.reason = PROFILE_REASON_UNSPECIFIED;
    bt_sal_avrcp_target_event_callback(msg);

    bt_sal_cm_profile_connected_callback(&avrcp_info->bd_addr, PROFILE_AVRCP_TG, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&avrcp_info->bd_addr, PROFILE_AVRCP_TG, CONN_ID_DEFAULT, PRIMARY_ADAPTER, bt_sal_avrcp_disconnect, NULL);
#endif
}

static void zblue_on_tg_disconnected(struct bt_avrcp_tg* tg)
{
    zblue_avrcp_info_t* avrcp_info;

    if (!bt_avrcp_conn) {
        BT_LOGW("%s, bt_avrcp_conn not initialized", __func__);
        return;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_tg, tg);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    avrcp_info->tg = NULL;

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    avrcp_msg_t* msg;
    msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, &avrcp_info->bd_addr);
    msg->data.conn_state.conn_state = PROFILE_STATE_DISCONNECTED;
    msg->data.conn_state.reason = PROFILE_REASON_UNSPECIFIED;
    bt_sal_avrcp_target_event_callback(msg);

    bt_sal_cm_profile_disconnected_callback(&avrcp_info->bd_addr, PROFILE_AVRCP_TG, CONN_ID_DEFAULT);
#endif

    bt_list_remove_avrcp_info(avrcp_info);
}

static void bt_sal_avrcp_target_send_get_caps_rsp(struct bt_avrcp_tg* tg, uint8_t tid, uint8_t cap_id)
{
    struct bt_avrcp_get_caps_rsp* rsp;
    struct net_buf* buf;
    int err;

    buf = bt_avrcp_create_vendor_pdu(NULL);
    if (buf == NULL) {
        BT_LOGW("Failed to allocate buffer for AVRCP get caps rsp");
        return;
    }

    if (net_buf_tailroom(buf) < sizeof(*rsp)) {
        BT_LOGW("Not enough tailroom in buffer for get caps rsp");
        goto failed;
    }

    rsp = net_buf_add(buf, sizeof(*rsp));
    rsp->cap_id = cap_id;

    switch (cap_id) {
    case BT_AVRCP_CAP_COMPANY_ID:
        rsp->cap_cnt = 1;
        if (net_buf_tailroom(buf) < BT_AVRCP_COMPANY_ID_SIZE) {
            BT_LOGW("Not enough tailroom for company ID capability rsp");
            goto failed;
        }
        net_buf_add(buf, BT_AVRCP_COMPANY_ID_SIZE);
        sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, rsp->cap);
        break;
    case BT_AVRCP_CAP_EVENTS_SUPPORTED:
        rsp->cap_cnt = ARRAY_SIZE(bt_supported_avrcp_events);
        if (net_buf_tailroom(buf) < rsp->cap_cnt) {
            BT_LOGW("Not enough tailroom for events supported capability rsp");
            goto failed;
        }

        net_buf_add_mem(buf, bt_supported_avrcp_events, rsp->cap_cnt);
        break;
    default:
        BT_LOGW("Unknown capability ID: 0x%02x", cap_id);
        return;
    }

    err = bt_avrcp_tg_get_caps(tg, tid, BT_AVRCP_STATUS_SUCCESS, buf);
    if (err < 0)
        goto failed;

    return;

failed:
    net_buf_unref(buf);
}

static void zblue_on_tg_get_caps_req(struct bt_avrcp_tg* tg, uint8_t tid, uint8_t cap_id)
{
    zblue_avrcp_info_t* avrcp_info;
    struct net_buf* buf;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_tg, tg);
    if (avrcp_info) {
        bt_sal_avrcp_target_send_get_caps_rsp(tg, tid, cap_id);
        return;
    }

    BT_LOGW("avrcp_info not found");

    buf = bt_avrcp_create_vendor_pdu(NULL);
    if (buf == NULL) {
        BT_LOGE("Failed to allocate response buffer");
        return;
    }

    err = bt_avrcp_tg_get_caps(tg, tid, BT_AVRCP_STATUS_NOT_IMPLEMENTED, buf);
    if (err < 0)
        net_buf_unref(buf);
}

static void zblue_on_tg_register_notification_req(struct bt_avrcp_tg* tg, uint8_t tid, uint8_t event_id, uint32_t interval)
{
    avrcp_msg_t* msg;
    zblue_avrcp_info_t* avrcp_info;
    avrcp_notification_event_t event;
    bt_status_t status;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_tg, tg);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    status = zephyr_event_2_sal_event(&event, event_id);
    if (status != BT_STATUS_SUCCESS)
        return;

    if (event_id == BT_AVRCP_EVT_VOLUME_CHANGED) {
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_REQ, &avrcp_info->bd_addr);
        msg->data.notify_req.event = event;
        msg->data.notify_req.interval = interval;
        bt_sal_avrcp_control_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */
#endif /* CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME */
    } else {
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
        msg = avrcp_msg_new(AVRC_REGISTER_NOTIFICATION_REQ, &avrcp_info->bd_addr);
        msg->data.notify_req.event = event;
        msg->data.notify_req.interval = interval;
        bt_sal_avrcp_target_event_callback(msg);
#endif /* CONFIG_BLUETOOTH_AVRCP_TARGET */
    }

    zblue_tg_tid_t* tg_tid = (zblue_tg_tid_t*)calloc(1, sizeof(zblue_tg_tid_t));
    tg_tid->tid = tid;
    tg_tid->next_rsp = BT_AVRCP_RSP_INTERIM;

    switch (event_id) {
    case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
        tg_tid->msg_id = SAL_AVRCP_REG_NTF_PLAYBACK_STATUS_CHANGED;
        break;
    case BT_AVRCP_EVT_TRACK_CHANGED:
        tg_tid->msg_id = SAL_AVRCP_REG_NTF_TRACK_CHANGED;
        break;
    case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
        tg_tid->msg_id = SAL_AVRCP_REG_NTF_PLAYBACK_POS_CHANGED;
        break;
#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
    case BT_AVRCP_EVT_VOLUME_CHANGED:
        tg_tid->msg_id = SAL_AVRCP_REG_NTF_VOLUME_CHANGED;
        break;
#endif
    default:
        free(tg_tid);
        return;
    }

    bt_list_add_tail(avrcp_info->tg_tid, tg_tid);
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
static void bt_avrcp_target_send_subunit_info_rsp(struct bt_avrcp_tg* tg, uint8_t tid)
{
    bt_avrcp_tg_send_subunit_info_rsp(tg, tid);
}

static void zblue_on_tg_subunit_info_req(struct bt_avrcp_tg* tg, uint8_t tid)
{
    bt_avrcp_target_send_subunit_info_rsp(tg, tid);
}

static void bt_sal_avrcp_tg_send_passthrough_rsp(struct bt_avrcp_tg* tg, struct bt_avrcp_passthrough_cmd* cmd,
    bt_avrcp_rsp_t result, uint8_t tid)
{
    struct bt_avrcp_passthrough_rsp* rsp;
    struct bt_avrcp_passthrough_opvu_data* opvu = NULL;
    struct net_buf* buf;
    bt_avrcp_opid_t opid;
    bt_avrcp_button_state_t state;
    int err;

    buf = bt_avrcp_create_pdu(NULL);
    if (buf == NULL) {
        BT_LOGE("Failed to allocate buffer for AVRCP passthrough response");
        return;
    }

    if (result != BT_AVRCP_RSP_ACCEPTED)
        goto send;

    if (net_buf_tailroom(buf) < sizeof(struct bt_avrcp_passthrough_rsp)) {
        BT_LOGW("Not enough tailroom in buffer for passthrough rsp");
        result = BT_AVRCP_RSP_REJECTED;
        goto send;
    }
    rsp = net_buf_add(buf, sizeof(*rsp));

    state = BT_AVRCP_PASSTHROUGH_GET_STATE(cmd);
    opid = BT_AVRCP_PASSTHROUGH_GET_OPID(cmd);
    BT_AVRCP_PASSTHROUGH_SET_STATE_OPID(rsp, state, opid);

    if (net_buf_tailroom(buf) < sizeof(*opvu)) {
        BT_LOGW("Not enough tailroom in buffer for opvu");
        result = BT_AVRCP_RSP_REJECTED;
        goto send;
    }

    opvu = net_buf_add(buf, sizeof(*opvu));
    sys_put_be24(BT_AVRCP_COMPANY_ID_BLUETOOTH_SIG, opvu->company_id);
    opvu->opid_vu = sys_cpu_to_be16(opid);
    rsp->data_len = sizeof(*opvu);

send:
    err = bt_avrcp_tg_send_passthrough_rsp(tg, tid, result, buf);
    if (err < 0) {
        BT_LOGE("Failed to send passthrough response: %d", err);
        net_buf_unref(buf);
    }
}

static void zblue_on_tg_passthrough_req(struct bt_avrcp_tg* tg, uint8_t tid, struct net_buf* buf)
{
    zblue_avrcp_info_t* avrcp_info;
    avrcp_passthr_cmd_t bt_cmd;
    avrcp_msg_t* msg;
    struct bt_avrcp_passthrough_cmd* cmd;
    bt_avrcp_opid_t opid;
    bt_avrcp_button_state_t state;
    bt_avrcp_rsp_t result;

    cmd = net_buf_pull_mem(buf, sizeof(*cmd));
    opid = BT_AVRCP_PASSTHROUGH_GET_OPID(cmd);
    state = BT_AVRCP_PASSTHROUGH_GET_STATE(cmd);
    bt_cmd = zephyr_op_2_sal_op(opid);

    switch (bt_cmd) {
    case PASSTHROUGH_CMD_ID_PLAY:
    case PASSTHROUGH_CMD_ID_STOP:
    case PASSTHROUGH_CMD_ID_PAUSE:
    case PASSTHROUGH_CMD_ID_FORWARD:
    case PASSTHROUGH_CMD_ID_BACKWARD:
        break;
    default:
        BT_LOGW("%s, operation 0x%x not recognized", __func__, opid);
        result = BT_AVRCP_RSP_NOT_IMPLEMENTED;
        goto send;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_tg, tg);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        result = BT_AVRCP_RSP_REJECTED;
        goto send;
    }

    msg = avrcp_msg_new(AVRC_PASSTHROUHT_CMD, &avrcp_info->bd_addr);
    if (msg == NULL) {
        result = BT_AVRCP_RSP_REJECTED;
        goto send;
    }

    msg->data.passthr_cmd.opcode = bt_cmd;
    msg->data.passthr_rsp.state = (state == BT_AVRCP_BUTTON_PRESSED) ? AVRCP_KEY_PRESSED : AVRCP_KEY_RELEASED;
    bt_sal_avrcp_target_event_callback(msg);

    result = BT_AVRCP_RSP_ACCEPTED;

send:
    bt_sal_avrcp_tg_send_passthrough_rsp(tg, cmd, result, tid);
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME
static void zblue_on_tg_set_absolute_volume_req(struct bt_avrcp_tg* tg, uint8_t tid, uint8_t absolute_volume)
{
    bt_avrcp_status_t status = BT_AVRCP_STATUS_NOT_IMPLEMENTED;

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    avrcp_msg_t* msg;
    zblue_avrcp_info_t* avrcp_info;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_tg, tg);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        goto send;
    }

    msg = avrcp_msg_new(AVRC_SET_ABSOLUTE_VOLUME, &avrcp_info->bd_addr);
    if (msg == NULL)
        goto send;

    msg->data.absvol.volume = absolute_volume;
    bt_sal_avrcp_control_event_callback(msg);

    status = BT_AVRCP_STATUS_SUCCESS;
#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL */

send:
    bt_avrcp_tg_absolute_volume(tg, tid, status, absolute_volume);
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
static void zblue_on_tg_get_play_status_req(struct bt_avrcp_tg* tg, uint8_t tid)
{
    avrcp_msg_t* msg;
    zblue_avrcp_info_t* avrcp_info;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_tg, tg);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    zblue_tg_tid_t* tg_tid = (zblue_tg_tid_t*)calloc(1, sizeof(zblue_tg_tid_t));
    tg_tid->tid = tid;
    tg_tid->msg_id = SAL_AVRCP_GET_PLAY_STATUS;
    tg_tid->next_rsp = BT_AVRCP_RSP_INTERIM;
    bt_list_add_tail(avrcp_info->tg_tid, tg_tid);

    msg = avrcp_msg_new(AVRC_GET_PLAY_STATUS_REQ, &avrcp_info->bd_addr);
    bt_sal_avrcp_target_event_callback(msg);
}
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
static void avrcp_on_connection_state_changed(bt_address_t* bd_addr,
    profile_connection_state_t state)
{
    avrcp_msg_t* msg;

    msg = avrcp_msg_new(AVRC_CONNECTION_STATE_CHANGED, bd_addr);
    if (!msg) {
        BT_LOGE("%s, avrcp_msg_new failed", __func__);
        return;
    }

    msg->data.conn_state.conn_state = state;
    msg->data.conn_state.reason = PROFILE_REASON_UNSPECIFIED;
    bt_sal_avrcp_control_event_callback(msg);
}

static bt_status_t avrcp_control_connect(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data)
{
    int err;
    struct bt_conn* conn;

    conn = bt_conn_lookup_addr_br((bt_addr_t*)bd_addr);
    if (!conn) {
        BT_LOGW("BR/EDR connection not found for AVRCP connect");
        avrcp_on_connection_state_changed(bd_addr, PROFILE_STATE_DISCONNECTED);
        bt_sal_cm_profile_disconnected_callback(bd_addr, PROFILE_AVRCP_CT, CONN_ID_DEFAULT);
        return BT_STATUS_FAIL;
    }

    err = bt_avrcp_connect(conn);

    bt_conn_unref(conn);

    if (err < 0) {
        avrcp_on_connection_state_changed(bd_addr, PROFILE_STATE_DISCONNECTED);
        bt_sal_cm_profile_disconnected_callback(bd_addr, PROFILE_AVRCP_CT, CONN_ID_DEFAULT);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}
#endif

bt_status_t bt_sal_avrcp_control_connect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    return bt_sal_profile_connect_request(addr, PROFILE_AVRCP_CT, CONN_ID_DEFAULT, id, avrcp_control_connect, NULL);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

static bt_status_t bt_sal_avrcp_disconnect(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data)
{
    zblue_avrcp_info_t* avrcp_info;
    int err;

    if (!bt_avrcp_conn || !bd_addr) {
        BT_LOGW("%s, invalid params", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    if (avrcp_info->conn) {
        err = bt_avrcp_disconnect(avrcp_info->conn);
    } else {
        goto failed;
    }

    if (err < 0)
        goto failed;

    return BT_STATUS_SUCCESS;

failed:
    bt_list_free(avrcp_info->tg_tid);
    bt_list_remove(bt_avrcp_conn, avrcp_info);
    return BT_STATUS_FAIL;
}

bt_status_t bt_sal_avrcp_control_disconnect(bt_controller_id_t id, bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    return bt_sal_profile_disconnect_request(addr, PROFILE_AVRCP_CT, CONN_ID_DEFAULT, id, bt_sal_avrcp_disconnect, NULL);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bool bt_sal_avrcp_try_disconnect_avrcp_control(bt_controller_id_t id, bt_address_t* addr)
{
    if (bt_sal_avrcp_control_disconnect(id, addr) == BT_STATUS_SUCCESS)
        return true;

    return false;
}

bt_status_t bt_sal_avrcp_control_send_pass_through_cmd(bt_controller_id_t id,
    bt_address_t* bd_addr, avrcp_passthr_cmd_t key_code, avrcp_key_state_t key_state)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    uint8_t op_id = sal_op_2_zephyr_op(key_code);
    uint8_t state = key_state == AVRCP_KEY_PRESSED ? BT_AVRCP_BUTTON_PRESSED : BT_AVRCP_BUTTON_RELEASED;

    if (op_id == PASSTHROUGH_CMD_ID_RESERVED)
        return BT_STATUS_PARM_INVALID;

    err = bt_avrcp_ct_passthrough(avrcp_info->ct, get_next_ct_tid(avrcp_info), op_id, state, NULL, 0);
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
static void bt_avrcp_control_notification_cb(struct bt_avrcp_ct* ct, uint8_t event_id, struct bt_avrcp_event_data* data)
{
    zblue_avrcp_info_t* avrcp_info;
    uint32_t interval;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_by_ct, ct);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return;
    }

    zblue_on_ct_notification_rsp(ct, 0, BT_AVRCP_STATUS_SUCCESS, event_id, data);

    switch (event_id) {
    case BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED:
    case BT_AVRCP_EVT_TRACK_CHANGED:
    case BT_AVRCP_EVT_VOLUME_CHANGED:
        interval = 0;
        break;
    case BT_AVRCP_EVT_PLAYBACK_POS_CHANGED:
        interval = 2;
        break;
    case BT_AVRCP_EVT_BATT_STATUS_CHANGED:
    case BT_AVRCP_EVT_SYSTEM_STATUS_CHANGED:
    case BT_AVRCP_EVT_PLAYER_APP_SETTING_CHANGED:
    case BT_AVRCP_EVT_ADDRESSED_PLAYER_CHANGED:
    case BT_AVRCP_EVT_UIDS_CHANGED:
    case BT_AVRCP_EVT_TRACK_REACHED_END:
    case BT_AVRCP_EVT_TRACK_REACHED_START:
    case BT_AVRCP_EVT_AVAILABLE_PLAYERS_CHANGED:
    case BT_AVRCP_EVT_NOW_PLAYING_CONTENT_CHANGED:
        BT_LOGW("Unsupported event_id: 0x%02x", event_id);
        return;
    default:
        BT_LOGW("Unknown event_id: 0x%02x", event_id);
        return;
    }

    bt_avrcp_ct_register_notification(ct, get_next_ct_tid(avrcp_info), event_id, interval, bt_avrcp_control_notification_cb);
}
#endif

bt_status_t bt_sal_avrcp_control_register_notification(bt_controller_id_t id,
    bt_address_t* bd_addr, avrcp_notification_event_t event, uint32_t interval)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    uint8_t event_id = 0;
    bt_status_t status;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    status = sal_event_2_zephyr_event(&event_id, event);
    if (status != BT_STATUS_SUCCESS)
        return BT_STATUS_PARM_INVALID;

    err = bt_avrcp_ct_register_notification(avrcp_info->ct, get_next_ct_tid(avrcp_info), event_id, interval, bt_avrcp_control_notification_cb);
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_set_absolute_volume(bt_controller_id_t id, bt_address_t* bd_addr,
    uint8_t volume)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    zblue_avrcp_info_t* avrcp_info;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    err = bt_avrcp_ct_set_absolute_volume(avrcp_info->ct, get_next_ct_tid(avrcp_info), volume);
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_capabilities(bt_controller_id_t id, bt_address_t* bd_addr,
    uint8_t cap_id)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    if (cap_id == AVRCP_CAPABILITY_ID_EVENTS_SUPPORTED)
        err = bt_avrcp_ct_get_caps(avrcp_info->ct, get_next_ct_tid(avrcp_info), BT_AVRCP_CAP_EVENTS_SUPPORTED);
    else
        return BT_STATUS_NOT_SUPPORTED;

    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_playback_state(bt_controller_id_t id, bt_address_t* bd_addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    err = bt_avrcp_ct_get_play_status(avrcp_info->ct, get_next_ct_tid(avrcp_info));
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_get_play_status_rsp(bt_controller_id_t id, bt_address_t* bd_addr,
    avrcp_play_status_t status, uint32_t song_len, uint32_t song_pos)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    zblue_avrcp_info_t* avrcp_info;
    struct bt_avrcp_get_play_status_rsp* rsp;
    struct net_buf* buf;
    int err;
    int tid;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    buf = bt_avrcp_create_vendor_pdu(&bt_avrcp_tx_pool);
    if (buf == NULL) {
        BT_LOGW("Failed to allocate buffer for AVRCP response");
        return BT_STATUS_FAIL;
    }

    if (net_buf_tailroom(buf) < sizeof(*rsp)) {
        BT_LOGW("Not enough tailroom in buffer");
        goto failed;
    }

    rsp = net_buf_add(buf, sizeof(*rsp));
    rsp->song_length = song_len;
    rsp->song_position = song_pos;
    rsp->play_status = sal_playback_state_2_zephyr_state(status);

    tid = tg_get_and_remove_tid(avrcp_info, SAL_AVRCP_GET_PLAY_STATUS);
    if (tid < 0)
        goto failed;

    err = bt_avrcp_tg_get_play_status(avrcp_info->tg, tid, BT_AVRCP_STATUS_SUCCESS, buf);
    if (err < 0) {
        BT_LOGE("Failed to send GetPlayStatus rsp: %d", err);
        goto failed;
    }

    return BT_STATUS_SUCCESS;

failed:
    net_buf_unref(buf);
    return BT_STATUS_FAIL;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_play_status_notify(bt_controller_id_t id, bt_address_t* bd_addr,
    avrcp_play_status_t status)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    zblue_avrcp_info_t* avrcp_info;
    struct bt_avrcp_event_data data;
    int err;
    int tid;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    memset(&data, 0, sizeof(data));
    data.play_status = sal_playback_state_2_zephyr_state(status);

    tid = tg_get_and_remove_tid(avrcp_info, SAL_AVRCP_REG_NTF_PLAYBACK_STATUS_CHANGED);
    if (tid < 0)
        return BT_STATUS_FAIL;

    err = bt_avrcp_tg_notification(avrcp_info->tg, tid, BT_AVRCP_STATUS_SUCCESS, BT_AVRCP_EVT_PLAYBACK_STATUS_CHANGED, &data);
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_notify_track_changed(bt_controller_id_t id, bt_address_t* bd_addr,
    bool selected)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    zblue_avrcp_info_t* avrcp_info;
    struct bt_avrcp_event_data data;
    int err;
    int tid;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    if (selected) {
        memset(data.identifier, 0, 8);
    } else {
        memset(data.identifier, 0xFF, 8);
    }

    tid = tg_get_and_remove_tid(avrcp_info, SAL_AVRCP_REG_NTF_TRACK_CHANGED);
    if (tid < 0)
        return BT_STATUS_FAIL;

    err = bt_avrcp_tg_notification(avrcp_info->tg, tid, BT_AVRCP_STATUS_SUCCESS, BT_AVRCP_EVT_TRACK_CHANGED, &data);
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_notify_play_position_changed(bt_controller_id_t id,
    bt_address_t* bd_addr, uint32_t position)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    zblue_avrcp_info_t* avrcp_info;
    struct bt_avrcp_event_data data;
    int err;
    int tid;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    memset(&data, 0, sizeof(data));
    data.playback_pos = position;

    tid = tg_get_and_remove_tid(avrcp_info, SAL_AVRCP_REG_NTF_PLAYBACK_POS_CHANGED);
    if (tid < 0)
        return BT_STATUS_FAIL;

    err = bt_avrcp_tg_notification(avrcp_info->tg, tid, BT_AVRCP_STATUS_SUCCESS, BT_AVRCP_EVT_PLAYBACK_POS_CHANGED, &data);
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_volume_changed_notify(bt_controller_id_t id,
    bt_address_t* bd_addr, uint8_t volume)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    struct bt_avrcp_event_data data;
    int err;
    int tid;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    memset(&data, 0, sizeof(data));
    data.absolute_volume = volume;

    tid = tg_get_and_remove_tid(avrcp_info, SAL_AVRCP_REG_NTF_VOLUME_CHANGED);
    if (tid < 0)
        return BT_STATUS_FAIL;

    err = bt_avrcp_tg_notification(avrcp_info->tg, tid, BT_AVRCP_STATUS_SUCCESS, BT_AVRCP_EVT_VOLUME_CHANGED, &data);
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_element_attributes(bt_controller_id_t id,
    bt_address_t* bd_addr, uint8_t attrs_count, avrcp_media_attr_type_t* types)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    struct bt_avrcp_get_element_attrs_cmd* cmd;
    struct net_buf* buf;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    buf = bt_avrcp_create_vendor_pdu(&bt_avrcp_tx_pool);
    if (buf == NULL) {
        BT_LOGW("Failed to allocate vendor dependent command buffer");
        return BT_STATUS_FAIL;
    }

    if (net_buf_tailroom(buf) < sizeof(*cmd) + (7 * sizeof(uint32_t))) {
        BT_LOGW("Not enough tailroom in buffer for browsed player rsp");
        goto failed;
    }
    cmd = net_buf_add(buf, sizeof(*cmd));

    if (attrs_count > 0) {
        cmd->num_attrs = attrs_count;
        memset(cmd->identifier, 0, sizeof(cmd->identifier));
        for (int i = 0; i < cmd->num_attrs; i++) {
            net_buf_add_be32(buf, *types + i);
        }
    } else {
        cmd->num_attrs = 7U; // bt_avrcp_media_attr_t only supports 7 attribute types.
        memset(cmd->identifier, 0, sizeof(cmd->identifier));
        for (int i = 0; i < cmd->num_attrs; i++) {
            net_buf_add_be32(buf, BT_AVRCP_MEDIA_ATTR_TITLE + i);
        }
    }

    err = bt_avrcp_ct_get_element_attrs(avrcp_info->ct, get_next_ct_tid(avrcp_info), buf);
    if (err < 0)
        goto failed;

    return BT_STATUS_SUCCESS;

failed:
    net_buf_unref(buf);
    return BT_STATUS_FAIL;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_unit_info(bt_controller_id_t id,
    bt_address_t* bd_addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    err = bt_avrcp_ct_get_unit_info(avrcp_info->ct, get_next_ct_tid(avrcp_info));
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_get_subunit_info(bt_controller_id_t id,
    bt_address_t* bd_addr)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    zblue_avrcp_info_t* avrcp_info;
    int err;

    avrcp_info = bt_list_find(bt_avrcp_conn, bt_avrcp_info_find_addr, bd_addr);
    if (!avrcp_info) {
        BT_LOGW("avrcp_info not found");
        return BT_STATUS_FAIL;
    }

    err = bt_avrcp_ct_get_subunit_info(avrcp_info->ct, get_next_ct_tid(avrcp_info));
    if (err < 0)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_control_init(void)
{
#if defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
    if (avrcp_ct_registered)
        return BT_STATUS_SUCCESS;

#ifdef AVRCP_SDP_BY_APP
    bt_sdp_register_service(&avrcp_ct_rec);
#endif

    bt_avrcp_ct_register_cb(&avrcp_ct_cbks);
    avrcp_ct_registered = true;

    if (!bt_avrcp_conn)
        bt_avrcp_conn = bt_list_new(free);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_avrcp_target_init(void)
{
#if defined(CONFIG_BLUETOOTH_AVRCP_TARGET) || defined(CONFIG_BLUETOOTH_AVRCP_ABSOLUTE_VOLUME)
    if (avrcp_tg_registered)
        return BT_STATUS_SUCCESS;

#ifdef AVRCP_SDP_BY_APP
    bt_sdp_register_service(&avrcp_tg_rec);
#endif

    bt_avrcp_tg_register_cb(&avrcp_tg_cbks);
    avrcp_tg_registered = true;

    if (!bt_avrcp_conn)
        bt_avrcp_conn = bt_list_new(free);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void bt_sal_avrcp_control_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    bt_list_t* list = bt_avrcp_conn;
    bt_list_node_t* node;

    if (!avrcp_ct_registered)
        return;

#ifdef AVRCP_SDP_BY_APP
    bt_sdp_unregister_service(&avrcp_ct_rec);
#endif

    bt_avrcp_ct_unregister_cb(&avrcp_ct_cbks);
    avrcp_ct_registered = false;

    if (!list)
        return;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        zblue_avrcp_info_t* avrcp_info = bt_list_node(node);

        avrcp_info->is_cleanup = true;
        bt_sal_avrcp_control_disconnect(PRIMARY_ADAPTER, &avrcp_info->bd_addr);
    }

    if (bt_list_length(bt_avrcp_conn) != 0)
        return;

    bt_list_free(bt_avrcp_conn);
    bt_avrcp_conn = NULL;
#endif
}

void bt_sal_avrcp_target_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    bt_list_t* list = bt_avrcp_conn;
    bt_list_node_t* node;

    if (!avrcp_tg_registered)
        return;

#ifdef AVRCP_SDP_BY_APP
    bt_sdp_unregister_service(&avrcp_tg_rec);
#endif

    bt_avrcp_tg_unregister_cb(&avrcp_tg_cbks);
    avrcp_tg_registered = false;

    if (!list)
        return;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        zblue_avrcp_info_t* avrcp_info = bt_list_node(node);

        avrcp_info->is_cleanup = true;
        bt_sal_avrcp_control_disconnect(PRIMARY_ADAPTER, &avrcp_info->bd_addr);
    }

    if (bt_list_length(bt_avrcp_conn) != 0)
        return;

    bt_list_free(bt_avrcp_conn);
    bt_avrcp_conn = NULL;
#endif
}

#endif /* CONFIG_BLUETOOTH_AVRCP_CONTROL || CONFIG_BLUETOOTH_AVRCP_TARGET */
