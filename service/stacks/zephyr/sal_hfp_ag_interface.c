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
#define LOG_TAG "zblue_sal_ag"

#include "sal_hfp_ag_interface.h"
#include "bt_debug.h"
#include "bt_hfp.h"
#include "bt_hfp_ag.h"
#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_zblue.h"
#include "service_loop.h"

#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128

#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/at.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>
#include <zephyr/bluetooth/classic/sdp.h>

#define HFP_AG_CODEC_Z_BIT_CVSD BIT(BT_HFP_AG_CODEC_CVSD)
#define HFP_AG_CODEC_Z_BIT_MSBC BIT(BT_HFP_AG_CODEC_MSBC)
#define HFP_AG_CODEC_Z_BIT_LC3_SWB BIT(BT_HFP_AG_CODEC_LC3_SWB)

static bt_list_t* g_sal_ag_conn_list = NULL;

extern struct net_buf_pool sdp_pool;

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result, const struct bt_sdp_discover_params* ignore);

static struct bt_sdp_discover_params sdp_discover = {
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
    struct bt_conn* sco_context;
    struct bt_hfp_ag* ag;
    uint8_t preferred_codec;
    bt_list_t* calls;
} bt_hfp_ag_connection_t;

typedef struct _ag_connect_params {
    struct bt_conn* conn;
    uint8_t channel;
} ag_connect_params_t;

typedef struct _ag_connect_sco_params {
    struct bt_hfp_ag* ag;
    uint8_t codec; /* e.g., BT_HFP_AG_CODEC_CVSD */
} ag_connect_sco_params_t;

typedef struct _ag_disconnect_sco_params {
    struct bt_conn* sco_context;
} ag_disconnect_sco_params_t;

// TODO: remove g_conn_params later when bt_sal_profile_connect_request can carry a userdata.
static ag_connect_params_t* g_conn_params = NULL;

static void free_connection(void* data)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)data;
    if (sal_conn->calls) {
        bt_list_free(sal_conn->calls);
        sal_conn->calls = NULL;
    }
    free(sal_conn);
}

static void free_call(void* data)
{
    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)data;
    free(sal_call);
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

static bool sal_conn_sco_context_cmp(void* sal_context, void* z_context)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)sal_context;
    struct bt_conn* sco_conn = (struct bt_conn*)z_context;
    return sal_conn && sal_conn->sco_context == sco_conn;
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

static bt_hfp_ag_connection_t* find_connection_by_sco_context(struct bt_conn* sco_conn)
{
    if (!g_sal_ag_conn_list) {
        BT_LOGE("%s, ag conn list not initialized", __func__);
        return NULL;
    }
    return (bt_hfp_ag_connection_t*)bt_list_find(g_sal_ag_conn_list, sal_conn_sco_context_cmp, sco_conn);
}

static bt_hfp_ag_connection_t* new_sal_connection(struct bt_conn* conn, struct bt_hfp_ag* ag)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)zalloc(sizeof(bt_hfp_ag_connection_t));

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

    sal_conn->context = conn;
    sal_conn->ag = ag;
    sal_conn->preferred_codec = BT_HFP_AG_CODEC_CVSD; /* Initialize to default CVSD codec */
    sal_conn->calls = bt_list_new(free_call);

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
        return BT_HFP_AG_CALL_STATUS_UNKNOWN;
    }
}

static bool sal_call_context_cmp(void* sal_context, void* z_context)
{
    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)sal_context;
    struct bt_hfp_ag_call* call = (struct bt_hfp_ag_call*)z_context;
    return sal_call->context == call;
}

static bool sal_call_number_cmp(void* sal_context, void* data)
{
    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)sal_context;
    const char* number = (const char*)data;
    return sal_call && strcmp(sal_call->number, number) == 0;
}

static bt_hfp_ag_call_info_t* find_call_by_context(struct bt_hfp_ag_call* z_context,
    bt_hfp_ag_connection_t** sal_conn_out)
{
    if (!g_sal_ag_conn_list || !z_context) {
        return NULL;
    }

    bt_list_node_t* node;
    for (node = bt_list_head(g_sal_ag_conn_list); node != NULL;
         node = bt_list_next(g_sal_ag_conn_list, node)) {
        bt_hfp_ag_connection_t* sal_conn = bt_list_node(node);
        if (!sal_conn || !sal_conn->calls) {
            continue;
        }
        bt_hfp_ag_call_info_t* call = (bt_hfp_ag_call_info_t*)bt_list_find(sal_conn->calls, sal_call_context_cmp, z_context);
        if (call) {
            if (sal_conn_out) {
                *sal_conn_out = sal_conn;
            }
            return call;
        }
    }

    return NULL;
}

static bt_hfp_ag_call_info_t* find_call_by_number(bt_hfp_ag_connection_t* sal_conn, const char* number)
{
    if (!sal_conn || !sal_conn->calls || !number) {
        return NULL;
    }

    return (bt_hfp_ag_call_info_t*)bt_list_find(sal_conn->calls, sal_call_number_cmp, (void*)number);
}

static enum bt_hfp_ag_call_dir service_call_dir_to_sal_dir(hfp_call_direction_t tele_dir)
{
    switch (tele_dir) {
    case HFP_CALL_DIRECTION_OUTGOING:
        return BT_HFP_AG_CALL_DIR_OUTGOING;

    case HFP_CALL_DIRECTION_INCOMING:
        return BT_HFP_AG_CALL_DIR_INCOMING;

    default:
        return BT_HFP_AG_CALL_DIR_UNKNOWN;
    }
}

static bt_hfp_ag_call_info_t* new_sal_call()
{
    bt_hfp_ag_call_info_t* call = (bt_hfp_ag_call_info_t*)zalloc(sizeof(bt_hfp_ag_call_info_t));
    if (!call) {
        BT_LOGE("%s, failed to allocate call entry", __func__);
        return NULL;
    }

    call->context = NULL;
    return call;
}

static bt_hfp_ag_call_info_t* build_sal_call(
    hfp_call_direction_t dir, hfp_ag_call_state_t call,
    hfp_call_addrtype_t type, const char* number)
{
    enum bt_hfp_ag_call_status state = tele_call_state_to_sal_status(call);
    if (state == BT_HFP_AG_CALL_STATUS_UNKNOWN) {
        return NULL;
    }

    bt_hfp_ag_call_info_t* sal_call = new_sal_call();
    if (!sal_call) {
        BT_LOGE("%s, failed to allocate memory", __func__);
        return NULL;
    }

    sal_call->state = state;
    sal_call->dir = service_call_dir_to_sal_dir(dir);
    if (sal_call->dir == BT_HFP_AG_CALL_DIR_UNKNOWN) {
        BT_LOGE("%s, invalid call direction", __func__);
        free(sal_call);
        return NULL;
    }

    sal_call->type = type;
    if (number) {
        strlcpy(sal_call->number, number, sizeof(sal_call->number));
    } else {
        sal_call->number[0] = '\0';
    }

    return sal_call;
}

static bt_hfp_ag_call_info_t* update_sal_call(bt_hfp_ag_connection_t* conn,
    hfp_call_direction_t dir, hfp_ag_call_state_t call, hfp_call_mode_t mode,
    hfp_call_mpty_type_t mpty, hfp_call_addrtype_t type, const char* number)
{
    bt_hfp_ag_call_info_t* sal_call = find_call_by_number(conn, number);
    if (!sal_call) {
        sal_call = build_sal_call(dir, call, type, number);
        if (!sal_call) {
            return NULL;
        }

        if (!conn->calls) {
            conn->calls = bt_list_new(free_call);
        }

        bt_list_add_head(conn->calls, sal_call);
        return sal_call;
    }

    sal_call->state = tele_call_state_to_sal_status(call);
    sal_call->dir = service_call_dir_to_sal_dir(dir);
    if (sal_call->state == BT_HFP_AG_CALL_STATUS_UNKNOWN || sal_call->dir == BT_HFP_AG_CALL_DIR_UNKNOWN) {
        bt_hfp_ag_connection_t* owner = NULL;
        find_call_by_context(sal_call->context, &owner);
        if (owner && owner->calls) {
            bt_list_remove(owner->calls, sal_call);
        }
        return NULL;
    }

    sal_call->type = type;

    return sal_call;
}

static bt_status_t do_ag_connect(bt_controller_id_t id, bt_address_t* addr, void* userdata)
{
    struct bt_hfp_ag* ag = NULL;
    uint8_t channel;

    /** It is assumed that ACL is established hereafter */
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    struct bt_conn* conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    /** Remember to unref @p conn once SLC is initiated or cancelled */
    if (!conn) {
        BT_LOGE("%s, Failed to lookup connection", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    /** Step 1: SDP discovery */
    if (g_conn_params == NULL) {
        BT_LOGD("%s, SDP not discovered", __func__);
        if (bt_sdp_discover(conn, &sdp_discover) < 0) {
            BT_LOGE("%s, Failed to start a SDP discovery", __func__);
            bt_conn_unref(conn);
            return BT_STATUS_FAIL;
        }

        bt_conn_unref(conn);
        return BT_STATUS_SUCCESS;
    }

    /** Step 2: SLC initiating */
    channel = g_conn_params->channel;
    free(g_conn_params);
    g_conn_params = NULL;

    BT_LOGD("%s, SLC initiating", __func__);
    if (Z_API(bt_hfp_ag_connect)(conn, &ag, channel)) {
        BT_LOGE("%s, Failed to initiate HFP ag connection", __func__);
        bt_conn_unref(conn);
        return BT_STATUS_FAIL;
    }

    if (new_sal_connection(conn, ag) == NULL) {
        BT_LOGE("%s, Failed to create HFP AG connection", __func__);
        if (Z_API(bt_hfp_ag_disconnect)(ag)) {
            BT_LOGE("%s, Failed disconnect HFP", __func__);
        }
        bt_conn_unref(conn);
        return BT_STATUS_NOMEM;
    }

    hfp_ag_on_connection_state_changed(addr, PROFILE_STATE_CONNECTING, 0, 0);
    bt_conn_unref(conn);

    BT_LOGD("%s, HFP AG connecting", __func__);
    return BT_STATUS_SUCCESS;
}

bt_status_t do_ag_disconnect(bt_controller_id_t id, bt_address_t* addr, void* userdata)
{
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }
    if (!sal_conn->ag) {
        BT_LOGE("%s, HFP AG not connected", __func__);
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_disconnect)(sal_conn->ag), 0);
    return BT_STATUS_SUCCESS;
}

static int hfp_codec_to_service_cfg(uint8_t codec_id, hfp_codec_config_t* cfg)
{
    if (!cfg) {
        return -EINVAL;
    }

    switch (codec_id) {
    case BT_HFP_AG_CODEC_MSBC:
        cfg->codec = HFP_CODEC_MSBC;
        cfg->sample_rate = 16000;
        cfg->bit_width = 16;
        return 0;
    case BT_HFP_AG_CODEC_CVSD:
        cfg->codec = HFP_CODEC_CVSD;
        cfg->sample_rate = 8000;
        cfg->bit_width = 16;
        return 0;
    default:
        return -ENOTSUP;
    }
}

static void do_ag_sco_connect(service_work_t* work, void* userdata)
{
    ag_connect_sco_params_t* params;
    struct bt_hfp_ag* ag;
    uint8_t codec;
    bt_hfp_ag_connection_t* sal_conn;
    hfp_codec_config_t cfg = { 0 };
    int err;

    params = (ag_connect_sco_params_t*)userdata;
    if (!params) {
        BT_LOGE("%s, Invalid parameters", __func__);
        return;
    }

    if (!params->ag) {
        BT_LOGE("%s, Invalid ag parameter", __func__);
        free(params);
        return;
    }

    ag = params->ag;
    codec = params->codec;
    free(params);

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGW("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    hfp_ag_on_audio_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_CONNECTING, 0xFFFF); // sco conn handle not supported

    err = Z_API(bt_hfp_ag_audio_connect)(ag, codec);
    if (err == -EALREADY) {
        BT_LOGW("%s, Audio already connected, notify CONNECTED", __func__);
        hfp_ag_on_audio_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_CONNECTED, 0xFFFF);
    } else if ((err == -ENOTSUP || err == -EINVAL) && codec != BT_HFP_AG_CODEC_CVSD) {
        BT_LOGW("%s, codec=%d not supported, fallback to CVSD", __func__, codec);
        sal_conn->preferred_codec = BT_HFP_AG_CODEC_CVSD;
        hfp_codec_to_service_cfg(BT_HFP_AG_CODEC_CVSD, &cfg);
        hfp_ag_on_codec_changed(&sal_conn->addr, &cfg);
        err = Z_API(bt_hfp_ag_audio_connect)(ag, BT_HFP_AG_CODEC_CVSD);
        if (err) {
            BT_LOGE("%s, Failed to connect HFP AG SCO with CVSD fallback, err=%d", __func__, err);
            hfp_ag_on_audio_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_DISCONNECTED, 0xFFFF);
        }
    } else if (err) {
        BT_LOGE("%s, Failed to connect HFP AG SCO, err=%d", __func__, err);
        hfp_ag_on_audio_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_DISCONNECTED, 0xFFFF);
    }
}

static void do_ag_sco_disconnect(service_work_t* work, void* userdata)
{
    ag_disconnect_sco_params_t* params;
    struct bt_conn* sco_context;
    bt_hfp_ag_connection_t* sal_conn;
    int err;

    params = (ag_disconnect_sco_params_t*)userdata;
    if (!params) {
        BT_LOGE("%s, Invalid parameters", __func__);
        return;
    }

    if (!params->sco_context) {
        BT_LOGE("%s, Invalid sco_context parameter", __func__);
        free(params);
        return;
    }

    sco_context = params->sco_context;
    free(params);

    sal_conn = find_connection_by_sco_context(sco_context);
    if (!sal_conn) {
        BT_LOGW("%s, sco_context no longer tracked, skip disconnect", __func__);
        return;
    }

    hfp_ag_on_audio_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_DISCONNECTING, 0xFFFF);

    err = bt_conn_disconnect(sco_context, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        BT_LOGE("%s, Failed to disconnect HFP AG SCO, err=%d", __func__, err);
    }
}

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result,
    const struct bt_sdp_discover_params* ignore)
{
    int err;
    uint16_t port;

    bt_address_t bd_addr;
    if (bt_sal_get_remote_address(conn, &bd_addr) != BT_STATUS_SUCCESS) {
        return BT_SDP_DISCOVER_UUID_STOP;
    }

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
        BT_LOGE("Fail to parse AG RFCOMM port!");
        goto error;
    }

    BT_LOGD("%s, SDP discovery done for HFP AG, AG RFCOMM port: %u", __func__, port);
    if (g_conn_params != NULL) {
        BT_LOGE("%s, Previous connection ongoing", __func__);
        goto error;
    }

    /** TODO: remove @p g_conn_params, send @p port as a context to
     *        @ref bt_sal_profile_connect_request */
    g_conn_params = (ag_connect_params_t*)zalloc(sizeof(ag_connect_params_t));
    if (g_conn_params == NULL) {
        BT_LOGE("%s, Failed to allocate memory for new HFP AG connection", __func__);
        goto error;
    }

    g_conn_params->channel = (uint8_t)port;

    if (do_ag_connect(0 /* bt_controller_id_t */, &bd_addr, NULL) != BT_STATUS_SUCCESS) {
        free(g_conn_params);
        g_conn_params = NULL;
        goto error;
    }

    return BT_SDP_DISCOVER_UUID_STOP;

error:
    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
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

    bt_sal_cm_profile_connected_callback(&bd_addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&bd_addr, PROFILE_HFP_AG, CONN_ID_DEFAULT, PRIMARY_ADAPTER, do_ag_disconnect, NULL);

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

    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);

    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTING, 0, 0);
    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
}

static void zblue_on_ag_sco_connected(struct bt_hfp_ag* ag, struct bt_conn* sco_conn)
{
    BT_LOGD("%s, HFP AG SCO connected, ag=%p", __func__, ag);
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    sal_conn->sco_context = sco_conn;

    hfp_ag_on_audio_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_CONNECTED, 0xFFFF); // sco conn handle not supported
}

static void zblue_on_ag_sco_disconnected(struct bt_conn* sco_conn, uint8_t reason)
{
    BT_LOGD("%s, HFP AG SCO disconnected, reason=%d", __func__, reason);
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_sco_context(sco_conn);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    sal_conn->sco_context = NULL;

    hfp_ag_on_audio_state_changed(&sal_conn->addr, HFP_AUDIO_STATE_DISCONNECTED, 0xFFFF); // sco conn handle not supported
}

static int zblue_on_ag_vendor_at_cmd(struct bt_hfp_ag* ag, const char* cmd, uint8_t* cme_code)
{
    bt_hfp_ag_connection_t* sal_conn;

    BT_LOGD("%s, cmd:%s", __func__, cmd ? cmd : "(null)");

    if (!ag || !cmd) {
        return -EINVAL;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return -ENOTCONN;
    }

    hfp_ag_on_received_at_cmd(&sal_conn->addr, cmd, (uint16_t)strlen(cmd));

    return -EINPROGRESS;
}

static int zblue_on_ag_get_ongoing_call(struct bt_hfp_ag* ag)
{
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_ag(ag);

    if (!sal_conn) {
        struct bt_conn* conn = Z_API(bt_hfp_ag_get_conn)(ag);
        if (!conn) {
            BT_LOGE("%s, failed to get conn for ag=%p", __func__, ag);
            return -EINVAL;
        }
        BT_LOGD("%s, connection not found for ag=%p, creating new", __func__, ag);
        sal_conn = new_sal_connection(conn, ag);
        bt_conn_unref(conn);
        if (!sal_conn) {
            BT_LOGE("%s, failed to create new sal conn", __func__);
            return -EINVAL;
        }
        hfp_ag_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_CONNECTING, 0, 0);
    }

    hfp_ag_on_call_sync(&sal_conn->addr);
    hfp_ag_on_received_cind_request(&sal_conn->addr);
    return 0;
}

static int zblue_on_ag_memory_dial(struct bt_hfp_ag* ag, const char* location, char** number)
{
    return -ENOTSUP;
}

static int zblue_on_ag_number_call(struct bt_hfp_ag* ag, const char* number)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!ag || !number) {
        return -EINVAL;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return -EINVAL;
    }

    hfp_ag_on_dial_number(&sal_conn->addr, (char*)number, strlen(number));

    return -EINPROGRESS;
}

static void zblue_on_ag_outgoing(struct bt_hfp_ag* ag, struct bt_hfp_ag_call* call, const char* number)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_hfp_ag_call_info_t* sal_call;

    if (!ag || !call || !number) {
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    sal_call = find_call_by_number(sal_conn, number);
    if (!sal_call) {
        BT_LOGE("%s, call with number=%s not tracked", __func__, number);
    } else {
        sal_call->dir = BT_HFP_AG_CALL_DIR_OUTGOING;
        sal_call->state = tele_call_state_to_sal_status(HFP_AG_CALL_STATE_DIALING);
        sal_call->context = call;
    }
}

static void zblue_on_ag_incoming(struct bt_hfp_ag* ag, struct bt_hfp_ag_call* call, const char* number)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_hfp_ag_call_info_t* sal_call;

    if (!ag || !call || !number) {
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    sal_call = find_call_by_number(sal_conn, number);
    if (!sal_call) {
        BT_LOGE("%s, call with number=%s not tracked", __func__, number);
    } else {
        sal_call->dir = BT_HFP_AG_CALL_DIR_INCOMING;
        sal_call->state = tele_call_state_to_sal_status(HFP_AG_CALL_STATE_INCOMING);
        sal_call->context = call;
    }
}

static void zblue_on_ag_incoming_held(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;

    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }
    if (!sal_call) {
        BT_LOGE("%s, call not tracked", __func__);
    } else {
        sal_call->dir = BT_HFP_AG_CALL_DIR_INCOMING;
        sal_call->state = tele_call_state_to_sal_status(HFP_AG_CALL_STATE_WAITING);
        sal_call->context = call;
    }
}

static void zblue_on_ag_accept(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;

    if (!call) {
        return;
    }

    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_INCOMING && sal_call->state != BT_HFP_AG_CALL_STATUS_WAITING) {
        return;
    }

    hfp_ag_on_answer_call(&sal_conn->addr);
}

static void zblue_on_ag_held(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;

    if (!call) {
        return;
    }

    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_ACTIVE) {
        return;
    }

    hfp_ag_on_hangup_call(&sal_conn->addr);
}

static void zblue_on_ag_retrieve(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;

    if (!call) {
        return;
    }

    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_HELD) {
        return;
    }

    hfp_ag_on_call_control(&sal_conn->addr, HFP_HF_CALL_CONTROL_CHLD_2);
}

static void zblue_on_ag_reject(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;

    if (!call) {
        return;
    }

    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_INCOMING) {
        return;
    }

    hfp_ag_on_reject_call(&sal_conn->addr);
}

static void zblue_on_ag_terminate(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;

    if (!call) {
        return;
    }

    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_ACTIVE) {
        return;
    }

    hfp_ag_on_call_control(&sal_conn->addr, HFP_HF_CALL_CONTROL_CHLD_1);
}

static void zblue_on_ag_available_codec(struct bt_hfp_ag* ag, uint32_t codec_ids)
{
    bt_hfp_ag_connection_t* sal_conn;
    hfp_codec_config_t cfg = { 0 };
    int err;

    if (!ag) {
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        struct bt_conn* conn = Z_API(bt_hfp_ag_get_conn)(ag);
        if (!conn) {
            BT_LOGE("%s, failed to get conn for ag=%p", __func__, ag);
            return;
        }
        BT_LOGD("%s, connection not found for ag=%p, creating new", __func__, ag);
        sal_conn = new_sal_connection(conn, ag);
        bt_conn_unref(conn);
        if (!sal_conn) {
            BT_LOGE("%s, failed to create new sal conn", __func__);
            return;
        }

        hfp_ag_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_CONNECTING, 0, 0);
    }

    if (codec_ids & HFP_AG_CODEC_Z_BIT_MSBC) {
        BT_LOGD("%s, prefer mSBC", __func__);
        sal_conn->preferred_codec = BT_HFP_AG_CODEC_MSBC;
    } else if (codec_ids & HFP_AG_CODEC_Z_BIT_CVSD) {
        BT_LOGD("%s, prefer CVSD", __func__);
        sal_conn->preferred_codec = BT_HFP_AG_CODEC_CVSD;
    } else {
        BT_LOGW("%s, could not find support codec in codec_ids: %" PRIu32,
            __func__, codec_ids);
        sal_conn->preferred_codec = 0;
    }

    err = hfp_codec_to_service_cfg(sal_conn->preferred_codec, &cfg);
    if (err != 0) {
        if (err == -EINVAL) {
            BT_LOGE("%s, invalid cfg pointer", __func__);
        } else {
            BT_LOGE("%s, unsupported codec id: %d", __func__, sal_conn->preferred_codec);
        }
        return;
    }

    hfp_ag_on_codec_changed(&sal_conn->addr, &cfg);
}

static void zblue_on_ag_audio_connect_req(struct bt_hfp_ag* ag)
{
    bt_hfp_ag_connection_t* sal_conn;
    hfp_codec_config_t cfg = { 0 };
    ag_connect_sco_params_t* params;
    uint8_t codec;
    int err;

    if (!ag) {
        BT_LOGE("%s, ag is NULL", __func__);
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    codec = sal_conn->preferred_codec ? sal_conn->preferred_codec : BT_HFP_AG_CODEC_CVSD;

    BT_LOGD("%s, HF requested audio connect, using codec=%d", __func__, codec);

    err = hfp_codec_to_service_cfg(codec, &cfg);
    if (err != 0) {
        if (err == -EINVAL) {
            BT_LOGE("%s, invalid cfg pointer", __func__);
            return;
        }
        BT_LOGE("%s, unsupported codec id: %d, fallback to CVSD", __func__, codec);
        codec = BT_HFP_AG_CODEC_CVSD;
        hfp_codec_to_service_cfg(BT_HFP_AG_CODEC_CVSD, &cfg);
    }

    /* Report the actual codec that will be used for this audio connection */
    hfp_ag_on_codec_changed(&sal_conn->addr, &cfg);

    params = (ag_connect_sco_params_t*)zalloc(sizeof(ag_connect_sco_params_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate memory", __func__);
        return;
    }

    params->ag = ag;
    params->codec = codec;

    if (!service_loop_work(params, do_ag_sco_connect, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
    }
}

static void zblue_on_ag_codec_negotiation(struct bt_hfp_ag* ag, int zblue_err)
{
    bt_hfp_ag_connection_t* sal_conn;
    hfp_codec_config_t cfg = { 0 };
    int err;

    if (!ag) {
        return;
    }

    if (!zblue_err) {
        BT_LOGD("%s, codec negotiation success", __func__);
        return;
    }

    BT_LOGE("%s, fail: %d, fallback to CVSD", __func__, zblue_err);

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    sal_conn->preferred_codec = BT_HFP_AG_CODEC_CVSD;
    err = hfp_codec_to_service_cfg(BT_HFP_AG_CODEC_CVSD, &cfg);
    if (err != 0) {
        BT_LOGE("%s, fallback codec config failed: %d", __func__, err);
        return;
    }
    hfp_ag_on_codec_changed(&sal_conn->addr, &cfg);
}

static void zblue_on_ag_vgm(struct bt_hfp_ag* ag, uint8_t gain)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!ag) {
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    hfp_ag_on_volume_changed(&sal_conn->addr, HFP_VOLUME_TYPE_MIC, gain);
}

static void zblue_on_ag_vgs(struct bt_hfp_ag* ag, uint8_t gain)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!ag) {
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    hfp_ag_on_volume_changed(&sal_conn->addr, HFP_VOLUME_TYPE_SPK, gain);
}

static void zblue_on_ag_voice_recognition(struct bt_hfp_ag* ag, bool activate)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!ag) {
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    hfp_ag_on_voice_recognition_state_changed(&sal_conn->addr, activate);
}

static void zblue_on_ag_transmit_dtmf_code(struct bt_hfp_ag* ag, char code)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!ag) {
        return;
    }

    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    hfp_ag_on_received_dtmf(&sal_conn->addr, code);
}

static struct bt_hfp_ag_cb g_hfp_ag_cb = {
    .connected = zblue_on_ag_connected,
    .disconnected = zblue_on_ag_disconnected,
    .sco_connected = zblue_on_ag_sco_connected,
    .sco_disconnected = zblue_on_ag_sco_disconnected,
    .get_ongoing_call = zblue_on_ag_get_ongoing_call,
    .memory_dial = zblue_on_ag_memory_dial,
    .number_call = zblue_on_ag_number_call,
    .outgoing = zblue_on_ag_outgoing,
    .incoming = zblue_on_ag_incoming,
    .incoming_held = zblue_on_ag_incoming_held,
    .ringing = NULL,
    .accept = zblue_on_ag_accept,
    .held = zblue_on_ag_held,
    .retrieve = zblue_on_ag_retrieve,
    .reject = zblue_on_ag_reject,
    .terminate = zblue_on_ag_terminate,
    .codec = zblue_on_ag_available_codec,
    .codec_negotiate = zblue_on_ag_codec_negotiation,
    .audio_connect_req = zblue_on_ag_audio_connect_req,
    .vgm = zblue_on_ag_vgm,
    .vgs = zblue_on_ag_vgs,
    .ecnr_turn_off = NULL,
    .explicit_call_transfer = NULL,
    .voice_recognition = zblue_on_ag_voice_recognition,
    .ready_to_accept_audio = NULL,
    .request_phone_number = NULL,
    .transmit_dtmf_code = zblue_on_ag_transmit_dtmf_code,
    .subscriber_number = NULL,
    .hf_indicator_value = NULL,
    .vendor_at_cmd = zblue_on_ag_vendor_at_cmd,
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
    if (g_sal_ag_conn_list) {
        bt_list_free(g_sal_ag_conn_list);
        g_sal_ag_conn_list = NULL;
    }

    if (Z_API(bt_hfp_ag_unregister)()) {
        BT_LOGE("%s, Failed to unregister HFP AG", __func__);
    }
}

bt_status_t bt_sal_hfp_ag_connect(bt_address_t* addr)
{
    /** FIXME: @p g_conn_params might be NULL even when the previous ACL is connecting */
    if (g_conn_params != NULL) {
        BT_LOGE("%s, Previous connection ongoing", __func__);
        return BT_STATUS_BUSY;
    }

    return bt_sal_profile_connect_request(addr, PROFILE_HFP_AG, CONN_ID_DEFAULT, 0, do_ag_connect, NULL);
}

bt_status_t bt_sal_hfp_ag_disconnect(bt_address_t* addr)
{
    bt_hfp_ag_connection_t* conn = find_connection_by_addr(addr);
    if (!conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_sal_profile_disconnect_request(addr, PROFILE_HFP_AG, CONN_ID_DEFAULT, 0, do_ag_disconnect, NULL);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_connect_audio(bt_address_t* addr)
{
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    ag_connect_sco_params_t* params = (ag_connect_sco_params_t*)zalloc(sizeof(ag_connect_sco_params_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate memory", __func__);
        return BT_STATUS_NOMEM;
    }

    params->ag = sal_conn->ag;
    params->codec = sal_conn->preferred_codec ? sal_conn->preferred_codec : BT_HFP_AG_CODEC_CVSD;

    if (!service_loop_work(params, do_ag_sco_connect, NULL)) {
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_disconnect_audio(bt_address_t* addr)
{
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (!sal_conn->context) {
        BT_LOGE("%s, ACL conn context not initiated", __func__);
        return BT_STATUS_FAIL;
    }

    if (!sal_conn->sco_context) {
        BT_LOGE("%s, SCO connection not initiated", __func__);
        return BT_STATUS_FAIL;
    }

    ag_disconnect_sco_params_t* params = (ag_disconnect_sco_params_t*)zalloc(sizeof(ag_disconnect_sco_params_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate memory", __func__);
        return BT_STATUS_NOMEM;
    }

    params->sco_context = sal_conn->sco_context;

    if (!service_loop_work(params, do_ag_sco_disconnect, NULL)) {
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_start_voice_recognition(bt_address_t* addr)
{
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_voice_recognition)(sal_conn->ag, true), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_stop_voice_recognition(bt_address_t* addr)
{
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_voice_recognition)(sal_conn->ag, false), 0);
    return BT_STATUS_SUCCESS;
}

typedef struct {
    bt_hfp_ag_call_info_t* call_info; /* includes call_context + number */
    bt_hfp_ag_connection_t* connection; /* includes ag pointer */
} hfp_ag_operation_context_t;

/* unified wrapper signature */
typedef int (*call_operation_t)(hfp_ag_operation_context_t* operation_context);

/* ============================================================
 * Wrapper
 * ============================================================ */

static int accept_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_accept)(operation_context->call_info->context);
}

static int remote_accept_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_remote_accept)(operation_context->call_info->context);
}

static int hold_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_hold)(operation_context->call_info->context);
}

static int hold_incoming_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_hold_incoming)(operation_context->call_info->context);
}

static int retrieve_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_retrieve)(operation_context->call_info->context);
}

static int remote_ringing_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_remote_ringing)(operation_context->call_info->context);
}

static int terminate_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_terminate)(operation_context->call_info->context);
}

static int reject_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_reject)(operation_context->call_info->context);
}

static int remote_reject_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_remote_reject)(operation_context->call_info->context);
}

static int outgoing_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_outgoing)(
        operation_context->connection->ag,
        operation_context->call_info->number);
}

static int incoming_call(hfp_ag_operation_context_t* operation_context)
{
    return Z_API(bt_hfp_ag_remote_incoming)(
        operation_context->connection->ag,
        operation_context->call_info->number);
}

/* ============================================================
 * Transition Table
 * ============================================================ */

typedef struct {
    enum bt_hfp_ag_call_status new_state;
    call_operation_t op;
} new_call_entry_t;

typedef struct {
    enum bt_hfp_ag_call_status previous;
    hfp_ag_call_state_t next;
    call_operation_t op;
} call_transition_t;

/* ------ new call operation table ------ */

static const new_call_entry_t new_call_map[] = {
    { BT_HFP_AG_CALL_STATUS_INCOMING, incoming_call },
    { BT_HFP_AG_CALL_STATUS_WAITING, incoming_call },
    { BT_HFP_AG_CALL_STATUS_DIALING, outgoing_call },
};

/* ------ existing call operation table ------ */

static const call_transition_t call_transition_map[] = {

    { BT_HFP_AG_CALL_STATUS_INCOMING, HFP_AG_CALL_STATE_ACTIVE, accept_call },
    { BT_HFP_AG_CALL_STATUS_WAITING, HFP_AG_CALL_STATE_ACTIVE, accept_call },

    { BT_HFP_AG_CALL_STATUS_INCOMING, HFP_AG_CALL_STATE_HELD, hold_incoming_call },
    { BT_HFP_AG_CALL_STATUS_WAITING, HFP_AG_CALL_STATE_HELD, hold_incoming_call },

    { BT_HFP_AG_CALL_STATUS_INCOMING, HFP_AG_CALL_STATE_IDLE, reject_call },
    { BT_HFP_AG_CALL_STATUS_INCOMING, HFP_AG_CALL_STATE_DISCONNECTED, reject_call },
    { BT_HFP_AG_CALL_STATUS_WAITING, HFP_AG_CALL_STATE_IDLE, reject_call },
    { BT_HFP_AG_CALL_STATUS_WAITING, HFP_AG_CALL_STATE_DISCONNECTED, reject_call },

    { BT_HFP_AG_CALL_STATUS_DIALING, HFP_AG_CALL_STATE_ALERTING, remote_ringing_call },
    { BT_HFP_AG_CALL_STATUS_DIALING, HFP_AG_CALL_STATE_ACTIVE, remote_accept_call },
    { BT_HFP_AG_CALL_STATUS_DIALING, HFP_AG_CALL_STATE_IDLE, remote_reject_call },
    { BT_HFP_AG_CALL_STATUS_DIALING, HFP_AG_CALL_STATE_DISCONNECTED, remote_reject_call },

    { BT_HFP_AG_CALL_STATUS_ALERTING, HFP_AG_CALL_STATE_ACTIVE, remote_accept_call },
    { BT_HFP_AG_CALL_STATUS_ALERTING, HFP_AG_CALL_STATE_IDLE, remote_reject_call },
    { BT_HFP_AG_CALL_STATUS_ALERTING, HFP_AG_CALL_STATE_DISCONNECTED, remote_reject_call },

    { BT_HFP_AG_CALL_STATUS_ACTIVE, HFP_AG_CALL_STATE_HELD, hold_call },
    { BT_HFP_AG_CALL_STATUS_HELD, HFP_AG_CALL_STATE_ACTIVE, retrieve_call },

    { BT_HFP_AG_CALL_STATUS_ACTIVE, HFP_AG_CALL_STATE_IDLE, terminate_call },
    { BT_HFP_AG_CALL_STATUS_ACTIVE, HFP_AG_CALL_STATE_DISCONNECTED, terminate_call },
    { BT_HFP_AG_CALL_STATUS_HELD, HFP_AG_CALL_STATE_IDLE, terminate_call },
    { BT_HFP_AG_CALL_STATUS_HELD, HFP_AG_CALL_STATE_DISCONNECTED, terminate_call },
};

static const new_call_entry_t* find_new_call_entry(enum bt_hfp_ag_call_status state)
{
    for (size_t i = 0; i < ARRAY_SIZE(new_call_map); i++) {
        if (new_call_map[i].new_state == state) {
            return &new_call_map[i];
        }
    }
    return NULL;
}

static const call_transition_t* find_call_transition(
    enum bt_hfp_ag_call_status prev,
    hfp_ag_call_state_t next)
{
    for (size_t i = 0; i < ARRAY_SIZE(call_transition_map); i++) {
        if (call_transition_map[i].previous == prev && call_transition_map[i].next == next) {
            return &call_transition_map[i];
        }
    }
    return NULL;
}

bt_status_t bt_sal_hfp_ag_phone_state_change(bt_address_t* addr, uint8_t num_active,
    uint8_t num_held, hfp_ag_call_state_t call_state, hfp_call_addrtype_t type,
    const char* number, const char* name)
{
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        return BT_STATUS_FAIL;
    }

    bt_hfp_ag_call_info_t* call_info = find_call_by_number(sal_conn, number);
    hfp_ag_operation_context_t operation_context = {
        .call_info = call_info,
        .connection = sal_conn,
    };

    if (!call_info) {

        const new_call_entry_t* entry = find_new_call_entry(tele_call_state_to_sal_status(call_state));
        if (!entry) {
            return BT_STATUS_FAIL;
        }

        call_info = build_sal_call(HFP_CALL_DIRECTION_INCOMING, call_state,
            type, number);
        if (!call_info) {
            return BT_STATUS_FAIL;
        }

        bt_list_add_head(sal_conn->calls, call_info);
        operation_context.call_info = call_info;

        SAL_CHECK_RET(entry->op(&operation_context), 0);
        return BT_STATUS_SUCCESS;
    }

    const call_transition_t* transition = find_call_transition(call_info->state, call_state);

    if (!transition) {
        BT_LOGE("%s, no valid transition from %d to %d",
            __func__, call_info->state, call_state);
        return BT_STATUS_FAIL;
    }

    SAL_CHECK_RET(transition->op(&operation_context), 0);
    call_info->state = tele_call_state_to_sal_status(call_state);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_call_sync(bt_address_t* bd_addr,
    hfp_call_direction_t dir, hfp_ag_call_state_t call,
    hfp_call_mode_t mode, hfp_call_mpty_type_t mpty,
    hfp_call_addrtype_t type, const char* number)
{
    bt_hfp_ag_connection_t* conn = find_connection_by_addr(bd_addr);

    if (!conn) {
        BT_LOGW("%s, no sync connection set, ignore", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    update_sal_call(conn, dir, call, mode, mpty, type, number);
    return BT_STATUS_SUCCESS;
}

typedef struct {
    struct bt_hfp_ag_ongoing_call* calls;
    size_t* count;
} call_list_context_t;

static void fill_call_info(void* data, void* context)
{
    bt_hfp_ag_call_info_t* sal_call = (bt_hfp_ag_call_info_t*)data;
    call_list_context_t* ctx = (call_list_context_t*)context;

    if (*(ctx->count) >= HFP_CALL_LIST_MAX) {
        return;
    }

    ctx->calls[*(ctx->count)].dir = sal_call->dir;
    ctx->calls[*(ctx->count)].status = sal_call->state;
    ctx->calls[*(ctx->count)].type = sal_call->type;
    strlcpy(ctx->calls[*(ctx->count)].number, sal_call->number, sizeof(ctx->calls[*(ctx->count)].number));
    (*(ctx->count))++;
}

bt_status_t bt_sal_hfp_ag_cind_response(bt_address_t* addr, hfp_ag_cind_resopnse_t* response)
{
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag_ongoing_call calls[HFP_CALL_LIST_MAX];
    struct bt_hfp_ag_indicator_value indicators[4] = { 0 };
    size_t count = 0;

    if (!addr || !response) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_FAIL;
    }

    indicators[0].indicator = BT_HFP_AG_SERVICE_IND;
    indicators[0].value = response->network ? 1 : 0;
    indicators[1].indicator = BT_HFP_AG_ROAM_IND;
    indicators[1].value = response->roam ? 1 : 0;
    indicators[2].indicator = BT_HFP_AG_SIGNAL_IND;
    indicators[2].value = response->signal > 5 ? 5 : response->signal;
    indicators[3].indicator = BT_HFP_AG_BATTERY_IND;
    indicators[3].value = response->battery > 5 ? 5 : response->battery;

    memset(calls, 0, sizeof(calls));

    call_list_context_t ctx = {
        .calls = calls,
        .count = &count
    };

    if (sal_conn->calls) {
        bt_list_foreach(sal_conn->calls, fill_call_info, &ctx);
    }

    if (count > HFP_CALL_LIST_MAX) {
        BT_LOGW("%s, reached max call list size", __func__);
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_ongoing_calls)(sal_conn->ag, calls, count, indicators, 4), 0);
    return BT_STATUS_SUCCESS;
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
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_dial_response(bt_address_t* addr, hfp_atcmd_result_t result)
{
    bt_hfp_ag_connection_t* sal_conn;
    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);

    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_send_vendor)(sal_conn->ag, NULL), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_cops_response(bt_address_t* addr, const char* operator_name, uint16_t length)
{
    bt_hfp_ag_connection_t* sal_conn;
    (void)length;

    if (!addr || !operator_name) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_set_operator)(sal_conn->ag, 0, (char*)operator_name), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_notify_device_status_changed(bt_address_t* addr, hfp_network_state_t network,
    hfp_roaming_state_t roam, uint8_t signal, uint8_t battery)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_service_availability)(sal_conn->ag, network ? true : false), 0);

    SAL_CHECK_RET(Z_API(bt_hfp_ag_roaming_status)(sal_conn->ag, roam ? 1 : 0), 0);

    SAL_CHECK_RET(Z_API(bt_hfp_ag_signal_strength)(sal_conn->ag, signal > 5 ? 5 : signal), 0);

    SAL_CHECK_RET(Z_API(bt_hfp_ag_battery_level)(sal_conn->ag, battery > 5 ? 5 : battery), 0);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_set_inband_ring_enable(bt_address_t* addr, bool enable)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    SAL_CHECK_RET(Z_API(bt_hfp_ag_inband_ringtone)(sal_conn->ag, enable), 0);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (type == HFP_VOLUME_TYPE_SPK) {
        SAL_CHECK_RET(Z_API(bt_hfp_ag_vgs)(sal_conn->ag, volume), 0);
    } else if (type == HFP_VOLUME_TYPE_MIC) {
        SAL_CHECK_RET(Z_API(bt_hfp_ag_vgm)(sal_conn->ag, volume), 0);
    } else {
        BT_LOGE("%s, invalid volume type: %d", __func__, type);
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

static enum bt_at_cme hfp_at_result_to_cme(hfp_atcmd_result_t result)
{
    switch (result) {
    case HFP_ATCMD_RESULT_CMEERR_AGFAILURE:
        return BT_AT_CME_ERROR_AG_FAILURE;
    case HFP_ATCMD_RESULT_CMEERR_NOCONN2PHONE:
        return BT_AT_CME_ERROR_NO_CONNECTION_TO_PHONE;
    case HFP_ATCMD_RESULT_CMEERR_OPERATION_NOTALLOWED:
        return BT_AT_CME_ERROR_OPERATION_NOT_ALLOWED;
    case HFP_ATCMD_RESULT_CMEERR_OPERATION_NOTSUPPORTED:
        return BT_AT_CME_ERROR_OPERATION_NOT_SUPPORTED;
    case HFP_ATCMD_RESULT_CMEERR_PHSIMPIN_REQUIRED:
        return BT_AT_CME_ERROR_PH_SIM_PIN_REQUIRED;
    case HFP_ATCMD_RESULT_CMEERR_SIMNOT_INSERTED:
        return BT_AT_CME_ERROR_SIM_NOT_INSERTED;
    case HFP_ATCMD_RESULT_CMEERR_SIMPIN_REQUIRED:
        return BT_AT_CME_ERROR_SIM_PIN_REQUIRED;
    case HFP_ATCMD_RESULT_CMEERR_SIMPUK_REQUIRED:
        return BT_AT_CME_ERROR_SIM_PUK_REQUIRED;
    case HFP_ATCMD_RESULT_CMEERR_SIM_FAILURE:
        return BT_AT_CME_ERROR_SIM_FAILURE;
    case HFP_ATCMD_RESULT_CMEERR_SIM_BUSY:
        return BT_AT_CME_ERROR_SIM_BUSY;
    case HFP_ATCMD_RESULT_CMEERR_INCORRECT_PASSWORD:
        return BT_AT_CME_ERROR_INCORRECT_PASSWORD;
    case HFP_ATCMD_RESULT_CMEERR_SIMPIN2_REQUIRED:
        return BT_AT_CME_ERROR_SIM_PIN2_REQUIRED;
    case HFP_ATCMD_RESULT_CMEERR_SIMPUK2_REQUIRED:
        return BT_AT_CME_ERROR_SIM_PUK2_REQUIRED;
    case HFP_ATCMD_RESULT_CMEERR_MEMORY_FULL:
        return BT_AT_CME_ERROR_MEMORY_FULL;
    case HFP_ATCMD_RESULT_CMEERR_INVALID_INDEX:
        return BT_AT_CME_ERROR_INVALID_INDEX;
    case HFP_ATCMD_RESULT_CMEERR_MEMORY_FAILURE:
        return BT_AT_CME_ERROR_MEMORY_FAILURE;
    case HFP_ATCMD_RESULT_CMEERR_TEXTSTRING_TOOLONG:
        return BT_AT_CME_ERROR_TEXT_STRING_TOO_LONG;
    case HFP_ATCMD_RESULT_CMEERR_INVALID_CHARACTERS_INTEXTSTRING:
        return BT_AT_CME_ERROR_INVALID_CHARS_IN_TEXT_STRING;
    case HFP_ATCMD_RESULT_CMEERR_DIAL_STRING_TOOLONG:
        return BT_AT_CME_ERROR_DIAL_STRING_TOO_LONG;
    case HFP_ATCMD_RESULT_CMEERR_INVALID_CHARACTERS_INDIALSTRING:
        return BT_AT_CME_ERROR_INVALID_CHARS_IN_DIAL_STRING;
    case HFP_ATCMD_RESULT_CMEERR_NETWORK_NOSERVICE:
        return BT_AT_CME_ERROR_NO_NETWORK_SERVICE;
    case HFP_ATCMD_RESULT_CMEERR_NETWORK_TIMEOUT:
        return BT_AT_CME_ERROR_NETWORK_TIMEOUT;
    case HFP_ATCMD_RESULT_CMEERR_NETWORK_NOTALLOWED_EMERGENCYCALL_ONLY:
        return BT_AT_CME_ERROR_NETWORK_NOT_ALLOWED;
    default:
        return BT_AT_CME_ERROR_UNKNOWN;
    }
}

bt_status_t bt_sal_hfp_ag_send_at_cmd(bt_address_t* addr, const char* atcmd, uint16_t length)
{
    bt_hfp_ag_connection_t* sal_conn;
    const char* start;
    const char* end;
    size_t line_len;
    char* line;

    if (!addr || !atcmd || length == 0) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    start = atcmd;
    end = atcmd + length;

    while (start < end && (*start == '\r' || *start == '\n')) {
        start++;
    }

    while (end > start && (end[-1] == '\r' || end[-1] == '\n')) {
        end--;
    }

    line_len = (size_t)(end - start);
    if (line_len > HFP_AT_LEN_MAX) {
        line_len = HFP_AT_LEN_MAX;
    }

    if (line_len == 0) {
        BT_LOGW("%s, empty AT payload after trimming", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    line = (char*)malloc(line_len + 1);
    if (!line) {
        BT_LOGE("%s, failed to allocate memory for AT command", __func__);
        return BT_STATUS_NOMEM;
    }

    strlcpy(line, start, line_len + 1);

    BT_LOGD("%s, send vendor rsp: %s", __func__, line);

    int ret = Z_API(bt_hfp_ag_send_vendor)(sal_conn->ag, line);

    free(line);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
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
    bt_hfp_ag_connection_t* sal_conn;
    const char* line;
    char buf[32] = { 0 };

    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (result >= HFP_ATCMD_RESULT_CMEERR) {
        enum bt_at_cme cme = hfp_at_result_to_cme(result);
        snprintf(buf, sizeof(buf), "+CME ERROR:%d", (int)cme);
        line = buf;
    } else if (result == HFP_ATCMD_RESULT_OK) {
        line = NULL;
    } else {
        line = "ERROR";
    }

    int ret = Z_API(bt_hfp_ag_send_vendor)(sal_conn->ag, line);
    if (ret == -ENOTSUP) {
        return BT_STATUS_UNSUPPORTED;
    }

    SAL_CHECK_RET(ret, 0);
    return BT_STATUS_SUCCESS;
}
