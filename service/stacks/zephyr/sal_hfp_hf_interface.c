/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#include "bt_device.h"
#include "sal_hfp_hf_interface.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "utils/log.h"

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/net_buf.h>
#ifdef CONFIG_BLUETOOTH_HFP_HF
static struct bt_hfp_hf *g_hf;
static struct bt_conn *g_conn;
NET_BUF_POOL_DEFINE(sdp_discover_pool, 10, BT_L2CAP_BUF_SIZE(CONFIG_BT_L2CAP_TX_MTU),
		    CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
static uint8_t sdp_discover_cb(struct bt_conn *conn, struct bt_sdp_client_result *result)
{
    int err;
    uint16_t value;

    BT_LOGD("Discover done");

    if (result->resp_buf != NULL) {
        err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &value);

        if (err != 0) {
            BT_LOGD("Fail to parser RFCOMM the SDP response!");
        } else {
            BT_LOGD("The server channel is %d", value);
            err = zblue_bt_hfp_hf_connect(conn, &g_hf, value);
            if (err != 0) {
                BT_LOGD("Fail to create hfp AG connection (err %d)", err);
            }
        }
    }

    return BT_SDP_DISCOVER_UUID_STOP;
}
static struct bt_sdp_discover_params sdp_discover = {
	.func = sdp_discover_cb,
	.pool = &sdp_discover_pool,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_SVCLASS),
};
static void zblue_on_connected(struct bt_conn *conn, struct bt_hfp_hf *hf)
{
    bt_address_t bd_addr;
    g_conn = conn;
    g_hf = hf;
    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_CONNECTED, 0, 0);
}

static void zblue_on_disconnected(struct bt_hfp_hf *hf)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
}

static void zblue_sco_connected(struct bt_hfp_hf *hf, struct bt_conn *sco_conn)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;
    
    hfp_hf_on_audio_connection_state_changed(&bd_addr, PROFILE_STATE_CONNECTED, sco_conn->handle);
}

static void zblue_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;
    
    hfp_hf_on_audio_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, sco_conn->handle);
}

static void zblue_vgm_changed(struct bt_hfp_hf *hf, uint8_t gain)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_hf_on_volume_changed(&bd_addr, HFP_VOLUME_TYPE_MIC, gain);
}

static void zblue_vgs_changed(struct bt_hfp_hf *hf, uint8_t gain)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_hf_on_volume_changed(&bd_addr, HFP_VOLUME_TYPE_SPK, gain);
}

static void zblue_codec_changed(struct bt_hfp_hf *hf, uint8_t id)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_codec_config_t codec = { .codec = id };
    hfp_hf_on_codec_changed(&bd_addr, &codec);
}

static void zblue_codec_negotiate(struct bt_hfp_hf *hf, uint8_t id)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_codec_config_t codec = { .codec = id };
    hfp_hf_on_codec_changed(&bd_addr, &codec);
}

static void zblue_outgoing(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call){
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_hf_on_call_setup_state_changed(&addr, 3);
}

static void zblue_incoming(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call){
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_hf_on_call_setup_state_changed(&addr, 1);
}

static void zblue_dialing(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call){
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(g_conn, &bd_addr) != BT_STATUS_SUCCESS)
        return;

    hfp_hf_on_call_setup_state_changed(&addr, 2);
}

static struct bt_hfp_hf_cb hf_callbacks = {
.connected = zblue_on_connected,
.disconnected = zblue_on_disconnected,
.sco_connected = zblue_sco_connected,
.sco_disconnected = zblue_sco_disconnected,
.service = NULL,
.outgoing = zblue_outgoing,
.remote_ringing = NULL,
.incoming = zblue_incoming,
.incoming_held = NULL,
.accept = NULL,
.reject = NULL,
.terminate = NULL,
.held = NULL,
.retrieve = NULL,
.signal = NULL,
.roam = NULL,
.battery = NULL,
.ring_indication = NULL,
.dialing = zblue_dialing,
.clip = NULL,
.vgm = zblue_vgm_changed,
.vgs = zblue_vgs_changed,
.inband_ring = NULL,
.operator = NULL,
.codec_negotiate = zblue_codec_negotiate,
.codec_changed = zblue_codec_changed,
.ecnr_turn_off = NULL,
.call_waiting = NULL,
.voice_recognition = NULL,
.vre_state = NULL,
.textual_representation = NULL,
.request_phone_number = NULL,
.subscriber_number = NULL,
};

bt_status_t bt_sal_hfp_hf_init(uint32_t hf_features, uint8_t max_connection)
{
    SAL_CHECK_RET(bt_hfp_hf_register(&hf_callbacks), 0);

    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_hf_cleanup(void)
{

}

bt_status_t bt_sal_hfp_hf_connect(bt_address_t* addr)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    if (!conn)
        BT_LOGD("conn is null");
    SAL_CHECK_RET(bt_sdp_discover(conn, &sdp_discover), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t* addr)
{
    SAL_CHECK_RET(zblue_bt_hfp_hf_disconnect(g_hf), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_reject_call(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hold_call(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hangup_call(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_number(bt_address_t* addr, const char* number)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_call_control(bt_address_t* addr, uint8_t chld, uint32_t index)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_set_volume(bt_address_t* addr, uint8_t type, uint8_t volume)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_start_voice_recognition(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_stop_voice_recognition(bt_address_t* addr)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_battery_level(bt_address_t* addr, uint8_t value)
{


    return BT_STATUS_SUCCESS;
}



bt_status_t bt_sal_hfp_hf_send_at_cmd(bt_address_t* addr, const char* cmd, uint16_t len)
{


    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_dtmf(bt_address_t* addr, char dtmf)
{


    return BT_STATUS_SUCCESS;
}

#endif
