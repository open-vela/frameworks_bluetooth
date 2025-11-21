/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#include "sal_hfp_ag_interface.h"
#include "bt_debug.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"

#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>
#include <zephyr/bluetooth/classic/sdp.h>

static bt_list_t* g_sal_ag_conn_list = NULL;
static bt_list_t* g_sal_ag_call_list = NULL;

extern struct net_buf_pool sdp_pool;

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result, const struct bt_sdp_discover_params* ignore);

static const struct bt_sdp_discover_params sdp_discover = {
    .func = zblue_on_sdp_done,
    .pool = &sdp_pool,
    .uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_SVCLASS),
    .type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR
};

typedef struct _bt_hfp_ag_call_info {
    char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
    struct bt_hfp_ag_call* context;
    uint8_t type;
    enum bt_hfp_ag_call_dir dir;
    enum bt_hfp_ag_call_status state;
} bt_hfp_ag_call_info_t;

typedef struct _bt_hfp_ag_connection {
    bt_address_t addr;
    struct bt_conn* context;
    struct bt_hfp_ag* ag;
} bt_hfp_ag_connection_t;

static void free_connection(void* data)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)data;
    bt_conn_unref(sal_conn->context);
    free(sal_conn);
    return;
}

static __attribute__((unused)) void free_call(void* data)
{
    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)data;
    free(sal_call);
    return;
}

static bool sal_conn_ag_cmp(void* sal_context, void* z_context)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)sal_context;
    struct bt_hfp_ag* ag = (struct bt_hfp_ag*)z_context;
    return sal_conn->ag == ag;
}

static bool sal_conn_addr_cmp(void* sal_context, void* z_context)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)sal_context;
    bt_address_t* addr = (bt_address_t*)z_context;
    return !bt_addr_compare(&sal_conn->addr, addr);
}

static bool sal_conn_context_cmp(void* sal_context, void* z_context)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)sal_context;
    struct bt_conn* conn = (struct bt_conn*)z_context;
    return sal_conn && sal_conn->context == conn;
}

static bt_hfp_ag_connection_t* find_connection_by_addr(bt_address_t* addr)
{
    if (!g_sal_ag_conn_list) {
        BT_LOGE("%s, ag conn list not initialized", __func__);
        return NULL;
    }
    return (bt_hfp_ag_connection_t*)bt_list_find(g_sal_ag_conn_list, sal_conn_addr_cmp, addr);
}

static bt_hfp_ag_connection_t* find_connection_by_ag(struct bt_hfp_ag* ag)
{
    if (!g_sal_ag_conn_list) {
        BT_LOGE("%s, ag conn list not initialized", __func__);
        return NULL;
    }
    return (bt_hfp_ag_connection_t*)bt_list_find(g_sal_ag_conn_list, sal_conn_ag_cmp, ag);
}

static __attribute__((unused)) bt_hfp_ag_connection_t* find_connection_by_conn(struct bt_conn* conn)
{
    if (!g_sal_ag_conn_list) {
        BT_LOGE("%s, ag conn list not initialized", __func__);
        return NULL;
    }
    return (bt_hfp_ag_connection_t*)bt_list_find(g_sal_ag_conn_list, sal_conn_context_cmp, conn);
}

static bt_hfp_ag_connection_t* new_sal_connection(struct bt_conn* conn, struct bt_hfp_ag* ag)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)zalloc(sizeof(bt_hfp_ag_connection_t));

    if (!sal_conn) {
        BT_LOGE("%s, malloc failed", __func__);
        return NULL;
    }
    bt_sal_get_remote_address(conn, &sal_conn->addr);
    sal_conn->context = conn;
    sal_conn->ag = ag;

    bt_list_add_tail(g_sal_ag_conn_list, sal_conn);

    return sal_conn;
}

static enum bt_hfp_ag_call_status tele_call_state_to_sal_status(hfp_ag_call_state_t tele_state)
{
    switch (tele_state) {
    case HFP_AG_CALL_STATE_ACTIVE:
        return BT_HFP_AG_CALL_STATUS_ACTIVE;

    case HFP_AG_CALL_STATE_HELD:
        return BT_HFP_AG_CALL_STATUS_HELD;

    case HFP_AG_CALL_STATE_DIALING:
        return BT_HFP_AG_CALL_STATUS_DIALING;

    case HFP_AG_CALL_STATE_ALERTING:
        return BT_HFP_AG_CALL_STATUS_ALERTING;

    case HFP_AG_CALL_STATE_INCOMING:
        return BT_HFP_AG_CALL_STATUS_INCOMING;

    case HFP_AG_CALL_STATE_WAITING:
        return BT_HFP_AG_CALL_STATUS_WAITING;

    default:
        return -1;
    }
}

static bool sal_call_context_cmp(void* sal_context, void* z_context)
{
    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)sal_context;
    struct bt_hfp_ag_call* call = (struct bt_hfp_ag_call*)z_context;
    return sal_call->context == call;
}

static bool sal_call_state_cmp(void* sal_context, void* z_state)
{
    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)sal_context;
    hfp_ag_call_state_t state = *((hfp_ag_call_state_t*)z_state);
    return sal_call && sal_call->state == tele_call_state_to_sal_status(state);
}

static bt_hfp_ag_call_info_t* find_call_by_context(struct bt_hfp_ag_call* z_context)
{
    if (!g_sal_ag_call_list) {
        BT_LOGE("%s, calls is NULL", __func__);
    }

    return (bt_hfp_ag_call_info_t*)bt_list_find(g_sal_ag_call_list, sal_call_context_cmp, z_context);
}

static __attribute__((unused)) bt_hfp_ag_call_info_t* find_call_by_state(hfp_ag_call_state_t state)
{
    if (!g_sal_ag_call_list) {
        return NULL;
    }
    return (bt_hfp_ag_call_info_t*)bt_list_find(g_sal_ag_call_list, sal_call_state_cmp, &state);
}

static __attribute__((unused)) bt_hfp_ag_call_info_t* tele_call_to_sal_call(tele_call_t* tele_call)
{
    if (!g_sal_ag_call_list) {
        BT_LOGE("%s, sal call list not initialized", __func__);
        return NULL;
    }
    enum bt_hfp_ag_call_status status = tele_call_state_to_sal_status(tele_call->call_state);
    if (status < 0) {
        return NULL;
    }

    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)zalloc(sizeof(bt_hfp_ag_call_info_t));
    if (!sal_call) {
        BT_LOGE("%s, failed to allocate memory", __func__);
        return NULL;
    }

    memcpy(sal_call->number,
        tele_call->line_identification,
        CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN);

    sal_call->type = HFP_CALL_ADDRTYPE_UNKNOWN;
    sal_call->dir = tele_call->is_incoming ? BT_HFP_AG_CALL_DIR_INCOMING : BT_HFP_AG_CALL_DIR_OUTGOING;

    sal_call->state = status;

    return sal_call;
}

static bt_hfp_ag_call_info_t* new_sal_call(void)
{
    bt_hfp_ag_call_info_t* call = (bt_hfp_ag_call_info_t*)zalloc(sizeof(bt_hfp_ag_call_info_t));
    if (!call) {
        BT_LOGE("%s, failed to allocate call entry", __func__);
        return NULL;
    }

    call->state = HFP_AG_CALL_STATE_DISCONNECTED;
    call->context = NULL;
    return call;
}

static __attribute__((unused)) bt_hfp_ag_call_info_t* find_or_create_call(bt_hfp_ag_connection_t* conn, struct bt_hfp_ag_call* z_context)
{
    if (!conn) {
        return NULL;
    }

    bt_hfp_ag_call_info_t* call = find_call_by_context(z_context);
    if (call) {
        return call;
    }

    call = new_sal_call();
    if (!call) {
        return NULL;
    }

    call->context = z_context;

    return call;
}

static void __attribute__((unused)) set_call_state(
    bt_hfp_ag_call_info_t* sal_call,
    hfp_ag_call_state_t state)
{
    if (!sal_call) {
        return;
    }

    sal_call->state = state;

    if (state == HFP_AG_CALL_STATE_DISCONNECTED) {
        bt_list_remove(g_sal_ag_call_list, sal_call);
    }
}

typedef struct _ag_connect_params {
    struct bt_conn* conn;
    uint8_t channel;
} ag_connect_params_t;

static void do_ag_connect(ag_connect_params_t* params)
{
    bt_address_t bd_addr;
    struct bt_conn* conn = params->conn;
    uint8_t channel = params->channel;
    struct bt_hfp_ag* ag = NULL;

    free(params);

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS) {
        return;
    }

    if (Z_API(bt_hfp_ag_connect)(conn, &ag, channel)) {
        BT_LOGE("%s, Failed to initiate HFP AG connection", __func__);
        return;
    }

    new_sal_connection(conn, ag);
    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_CONNECTING, 0, 0);

    BT_LOGD("%s, HFP AG connecting", __func__);
}

uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result, const struct bt_sdp_discover_params* ignore)
{
    int err;
    uint16_t port;

    if (result->resp_buf != NULL) {
        err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &port);

        if (err != 0) {
            BT_LOGE("Fail to parse HF RFCOMM port!");
        } else {
            BT_LOGD("%s, SDP discovery done for HFP HF, HF RFCOMM port: %u", __func__, port);
            ag_connect_params_t* params = (ag_connect_params_t*)zalloc(sizeof(ag_connect_params_t));
            if (params == NULL) {
                BT_LOGE("%s, Failed to allocate memory for new HFP AG connection", __func__);
                return BT_SDP_DISCOVER_UUID_STOP;
            }

            params->conn = conn;
            params->channel = (uint8_t)port;

            CALL_IN_SERVICE(do_ag_connect, params);

            bt_conn_unref(conn);
            params = NULL;
        }
    }
    return BT_SDP_DISCOVER_UUID_STOP;
}

static void zblue_on_ag_connected(struct bt_conn* conn, struct bt_hfp_ag* ag)
{
    BT_LOGD("%s, HFP AG connected, ag=%p", __func__, ag);
    bt_address_t bd_addr;
    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, Failed to get remote address", __func__);
        return;
    }

    if (!find_connection_by_addr(&bd_addr)) {
        bt_hfp_ag_connection_t* sal_conn = new_sal_connection(conn, ag);
        if (!sal_conn) {
            BT_LOGE("%s, Failed to create new HFP AG connection", __func__);
            return;
        }
        hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_CONNECTING, 0, 0);
    }

    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_CONNECTED, 0, 0);
}

static void zblue_on_ag_disconnected(struct bt_hfp_ag* ag)
{
    BT_LOGD("%s, HFP AG disconnected, ag=%p", __func__, ag);
    bt_address_t bd_addr;

    bt_hfp_ag_connection_t* sal_conn = find_connection_by_ag(ag);

    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    if (bt_sal_get_remote_address(sal_conn->context, &bd_addr) != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, Failed to get remote address", __func__);
        return;
    }

    bt_list_remove(g_sal_ag_conn_list, sal_conn);

    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTING, 0, 0);
    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
}

static struct bt_hfp_ag_cb g_hfp_ag_cb = {
    .connected = zblue_on_ag_connected,
    .disconnected = zblue_on_ag_disconnected,
    .sco_connected = NULL,
    .sco_disconnected = NULL,
    .get_ongoing_call = NULL,
    .memory_dial = NULL,
    .number_call = NULL,
    .outgoing = NULL,
    .incoming = NULL,
    .incoming_held = NULL,
    .ringing = NULL,
    .accept = NULL,
    .held = NULL,
    .retrieve = NULL,
    .reject = NULL,
    .terminate = NULL,
    .codec = NULL,
    .codec_negotiate = NULL,
    .audio_connect_req = NULL,
    .vgm = NULL,
    .vgs = NULL,
    .ecnr_turn_off = NULL,
    .explicit_call_transfer = NULL,
    .voice_recognition = NULL,
    .ready_to_accept_audio = NULL,
    .request_phone_number = NULL,
    .transmit_dtmf_code = NULL,
    .subscriber_number = NULL,
    .hf_indicator_value = NULL,
};

bt_status_t bt_sal_hfp_ag_init(uint32_t features, uint8_t max_connection)
{
    (void)features;
    (void)max_connection;
    BT_LOGD("%s, HFP AG init", __func__);
    g_sal_ag_conn_list = bt_list_new(free_connection);

    SAL_CHECK_RET(Z_API(bt_hfp_ag_register)(&g_hfp_ag_cb), 0);
    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_ag_cleanup(void)
{
    return;
}

bt_status_t bt_sal_hfp_ag_connect(bt_address_t* addr)
{
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    if (!conn) {
        BT_LOGE("%s, acl not connected.", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(bt_sdp_discover(conn, &sdp_discover), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_disconnect(bt_address_t* addr)
{
    bt_hfp_ag_connection_t* conn = find_connection_by_addr(addr);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    hfp_ag_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTING, 0, 0);

    SAL_CHECK_RET(Z_API(bt_hfp_ag_disconnect)(conn->ag), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_connect_audio(bt_address_t* addr)
{
    (void)addr;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_disconnect_audio(bt_address_t* addr)
{
    (void)addr;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_start_voice_recognition(bt_address_t* addr)
{
    (void)addr;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_stop_voice_recognition(bt_address_t* addr)
{
    (void)addr;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_phone_state_change(bt_address_t* addr, uint8_t num_active,
    uint8_t num_held, hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name)
{
    (void)num_active;
    (void)num_held;
    (void)call_state;
    (void)type;
    (void)number;
    (void)name;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_cind_response(bt_address_t* addr, hfp_ag_cind_resopnse_t* response)
{
    (void)addr;
    (void)response;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_clcc_response(bt_address_t* addr, uint32_t index,
    hfp_call_direction_t dir, hfp_ag_call_state_t call, hfp_call_mode_t mode,
    hfp_call_mpty_type_t mpty, hfp_call_addrtype_t type, const char* number)
{
    (void)addr;
    (void)index;
    (void)dir;
    (void)call;
    (void)mode;
    (void)mpty;
    (void)type;
    (void)number;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_dial_response(bt_address_t* addr, hfp_atcmd_result_t result)
{
    (void)addr;
    (void)result;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_cops_response(bt_address_t* addr, const char* operator_name, uint16_t length)
{
    (void)addr;
    (void)operator_name;
    (void)length;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_notify_device_status_changed(bt_address_t* addr, hfp_network_state_t network,
    hfp_roaming_state_t roam, uint8_t signal, uint8_t battery)
{
    (void)addr;
    (void)network;
    (void)roam;
    (void)signal;
    (void)battery;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_set_inband_ring_enable(bt_address_t* addr, bool enable)
{
    (void)addr;
    (void)enable;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    (void)addr;
    (void)type;
    (void)volume;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_send_at_cmd(bt_address_t* addr, const char* atcmd, uint16_t length)
{
    (void)addr;
    (void)atcmd;
    (void)length;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_manufacture_id_response(bt_address_t* addr,
    const char* manufacturer_id,
    uint16_t length)
{
    (void)addr;
    (void)manufacturer_id;
    (void)length;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_model_id_response(bt_address_t* addr, const char* model_id, uint16_t length)
{
    (void)addr;
    (void)model_id;
    (void)length;
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_ag_error_response(bt_address_t* addr, hfp_atcmd_result_t result)
{
    (void)addr;
    (void)result;
    return BT_STATUS_UNSUPPORTED;
}
