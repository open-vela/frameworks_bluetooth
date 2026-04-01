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
#include "sal_zblue_hfp.h"
#include "service_loop.h"

#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/at.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>
#include <zephyr/bluetooth/classic/sdp.h>

#define HFP_AG_CODEC_Z_BIT_CVSD BIT(BT_HFP_AG_CODEC_CVSD)
#define HFP_AG_CODEC_Z_BIT_MSBC BIT(BT_HFP_AG_CODEC_MSBC)
#define HFP_AG_CODEC_Z_BIT_LC3_SWB BIT(BT_HFP_AG_CODEC_LC3_SWB)

#define HFP_AG_INDICATOR_INVALID UINT8_MAX

static bt_list_t* g_sal_ag_conn_list = NULL;
static pthread_mutex_t g_sal_ag_conn_lock;

static void conn_list_lock(void)
{
    pthread_mutex_lock(&g_sal_ag_conn_lock);
}

static void conn_list_unlock(void)
{
    pthread_mutex_unlock(&g_sal_ag_conn_lock);
}

extern struct net_buf_pool sdp_pool;

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result, const struct bt_sdp_discover_params* ignore);
static void zblue_on_sdp_disconnected(struct bt_conn* conn, const struct bt_sdp_discover_params* params);
static enum bt_at_cme hfp_at_result_to_cme(hfp_atcmd_result_t result);

static struct bt_sdp_discover_params sdp_discover = {
    .func = zblue_on_sdp_done,
    .disconnected = zblue_on_sdp_disconnected,
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

typedef struct {
    uint8_t network;
    uint8_t roam;
    uint8_t signal;
    uint8_t battery;
} hfp_ag_indicators_t;

typedef struct _bt_hfp_ag_connection {
    bt_address_t addr;
    struct bt_conn* context;
    struct bt_conn* sco_context;
    struct bt_hfp_ag* ag;
    uint8_t preferred_codec;
    bt_list_t* calls;
    hfp_ag_indicators_t indicators;
} bt_hfp_ag_connection_t;

typedef struct _bt_hfp_ag_slc_connect_param {
    struct bt_conn* conn;
    uint8_t channel;
} bt_hfp_ag_slc_connect_param_t;

typedef struct _bt_hfp_ag_connect_sco_param {
    struct bt_hfp_ag* ag;
    uint8_t codec; /* e.g., BT_HFP_AG_CODEC_CVSD */
} bt_hfp_ag_connect_sco_param_t;

typedef struct _bt_hfp_ag_disconnect_sco_param {
    struct bt_conn* sco_context;
} bt_hfp_ag_disconnect_sco_param_t;

typedef struct _bt_hfp_ag_set_volume_param {
    bt_address_t addr;
    hfp_volume_type_t type;
    uint8_t gain;
} bt_hfp_ag_set_volume_param_t;

typedef struct _bt_hfp_ag_voice_recognition_param {
    bt_address_t addr;
    bool activate;
} bt_hfp_ag_voice_recognition_param_t;

typedef struct _bt_hfp_ag_cind_response_param {
    bt_address_t addr;
    hfp_ag_cind_resopnse_t response;
} bt_hfp_ag_cind_response_param_t;

typedef struct _bt_hfp_ag_dial_response_param {
    bt_address_t addr;
} bt_hfp_ag_dial_response_param_t;

typedef struct _bt_hfp_ag_cops_response_param {
    bt_address_t addr;
    char operator_name[64];
} bt_hfp_ag_cops_response_param_t;

typedef struct _bt_hfp_ag_device_status_param {
    bt_address_t addr;
    hfp_network_state_t network;
    hfp_roaming_state_t roam;
    uint8_t signal;
    uint8_t battery;
} bt_hfp_ag_device_status_param_t;

typedef struct _bt_hfp_ag_inband_ring_param {
    bt_address_t addr;
    bool enable;
} bt_hfp_ag_inband_ring_param_t;

typedef struct _bt_hfp_ag_send_at_cmd_param {
    bt_address_t addr;
    char line[HFP_AT_LEN_MAX + 1];
} bt_hfp_ag_send_at_cmd_param_t;

typedef struct _bt_hfp_ag_error_response_param {
    bt_address_t addr;
    hfp_atcmd_result_t result;
} bt_hfp_ag_error_response_param_t;

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

static bool sal_conn_context_cmp(void* sal_context, void* z_context)
{
    bt_hfp_ag_connection_t* sal_conn = (bt_hfp_ag_connection_t*)sal_context;
    struct bt_conn* conn = (struct bt_conn*)z_context;
    return sal_conn->context == conn;
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

static bt_hfp_ag_connection_t* find_connection_by_context(struct bt_conn* conn)
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
    /* Initialize indicators to 0xff to ensure first update is detected as change,
     * in case bt_sal_hfp_ag_cind_response is not called properly */
    sal_conn->indicators.network = HFP_AG_INDICATOR_INVALID;
    sal_conn->indicators.roam = HFP_AG_INDICATOR_INVALID;
    sal_conn->indicators.signal = HFP_AG_INDICATOR_INVALID;
    sal_conn->indicators.battery = HFP_AG_INDICATOR_INVALID;

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
    if (!z_context) {
        return NULL;
    }

    if (!g_sal_ag_conn_list) {
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

static bt_hfp_ag_call_info_t* update_sal_call(bt_hfp_ag_connection_t* sal_conn,
    hfp_call_direction_t dir, hfp_ag_call_state_t call, hfp_call_mode_t mode,
    hfp_call_mpty_type_t mpty, hfp_call_addrtype_t type, const char* number)
{
    conn_list_lock();
    bt_hfp_ag_call_info_t* sal_call = find_call_by_number(sal_conn, number);
    if (!sal_call) {
        sal_call = build_sal_call(dir, call, type, number);
        if (!sal_call) {
            conn_list_unlock();
            return NULL;
        }

        if (!sal_conn->calls) {
            sal_conn->calls = bt_list_new(free_call);
        }

        bt_list_add_head(sal_conn->calls, sal_call);
        conn_list_unlock();
        return sal_call;
    }

    sal_call->state = tele_call_state_to_sal_status(call);
    sal_call->dir = service_call_dir_to_sal_dir(dir);
    if (sal_call->state == BT_HFP_AG_CALL_STATUS_UNKNOWN || sal_call->dir == BT_HFP_AG_CALL_DIR_UNKNOWN) {
        /* use sal_conn directly instead of find_call_by_context,
         * because context may be NULL for call_sync entries */
        bt_list_remove(sal_conn->calls, sal_call);
        conn_list_unlock();
        return NULL;
    }

    sal_call->type = type;

    conn_list_unlock();
    return sal_call;
}

static bt_status_t do_ag_sdp_discover(bt_controller_id_t id, bt_address_t* addr, void* userdata)
{
    struct bt_conn* conn;
    bt_hfp_ag_connection_t* sal_conn;

    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(addr);
    if (sal_conn != NULL) {
        conn_list_unlock();
        BT_LOGI("%s, Connection already exists, skip", __func__);
        return BT_STATUS_SUCCESS;
    }

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    /** Remember to unref @p conn once SLC is initiated or cancelled */
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to lookup connection", __func__);
        hfp_ag_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
        return BT_STATUS_NOT_FOUND;
    }

    sal_conn = new_sal_connection(conn, NULL);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, could not create new ag connection", __func__);
        hfp_ag_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
        bt_conn_unref(conn);
        return BT_STATUS_NOMEM;
    }
    conn_list_unlock();

    BT_LOGD("%s, do sdp discover", __func__);
    if (bt_sdp_discover(conn, &sdp_discover) < 0) {
        BT_LOGE("%s, failed to start a SDP discovery", __func__);
        hfp_ag_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
        bt_conn_unref(conn);
        conn_list_lock();
        bt_list_remove(g_sal_ag_conn_list, sal_conn);
        conn_list_unlock();
        return BT_STATUS_FAIL;
    }

    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;
}

static void do_ag_slc_connect(service_work_t* work, void* userdata)
{
    bt_hfp_ag_slc_connect_param_t* params = (bt_hfp_ag_slc_connect_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag = NULL;
    bt_address_t addr;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    if (!params->conn) {
        BT_LOGE("%s, params->conn is NULL", __func__);
        free(params);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_context(params->conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, no pending connection found for conn", __func__);
        bt_conn_unref(params->conn);
        free(params);
        return;
    }

    if (sal_conn->ag) {
        conn_list_unlock();
        BT_LOGD("%s, already initiating SLC, skip SLC initiating", __func__);
        bt_conn_unref(params->conn);
        free(params);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    BT_LOGD("%s, SLC initiating", __func__);
    if (Z_API(bt_hfp_ag_connect)(params->conn, &ag, params->channel)) {
        BT_LOGE("%s, HFP AG connection initiation failed", __func__);
        goto error;
    }

    conn_list_lock();
    sal_conn = find_connection_by_context(params->conn);
    if (sal_conn) {
        sal_conn->ag = ag;
    }
    conn_list_unlock();

    bt_conn_unref(params->conn);
    free(params);

    hfp_ag_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTING, 0, 0);

    BT_LOGD("%s, HFP AG connecting", __func__);
    return;

error:
    bt_conn_unref(params->conn);
    free(params);
    hfp_ag_on_connection_state_changed(&addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
    conn_list_lock();
    bt_list_remove(g_sal_ag_conn_list, sal_conn);
    conn_list_unlock();
    return;
}

bt_status_t do_ag_disconnect(bt_controller_id_t id, bt_address_t* addr, void* userdata)
{
    struct bt_hfp_ag* ag;

    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }

    if (!sal_conn->ag) {
        BT_LOGI("%s, HFP AG not connected", __func__);
        bt_list_remove(g_sal_ag_conn_list, sal_conn);
        conn_list_unlock();
        return BT_STATUS_SUCCESS;
    }
    ag = sal_conn->ag;
    conn_list_unlock();

    SAL_CHECK_RET(Z_API(bt_hfp_ag_disconnect)(ag), 0);
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
        cfg->sample_rate = HFP_CODEC_MSBC_SAMPLE_RATE;
        cfg->bit_width = HFP_CODEC_BIT_WIDTH;
        return 0;
    case BT_HFP_AG_CODEC_CVSD:
        cfg->codec = HFP_CODEC_CVSD;
        cfg->sample_rate = HFP_CODEC_CVSD_SAMPLE_RATE;
        cfg->bit_width = HFP_CODEC_BIT_WIDTH;
        return 0;
    default:
        return -ENOTSUP;
    }
}

static void do_ag_sco_connect(service_work_t* work, void* userdata)
{
    bt_hfp_ag_connect_sco_param_t* params;
    struct bt_hfp_ag* ag;
    uint8_t codec;
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;
    hfp_codec_config_t cfg = { 0 };
    int err;

    params = (bt_hfp_ag_connect_sco_param_t*)userdata;
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

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, connection not found for ag=%p", __func__, ag);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_audio_state_changed(&addr, HFP_AUDIO_STATE_CONNECTING, 0xFFFF); // sco conn handle not supported

    err = Z_API(bt_hfp_ag_audio_connect)(ag, codec);
    if (err == -EALREADY) {
        BT_LOGW("%s, Audio already connected, notify CONNECTED", __func__);
        hfp_ag_on_audio_state_changed(&addr, HFP_AUDIO_STATE_CONNECTED, 0xFFFF);
    } else if ((err == -ENOTSUP || err == -EINVAL) && codec != BT_HFP_AG_CODEC_CVSD) {
        BT_LOGW("%s, codec=%d not supported, fallback to CVSD", __func__, codec);
        conn_list_lock();
        sal_conn = find_connection_by_ag(ag);
        if (sal_conn) {
            sal_conn->preferred_codec = BT_HFP_AG_CODEC_CVSD;
        }
        conn_list_unlock();
        hfp_codec_to_service_cfg(BT_HFP_AG_CODEC_CVSD, &cfg);
        hfp_ag_on_codec_changed(&addr, &cfg);
        err = Z_API(bt_hfp_ag_audio_connect)(ag, BT_HFP_AG_CODEC_CVSD);
        if (err) {
            BT_LOGE("%s, Failed to connect HFP AG SCO with CVSD fallback, err=%d", __func__, err);
            hfp_ag_on_audio_state_changed(&addr, HFP_AUDIO_STATE_DISCONNECTED, 0xFFFF);
        }
    } else if (err) {
        BT_LOGE("%s, Failed to connect HFP AG SCO, err=%d", __func__, err);
        hfp_ag_on_audio_state_changed(&addr, HFP_AUDIO_STATE_DISCONNECTED, 0xFFFF);
    }
}

static void do_ag_sco_disconnect(service_work_t* work, void* userdata)
{
    bt_hfp_ag_disconnect_sco_param_t* params;
    struct bt_conn* sco_context;
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;
    int err;

    params = (bt_hfp_ag_disconnect_sco_param_t*)userdata;
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

    conn_list_lock();
    sal_conn = find_connection_by_sco_context(sco_context);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, sco_context no longer tracked, skip disconnect", __func__);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_audio_state_changed(&addr, HFP_AUDIO_STATE_DISCONNECTING, 0xFFFF);

    err = bt_conn_disconnect(sco_context, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        BT_LOGE("%s, Failed to disconnect HFP AG SCO, err=%d", __func__, err);
    }
}

static void zblue_on_sdp_disconnected(struct bt_conn* conn, const struct bt_sdp_discover_params* params)
{
    bt_address_t bd_addr;

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_context(conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, no pending connection found", __func__);
        return;
    }
    memcpy(&bd_addr, &sal_conn->addr, sizeof(bt_address_t));

    BT_LOGW("%s, SDP disconnected during discovery", __func__);
    bt_list_remove(g_sal_ag_conn_list, sal_conn);
    conn_list_unlock();

    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
}

static void do_ag_set_volume(service_work_t* work, void* userdata)
{
    bt_hfp_ag_set_volume_param_t* params = (bt_hfp_ag_set_volume_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;
    int ret;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available, skip set volume", __func__);
        free(params);
        return;
    }
    ag = sal_conn->ag;
    conn_list_unlock();

    switch (params->type) {
    case HFP_VOLUME_TYPE_SPK:
        ret = Z_API(bt_hfp_ag_vgs)(ag, params->gain);
        break;
    case HFP_VOLUME_TYPE_MIC:
        ret = Z_API(bt_hfp_ag_vgm)(ag, params->gain);
        break;
    default:
        BT_LOGE("%s, Unknown volume type: %d", __func__, params->type);
        free(params);
        return;
    }

    if (ret) {
        BT_LOGE("%s, Failed to set volume, type=%d, ret=%d", __func__, params->type, ret);
    }

    free(params);
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

static void do_ag_voice_recognition(service_work_t* work, void* userdata)
{
    bt_hfp_ag_voice_recognition_param_t* params = (bt_hfp_ag_voice_recognition_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    ag = sal_conn->ag;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_ag_voice_recognition)(ag, params->activate);
    if (ret) {
        BT_LOGE("%s, Failed to %s voice recognition, ret=%d", __func__,
            params->activate ? "start" : "stop", ret);
    }

    free(params);
}

static void do_ag_cind_response(service_work_t* work, void* userdata)
{
    bt_hfp_ag_cind_response_param_t* params = (bt_hfp_ag_cind_response_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;
    struct bt_hfp_ag_ongoing_call calls[HFP_CALL_LIST_MAX];
    struct bt_hfp_ag_indicator_value indicators[4] = { 0 };
    size_t count = 0;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }

    ag = sal_conn->ag;

    sal_conn->indicators.network = params->response.network ? 1 : 0;
    sal_conn->indicators.roam = params->response.roam ? 1 : 0;
    sal_conn->indicators.signal = params->response.signal > 5 ? 5 : (uint8_t)params->response.signal;
    sal_conn->indicators.battery = params->response.battery > 5 ? 5 : (uint8_t)params->response.battery;

    indicators[0].indicator = BT_HFP_AG_SERVICE_IND;
    indicators[0].value = sal_conn->indicators.network;
    indicators[1].indicator = BT_HFP_AG_ROAM_IND;
    indicators[1].value = sal_conn->indicators.roam;
    indicators[2].indicator = BT_HFP_AG_SIGNAL_IND;
    indicators[2].value = sal_conn->indicators.signal;
    indicators[3].indicator = BT_HFP_AG_BATTERY_IND;
    indicators[3].value = sal_conn->indicators.battery;

    memset(calls, 0, sizeof(calls));

    call_list_context_t ctx = {
        .calls = calls,
        .count = &count
    };

    if (sal_conn->calls) {
        bt_list_foreach(sal_conn->calls, fill_call_info, &ctx);
    }
    conn_list_unlock();

    if (count > HFP_CALL_LIST_MAX) {
        BT_LOGW("%s, reached max call list size", __func__);
    }

    int ret = Z_API(bt_hfp_ag_ongoing_calls)(ag, calls, count, indicators, 4);
    if (ret) {
        BT_LOGE("%s, bt_hfp_ag_ongoing_calls failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_ag_dial_response(service_work_t* work, void* userdata)
{
    bt_hfp_ag_dial_response_param_t* params = (bt_hfp_ag_dial_response_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    ag = sal_conn->ag;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_ag_send_vendor)(ag, NULL);
    if (ret) {
        BT_LOGE("%s, bt_hfp_ag_send_vendor failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_ag_cops_response(service_work_t* work, void* userdata)
{
    bt_hfp_ag_cops_response_param_t* params = (bt_hfp_ag_cops_response_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    ag = sal_conn->ag;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_ag_set_operator)(ag, 0, params->operator_name);
    if (ret) {
        BT_LOGE("%s, bt_hfp_ag_set_operator failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_ag_device_status(service_work_t* work, void* userdata)
{
    bt_hfp_ag_device_status_param_t* params = (bt_hfp_ag_device_status_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;
    uint8_t net_val;
    uint8_t roam_val;
    uint8_t sig_val;
    uint8_t bat_val;
    uint8_t old_network;
    uint8_t old_roam;
    uint8_t old_signal;
    uint8_t old_battery;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }

    ag = sal_conn->ag;
    old_network = sal_conn->indicators.network;
    old_roam = sal_conn->indicators.roam;
    old_signal = sal_conn->indicators.signal;
    old_battery = sal_conn->indicators.battery;
    conn_list_unlock();

    net_val = params->network ? 1 : 0;
    roam_val = params->roam ? 1 : 0;
    sig_val = params->signal > 5 ? 5 : (uint8_t)params->signal;
    bat_val = params->battery > 5 ? 5 : (uint8_t)params->battery;

    if (net_val != old_network) {
        int ret = Z_API(bt_hfp_ag_service_availability)(ag, params->network ? true : false);
        if (ret) {
            BT_LOGE("%s, bt_hfp_ag_service_availability failed, ret=%d", __func__, ret);
        }
        conn_list_lock();
        sal_conn = find_connection_by_addr(&params->addr);
        if (sal_conn) {
            sal_conn->indicators.network = net_val;
        }
        conn_list_unlock();
    }

    if (roam_val != old_roam) {
        int ret = Z_API(bt_hfp_ag_roaming_status)(ag, params->roam ? 1 : 0);
        if (ret) {
            BT_LOGE("%s, bt_hfp_ag_roaming_status failed, ret=%d", __func__, ret);
        }
        conn_list_lock();
        sal_conn = find_connection_by_addr(&params->addr);
        if (sal_conn) {
            sal_conn->indicators.roam = roam_val;
        }
        conn_list_unlock();
    }

    if (sig_val != old_signal) {
        int ret = Z_API(bt_hfp_ag_signal_strength)(ag, sig_val);
        if (ret) {
            BT_LOGE("%s, bt_hfp_ag_signal_strength failed, ret=%d", __func__, ret);
        }
        conn_list_lock();
        sal_conn = find_connection_by_addr(&params->addr);
        if (sal_conn) {
            sal_conn->indicators.signal = sig_val;
        }
        conn_list_unlock();
    }

    if (bat_val != old_battery) {
        int ret = Z_API(bt_hfp_ag_battery_level)(ag, bat_val);
        if (ret) {
            BT_LOGE("%s, bt_hfp_ag_battery_level failed, ret=%d", __func__, ret);
        }
        conn_list_lock();
        sal_conn = find_connection_by_addr(&params->addr);
        if (sal_conn) {
            sal_conn->indicators.battery = bat_val;
        }
        conn_list_unlock();
    }

    free(params);
}

static void do_ag_inband_ring(service_work_t* work, void* userdata)
{
    bt_hfp_ag_inband_ring_param_t* params = (bt_hfp_ag_inband_ring_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    struct bt_hfp_ag* ag = sal_conn->ag;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_ag_inband_ringtone)(ag, params->enable);
    if (ret) {
        BT_LOGE("%s, bt_hfp_ag_inband_ringtone failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_ag_send_at_cmd(service_work_t* work, void* userdata)
{
    bt_hfp_ag_send_at_cmd_param_t* params = (bt_hfp_ag_send_at_cmd_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    ag = sal_conn->ag;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_ag_send_vendor)(ag, params->line);
    if (ret) {
        BT_LOGE("%s, bt_hfp_ag_send_vendor failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_ag_error_response(service_work_t* work, void* userdata)
{
    bt_hfp_ag_error_response_param_t* params = (bt_hfp_ag_error_response_param_t*)userdata;
    bt_hfp_ag_connection_t* sal_conn;
    struct bt_hfp_ag* ag;
    const char* line;
    char buf[32] = { 0 };

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    ag = sal_conn->ag;
    conn_list_unlock();

    if (params->result >= HFP_ATCMD_RESULT_CMEERR) {
        enum bt_at_cme cme = hfp_at_result_to_cme(params->result);
        snprintf(buf, sizeof(buf), "+CME ERROR:%d", (int)cme);
        line = buf;
    } else if (params->result == HFP_ATCMD_RESULT_OK) {
        line = NULL;
    } else {
        line = "ERROR";
    }

    int ret = Z_API(bt_hfp_ag_send_vendor)(ag, line);
    if (ret) {
        BT_LOGE("%s, bt_hfp_ag_send_vendor failed, ret=%d", __func__, ret);
    }

    free(params);
}

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result,
    const struct bt_sdp_discover_params* ignore)
{
    int err;
    uint16_t port;
    bt_address_t bd_addr;
    bt_hfp_ag_slc_connect_param_t* params;

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_context(conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, could not find sal_conn", __func__);
        return BT_SDP_DISCOVER_UUID_STOP;
    }
    memcpy(&bd_addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    if (!result) {
        BT_LOGE("%s, remote device does not support HFP HF feature", __func__);
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

    params = (bt_hfp_ag_slc_connect_param_t*)malloc(sizeof(bt_hfp_ag_slc_connect_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate slc params", __func__);
        goto error;
    }

    params->channel = (uint8_t)port;
    params->conn = bt_conn_ref(conn);

    if (!service_loop_work(params, do_ag_slc_connect, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        bt_conn_unref(params->conn);
        free(params);
        goto error;
    }

    return BT_SDP_DISCOVER_UUID_STOP;

error:
    conn_list_lock();
    bt_list_remove(g_sal_ag_conn_list, sal_conn);
    conn_list_unlock();
    hfp_ag_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
    return BT_SDP_DISCOVER_UUID_STOP;
}

static void zblue_on_ag_connected(struct bt_conn* conn, struct bt_hfp_ag* ag)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    conn_list_lock();
    sal_conn = find_connection_by_context(conn);
    if (!sal_conn) {
        BT_LOGD("%s, ag connection incoming", __func__);
        sal_conn = new_sal_connection(conn, ag);
        if (!sal_conn) {
            conn_list_unlock();
            BT_LOGE("%s, Failed to create HFP AG connection", __func__);
            if (Z_API(bt_hfp_ag_disconnect)(ag)) {
                BT_LOGE("%s, Failed to disconnect HFP AG connection", __func__);
            }
            return;
        }

        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();

        hfp_ag_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTING, 0, 0);
    } else {
        /* Override conn if both sides attempt to connect at the same time */
        sal_conn->context = conn;
        sal_conn->ag = ag;
        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();
    }

    bt_sal_cm_profile_connected_callback(&addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&addr, PROFILE_HFP_AG, CONN_ID_DEFAULT, PRIMARY_ADAPTER, do_ag_disconnect, NULL);

    hfp_ag_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTED, 0, 0);
}

static void zblue_on_ag_disconnected(struct bt_hfp_ag* ag)
{
    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    hfp_ag_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_DISCONNECTING, 0, 0);
    hfp_ag_on_connection_state_changed(&sal_conn->addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&sal_conn->addr, PROFILE_HFP_AG, CONN_ID_DEFAULT);

    bt_list_remove(g_sal_ag_conn_list, sal_conn);
    conn_list_unlock();
}

static void zblue_on_ag_sco_connected(struct bt_hfp_ag* ag, struct bt_conn* sco_conn)
{
    bt_address_t addr;

    BT_LOGD("%s, HFP AG SCO connected, ag=%p", __func__, ag);

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    sal_conn->sco_context = sco_conn;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_audio_state_changed(&addr, HFP_AUDIO_STATE_CONNECTED, 0xFFFF); // sco conn handle not supported
}

static void zblue_on_ag_sco_disconnected(struct bt_conn* sco_conn, uint8_t reason)
{
    bt_address_t addr;

    BT_LOGD("%s, HFP AG SCO disconnected, reason=%d", __func__, reason);

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_sco_context(sco_conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    sal_conn->sco_context = NULL;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_audio_state_changed(&addr, HFP_AUDIO_STATE_DISCONNECTED, 0xFFFF); // sco conn handle not supported
}

static int zblue_on_ag_vendor_at_cmd(struct bt_hfp_ag* ag, const char* cmd, uint8_t* cme_code)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    BT_LOGD("%s, cmd:%s", __func__, cmd ? cmd : "(null)");

    if (!ag || !cmd) {
        return -EINVAL;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return -ENOTCONN;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_received_at_cmd(&addr, cmd, (uint16_t)strlen(cmd));

    return -EINPROGRESS;
}

static int zblue_on_ag_get_ongoing_call(struct bt_hfp_ag* ag)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);

    if (!sal_conn) {
        conn_list_unlock();
        struct bt_conn* conn = Z_API(bt_hfp_ag_get_conn)(ag);
        if (!conn) {
            BT_LOGE("%s, failed to get conn for ag=%p", __func__, ag);
            return -EINVAL;
        }
        BT_LOGD("%s, connection not found for ag=%p, creating new", __func__, ag);
        conn_list_lock();
        sal_conn = new_sal_connection(conn, ag);
        bt_conn_unref(conn);
        if (!sal_conn) {
            conn_list_unlock();
            BT_LOGE("%s, failed to create new sal conn", __func__);
            return -EINVAL;
        }
        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();
        hfp_ag_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTING, 0, 0);
    } else {
        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();
    }

    hfp_ag_on_call_sync(&addr);
    hfp_ag_on_received_cind_request(&addr);
    return 0;
}

static int zblue_on_ag_memory_dial(struct bt_hfp_ag* ag, const char* location, char** number)
{
    return -ENOTSUP;
}

static int zblue_on_ag_number_call(struct bt_hfp_ag* ag, const char* number)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    if (!ag || !number) {
        return -EINVAL;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return -EINVAL;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_dial_number(&addr, (char*)number, strlen(number));

    return -EINPROGRESS;
}

static void zblue_on_ag_outgoing(struct bt_hfp_ag* ag, struct bt_hfp_ag_call* call, const char* number)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_hfp_ag_call_info_t* sal_call;

    if (!ag || !call || !number) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
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
    conn_list_unlock();
}

static void zblue_on_ag_incoming(struct bt_hfp_ag* ag, struct bt_hfp_ag_call* call, const char* number)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_hfp_ag_call_info_t* sal_call;

    if (!ag || !call || !number) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
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
    conn_list_unlock();
}

static void zblue_on_ag_incoming_held(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;

    conn_list_lock();
    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        conn_list_unlock();
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
    conn_list_unlock();
}

static void zblue_on_ag_accept(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;
    bt_address_t addr;

    if (!call) {
        return;
    }

    conn_list_lock();
    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        conn_list_unlock();
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_INCOMING && sal_call->state != BT_HFP_AG_CALL_STATUS_WAITING) {
        conn_list_unlock();
        return;
    }

    sal_call->state = BT_HFP_AG_CALL_STATUS_ACTIVE;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_answer_call(&addr);
}

static void zblue_on_ag_held(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;
    bt_address_t addr;

    if (!call) {
        return;
    }

    conn_list_lock();
    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        conn_list_unlock();
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_ACTIVE) {
        conn_list_unlock();
        return;
    }

    sal_call->state = BT_HFP_AG_CALL_STATUS_HELD;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_hangup_call(&addr);
}

static void zblue_on_ag_retrieve(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;
    bt_address_t addr;

    if (!call) {
        return;
    }

    conn_list_lock();
    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        conn_list_unlock();
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_HELD) {
        conn_list_unlock();
        return;
    }

    sal_call->state = BT_HFP_AG_CALL_STATUS_ACTIVE;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_call_control(&addr, HFP_HF_CALL_CONTROL_CHLD_2);
}

static void zblue_on_ag_reject(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;
    bt_address_t addr;

    if (!call) {
        return;
    }

    conn_list_lock();
    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        conn_list_unlock();
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_INCOMING
        && sal_call->state != BT_HFP_AG_CALL_STATUS_WAITING) {
        conn_list_unlock();
        return;
    }

    sal_call->state = BT_HFP_AG_CALL_STATUS_UNKNOWN;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_reject_call(&addr);
}

static void zblue_on_ag_terminate(struct bt_hfp_ag_call* call)
{
    bt_hfp_ag_connection_t* sal_conn = NULL;
    bt_hfp_ag_call_info_t* sal_call;
    bt_address_t addr;
    int call_count;

    if (!call) {
        return;
    }

    conn_list_lock();
    sal_call = find_call_by_context(call, &sal_conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for call=%p", __func__, call);
        return;
    }

    if (!sal_call) {
        conn_list_unlock();
        BT_LOGE("%s, call not tracked", __func__);
        return;
    }

    if (sal_call->state != BT_HFP_AG_CALL_STATUS_ACTIVE
        && sal_call->state != BT_HFP_AG_CALL_STATUS_HELD) {
        conn_list_unlock();
        return;
    }

    sal_call->state = BT_HFP_AG_CALL_STATUS_UNKNOWN;

    if (!sal_conn->calls) {
        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();
        hfp_ag_on_hangup_call(&addr);
        return;
    }

    call_count = bt_list_length(sal_conn->calls);
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    if (call_count == 1) {
        hfp_ag_on_hangup_call(&addr);
    } else {
        hfp_ag_on_call_control(&addr, HFP_HF_CALL_CONTROL_CHLD_1);
    }
}

static void zblue_on_ag_available_codec(struct bt_hfp_ag* ag, uint32_t codec_ids)
{
    bt_hfp_ag_connection_t* sal_conn;
    hfp_codec_config_t cfg = { 0 };
    bt_address_t addr;
    uint8_t preferred_codec;
    int err;

    if (!ag) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        struct bt_conn* conn = Z_API(bt_hfp_ag_get_conn)(ag);
        if (!conn) {
            BT_LOGE("%s, failed to get conn for ag=%p", __func__, ag);
            return;
        }
        BT_LOGD("%s, connection not found for ag=%p, creating new", __func__, ag);
        conn_list_lock();
        sal_conn = new_sal_connection(conn, ag);
        bt_conn_unref(conn);
        if (!sal_conn) {
            conn_list_unlock();
            BT_LOGE("%s, failed to create new sal conn", __func__);
            return;
        }

        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();

        hfp_ag_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTING, 0, 0);

        conn_list_lock();
        sal_conn = find_connection_by_ag(ag);
        if (!sal_conn) {
            conn_list_unlock();
            BT_LOGE("%s, connection lost after creating", __func__);
            return;
        }
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

    preferred_codec = sal_conn->preferred_codec;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    err = hfp_codec_to_service_cfg(preferred_codec, &cfg);
    if (err != 0) {
        if (err == -EINVAL) {
            BT_LOGE("%s, invalid cfg pointer", __func__);
        } else {
            BT_LOGE("%s, unsupported codec id: %d", __func__, preferred_codec);
        }
        return;
    }

    hfp_ag_on_codec_changed(&addr, &cfg);
}

static void zblue_on_ag_audio_connect_req(struct bt_hfp_ag* ag)
{
    bt_hfp_ag_connection_t* sal_conn;
    hfp_codec_config_t cfg = { 0 };
    bt_hfp_ag_connect_sco_param_t* params;
    bt_address_t addr;
    uint8_t codec;
    int err;

    if (!ag) {
        BT_LOGE("%s, ag is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    codec = sal_conn->preferred_codec ? sal_conn->preferred_codec : BT_HFP_AG_CODEC_CVSD;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

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
    hfp_ag_on_codec_changed(&addr, &cfg);

    params = (bt_hfp_ag_connect_sco_param_t*)zalloc(sizeof(bt_hfp_ag_connect_sco_param_t));
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

static void zblue_on_ag_codec_negotiation(struct bt_hfp_ag* ag, int zblue_err,
    uint8_t codec_id)
{
    bt_hfp_ag_connection_t* sal_conn;
    hfp_codec_config_t cfg = { 0 };
    bt_address_t addr;
    uint8_t preferred_codec;
    int err;

    if (!ag) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }

    BT_LOGD("%s, err=%d, codec_id=%u", __func__, zblue_err, codec_id);
    sal_conn->preferred_codec = zblue_err ? BT_HFP_AG_CODEC_CVSD : codec_id;
    preferred_codec = sal_conn->preferred_codec;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    err = hfp_codec_to_service_cfg(preferred_codec, &cfg);
    if (err != 0) {
        BT_LOGE("%s, codec config failed: %d", __func__, err);
        return;
    }
    hfp_ag_on_codec_changed(&addr, &cfg);
}

static void zblue_on_ag_vgm(struct bt_hfp_ag* ag, uint8_t gain)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    if (!ag) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_volume_changed(&addr, HFP_VOLUME_TYPE_MIC, gain);
}

static void zblue_on_ag_vgs(struct bt_hfp_ag* ag, uint8_t gain)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    if (!ag) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_volume_changed(&addr, HFP_VOLUME_TYPE_SPK, gain);
}

static void zblue_on_ag_voice_recognition(struct bt_hfp_ag* ag, bool activate)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    if (!ag) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_voice_recognition_state_changed(&addr, activate);
}

static void zblue_on_ag_transmit_dtmf_code(struct bt_hfp_ag* ag, char code)
{
    bt_hfp_ag_connection_t* sal_conn;
    bt_address_t addr;

    if (!ag) {
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_ag(ag);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found for ag=%p", __func__, ag);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_ag_on_received_dtmf(&addr, code);
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
    pthread_mutexattr_t attr;

    (void)features;
    (void)max_connection;
    BT_LOGD("%s, HFP AG init", __func__);
    g_sal_ag_conn_list = bt_list_new(free_connection);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_sal_ag_conn_lock, &attr);
    pthread_mutexattr_destroy(&attr);

    if (Z_API(bt_hfp_ag_register)(&g_hfp_ag_cb) != 0) {
        bt_list_free(g_sal_ag_conn_list);
        g_sal_ag_conn_list = NULL;
        pthread_mutex_destroy(&g_sal_ag_conn_lock);
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_ag_cleanup(void)
{
    conn_list_lock();
    if (g_sal_ag_conn_list) {
        bt_list_free(g_sal_ag_conn_list);
        g_sal_ag_conn_list = NULL;
    }
    conn_list_unlock();
    pthread_mutex_destroy(&g_sal_ag_conn_lock);

    if (Z_API(bt_hfp_ag_unregister)()) {
        BT_LOGE("%s, Failed to unregister HFP AG", __func__);
    }
}

bt_status_t bt_sal_hfp_ag_connect(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, Connection already exists or in progress", __func__);
        return BT_STATUS_BUSY;
    }
    conn_list_unlock();

    return bt_sal_profile_connect_request(addr, PROFILE_HFP_AG, CONN_ID_DEFAULT, 0, do_ag_sdp_discover, NULL);
}

bt_status_t bt_sal_hfp_ag_disconnect(bt_address_t* addr)
{
    conn_list_lock();
    bt_hfp_ag_connection_t* conn = find_connection_by_addr(addr);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_sal_profile_disconnect_request(addr, PROFILE_HFP_AG, CONN_ID_DEFAULT, 0, do_ag_disconnect, NULL);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_connect_audio(bt_address_t* addr)
{
    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_ag_connect_sco_param_t* params = (bt_hfp_ag_connect_sco_param_t*)zalloc(sizeof(bt_hfp_ag_connect_sco_param_t));
    if (!params) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to allocate memory", __func__);
        return BT_STATUS_NOMEM;
    }

    params->ag = sal_conn->ag;
    params->codec = sal_conn->preferred_codec ? sal_conn->preferred_codec : BT_HFP_AG_CODEC_CVSD;
    conn_list_unlock();

    if (!service_loop_work(params, do_ag_sco_connect, NULL)) {
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_disconnect_audio(bt_address_t* addr)
{
    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    if (!sal_conn->context) {
        conn_list_unlock();
        BT_LOGE("%s, ACL conn context not initiated", __func__);
        return BT_STATUS_FAIL;
    }

    if (!sal_conn->sco_context) {
        conn_list_unlock();
        BT_LOGE("%s, SCO connection not initiated", __func__);
        return BT_STATUS_FAIL;
    }

    bt_hfp_ag_disconnect_sco_param_t* params = (bt_hfp_ag_disconnect_sco_param_t*)zalloc(sizeof(bt_hfp_ag_disconnect_sco_param_t));
    if (!params) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to allocate memory", __func__);
        return BT_STATUS_NOMEM;
    }

    params->sco_context = sal_conn->sco_context;
    conn_list_unlock();

    if (!service_loop_work(params, do_ag_sco_disconnect, NULL)) {
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_start_voice_recognition(bt_address_t* addr)
{
    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_voice_recognition_param_t* params = zalloc(sizeof(bt_hfp_ag_voice_recognition_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->activate = true;

    if (!service_loop_work(params, do_ag_voice_recognition, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_stop_voice_recognition(bt_address_t* addr)
{
    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_voice_recognition_param_t* params = zalloc(sizeof(bt_hfp_ag_voice_recognition_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->activate = false;

    if (!service_loop_work(params, do_ag_voice_recognition, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

typedef struct {
    bt_hfp_ag_call_info_t* call_info; /* includes call_context + number */
    bt_hfp_ag_connection_t* connection; /* includes ag pointer */
} hfp_ag_operation_context_t;

/* unified wrapper signature */
typedef int (*call_operation_t)(hfp_ag_operation_context_t* operation_context);

typedef struct {
    enum bt_hfp_ag_call_status new_state;
    call_operation_t op;
} new_call_entry_t;

typedef struct {
    enum bt_hfp_ag_call_status previous;
    hfp_ag_call_state_t next;
    call_operation_t op;
} call_transition_t;

/* forward declarations */
static const new_call_entry_t* find_new_call_entry(enum bt_hfp_ag_call_status state);
static const call_transition_t* find_call_transition(
    enum bt_hfp_ag_call_status prev, hfp_ag_call_state_t next);

typedef struct _bt_hfp_ag_call_op_param {
    bt_address_t addr;
    char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
    hfp_ag_call_state_t call_state;
    hfp_call_addrtype_t type;
} bt_hfp_ag_call_op_param_t;

static void do_ag_call_op(service_work_t* work, void* userdata)
{
    bt_hfp_ag_call_op_param_t* params = (bt_hfp_ag_call_op_param_t*)userdata;
    if (!params) {
        BT_LOGE("%s, Invalid parameters", __func__);
        return;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer valid, skip", __func__);
        free(params);
        return;
    }

    enum bt_hfp_ag_call_status new_state = tele_call_state_to_sal_status(params->call_state);
    bt_hfp_ag_call_info_t* call_info = find_call_by_number(sal_conn, params->number);

    if (!call_info) {
        /* new call path */
        const new_call_entry_t* entry = find_new_call_entry(new_state);
        if (!entry) {
            conn_list_unlock();
            BT_LOGE("%s, no new_call_entry for state %d, number: %s",
                __func__, params->call_state, params->number);
            free(params);
            return;
        }

        call_info = build_sal_call(HFP_CALL_DIRECTION_INCOMING, params->call_state,
            params->type, params->number);
        if (!call_info) {
            conn_list_unlock();
            BT_LOGE("%s, failed to build sal call", __func__);
            free(params);
            return;
        }

        bt_list_add_head(sal_conn->calls, call_info);
        conn_list_unlock();

        hfp_ag_operation_context_t context = {
            .call_info = call_info,
            .connection = sal_conn,
        };

        int ret = entry->op(&context);
        if (ret) {
            BT_LOGE("%s, new call op failed, err=%d, rolling back", __func__, ret);
            conn_list_lock();
            bt_list_remove(sal_conn->calls, call_info);
            conn_list_unlock();
        }

        free(params);
        return;
    }

    /* existing call path */
    const call_transition_t* transition = find_call_transition(call_info->state, params->call_state);

    if (!transition) {
        if (call_info->state == new_state) {
            BT_LOGI("%s, state already %d, skip transition", __func__, new_state);
            if (new_state == BT_HFP_AG_CALL_STATUS_UNKNOWN) {
                bt_list_remove(sal_conn->calls, call_info);
            }
        } else {
            BT_LOGE("%s, no valid transition from %d to %d",
                __func__, call_info->state, params->call_state);
        }
        conn_list_unlock();
        free(params);
        return;
    }
    conn_list_unlock();

    hfp_ag_operation_context_t context = {
        .call_info = call_info,
        .connection = sal_conn,
    };

    int ret = transition->op(&context);
    if (ret) {
        BT_LOGE("%s, call transition op failed, err=%d", __func__, ret);
        free(params);
        return;
    }

    conn_list_lock();
    if (new_state == BT_HFP_AG_CALL_STATUS_UNKNOWN) {
        bt_list_remove(sal_conn->calls, call_info);
    } else {
        call_info->state = new_state;
    }
    conn_list_unlock();

    free(params);
}

/* ============================================================
 * Wrapper
 * ============================================================ */

static int accept_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_accept)(operation_context->call_info->context);
}

static int remote_accept_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_remote_accept)(operation_context->call_info->context);
}

static int hold_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_hold)(operation_context->call_info->context);
}

static int hold_incoming_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_hold_incoming)(operation_context->call_info->context);
}

static int retrieve_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_retrieve)(operation_context->call_info->context);
}

static int remote_ringing_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_remote_ringing)(operation_context->call_info->context);
}

static int terminate_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_terminate)(operation_context->call_info->context);
}

static int reject_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_reject)(operation_context->call_info->context);
}

static int remote_reject_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_remote_reject)(operation_context->call_info->context);
}

static int outgoing_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_outgoing)(
        operation_context->connection->ag,
        operation_context->call_info->number);
}

static int incoming_call(hfp_ag_operation_context_t* operation_context)
{
    BT_LOGD("%s, number: %s, state: %d", __func__,
        operation_context->call_info->number, operation_context->call_info->state);
    return Z_API(bt_hfp_ag_remote_incoming)(
        operation_context->connection->ag,
        operation_context->call_info->number);
}

/* ============================================================
 * Transition Table
 * ============================================================ */

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
    BT_LOGI("%s, num_active: %d, num_held: %d, call_state: %d, type: %d, number: %s, name: %s",
        __func__, num_active, num_held, call_state, type,
        number ? number : "(null)", name ? name : "(null)");

    bt_hfp_ag_call_op_param_t* params = (bt_hfp_ag_call_op_param_t*)zalloc(sizeof(bt_hfp_ag_call_op_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate memory", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->call_state = call_state;
    params->type = type;
    if (number) {
        strlcpy(params->number, number, sizeof(params->number));
    }

    if (!service_loop_work(params, do_ag_call_op, NULL)) {
        BT_LOGE("%s, Failed to schedule call op to service loop", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_call_sync(bt_address_t* bd_addr,
    hfp_call_direction_t dir, hfp_ag_call_state_t call,
    hfp_call_mode_t mode, hfp_call_mpty_type_t mpty,
    hfp_call_addrtype_t type, const char* number)
{
    conn_list_lock();
    bt_hfp_ag_connection_t* conn = find_connection_by_addr(bd_addr);

    if (!conn) {
        conn_list_unlock();
        BT_LOGW("%s, no sync connection set, ignore", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    update_sal_call(conn, dir, call, mode, mpty, type, number);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_cind_response(bt_address_t* addr, hfp_ag_cind_resopnse_t* response)
{
    if (!addr || !response) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_FAIL;
    }
    conn_list_unlock();

    bt_hfp_ag_cind_response_param_t* params = zalloc(sizeof(bt_hfp_ag_cind_response_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    memcpy(&params->response, response, sizeof(hfp_ag_cind_resopnse_t));

    if (!service_loop_work(params, do_ag_cind_response, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

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
    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_dial_response_param_t* params = zalloc(sizeof(bt_hfp_ag_dial_response_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));

    if (!service_loop_work(params, do_ag_dial_response, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_cops_response(bt_address_t* addr, const char* operator_name, uint16_t length)
{
    (void)length;

    if (!addr || !operator_name) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_cops_response_param_t* params = zalloc(sizeof(bt_hfp_ag_cops_response_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    strlcpy(params->operator_name, operator_name, sizeof(params->operator_name));

    if (!service_loop_work(params, do_ag_cops_response, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_notify_device_status_changed(bt_address_t* addr, hfp_network_state_t network,
    hfp_roaming_state_t roam, uint8_t signal, uint8_t battery)
{
    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_device_status_param_t* params = zalloc(sizeof(bt_hfp_ag_device_status_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->network = network;
    params->roam = roam;
    params->signal = signal;
    params->battery = battery;

    if (!service_loop_work(params, do_ag_device_status, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_set_inband_ring_enable(bt_address_t* addr, bool enable)
{
    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_inband_ring_param_t* params = zalloc(sizeof(bt_hfp_ag_inband_ring_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->enable = enable;

    if (!service_loop_work(params, do_ag_inband_ring, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_ag_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    bt_hfp_ag_connection_t* sal_conn;

    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_set_volume_param_t* params = zalloc(sizeof(bt_hfp_ag_set_volume_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->type = type;
    params->gain = volume > 15 ? 15 : volume;

    if (!service_loop_work(params, do_ag_set_volume, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
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
    const char* start;
    const char* end;
    size_t line_len;

    if (!addr || !atcmd || length == 0) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

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

    bt_hfp_ag_send_at_cmd_param_t* params = zalloc(sizeof(bt_hfp_ag_send_at_cmd_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    strlcpy(params->line, start, line_len + 1);

    BT_LOGD("%s, send vendor rsp: %s", __func__, params->line);

    if (!service_loop_work(params, do_ag_send_at_cmd, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

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
    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_ag_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn || !sal_conn->ag) {
        conn_list_unlock();
        BT_LOGE("%s, connection not found", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_ag_error_response_param_t* params = zalloc(sizeof(bt_hfp_ag_error_response_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->result = result;

    if (!service_loop_work(params, do_ag_error_response, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}
