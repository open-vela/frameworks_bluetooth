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

#include "sal_hfp_hf_interface.h"
#include "bt_debug.h"
#include "bt_list.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "service_loop.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/sdp.h>

static bt_list_t* g_sal_hf_conn_list = NULL;

extern struct net_buf_pool sdp_pool;

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result, const struct bt_sdp_discover_params* ignore);

static const struct bt_sdp_discover_params sdp_discover = {
    .func = zblue_on_sdp_done,
    .pool = &sdp_pool,
    .uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_AGW_SVCLASS),
    .type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR
};

typedef struct _bt_hfp_hf_call_info {
    uint8_t type;
    hfp_hf_call_state_t state;
    struct bt_hfp_hf_call* context;
} bt_hfp_hf_call_info_t;

typedef struct _bt_hfp_hf_connection {
    bt_address_t addr;
    struct bt_conn* conn;
    bt_list_t* calls;
    struct bt_hfp_hf* hf;
    hfp_callsetup_t callsetup_state;
    hfp_call_t call_state;
    hfp_callheld_t held_state;
} bt_hfp_hf_connection_t;

static void free_connection(void* data)
{
    bt_hfp_hf_connection_t* sal_conn = (bt_hfp_hf_connection_t*)data;
    if (sal_conn->calls) {
        bt_list_free(sal_conn->calls);
    }
    free(sal_conn);
    return;
}

static void free_call(void* data)
{
    bt_hfp_hf_call_info_t* sal_call = (bt_hfp_hf_call_info_t*)data;
    free(sal_call);
}

static bool sal_conn_hf_cmp(void* data, void* context)
{
    bt_hfp_hf_connection_t* sal_conn = (bt_hfp_hf_connection_t*)data;
    struct bt_hfp_hf* hf = (struct bt_hfp_hf*)context;
    return sal_conn->hf == hf;
}

static bool sal_conn_addr_cmp(void* sal_context, void* context)
{
    bt_hfp_hf_connection_t* sal_conn = (bt_hfp_hf_connection_t*)sal_context;
    bt_address_t* addr = (bt_address_t*)context;
    return !bt_addr_compare(&sal_conn->addr, addr);
}

static bool sal_call_context_cmp(void* sal_context, void* z_context)
{
    bt_hfp_hf_call_info_t* sal_call = (bt_hfp_hf_call_info_t*)sal_context;
    struct bt_hfp_hf_call* call = (struct bt_hfp_hf_call*)z_context;
    return sal_call->context == call;
}

static bt_hfp_hf_call_info_t* find_call_by_context(bt_hfp_hf_connection_t* sal_conn, struct bt_hfp_hf_call* z_context)
{
    if (!sal_conn->calls) {
        BT_LOGE("%s, calls is NULL", __func__);
    }

    bt_list_t* call_list = sal_conn->calls;

    return (bt_hfp_hf_call_info_t*)bt_list_find(call_list, sal_call_context_cmp, z_context);
}

static bt_hfp_hf_call_info_t* find_call_by_state(bt_hfp_hf_connection_t* sal_conn, hfp_hf_call_state_t state)
{
    bt_list_node_t* node;
    if (!sal_conn || !sal_conn->calls) {
        return NULL;
    }

    for (node = bt_list_head(sal_conn->calls); node != NULL; node = bt_list_next(sal_conn->calls, node)) {
        bt_hfp_hf_call_info_t* sal_call = bt_list_node(node);
        if (sal_call->state == state) {
            return sal_call;
        }
    }

    return NULL;
}

static bt_hfp_hf_call_info_t* new_call()
{
    bt_hfp_hf_call_info_t* call = (bt_hfp_hf_call_info_t*)zalloc(sizeof(bt_hfp_hf_call_info_t));
    if (!call) {
        BT_LOGE("%s, failed to allocate call entry", __func__);
        return NULL;
    }

    call->state = HFP_HF_CALL_STATE_DISCONNECTED;
    call->context = NULL;
    return call;
}

static bt_hfp_hf_call_info_t* find_or_create_call(bt_hfp_hf_connection_t* sal_conn, struct bt_hfp_hf_call* z_context)
{
    if (!sal_conn || !sal_conn->calls) {
        return NULL;
    }

    bt_hfp_hf_call_info_t* call = find_call_by_context(sal_conn, z_context);
    if (call) {
        return call;
    }

    call = new_call(z_context);
    if (!call) {
        return NULL;
    }

    call->context = z_context;

    bt_list_add_tail(sal_conn->calls, call);
    return call;
}

static int remove_call(bt_hfp_hf_connection_t* sal_conn, bt_hfp_hf_call_info_t* sal_call)
{
    if (!sal_conn || !sal_conn->calls || !sal_call) {
        return -EINVAL;
    }

    bt_list_remove(sal_conn->calls, sal_call);
    return 0;
}

static bt_hfp_hf_connection_t* find_connection_by_call_context(
    struct bt_hfp_hf_call* z_context,
    bt_hfp_hf_call_info_t** call_info)
{
    bt_list_node_t* node;
    if (!g_sal_hf_conn_list || !z_context) {
        return NULL;
    }

    for (node = bt_list_head(g_sal_hf_conn_list); node != NULL; node = bt_list_next(g_sal_hf_conn_list, node)) {
        bt_hfp_hf_connection_t* conn = bt_list_node(node);

        if (!conn || !conn->calls) {
            continue;
        }

        bt_hfp_hf_call_info_t* sal_call = find_call_by_context(conn, z_context);
        if (sal_call) {
            if (call_info) {
                *call_info = sal_call;
            }
            return conn;
        }
    }

    return NULL;
}

static inline bt_hfp_hf_connection_t* find_connection_by_addr(bt_address_t* addr)
{
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_addr_cmp, addr);
}

static inline bt_hfp_hf_connection_t* find_connection_by_hf(struct bt_hfp_hf* hf)
{
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_hf_cmp, hf);
}

static bt_hfp_hf_connection_t* new_hf_connection(struct bt_conn* conn, struct bt_hfp_hf* hf)
{
    bt_hfp_hf_connection_t* sal_conn = (bt_hfp_hf_connection_t*)zalloc(sizeof(bt_hfp_hf_connection_t));

    if (!sal_conn) {
        BT_LOGE("%s, malloc failed", __func__);
        return NULL;
    }

    bt_sal_get_remote_address(conn, &sal_conn->addr);
    sal_conn->conn = conn;
    sal_conn->hf = hf;

    sal_conn->calls = bt_list_new(free_call);

    sal_conn->callsetup_state = HFP_CALLSETUP_NONE;
    sal_conn->call_state = HFP_CALL_NO_CALLS_IN_PROGRESS;
    sal_conn->held_state = HFP_CALLHELD_NONE;

    bt_list_add_tail(g_sal_hf_conn_list, sal_conn);

    return sal_conn;
}

static void set_call_state(
    bt_hfp_hf_connection_t* sal_conn,
    bt_hfp_hf_call_info_t* sal_call,
    hfp_hf_call_state_t state)
{
    if (!sal_call) {
        return;
    }

    sal_call->state = state;
}

typedef struct _hf_connect_params {
    struct bt_conn* conn;
    uint8_t channel;
} hf_connect_params_t;

static void do_hf_connect(hf_connect_params_t* params)
{
    bt_address_t bd_addr;
    struct bt_conn* conn = params->conn;
    uint8_t channel = params->channel;
    struct bt_hfp_hf* hf = NULL;

    free(params);

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS) {
        return;
    }

    if (Z_API(bt_hfp_hf_connect)(conn, &hf, channel)) {
        BT_LOGE("%s, Failed to initiate HFP HF connection", __func__);
        return;
    }

    if(!new_hf_connection(conn, hf)) {
        BT_LOGE("%s, Failed to create HFP HF connection", __func__);
        Z_API(bt_hfp_hf_disconnect)(hf);
        bt_conn_unref(conn);
        return BT_STATUS_NOMEM;
    }

    hfp_hf_on_connection_state_changed(addr, PROFILE_STATE_CONNECTING, 0, 0);

    BT_LOGD("%s, HFP HF connecting", __func__);
}

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result, const struct bt_sdp_discover_params* ignore)
{
    int err;
    uint16_t port;

    if (!result) {
        bt_address_t bd_addr;
        if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS) {
            return BT_SDP_DISCOVER_UUID_STOP;
        }
        BT_LOGE("%s, remote device does not support HFP AG feature", __func__);
        bt_conn_unref(conn);

        hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        return BT_SDP_DISCOVER_UUID_STOP;
    }

    if (result->resp_buf) {
        err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &port);

        if (err) {
            BT_LOGE("Fail to parse HF RFCOMM port!");
        } else {
            BT_LOGD("%s, SDP discovery done for HFP HF, HF RFCOMM port: %u", __func__, port);
            hf_connect_params_t* params = (hf_connect_params_t*)zalloc(sizeof(hf_connect_params_t));
            if (params == NULL) {
                BT_LOGE("%s, Failed to allocate memory for new HFP HF connection", __func__);
                return BT_SDP_DISCOVER_UUID_STOP;
            }

            params->conn = conn;
            params->channel = (uint8_t)port;

            CALL_IN_SERVICE(do_hf_connect, params);

            bt_conn_unref(conn);
            params = NULL;
        }
    }

    return BT_SDP_DISCOVER_UUID_STOP;
}

static void zblue_on_connected(struct bt_conn* conn, struct bt_hfp_hf* hf)
{
    bt_address_t bd_addr;

    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS) {
        return;
    }

    if (!find_connection_by_addr(&bd_addr)) {
        if(!new_hf_connection(conn, hf)) {
            BT_LOGE("%s, Failed to create HFP HF connection", __func__);
            Z_API(bt_hfp_hf_disconnect)(hf);
            return;
        }

        hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_CONNECTING, 0, 0);
    }

    hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_CONNECTED, 0, 0);
}

static void zblue_hf_disconnected(struct bt_hfp_hf* hf)
{
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }
    bt_address_t* bd_addr = &conn->addr;

    hfp_hf_on_connection_state_changed(bd_addr, PROFILE_STATE_DISCONNECTING, 0, 0);
    hfp_hf_on_connection_state_changed(bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);

    bt_list_remove(g_sal_hf_conn_list, conn);
}

static void zblue_on_outgoing_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(hf);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    bt_hfp_hf_call_info_t* sal_call = find_or_create_call(sal_conn, call);
    if (!sal_call) {
        BT_LOGE("%s, Failed to track outgoing call", __func__);
        return;
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_DIALING);
    hfp_hf_on_call_setup_state_changed(&sal_conn->addr, HFP_CALLSETUP_OUTGOING);
}

static void zblue_on_incoming_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(hf);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    bt_hfp_hf_call_info_t* sal_call = find_or_create_call(sal_conn, call);
    if (!sal_call) {
        BT_LOGE("%s, Failed to track incoming call", __func__);
        return;
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_INCOMING);
    hfp_hf_on_call_setup_state_changed(&sal_conn->addr, HFP_CALLSETUP_INCOMING);
}

static void zblue_on_remote_ringing(struct bt_hfp_hf_call *call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, &sal_call);

    if (!conn) {
        BT_LOGW("%s, Failed to find connection for remote ringing", __func__);
        return;
    }

    if (sal_call) {
        set_call_state(conn, sal_call, HFP_HF_CALL_STATE_ALERTING);
    }

    hfp_hf_on_call_setup_state_changed(&conn->addr, HFP_CALLSETUP_ALERTING);
}

static void zblue_on_call_accept(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, &sal_call);

    if (!conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to accept", __func__);
        return;
    }

    set_call_state(conn, sal_call, HFP_HF_CALL_STATE_ACTIVE);
    hfp_hf_on_call_active_state_changed(&conn->addr, HFP_CALL_CALLS_IN_PROGRESS);
    hfp_hf_on_call_setup_state_changed(&conn->addr, HFP_CALLSETUP_NONE);
}

static void zblue_on_call_reject(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to reject", __func__);
        return;
    }

    remove_call(sal_conn, sal_call);
    hfp_hf_on_call_setup_state_changed(&sal_conn->addr, HFP_CALLSETUP_NONE);
}

static void zblue_on_call_terminate(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to terminate", __func__);
        return;
    }

    if (sal_call->state == HFP_HF_CALL_STATE_ACTIVE) {
        hfp_hf_on_call_active_state_changed(&sal_conn->addr, HFP_CALL_NO_CALLS_IN_PROGRESS);
    } else if (sal_call->state == HFP_HF_CALL_STATE_HELD) {
        hfp_hf_on_call_held_state_changed(&sal_conn->addr, HFP_CALLHELD_NONE);
    } else {
        BT_LOGW("Unknow previous state %d.", sal_call->state);
    }

    remove_call(sal_conn, sal_call);
}

static void zblue_on_call_held(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to hold", __func__);
        return;
    }

    hfp_hf_on_call_held_state_changed(&sal_conn->addr, HFP_CALLHELD_HELD);

    if (sal_call->state == HFP_HF_CALL_STATE_ACTIVE) {
        hfp_hf_on_call_active_state_changed(&sal_conn->addr, HFP_CALL_NO_CALLS_IN_PROGRESS);
    } else {
        BT_LOGW("Unexpected previous state %d.", sal_call->state);
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_HELD);
}

static void zblue_on_call_retrieve(struct bt_hfp_hf_call *call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to retrieve", __func__);
        return;
    }

    if (sal_call->state == HFP_HF_CALL_STATE_HELD) {
        hfp_hf_on_call_held_state_changed(&sal_conn->addr, HFP_CALLHELD_NONE);
    } else {
        BT_LOGW("Unexpected previous state %d.", sal_call->state);
    }

    hfp_hf_on_call_active_state_changed(&sal_conn->addr, HFP_CALL_CALLS_IN_PROGRESS);

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_ACTIVE);
}

static void zblue_on_subscriber_number(struct bt_hfp_hf* hf, const char* number, uint8_t type, uint8_t service)
{
    bt_address_t* bd_addr = zalloc(sizeof(bt_address_t));

    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    hfp_subscriber_number_service_t fw_service = 0;
    switch (service) {
    case 4:
        fw_service = HFP_HF_SERVICE_VOICE;
        break;
    case 5:
        fw_service = HFP_HF_SERVICE_FAX;
        break;
    default:
        BT_LOGW("%s, Unknown service: %d", __func__, service);
        break;
    }

    bt_sal_get_remote_address(conn->conn, bd_addr);
    hfp_hf_on_subscriber_number_response(bd_addr, number, fw_service);
}

static void zblue_on_vgm(struct bt_hfp_hf *hf, uint8_t gain)
{
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    hfp_hf_on_volume_changed(&conn->addr, HFP_VOLUME_TYPE_MIC, gain);
}

static void zblue_on_vgs(struct bt_hfp_hf *hf, uint8_t gain)
{
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    hfp_hf_on_volume_changed(&conn->addr, HFP_VOLUME_TYPE_SPK, gain);
}

static void zblue_on_voice_recognition(struct bt_hfp_hf *hf, bool activate)
{
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    hfp_hf_on_voice_recognition_state_changed(&conn->addr, activate);
}

static void zblue_on_ring_indication(struct bt_hfp_hf_call *call)
{
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, NULL);
    if (!conn) {
        BT_LOGW("%s, Failed to find connection for ring", __func__);
        return;
    }

    hfp_hf_on_ring_active_state_changed(&conn->addr, true, HFP_IN_BAND_RINGTONE_NOT_PROVIDED);
}

static void zblue_on_clip(struct bt_hfp_hf_call *call, char *number, uint8_t type)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, &sal_call);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection for CLIP", __func__);
        return;
    }

    if (sal_call) {
        sal_call->type = type;
    }

    const char *num = number ? number : "";
    hfp_hf_on_clip(&conn->addr, num, "");
}

static void zblue_on_codec_negotiate(struct bt_hfp_hf* hf, uint8_t id)
{
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    int ret = Z_API(bt_hfp_hf_select_codec)(hf, id);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_select_codec failed: %d", __func__, ret);
    }

    hfp_codec_config_t cfg = { 0 };
    switch (id) {
    case BT_HFP_HF_CODEC_MSBC:
        cfg.codec = HFP_CODEC_MSBC;
        cfg.sample_rate = 16000;
        cfg.bit_width = 16;
        break;
    case BT_HFP_HF_CODEC_CVSD:
    default:
        cfg.codec = HFP_CODEC_CVSD;
        cfg.sample_rate = 8000;
        cfg.bit_width = 16;
        break;
    }

    hfp_hf_on_codec_changed(&conn->addr, &cfg);
}

static void zblue_on_current_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_current_call* call)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(hf);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    bt_address_t bd_addr;
    bt_sal_get_remote_address(sal_conn->conn, &bd_addr);

    if (!call) {
        BT_ADDR_LOG("CLCC finished from %s", &bd_addr);
        hfp_hf_on_current_call_response(&bd_addr, 0, 0, 0, 0, NULL, 0);
        return;
    }

    BT_LOGD("%s, CLCC %d: %s", __func__, call->index, call->number);

    uint32_t idx = call->index;
    hfp_call_direction_t dir = HFP_CALL_DIRECTION_OUTGOING;
    switch (call->dir) {
    case BT_HFP_HF_CALL_DIR_OUTGOING:
        dir = HFP_CALL_DIRECTION_OUTGOING;
        break;
    case BT_HFP_HF_CALL_DIR_INCOMING:
        dir = HFP_CALL_DIRECTION_INCOMING;
        break;
    default:
        BT_LOGW("%s, Unknown direction: %d", __func__, call->dir);
        break;
    }

    hfp_hf_call_state_t status = 0;
    switch (call->status) {
    case BT_HFP_HF_CALL_STATUS_ACTIVE:
        status = HFP_HF_CALL_STATE_ACTIVE;
        break;
    case BT_HFP_HF_CALL_STATUS_HELD:
        status = HFP_HF_CALL_STATE_HELD;
        break;
    case BT_HFP_HF_CALL_STATUS_DIALING:
        status = HFP_HF_CALL_STATE_DIALING;
        break;
    case BT_HFP_HF_CALL_STATUS_ALERTING:
        status = HFP_HF_CALL_STATE_ALERTING;
        break;
    case BT_HFP_HF_CALL_STATUS_INCOMING:
        status = HFP_HF_CALL_STATE_INCOMING;
        break;
    case BT_HFP_HF_CALL_STATUS_WAITING:
        status = HFP_HF_CALL_STATE_WAITING;
        break;
    case BT_HFP_HF_CALL_STATUS_INCOMING_HELD:
        status = HFP_HF_CALL_STATE_HELD_BY_RESP_HOLD;
        break;
    default:
        BT_LOGW("%s, Unknown status: %d", __func__, call->status);
        break;
    }

    hfp_call_mpty_type_t mpty = call->multiparty ? HFP_CALL_MPTY_TYPE_MULTI : HFP_CALL_MPTY_TYPE_SINGLE;

    hfp_hf_on_current_call_response(&bd_addr, idx, dir, status, mpty, call->number, call->type);
}

static struct bt_hfp_hf_cb hf_callbacks = {
    .connected = zblue_on_connected,
    .disconnected = zblue_hf_disconnected,
    .sco_connected = NULL,
    .sco_disconnected = NULL,
    .service = NULL,
    .outgoing = zblue_on_outgoing_call,
    .remote_ringing = zblue_on_remote_ringing,
    .incoming = zblue_on_incoming_call,
    .incoming_held = NULL,
    .accept = zblue_on_call_accept,
    .reject = zblue_on_call_reject,
    .terminate = zblue_on_call_terminate,
    .held = zblue_on_call_held,
    .retrieve = zblue_on_call_retrieve,
    .signal = NULL,
    .roam = NULL,
    .battery = NULL,
    .ring_indication = zblue_on_ring_indication,
    .dialing = NULL,
    .clip = zblue_on_clip,
    .vgm = zblue_on_vgm,
    .vgs = zblue_on_vgs,
    .inband_ring = NULL,
    .operator = NULL,
    .codec_negotiate = zblue_on_codec_negotiate,
    .ecnr_turn_off = NULL,
    .call_waiting = NULL,
    .voice_recognition = zblue_on_voice_recognition,
    .vre_state = NULL,
    .textual_representation = NULL,
    .request_phone_number = NULL,
    .subscriber_number = zblue_on_subscriber_number,
    .query_call = zblue_on_current_call,
};

bt_status_t bt_sal_hfp_hf_init(uint32_t hf_features, uint8_t max_connection)
{
    (void)hf_features;
    (void)max_connection;
    int err;
    g_sal_hf_conn_list = bt_list_new(free_connection);

    err = Z_API(bt_hfp_hf_register)(&hf_callbacks);

    if (err) {
        bt_list_free(g_sal_hf_conn_list);
        g_sal_hf_conn_list = NULL;
    }

    SAL_CHECK_RET(err, 0);
    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_hf_cleanup(void)
{
    return;
}

bt_status_t bt_sal_hfp_hf_connect(bt_address_t* addr)
{
    int err;
    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);

    if (!conn) {
        BT_LOGE("%s, acl not conneted", __func__);
        return BT_STATUS_FAIL;
    }

    BT_LOGD("%s, Start SDP discovery for HFP HF", __func__);

    err = bt_sdp_discover(conn, &sdp_discover);

    bt_conn_unref(conn);

    SAL_CHECK_RET(err, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t* addr)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }
    if (!sal_conn->hf) {
        BT_LOGE("%s, HFP HF not connected", __func__);
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_hf_disconnect)(sal_conn->hf), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t* addr)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }

    int ret = Z_API(bt_hfp_hf_audio_connect)(sal_conn->hf);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t* addr)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_call_info_t* incoming = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_INCOMING);
    if (!incoming) {
        BT_LOGE("%s, No incoming call to answer", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_hf_accept)(incoming->context), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_reject_call(bt_address_t* addr)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }

    bt_hfp_hf_call_info_t* incoming_call = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_INCOMING);
    if (!incoming_call) {
        BT_LOGE("%s, No incoming call to reject", __func__);
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_hf_reject)(incoming_call->context), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hold_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_hangup_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_dial_number(bt_address_t* addr, const char* number)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    SAL_CHECK_RET(Z_API(bt_hfp_hf_number_call)(sal_conn->hf, number), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    char mem_in_str[HFP_PHONENUM_DIGITS_MAX + 1];

    snprintf(mem_in_str, sizeof(mem_in_str), "%" PRIu32, memory);
    SAL_CHECK_RET(Z_API(bt_hfp_hf_memory_dial)(sal_conn->hf, mem_in_str), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_call_control(bt_address_t* addr, hfp_call_control_t chld, uint32_t index)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t* addr)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    SAL_CHECK_RET(Z_API(bt_hfp_hf_query_list_of_current_calls)(sal_conn->hf), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_start_voice_recognition(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_stop_voice_recognition(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_send_battery_level(bt_address_t* addr, uint8_t value)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_send_at_cmd(bt_address_t* addr, const char* cmd, uint16_t len)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_send_dtmf(bt_address_t* addr, char dtmf)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_get_subscriber_number(bt_address_t* addr)
{
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);

    SAL_CHECK_RET(Z_API(bt_hfp_hf_query_subscriber)(sal_conn->hf), 0);

    return BT_STATUS_SUCCESS;
}
