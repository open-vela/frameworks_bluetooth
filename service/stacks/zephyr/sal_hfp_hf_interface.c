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
#define LOG_TAG "zblue_sal_hf"

#include "sal_hfp_hf_interface.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "sal_zblue_hfp.h"

#include "bt_debug.h"
#include "bt_list.h"
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
static void zblue_on_sdp_disconnected(struct bt_conn* conn, const struct bt_sdp_discover_params* params);

static struct bt_sdp_discover_params sdp_discover = {
    .func = zblue_on_sdp_done,
    .disconnected = zblue_on_sdp_disconnected,
    .pool = &sdp_pool,
    .uuid = BT_UUID_DECLARE_16(BT_SDP_HANDSFREE_AGW_SVCLASS),
    .type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR
};

typedef struct _bt_hfp_hf_slc_connect_param {
    struct bt_conn* conn;
    uint8_t channel;
} bt_hfp_hf_slc_connect_param_t;

typedef struct _bt_hfp_hf_call_info {
    uint8_t index;
    uint8_t type;
    hfp_hf_call_state_t state;
    struct bt_hfp_hf_call* context;
} bt_hfp_hf_call_info_t;

typedef struct _bt_hfp_hf_connection {
    bt_address_t addr;
    struct bt_conn* conn;
    struct bt_conn* sco_conn;
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

static bool sal_conn_context_cmp(void* sal_context, void* context)
{
    bt_hfp_hf_connection_t* sal_conn = (bt_hfp_hf_connection_t*)sal_context;
    struct bt_conn* conn = (struct bt_conn*)context;
    return sal_conn->conn == conn;
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

static bt_hfp_hf_call_info_t* find_call_by_index(bt_hfp_hf_connection_t* conn, uint8_t index)
{
    bt_list_node_t* node;

    if (!conn || !conn->calls || index == 0) {
        return NULL;
    }

    for (node = bt_list_head(conn->calls); node != NULL; node = bt_list_next(conn->calls, node)) {
        bt_hfp_hf_call_info_t* sal_call = bt_list_node(node);
        if (sal_call->index == index) {
            return sal_call;
        }
    }
    return NULL;
}

static int count_call(bt_hfp_hf_connection_t* conn)
{
    if (!conn || !conn->calls) {
        return -EINVAL;
    }

    return bt_list_length(conn->calls);
}

static bt_hfp_hf_call_info_t* new_call(void)
{
    bt_hfp_hf_call_info_t* call = (bt_hfp_hf_call_info_t*)zalloc(sizeof(bt_hfp_hf_call_info_t));
    if (!call) {
        BT_LOGE("%s, failed to allocate call entry", __func__);
        return NULL;
    }

    call->state = HFP_HF_CALL_STATE_DISCONNECTED;
    return call;
}

static bt_hfp_hf_call_info_t* find_or_create_call(bt_hfp_hf_connection_t* sal_conn, struct bt_hfp_hf_call* z_context)
{
    if (!sal_conn || !sal_conn->calls || !z_context) {
        return NULL;
    }

    bt_hfp_hf_call_info_t* call = find_call_by_context(sal_conn, z_context);
    if (call) {
        return call;
    }

    call = new_call();
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

static inline bt_hfp_hf_connection_t* find_connection_by_context(struct bt_conn* conn)
{
    if (!g_sal_hf_conn_list || !conn) {
        return NULL;
    }
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_context_cmp, conn);
}

static inline bt_hfp_hf_connection_t* find_connection_by_addr(bt_address_t* addr)
{
    if (!g_sal_hf_conn_list || !addr) {
        return NULL;
    }
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_addr_cmp, addr);
}

static inline bt_hfp_hf_connection_t* find_connection_by_hf(struct bt_hfp_hf* hf)
{
    if (!g_sal_hf_conn_list || !hf) {
        return NULL;
    }
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_hf_cmp, hf);
}

static bt_hfp_hf_connection_t* find_connection_by_sco(struct bt_conn* sco)
{
    bt_list_node_t* node;

    if (!g_sal_hf_conn_list || !sco) {
        return NULL;
    }

    for (node = bt_list_head(g_sal_hf_conn_list); node != NULL; node = bt_list_next(g_sal_hf_conn_list, node)) {
        bt_hfp_hf_connection_t* conn = bt_list_node(node);
        if (conn && conn->sco_conn == sco) {
            return conn;
        }
    }

    return NULL;
}

static bt_hfp_hf_connection_t* new_hf_connection(struct bt_conn* conn, struct bt_hfp_hf* hf)
{
    bt_hfp_hf_connection_t* sal_conn = (bt_hfp_hf_connection_t*)zalloc(sizeof(bt_hfp_hf_connection_t));

    if (!sal_conn) {
        BT_LOGE("%s, malloc failed", __func__);
        return NULL;
    }

    bt_status_t status = bt_sal_get_remote_address(conn, &sal_conn->addr);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, failed to get remote address", __func__);
        free(sal_conn);
        return NULL;
    }

    sal_conn->conn = conn;
    sal_conn->hf = hf;

    sal_conn->calls = bt_list_new(free_call);
    if (!sal_conn->calls) {
        BT_LOGE("%s, failed to allocate calls list", __func__);
        free(sal_conn);
        return NULL;
    }
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

static bt_status_t do_hf_sdp_discover(bt_controller_id_t id, bt_address_t* addr, void* user_data)
{
    struct bt_conn* conn;
    bt_hfp_hf_connection_t* sal_conn;
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (sal_conn != NULL) {
        BT_LOGI("%s, Connection already exists, skip", __func__);
        return BT_STATUS_SUCCESS;
    }

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    /** Remeber to unref @p conn once SLC is initiated or cancelled */
    if (!conn) {
        BT_LOGE("%s, Failed to lookup connection", __func__);
        hfp_hf_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
        return BT_STATUS_NOT_FOUND;
    }

    /* Request bonding/encryption before SDP to avoid ACL disconnect/reconnect
     * when RFCOMM later triggers pairing */
    if (bt_conn_set_security(conn, BT_SECURITY_L2) < 0) {
        BT_LOGW("%s, bt_conn_set_security failed, proceeding anyway", __func__);
    }

    sal_conn = new_hf_connection(conn, NULL);
    if (!sal_conn) {
        BT_LOGE("%s, could not create new hf connection", __func__);
        hfp_hf_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
        bt_conn_unref(conn);
        return BT_STATUS_NOMEM;
    }

    BT_LOGD("%s, do sdp discover", __func__);
    if (bt_sdp_discover(conn, &sdp_discover) < 0) {
        BT_LOGE("%s, failed to start a SDP discovery", __func__);
        hfp_hf_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
        bt_conn_unref(conn);
        bt_list_remove(g_sal_hf_conn_list, sal_conn);
        return BT_STATUS_FAIL;
    }

    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;
}

bt_status_t do_hf_disconnect(bt_controller_id_t id, bt_address_t* addr, void* user_data)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }

    if (!sal_conn->hf) {
        BT_LOGI("%s, HFP HF not connected", __func__);
        bt_list_remove(g_sal_hf_conn_list, sal_conn);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_hf_disconnect)(sal_conn->hf), 0);
    return BT_STATUS_SUCCESS;
}

/* ============================================================
 * Param structs for do_in_service_loop dispatch
 * ============================================================ */

/* HF + conn (used by connected callback) */
typedef struct {
    struct bt_conn* conn;
    struct bt_hfp_hf* hf;
} hf_connected_params_t;

/* HF only (used by disconnected, ring_indication, etc.) */
typedef struct {
    struct bt_hfp_hf* hf;
} hf_hf_params_t;

/* HF + SCO conn (used by sco_connected) */
typedef struct {
    struct bt_hfp_hf* hf;
    struct bt_conn* sco_conn;
} hf_sco_connected_params_t;

/* SCO conn + reason (used by sco_disconnected) */
typedef struct {
    struct bt_conn* sco_conn;
    uint8_t reason;
} hf_sco_disconnected_params_t;

/* HF + call (used by outgoing, incoming) */
typedef struct {
    struct bt_hfp_hf* hf;
    struct bt_hfp_hf_call* call;
} hf_hf_call_params_t;

/* call only (used by remote_ringing, accept, reject, terminate, held, retrieve) */
typedef struct {
    struct bt_hfp_hf_call* call;
} hf_call_event_params_t;

/* HF + uint8_t (used by vgm, vgs, codec_negotiate) */
typedef struct {
    struct bt_hfp_hf* hf;
    uint8_t value;
} hf_u8_event_params_t;

/* HF + bool (used by voice_recognition) */
typedef struct {
    struct bt_hfp_hf* hf;
    bool activate;
} hf_bool_event_params_t;

/* HF + string (used by subscriber_number) */
typedef struct {
    struct bt_hfp_hf* hf;
    char number[HFP_PHONENUM_DIGITS_MAX + 1];
    uint8_t type;
    uint8_t service;
} hf_subscriber_number_params_t;

/* call + string (used by clip) */
typedef struct {
    struct bt_hfp_hf_call* call;
    char number[HFP_PHONENUM_DIGITS_MAX + 1];
    uint8_t type;
} hf_clip_params_t;

/* HF + vendor specific (cmd + value) */
typedef struct {
    struct bt_hfp_hf* hf;
    char cmd[128];
    char value[256];
} hf_vendor_specific_params_t;

/* HF + AT cmd complete */
typedef struct {
    struct bt_hfp_hf* hf;
    enum bt_hfp_hf_at_cmd cmd;
    enum bt_at_result result;
    enum bt_at_cme err;
} hf_at_cmd_complete_params_t;

/* HF + current call */
typedef struct {
    struct bt_hfp_hf* hf;
    bool has_call;
    struct bt_hfp_hf_current_call call_data;
} hf_current_call_params_t;

/* SDP disconnected */
typedef struct {
    struct bt_conn* conn;
} hf_sdp_disconnected_params_t;

/* call + ring indication (uses call event) - reuse hf_call_event_params_t */

static void hf_slc_connect_handler(void* data)
{
    bt_hfp_hf_slc_connect_param_t* params = (bt_hfp_hf_slc_connect_param_t*)data;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf = NULL;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    if (!params->conn) {
        BT_LOGE("%s, params->conn is NULL", __func__);
        free(params);
        return;
    }

    sal_conn = find_connection_by_context(params->conn);
    if (!sal_conn) {
        BT_LOGW("%s, no pending connection found for conn", __func__);
        bt_conn_unref(params->conn);
        free(params);
        return;
    }

    if (sal_conn->hf) {
        BT_LOGD("%s, already initiating SLC, skip SLC initiating", __func__);
        bt_conn_unref(params->conn);
        free(params);
        return;
    }

    BT_LOGD("%s, SLC initiating", __func__);
    if (Z_API(bt_hfp_hf_connect)(params->conn, &hf, params->channel)) {
        BT_LOGE("%s, HFP HF connection initiation failed", __func__);
        goto error;
    }

    bt_conn_unref(params->conn);
    sal_conn->hf = hf;
    free(params);

    hfp_hf_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_CONNECTING, 0, 0);

    BT_LOGD("%s, HFP HF connecting", __func__);
    return;

error:
    bt_conn_unref(params->conn);
    free(params);
    hfp_hf_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&sal_conn->addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
    bt_list_remove(g_sal_hf_conn_list, sal_conn);
    return;
}

static void hf_sdp_disconnected_handler(void* data)
{
    hf_sdp_disconnected_params_t* p = (hf_sdp_disconnected_params_t*)data;
    bt_address_t bd_addr;

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_context(p->conn);
    bt_conn_unref(p->conn);

    if (!sal_conn) {
        BT_LOGW("%s, no pending connection found", __func__);
        free(p);
        return;
    }
    memcpy(&bd_addr, &sal_conn->addr, sizeof(bt_address_t));

    BT_LOGW("%s, SDP disconnected during discovery", __func__);
    bt_list_remove(g_sal_hf_conn_list, sal_conn);
    hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
    free(p);
}

static void zblue_on_sdp_disconnected(struct bt_conn* conn, const struct bt_sdp_discover_params* params)
{
    hf_sdp_disconnected_params_t* p = malloc(sizeof(hf_sdp_disconnected_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->conn = bt_conn_ref(conn);
    do_in_service_loop(hf_sdp_disconnected_handler, p);
}

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result,
    const struct bt_sdp_discover_params* ignore)
{
    int err;
    uint16_t port;
    bt_address_t bd_addr;
    bt_hfp_hf_slc_connect_param_t* params;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_context(conn);
    if (!sal_conn) {
        BT_LOGE("%s, could not find sal_conn", __func__);
        return BT_SDP_DISCOVER_UUID_STOP;
    }
    memcpy(&bd_addr, &sal_conn->addr, sizeof(bt_address_t));

    if (!result) {
        BT_LOGE("%s, remote device does not support HFP AG feature", __func__);
        goto error;
    }

    if (!result->resp_buf) {
        BT_LOGE("%s, resp_buf is null", __func__);
        goto error;
    }

    err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &port);

    if (err) {
        BT_LOGE("Fail to parse HF RFCOMM port!");
        goto error;
    }

    BT_LOGD("%s, SDP discovery done for HFP HF, HF RFCOMM port: %u", __func__, port);

    params = (bt_hfp_hf_slc_connect_param_t*)malloc(sizeof(bt_hfp_hf_slc_connect_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate slc params", __func__);
        goto error;
    }

    params->channel = (uint8_t)port;
    params->conn = bt_conn_ref(conn);

    do_in_service_loop(hf_slc_connect_handler, params);

    return BT_SDP_DISCOVER_UUID_STOP;

error:
    bt_list_remove(g_sal_hf_conn_list, sal_conn);
    hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
    return BT_SDP_DISCOVER_UUID_STOP;
}

static void hf_connected_handler(void* data)
{
    hf_connected_params_t* p = (hf_connected_params_t*)data;
    bt_hfp_hf_connection_t* sal_conn;

    sal_conn = find_connection_by_context(p->conn);
    if (!sal_conn) {
        BT_LOGD("%s, hf connection incoming", __func__);
        sal_conn = new_hf_connection(p->conn, p->hf);
        if (!sal_conn) {
            BT_LOGE("%s, Failed to create HFP HF connection", __func__);
            if (Z_API(bt_hfp_hf_disconnect)(p->hf)) {
                BT_LOGE("%s, Failed to disconnect HFP HF connection", __func__);
            }
            bt_conn_unref(p->conn);
            free(p);
            return;
        }

        hfp_hf_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_CONNECTING, 0, 0);
    } else {
        /* Override conn if both sides attempt to connect at the same time */
        sal_conn->conn = p->conn;
        sal_conn->hf = p->hf;
    }

    bt_conn_unref(p->conn);

    bt_sal_cm_profile_connected_callback(&sal_conn->addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&sal_conn->addr, PROFILE_HFP_HF, CONN_ID_DEFAULT, PRIMARY_ADAPTER, do_hf_disconnect, NULL);

    hfp_hf_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_CONNECTED, 0, 0);

    /* Start post-SLC AT command init (VGM, VGS, CMEE, COPS, CLIP, CCWA)
     * after framework has processed the connected event and sent its own
     * initial AT commands (e.g. VGS for volume sync). This ensures the
     * correct AT command ordering that PTS expects.
     */
    Z_API(bt_hfp_hf_post_slc_init)(sal_conn->hf);

    free(p);
}

static void zblue_on_connected(struct bt_conn* conn, struct bt_hfp_hf* hf)
{
    hf_connected_params_t* p = malloc(sizeof(hf_connected_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->conn = bt_conn_ref(conn);
    p->hf = hf;
    do_in_service_loop(hf_connected_handler, p);
}

static void hf_disconnected_handler(void* data)
{
    hf_hf_params_t* p = (hf_hf_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    free(p);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    hfp_hf_on_connection_state_changed(&conn->addr, PROFILE_STATE_DISCONNECTING, 0, 0);
    hfp_hf_on_connection_state_changed(&conn->addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&conn->addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);

    bt_list_remove(g_sal_hf_conn_list, conn);
}

static void zblue_hf_disconnected(struct bt_hfp_hf* hf)
{
    hf_hf_params_t* p = malloc(sizeof(hf_hf_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    do_in_service_loop(hf_disconnected_handler, p);
}

static void hf_sco_connected_handler(void* data)
{
    hf_sco_connected_params_t* p = (hf_sco_connected_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection for SCO", __func__);
        free(p);
        return;
    }

    conn->sco_conn = p->sco_conn;
    hfp_hf_on_audio_connection_state_changed(&conn->addr, HFP_AUDIO_STATE_CONNECTED, 0);
    free(p);
}

static void zblue_on_sco_connected(struct bt_hfp_hf* hf, struct bt_conn* sco_conn)
{
    hf_sco_connected_params_t* p = malloc(sizeof(hf_sco_connected_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->sco_conn = sco_conn;
    do_in_service_loop(hf_sco_connected_handler, p);
}

static void hf_sco_disconnected_handler(void* data)
{
    hf_sco_disconnected_params_t* p = (hf_sco_disconnected_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_sco(p->sco_conn);

    if (!conn) {
        BT_LOGW("%s, Failed to find connection for SCO disconn", __func__);
        free(p);
        return;
    }

    hfp_hf_on_audio_connection_state_changed(&conn->addr, HFP_AUDIO_STATE_DISCONNECTED, 0);
    conn->sco_conn = NULL;
    free(p);
}

static void zblue_on_sco_disconnected(struct bt_conn* sco_conn, uint8_t reason)
{
    hf_sco_disconnected_params_t* p = malloc(sizeof(hf_sco_disconnected_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->sco_conn = sco_conn;
    p->reason = reason;
    do_in_service_loop(hf_sco_disconnected_handler, p);
}

static void hf_outgoing_call_handler(void* data)
{
    hf_hf_call_params_t* p = (hf_hf_call_params_t*)data;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(p->hf);

    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    bt_hfp_hf_call_info_t* sal_call = find_or_create_call(sal_conn, p->call);
    if (!sal_call) {
        BT_LOGE("%s, Failed to track outgoing call", __func__);
        free(p);
        return;
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_DIALING);
    hfp_hf_on_call_setup_state_changed(&sal_conn->addr, HFP_CALLSETUP_OUTGOING);
    free(p);
}

static void zblue_on_outgoing_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_call* call)
{
    hf_hf_call_params_t* p = malloc(sizeof(hf_hf_call_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->call = call;
    do_in_service_loop(hf_outgoing_call_handler, p);
}

static void hf_incoming_call_handler(void* data)
{
    hf_hf_call_params_t* p = (hf_hf_call_params_t*)data;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(p->hf);

    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    bt_hfp_hf_call_info_t* sal_call = find_or_create_call(sal_conn, p->call);
    if (!sal_call) {
        BT_LOGE("%s, Failed to track incoming call", __func__);
        free(p);
        return;
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_INCOMING);
    hfp_hf_on_call_setup_state_changed(&sal_conn->addr, HFP_CALLSETUP_INCOMING);
    free(p);
}

static void zblue_on_incoming_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_call* call)
{
    hf_hf_call_params_t* p = malloc(sizeof(hf_hf_call_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->call = call;
    do_in_service_loop(hf_incoming_call_handler, p);
}

static void hf_remote_ringing_handler(void* data)
{
    hf_call_event_params_t* p = (hf_call_event_params_t*)data;
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(p->call, &sal_call);

    if (!conn) {
        BT_LOGW("%s, Failed to find connection for remote ringing", __func__);
        free(p);
        return;
    }

    if (sal_call) {
        set_call_state(conn, sal_call, HFP_HF_CALL_STATE_ALERTING);
    }

    hfp_hf_on_call_setup_state_changed(&conn->addr, HFP_CALLSETUP_ALERTING);
    free(p);
}

static void zblue_on_remote_ringing(struct bt_hfp_hf_call* call)
{
    hf_call_event_params_t* p = malloc(sizeof(hf_call_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    do_in_service_loop(hf_remote_ringing_handler, p);
}

static void hf_call_accept_handler(void* data)
{
    hf_call_event_params_t* p = (hf_call_event_params_t*)data;
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(p->call, &sal_call);

    if (!conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to accept", __func__);
        free(p);
        return;
    }

    set_call_state(conn, sal_call, HFP_HF_CALL_STATE_ACTIVE);
    hfp_hf_on_call_active_state_changed(&conn->addr, HFP_CALL_CALLS_IN_PROGRESS);
    hfp_hf_on_call_setup_state_changed(&conn->addr, HFP_CALLSETUP_NONE);
    free(p);
}

static void zblue_on_call_accept(struct bt_hfp_hf_call* call)
{
    hf_call_event_params_t* p = malloc(sizeof(hf_call_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    do_in_service_loop(hf_call_accept_handler, p);
}

static void hf_call_reject_handler(void* data)
{
    hf_call_event_params_t* p = (hf_call_event_params_t*)data;
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(p->call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to reject", __func__);
        free(p);
        return;
    }

    remove_call(sal_conn, sal_call);
    hfp_hf_on_call_setup_state_changed(&sal_conn->addr, HFP_CALLSETUP_NONE);
    free(p);
}

static void zblue_on_call_reject(struct bt_hfp_hf_call* call)
{
    hf_call_event_params_t* p = malloc(sizeof(hf_call_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    do_in_service_loop(hf_call_reject_handler, p);
}

static void hf_call_terminate_handler(void* data)
{
    hf_call_event_params_t* p = (hf_call_event_params_t*)data;
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(p->call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to terminate", __func__);
        free(p);
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
    free(p);
}

static void zblue_on_call_terminate(struct bt_hfp_hf_call* call)
{
    hf_call_event_params_t* p = malloc(sizeof(hf_call_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    do_in_service_loop(hf_call_terminate_handler, p);
}

static void hf_call_held_handler(void* data)
{
    hf_call_event_params_t* p = (hf_call_event_params_t*)data;
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(p->call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to hold", __func__);
        free(p);
        return;
    }

    hfp_hf_on_call_held_state_changed(&sal_conn->addr, HFP_CALLHELD_HELD);

    if (sal_call->state == HFP_HF_CALL_STATE_ACTIVE) {
        hfp_hf_on_call_active_state_changed(&sal_conn->addr, HFP_CALL_NO_CALLS_IN_PROGRESS);
    } else {
        BT_LOGW("Unexpected previous state %d.", sal_call->state);
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_HELD);
    free(p);
}

static void zblue_on_call_held(struct bt_hfp_hf_call* call)
{
    hf_call_event_params_t* p = malloc(sizeof(hf_call_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    do_in_service_loop(hf_call_held_handler, p);
}

static void hf_call_retrieve_handler(void* data)
{
    hf_call_event_params_t* p = (hf_call_event_params_t*)data;
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(p->call, &sal_call);

    if (!sal_conn || !sal_call) {
        BT_LOGW("%s, Failed to find call to retrieve", __func__);
        free(p);
        return;
    }

    if (sal_call->state == HFP_HF_CALL_STATE_HELD) {
        hfp_hf_on_call_held_state_changed(&sal_conn->addr, HFP_CALLHELD_NONE);
    } else {
        BT_LOGW("Unexpected previous state %d.", sal_call->state);
    }

    hfp_hf_on_call_active_state_changed(&sal_conn->addr, HFP_CALL_CALLS_IN_PROGRESS);

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_ACTIVE);
    free(p);
}

static void zblue_on_call_retrieve(struct bt_hfp_hf_call* call)
{
    hf_call_event_params_t* p = malloc(sizeof(hf_call_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    do_in_service_loop(hf_call_retrieve_handler, p);
}

static void hf_subscriber_number_handler(void* data)
{
    hf_subscriber_number_params_t* p = (hf_subscriber_number_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    hfp_subscriber_number_service_t fw_service = 0;
    switch (p->service) {
    case 4:
        fw_service = HFP_HF_SERVICE_VOICE;
        break;
    case 5:
        fw_service = HFP_HF_SERVICE_FAX;
        break;
    default:
        BT_LOGW("%s, Unknown service: %d", __func__, p->service);
        break;
    }

    hfp_hf_on_subscriber_number_response(&conn->addr, p->number, fw_service);
    free(p);
}

static void zblue_on_subscriber_number(struct bt_hfp_hf* hf, const char* number, uint8_t type, uint8_t service)
{
    hf_subscriber_number_params_t* p = malloc(sizeof(hf_subscriber_number_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    if (number) {
        strlcpy(p->number, number, sizeof(p->number));
    } else {
        p->number[0] = '\0';
    }
    p->type = type;
    p->service = service;
    do_in_service_loop(hf_subscriber_number_handler, p);
}

static void hf_vgm_handler(void* data)
{
    hf_u8_event_params_t* p = (hf_u8_event_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    hfp_hf_on_volume_changed(&conn->addr, HFP_VOLUME_TYPE_MIC, p->value);
    free(p);
}

static void zblue_on_vgm(struct bt_hfp_hf* hf, uint8_t gain)
{
    hf_u8_event_params_t* p = malloc(sizeof(hf_u8_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->value = gain;
    do_in_service_loop(hf_vgm_handler, p);
}

static void hf_vgs_handler(void* data)
{
    hf_u8_event_params_t* p = (hf_u8_event_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    hfp_hf_on_volume_changed(&conn->addr, HFP_VOLUME_TYPE_SPK, p->value);
    free(p);
}

static void zblue_on_vgs(struct bt_hfp_hf* hf, uint8_t gain)
{
    hf_u8_event_params_t* p = malloc(sizeof(hf_u8_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->value = gain;
    do_in_service_loop(hf_vgs_handler, p);
}

static void hf_voice_recognition_handler(void* data)
{
    hf_bool_event_params_t* p = (hf_bool_event_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    hfp_hf_on_voice_recognition_state_changed(&conn->addr, p->activate);
    free(p);
}

static void zblue_on_voice_recognition(struct bt_hfp_hf* hf, bool activate)
{
    hf_bool_event_params_t* p = malloc(sizeof(hf_bool_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->activate = activate;
    do_in_service_loop(hf_voice_recognition_handler, p);
}

static void hf_ring_indication_handler(void* data)
{
    hf_call_event_params_t* p = (hf_call_event_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(p->call, NULL);

    if (!conn) {
        BT_LOGW("%s, Failed to find connection for ring", __func__);
        free(p);
        return;
    }

    hfp_hf_on_ring_active_state_changed(&conn->addr, true, HFP_IN_BAND_RINGTONE_NOT_PROVIDED);
    free(p);
}

static void zblue_on_ring_indication(struct bt_hfp_hf_call* call)
{
    hf_call_event_params_t* p = malloc(sizeof(hf_call_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    do_in_service_loop(hf_ring_indication_handler, p);
}

static void hf_clip_handler(void* data)
{
    hf_clip_params_t* p = (hf_clip_params_t*)data;
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(p->call, &sal_call);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection for CLIP", __func__);
        free(p);
        return;
    }

    if (sal_call) {
        sal_call->type = p->type;
    }

    hfp_hf_on_clip(&conn->addr, p->number, "");
    free(p);
}

static void zblue_on_clip(struct bt_hfp_hf_call* call, char* number, uint8_t type)
{
    hf_clip_params_t* p = malloc(sizeof(hf_clip_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->call = call;
    if (number) {
        strlcpy(p->number, number, sizeof(p->number));
    } else {
        p->number[0] = '\0';
    }
    p->type = type;
    do_in_service_loop(hf_clip_handler, p);
}

static void hf_vendor_specific_handler(void* data)
{
    hf_vendor_specific_params_t* p = (hf_vendor_specific_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection for vendor specific response", __func__);
        free(p);
        return;
    }

    size_t cmd_len = strlen(p->cmd);
    size_t val_len = strlen(p->value);
    size_t len = cmd_len + val_len + 2; /* '+' and ':' */

    char* rsp = malloc(len + 1);
    if (!rsp) {
        BT_LOGE("%s, Failed to allocate vendor response", __func__);
        free(p);
        return;
    }

    snprintf(rsp, len + 1, "+%s:%s", p->cmd, p->value);
    hfp_hf_on_received_at_cmd_resp(&conn->addr, rsp, len);
    free(rsp);
    free(p);
}

static void zblue_on_vendor_specific(struct bt_hfp_hf* hf, const char* cmd, const char* value)
{
    if (!cmd || !value) {
        return;
    }

    hf_vendor_specific_params_t* p = malloc(sizeof(hf_vendor_specific_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    strlcpy(p->cmd, cmd, sizeof(p->cmd));
    strlcpy(p->value, value, sizeof(p->value));
    do_in_service_loop(hf_vendor_specific_handler, p);
}

static hfp_atcmd_code_t zblue_at_cmd_to_service_cmd(
    enum bt_hfp_hf_at_cmd at_cmd)
{
    switch (at_cmd) {
    case BT_HFP_HF_AT_CMD_ATA:
        return HFP_ATCMD_CODE_ATA;
    case BT_HFP_HF_AT_CMD_ATD_NUMBER:
    case BT_HFP_HF_AT_CMD_ATD_MEMORY:
        return HFP_ATCMD_CODE_ATD;
    case BT_HFP_HF_AT_CMD_BLDN:
        return HFP_ATCMD_CODE_BLDN;
    default:
        return HFP_ATCMD_CODE_UNKNOWN;
    }
}

static void hf_at_cmd_complete_handler(void* data)
{
    hf_at_cmd_complete_params_t* p = (hf_at_cmd_complete_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection for AT cmd complete", __func__);
        free(p);
        return;
    }

    uint32_t result_code = p->result == BT_AT_RESULT_CME_ERROR ? p->err : p->result;

    hfp_hf_on_at_command_result_response(&conn->addr, zblue_at_cmd_to_service_cmd(p->cmd), result_code);
    free(p);
}

static void zblue_on_at_cmd_complete(struct bt_hfp_hf* hf, enum bt_hfp_hf_at_cmd cmd,
    enum bt_at_result result, enum bt_at_cme err)
{
    BT_LOGD("%s, AT cmd complete: cmd=%d, result=%d, err=%d", __func__, cmd, result, err);

    hf_at_cmd_complete_params_t* p = malloc(sizeof(hf_at_cmd_complete_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->cmd = cmd;
    p->result = result;
    p->err = err;
    do_in_service_loop(hf_at_cmd_complete_handler, p);
}

static void hf_codec_negotiate_handler(void* data)
{
    hf_u8_event_params_t* p = (hf_u8_event_params_t*)data;
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(p->hf);

    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    int ret = Z_API(bt_hfp_hf_select_codec)(p->hf, p->value);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_select_codec failed: %d", __func__, ret);
    }

    hfp_codec_config_t cfg = { 0 };
    switch (p->value) {
    case BT_HFP_HF_CODEC_MSBC:
        cfg.codec = HFP_CODEC_MSBC;
        cfg.sample_rate = HFP_CODEC_MSBC_SAMPLE_RATE;
        cfg.bit_width = HFP_CODEC_BIT_WIDTH;
        break;
    case BT_HFP_HF_CODEC_CVSD:
    default:
        cfg.codec = HFP_CODEC_CVSD;
        cfg.sample_rate = HFP_CODEC_CVSD_SAMPLE_RATE;
        cfg.bit_width = HFP_CODEC_BIT_WIDTH;
        break;
    }

    hfp_hf_on_codec_changed(&conn->addr, &cfg);
    free(p);
}

static void zblue_on_codec_negotiate(struct bt_hfp_hf* hf, uint8_t id)
{
    hf_u8_event_params_t* p = malloc(sizeof(hf_u8_event_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    p->value = id;
    do_in_service_loop(hf_codec_negotiate_handler, p);
}

static void hf_current_call_handler(void* data)
{
    hf_current_call_params_t* p = (hf_current_call_params_t*)data;
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(p->hf);

    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        free(p);
        return;
    }

    if (!p->has_call) {
        BT_ADDR_LOG("CLCC finished from %s", &sal_conn->addr);
        hfp_hf_on_current_call_response(&sal_conn->addr, 0, 0, 0, 0, NULL, 0);
        free(p);
        return;
    }

    struct bt_hfp_hf_current_call* call = &p->call_data;
    BT_LOGD("%s, CLCC %d: %s", __func__, call->index, call->number);

    bt_hfp_hf_call_info_t* sal_call = find_or_create_call(sal_conn, call->call);

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

    sal_call->index = idx;
    sal_call->state = status;

    hfp_hf_on_current_call_response(&sal_conn->addr, idx, dir, status, mpty, call->number, call->type);
    free(p);
}

static void zblue_on_current_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_current_call* call)
{
    hf_current_call_params_t* p = malloc(sizeof(hf_current_call_params_t));
    if (!p) {
        BT_LOGE("%s, malloc failed", __func__);
        return;
    }

    p->hf = hf;
    if (call) {
        p->has_call = true;
        memcpy(&p->call_data, call, sizeof(struct bt_hfp_hf_current_call));
    } else {
        p->has_call = false;
    }
    do_in_service_loop(hf_current_call_handler, p);
}

static struct bt_hfp_hf_cb hf_callbacks = {
    .connected = zblue_on_connected,
    .disconnected = zblue_hf_disconnected,
    .sco_connected = zblue_on_sco_connected,
    .sco_disconnected = zblue_on_sco_disconnected,
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
    .codec_negotiate = zblue_on_codec_negotiate,
    .ecnr_turn_off = NULL,
    .call_waiting = NULL,
    .voice_recognition = zblue_on_voice_recognition,
    .vre_state = NULL,
    .textual_representation = NULL,
    .request_phone_number = NULL,
    .subscriber_number = zblue_on_subscriber_number,
    .query_call = zblue_on_current_call,
    .vendor_specific = zblue_on_vendor_specific,
    .at_cmd_complete = zblue_on_at_cmd_complete,
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
    if (Z_API(bt_hfp_hf_unregister)()) {
        BT_LOGE("%s, Failed to unregister HFP HF callbacks", __func__);
    }

    if (g_sal_hf_conn_list) {
        bt_list_free(g_sal_hf_conn_list);
        g_sal_hf_conn_list = NULL;
    }

    return;
}

bt_status_t bt_sal_hfp_hf_connect(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (sal_conn) {
        BT_LOGW("%s, Connection already exists or in progress", __func__);
        return BT_STATUS_BUSY;
    }

    return bt_sal_profile_connect_request(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT, 0, do_hf_sdp_discover, NULL);
}

bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }

    return bt_sal_profile_disconnect_request(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT, 0, do_hf_disconnect, NULL);
}

bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }

    if (!sal_conn->hf) {
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, connection is initializing for address: %s", __func__, addr_str);
        return BT_STATUS_NOT_READY;
    }

    int err = Z_API(bt_hfp_hf_audio_connect)(sal_conn->hf);
    if (err == -EALREADY) {
        BT_LOGW("%s, Audio already connected", __func__);
        hfp_hf_on_audio_connection_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_CONNECTED, 0);
    } else if (err) {
        BT_LOGE("%s, Failed to connect HFP HF SCO, err=%d", __func__, err);
        hfp_hf_on_audio_connection_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_DISCONNECTED, 0);
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    if (!sal_conn->sco_conn) {
        BT_LOGW("%s, SCO not connected", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    int err = bt_conn_disconnect(sal_conn->sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        sal_conn->sco_conn = NULL;
        BT_LOGE("%s, Failed to disconnect HFP HF SCO, err=%d", __func__, err);
        hfp_hf_on_audio_connection_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_DISCONNECTED, 0);
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
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
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
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
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }

    int ret = Z_API(bt_hfp_hf_hold_active_accept_other)(sal_conn->hf);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hangup_call(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }

    bt_hfp_hf_call_info_t* target = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_ACTIVE);
    if (!target) {
        target = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_DIALING);
    }
    if (!target) {
        target = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_ALERTING);
    }

    if (!target) {
        BT_LOGE("%s, No active/dialing/alerting call to hang up", __func__);
        return BT_STATUS_FAIL;
    }

    if (count_call(sal_conn) == 1) {
        SAL_CHECK_RET(Z_API(bt_hfp_hf_terminate)(target->context), 0);
    } else {
        SAL_CHECK_RET(Z_API(bt_hfp_hf_release_active_accept_other)(sal_conn->hf), 0);
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_number(bt_address_t* addr, const char* number)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    if (!number) {
        SAL_CHECK_RET(Z_API(bt_hfp_hf_redial)(sal_conn->hf), 0);
        return BT_STATUS_SUCCESS;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_hf_number_call)(sal_conn->hf, number), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    char mem_in_str[HFP_PHONENUM_DIGITS_MAX + 1];
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    snprintf(mem_in_str, sizeof(mem_in_str), "%" PRIu32, memory);
    SAL_CHECK_RET(Z_API(bt_hfp_hf_memory_dial)(sal_conn->hf, mem_in_str), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_call_control(bt_address_t* addr, hfp_call_control_t chld, uint32_t index)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    int ret = -ENOTSUP;
    switch (chld) {
    case HFP_HF_CALL_CONTROL_CHLD_0: {
        bt_hfp_hf_call_info_t* waiting = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_WAITING);
        if (waiting) {
            ret = Z_API(bt_hfp_hf_set_udub)(sal_conn->hf);
        } else {
            bt_hfp_hf_call_info_t* held = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_HELD);
            if (held) {
                ret = Z_API(bt_hfp_hf_release_all_held)(sal_conn->hf);
            } else {
                BT_LOGW("%s, No waiting/held call for CHLD=0", __func__);
                return BT_STATUS_PARM_INVALID;
            }
        }
        break;
    }
    case HFP_HF_CALL_CONTROL_CHLD_1:
        if (index > 0) {
            bt_hfp_hf_call_info_t* by_idx = find_call_by_index(sal_conn, (uint8_t)index);
            if (!by_idx) {
                BT_LOGE("%s, No call with index %u for CHLD=1<idx>", __func__, (unsigned)index);
                return BT_STATUS_PARM_INVALID;
            }
            ret = Z_API(bt_hfp_hf_release_specified_call)(by_idx->context);
        } else {
            ret = Z_API(bt_hfp_hf_release_active_accept_other)(sal_conn->hf);
        }
        break;
    case HFP_HF_CALL_CONTROL_CHLD_2:
        if (index > 0) {
            bt_hfp_hf_call_info_t* by_idx = find_call_by_index(sal_conn, (uint8_t)index);
            if (!by_idx) {
                BT_LOGE("%s, No call with index %u for CHLD=2<idx>", __func__, (unsigned)index);
                return BT_STATUS_PARM_INVALID;
            }
            ret = Z_API(bt_hfp_hf_private_consultation_mode)(by_idx->context);
        } else {
            ret = Z_API(bt_hfp_hf_hold_active_accept_other)(sal_conn->hf);
        }
        break;
    case HFP_HF_CALL_CONTROL_CHLD_3:
        ret = Z_API(bt_hfp_hf_join_conversation)(sal_conn->hf);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_4:
        ret = Z_API(bt_hfp_hf_explicit_call_transfer)(sal_conn->hf);
        break;
    default:
        return BT_STATUS_UNSUPPORTED;
    }

    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_hf_query_list_of_current_calls)(sal_conn->hf), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    uint8_t gain = volume > 15 ? 15 : volume;

    int ret;
    switch (type) {
    case HFP_VOLUME_TYPE_MIC:
        ret = Z_API(bt_hfp_hf_vgm)(sal_conn->hf, gain);
        break;
    case HFP_VOLUME_TYPE_SPK:
        ret = Z_API(bt_hfp_hf_vgs)(sal_conn->hf, gain);
        break;
    default:
        BT_LOGE("%s, Unknown volume type: %d", __func__, type);
        return BT_STATUS_PARM_INVALID;
    }

    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_start_voice_recognition(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    int ret = Z_API(bt_hfp_hf_voice_recognition)(sal_conn->hf, true);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_stop_voice_recognition(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    int ret = Z_API(bt_hfp_hf_voice_recognition)(sal_conn->hf, false);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_battery_level(bt_address_t* addr, uint8_t value)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    uint8_t level = (value > 100) ? 100 : value;

    int ret = Z_API(bt_hfp_hf_battery)(sal_conn->hf, level);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_at_cmd(bt_address_t* addr, const char* cmd, uint16_t len)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    if (!cmd || len == 0) {
        BT_LOGE("%s, Invalid AT command", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    BT_LOGD("%s, Sending AT command: %.*s", __func__, len, cmd);

    int ret = Z_API(bt_hfp_hf_send_vendor)(sal_conn->hf, cmd);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_dtmf(bt_address_t* addr, char dtmf)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_call_info_t* active = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_ACTIVE);
    if (!active) {
        BT_LOGE("%s, No active call for DTMF", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    int ret = Z_API(bt_hfp_hf_transmit_dtmf_code)(active->context, dtmf);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_get_subscriber_number(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_hf_query_subscriber)(sal_conn->hf), 0);

    return BT_STATUS_SUCCESS;
}
