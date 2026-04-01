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
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/sdp.h>

static bt_list_t* g_sal_hf_conn_list = NULL;
static pthread_mutex_t g_sal_hf_conn_lock;

static void conn_list_lock(void)
{
    pthread_mutex_lock(&g_sal_hf_conn_lock);
}

static void conn_list_unlock(void)
{
    pthread_mutex_unlock(&g_sal_hf_conn_lock);
}

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

typedef struct _bt_hfp_hf_set_volume_param {
    bt_address_t addr;
    hfp_volume_type_t type;
    uint8_t gain;
} bt_hfp_hf_set_volume_param_t;

typedef struct _bt_hfp_hf_simple_addr_param {
    bt_address_t addr;
} bt_hfp_hf_simple_addr_param_t;

typedef struct _bt_hfp_hf_call_control_param {
    bt_address_t addr;
    hfp_call_control_t chld;
    uint32_t index;
} bt_hfp_hf_call_control_param_t;

typedef struct _bt_hfp_hf_dial_number_param {
    bt_address_t addr;
    char number[HFP_PHONENUM_DIGITS_MAX + 1];
    bool has_number;
} bt_hfp_hf_dial_number_param_t;

typedef struct _bt_hfp_hf_dial_memory_param {
    bt_address_t addr;
    char mem_in_str[HFP_PHONENUM_DIGITS_MAX + 1];
} bt_hfp_hf_dial_memory_param_t;

typedef struct _bt_hfp_hf_voice_recognition_param {
    bt_address_t addr;
    bool activate;
} bt_hfp_hf_voice_recognition_param_t;

typedef struct _bt_hfp_hf_battery_level_param {
    bt_address_t addr;
    uint8_t level;
} bt_hfp_hf_battery_level_param_t;

typedef struct _bt_hfp_hf_send_at_cmd_param {
    bt_address_t addr;
    char cmd[HFP_AT_LEN_MAX + 1];
} bt_hfp_hf_send_at_cmd_param_t;

typedef struct _bt_hfp_hf_send_dtmf_param {
    bt_address_t addr;
    char dtmf;
} bt_hfp_hf_send_dtmf_param_t;

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

    if (!z_context) {
        return NULL;
    }

    if (!g_sal_hf_conn_list) {
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
    if (!conn) {
        return NULL;
    }
    if (!g_sal_hf_conn_list) {
        return NULL;
    }
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_context_cmp, conn);
}

static inline bt_hfp_hf_connection_t* find_connection_by_addr(bt_address_t* addr)
{
    if (!addr) {
        return NULL;
    }
    if (!g_sal_hf_conn_list) {
        return NULL;
    }
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_addr_cmp, addr);
}

static inline bt_hfp_hf_connection_t* find_connection_by_hf(struct bt_hfp_hf* hf)
{
    if (!hf) {
        return NULL;
    }
    if (!g_sal_hf_conn_list) {
        return NULL;
    }
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_hf_cmp, hf);
}

static bt_hfp_hf_connection_t* find_connection_by_sco(struct bt_conn* sco)
{
    bt_list_node_t* node;

    if (!sco) {
        return NULL;
    }

    if (!g_sal_hf_conn_list) {
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

    conn_list_lock();
    sal_conn = find_connection_by_addr(addr);
    if (sal_conn != NULL) {
        conn_list_unlock();
        BT_LOGI("%s, Connection already exists, skip", __func__);
        return BT_STATUS_SUCCESS;
    }

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    /** Remeber to unref @p conn once SLC is initiated or cancelled */
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to lookup connection", __func__);
        hfp_hf_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
        return BT_STATUS_NOT_FOUND;
    }

    sal_conn = new_hf_connection(conn, NULL);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, could not create new hf connection", __func__);
        hfp_hf_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
        bt_conn_unref(conn);
        return BT_STATUS_NOMEM;
    }
    conn_list_unlock();

    BT_LOGD("%s, do sdp discover", __func__);
    if (bt_sdp_discover(conn, &sdp_discover) < 0) {
        BT_LOGE("%s, failed to start a SDP discovery", __func__);
        hfp_hf_on_connection_state_changed(addr, PROFILE_STATE_DISCONNECTED, 0, 0);
        bt_sal_cm_profile_disconnected_callback(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
        bt_conn_unref(conn);
        conn_list_lock();
        bt_list_remove(g_sal_hf_conn_list, sal_conn);
        conn_list_unlock();
        return BT_STATUS_FAIL;
    }

    bt_conn_unref(conn);
    return BT_STATUS_SUCCESS;
}

bt_status_t do_hf_disconnect(bt_controller_id_t id, bt_address_t* addr, void* user_data)
{
    struct bt_hfp_hf* hf;

    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }

    if (!sal_conn->hf) {
        BT_LOGI("%s, HFP HF not connected", __func__);
        bt_list_remove(g_sal_hf_conn_list, sal_conn);
        conn_list_unlock();
        return BT_STATUS_SUCCESS;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    SAL_CHECK_RET(Z_API(bt_hfp_hf_disconnect)(hf), 0);
    return BT_STATUS_SUCCESS;
}

static void do_hf_sco_disconnect(service_work_t* work, void* userdata)
{
    struct bt_conn* sco_conn = (struct bt_conn*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    bt_address_t addr;
    int err;

    if (!sco_conn) {
        BT_LOGE("%s, Invalid parameters", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_sco(sco_conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, sco_conn no longer tracked, skip disconnect", __func__);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    err = bt_conn_disconnect(sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    if (err) {
        conn_list_lock();
        sal_conn = find_connection_by_sco(sco_conn);
        if (sal_conn) {
            sal_conn->sco_conn = NULL;
        }
        conn_list_unlock();
        BT_LOGE("%s, Failed to disconnect HFP HF SCO, err=%d", __func__, err);
        hfp_hf_on_audio_connection_state_changed(&addr, HFP_AUDIO_STATE_DISCONNECTED, 0);
    }
}

static void do_hf_sco_connect(service_work_t* work, void* userdata)
{
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf = (struct bt_hfp_hf*)userdata;
    bt_address_t addr;
    int err;

    if (!hf) {
        BT_LOGE("%s, Invalid parameters", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_hf(hf);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, hf no longer tracked, skip connect", __func__);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    err = Z_API(bt_hfp_hf_audio_connect)(hf);
    if (err == -EALREADY) {
        BT_LOGW("%s, Audio already connected", __func__);
        hfp_hf_on_audio_connection_state_changed(&addr, HFP_AUDIO_STATE_CONNECTED, 0);
    } else if (err) {
        BT_LOGE("%s, Failed to connect HFP HF SCO, err=%d", __func__, err);
        hfp_hf_on_audio_connection_state_changed(&addr, HFP_AUDIO_STATE_DISCONNECTED, 0);
    }
}

static void do_hf_set_volume(service_work_t* work, void* userdata)
{
    bt_hfp_hf_set_volume_param_t* params = (bt_hfp_hf_set_volume_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;
    int ret;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available, skip set volume", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    switch (params->type) {
    case HFP_VOLUME_TYPE_MIC:
        ret = Z_API(bt_hfp_hf_vgm)(hf, params->gain);
        break;
    case HFP_VOLUME_TYPE_SPK:
        ret = Z_API(bt_hfp_hf_vgs)(hf, params->gain);
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

static void do_hf_answer_call(service_work_t* work, void* userdata)
{
    bt_hfp_hf_simple_addr_param_t* params = (bt_hfp_hf_simple_addr_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf_call* call_context;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }

    bt_hfp_hf_call_info_t* incoming = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_INCOMING);
    if (!incoming) {
        conn_list_unlock();
        BT_LOGE("%s, No incoming call to answer", __func__);
        free(params);
        return;
    }
    call_context = incoming->context;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_accept)(call_context);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_accept failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_reject_call(service_work_t* work, void* userdata)
{
    bt_hfp_hf_simple_addr_param_t* params = (bt_hfp_hf_simple_addr_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf_call* call_context;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }

    bt_hfp_hf_call_info_t* incoming_call = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_INCOMING);
    if (!incoming_call) {
        conn_list_unlock();
        BT_LOGE("%s, No incoming call to reject", __func__);
        free(params);
        return;
    }
    call_context = incoming_call->context;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_reject)(call_context);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_reject failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_hold_call(service_work_t* work, void* userdata)
{
    bt_hfp_hf_simple_addr_param_t* params = (bt_hfp_hf_simple_addr_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_hold_active_accept_other)(hf);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_hold_active_accept_other failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_hangup_call(service_work_t* work, void* userdata)
{
    bt_hfp_hf_simple_addr_param_t* params = (bt_hfp_hf_simple_addr_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf_call* target_context;
    struct bt_hfp_hf* hf;
    int call_count;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }

    bt_hfp_hf_call_info_t* target = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_ACTIVE);
    if (!target) {
        target = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_DIALING);
    }
    if (!target) {
        target = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_ALERTING);
    }

    if (!target) {
        conn_list_unlock();
        BT_LOGE("%s, No active/dialing/alerting call to hang up", __func__);
        free(params);
        return;
    }

    target_context = target->context;
    hf = sal_conn->hf;
    call_count = count_call(sal_conn);
    conn_list_unlock();

    int ret;
    if (call_count == 1) {
        ret = Z_API(bt_hfp_hf_terminate)(target_context);
    } else {
        ret = Z_API(bt_hfp_hf_release_active_accept_other)(hf);
    }

    if (ret) {
        BT_LOGE("%s, hangup failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_dial_number(service_work_t* work, void* userdata)
{
    bt_hfp_hf_dial_number_param_t* params = (bt_hfp_hf_dial_number_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret;
    if (!params->has_number) {
        ret = Z_API(bt_hfp_hf_redial)(hf);
    } else {
        ret = Z_API(bt_hfp_hf_number_call)(hf, params->number);
    }

    if (ret) {
        BT_LOGE("%s, dial failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_dial_memory(service_work_t* work, void* userdata)
{
    bt_hfp_hf_dial_memory_param_t* params = (bt_hfp_hf_dial_memory_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_memory_dial)(hf, params->mem_in_str);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_memory_dial failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_call_control(service_work_t* work, void* userdata)
{
    bt_hfp_hf_call_control_param_t* params = (bt_hfp_hf_call_control_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;

    int ret = -ENOTSUP;
    switch (params->chld) {
    case HFP_HF_CALL_CONTROL_CHLD_0: {
        bt_hfp_hf_call_info_t* waiting = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_WAITING);
        if (waiting) {
            conn_list_unlock();
            ret = Z_API(bt_hfp_hf_set_udub)(hf);
        } else {
            bt_hfp_hf_call_info_t* held = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_HELD);
            if (held) {
                conn_list_unlock();
                ret = Z_API(bt_hfp_hf_release_all_held)(hf);
            } else {
                conn_list_unlock();
                BT_LOGW("%s, No waiting/held call for CHLD=0", __func__);
            }
        }
        break;
    }
    case HFP_HF_CALL_CONTROL_CHLD_1:
        if (params->index > 0) {
            bt_hfp_hf_call_info_t* by_idx = find_call_by_index(sal_conn, (uint8_t)params->index);
            if (!by_idx) {
                conn_list_unlock();
                BT_LOGE("%s, No call with index %u for CHLD=1<idx>", __func__, (unsigned)params->index);
                free(params);
                return;
            }
            struct bt_hfp_hf_call* call_ctx = by_idx->context;
            conn_list_unlock();
            ret = Z_API(bt_hfp_hf_release_specified_call)(call_ctx);
        } else {
            conn_list_unlock();
            ret = Z_API(bt_hfp_hf_release_active_accept_other)(hf);
        }
        break;
    case HFP_HF_CALL_CONTROL_CHLD_2:
        if (params->index > 0) {
            bt_hfp_hf_call_info_t* by_idx = find_call_by_index(sal_conn, (uint8_t)params->index);
            if (!by_idx) {
                conn_list_unlock();
                BT_LOGE("%s, No call with index %u for CHLD=2<idx>", __func__, (unsigned)params->index);
                free(params);
                return;
            }
            struct bt_hfp_hf_call* call_ctx = by_idx->context;
            conn_list_unlock();
            ret = Z_API(bt_hfp_hf_private_consultation_mode)(call_ctx);
        } else {
            conn_list_unlock();
            ret = Z_API(bt_hfp_hf_hold_active_accept_other)(hf);
        }
        break;
    case HFP_HF_CALL_CONTROL_CHLD_3:
        conn_list_unlock();
        ret = Z_API(bt_hfp_hf_join_conversation)(hf);
        break;
    case HFP_HF_CALL_CONTROL_CHLD_4:
        conn_list_unlock();
        ret = Z_API(bt_hfp_hf_explicit_call_transfer)(hf);
        break;
    default:
        conn_list_unlock();
        BT_LOGE("%s, unsupported CHLD=%d", __func__, params->chld);
        break;
    }

    if (ret) {
        BT_LOGE("%s, call control failed, chld=%d, ret=%d", __func__, params->chld, ret);
    }

    free(params);
}

static void do_hf_get_current_calls(service_work_t* work, void* userdata)
{
    bt_hfp_hf_simple_addr_param_t* params = (bt_hfp_hf_simple_addr_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_query_list_of_current_calls)(hf);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_query_list_of_current_calls failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_voice_recognition(service_work_t* work, void* userdata)
{
    bt_hfp_hf_voice_recognition_param_t* params = (bt_hfp_hf_voice_recognition_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_voice_recognition)(hf, params->activate);
    if (ret) {
        BT_LOGE("%s, Failed to %s voice recognition, ret=%d", __func__,
            params->activate ? "start" : "stop", ret);
    }

    free(params);
}

static void do_hf_send_battery_level(service_work_t* work, void* userdata)
{
    bt_hfp_hf_battery_level_param_t* params = (bt_hfp_hf_battery_level_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_battery)(hf, params->level);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_battery failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_send_at_cmd(service_work_t* work, void* userdata)
{
    bt_hfp_hf_send_at_cmd_param_t* params = (bt_hfp_hf_send_at_cmd_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_send_vendor)(hf, params->cmd);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_send_vendor failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_send_dtmf(service_work_t* work, void* userdata)
{
    bt_hfp_hf_send_dtmf_param_t* params = (bt_hfp_hf_send_dtmf_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf_call* active_context;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }

    bt_hfp_hf_call_info_t* active = find_call_by_state(sal_conn, HFP_HF_CALL_STATE_ACTIVE);
    if (!active) {
        conn_list_unlock();
        BT_LOGE("%s, No active call for DTMF", __func__);
        free(params);
        return;
    }
    active_context = active->context;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_transmit_dtmf_code)(active_context, params->dtmf);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_transmit_dtmf_code failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_get_subscriber_number(service_work_t* work, void* userdata)
{
    bt_hfp_hf_simple_addr_param_t* params = (bt_hfp_hf_simple_addr_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf;

    if (!params) {
        BT_LOGE("%s, params is NULL", __func__);
        return;
    }

    conn_list_lock();
    sal_conn = find_connection_by_addr(&params->addr);
    if (!sal_conn || !sal_conn->hf) {
        conn_list_unlock();
        BT_LOGW("%s, connection no longer available", __func__);
        free(params);
        return;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_query_subscriber)(hf);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_query_subscriber failed, ret=%d", __func__, ret);
    }

    free(params);
}

static void do_hf_slc_connect(service_work_t* work, void* userdata)
{
    bt_hfp_hf_slc_connect_param_t* params = (bt_hfp_hf_slc_connect_param_t*)userdata;
    bt_hfp_hf_connection_t* sal_conn;
    struct bt_hfp_hf* hf = NULL;
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

    if (sal_conn->hf) {
        conn_list_unlock();
        BT_LOGD("%s, already initiating SLC, skip SLC initiating", __func__);
        bt_conn_unref(params->conn);
        free(params);
        return;
    }
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    BT_LOGD("%s, SLC initiating", __func__);
    if (Z_API(bt_hfp_hf_connect)(params->conn, &hf, params->channel)) {
        BT_LOGE("%s, HFP HF connection initiation failed", __func__);
        goto error;
    }

    conn_list_lock();
    sal_conn = find_connection_by_context(params->conn);
    if (sal_conn) {
        sal_conn->hf = hf;
    }
    conn_list_unlock();

    bt_conn_unref(params->conn);
    free(params);

    hfp_hf_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTING, 0, 0);

    BT_LOGD("%s, HFP HF connecting", __func__);
    return;

error:
    bt_conn_unref(params->conn);
    free(params);
    hfp_hf_on_connection_state_changed(&addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
    conn_list_lock();
    bt_list_remove(g_sal_hf_conn_list, sal_conn);
    conn_list_unlock();
    return;
}

static void zblue_on_sdp_disconnected(struct bt_conn* conn, const struct bt_sdp_discover_params* params)
{
    bt_address_t bd_addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_context(conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, no pending connection found", __func__);
        return;
    }
    memcpy(&bd_addr, &sal_conn->addr, sizeof(bt_address_t));

    BT_LOGW("%s, SDP disconnected during discovery", __func__);
    bt_list_remove(g_sal_hf_conn_list, sal_conn);
    conn_list_unlock();

    hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
}

static uint8_t zblue_on_sdp_done(struct bt_conn* conn, struct bt_sdp_client_result* result,
    const struct bt_sdp_discover_params* ignore)
{
    int err;
    uint16_t port;
    bt_address_t bd_addr;
    bt_hfp_hf_slc_connect_param_t* params;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_context(conn);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, could not find sal_conn", __func__);
        return BT_SDP_DISCOVER_UUID_STOP;
    }
    memcpy(&bd_addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

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

    if (!service_loop_work(params, do_hf_slc_connect, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        bt_conn_unref(params->conn);
        free(params);
        goto error;
    }

    return BT_SDP_DISCOVER_UUID_STOP;

error:
    conn_list_lock();
    bt_list_remove(g_sal_hf_conn_list, sal_conn);
    conn_list_unlock();
    hfp_hf_on_connection_state_changed(&bd_addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&bd_addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
    return BT_SDP_DISCOVER_UUID_STOP;
}

static void zblue_on_connected(struct bt_conn* conn, struct bt_hfp_hf* hf)
{
    bt_hfp_hf_connection_t* sal_conn;
    bt_address_t addr;

    conn_list_lock();
    sal_conn = find_connection_by_context(conn);
    if (!sal_conn) {
        BT_LOGD("%s, hf connection incoming", __func__);
        sal_conn = new_hf_connection(conn, hf);
        if (!sal_conn) {
            conn_list_unlock();
            BT_LOGE("%s, Failed to create HFP HF connection", __func__);
            if (Z_API(bt_hfp_hf_disconnect)(hf)) {
                BT_LOGE("%s, Failed to disconnect HFP HF connection", __func__);
            }
            return;
        }

        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();
        hfp_hf_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTING, 0, 0);
    } else {
        /* Override conn if both sides attempt to connect at the same time */
        sal_conn->conn = conn;
        sal_conn->hf = hf;
        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();
    }

    bt_sal_cm_profile_connected_callback(&addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
    bt_sal_profile_disconnect_register(&addr, PROFILE_HFP_HF, CONN_ID_DEFAULT, PRIMARY_ADAPTER, do_hf_disconnect, NULL);

    hfp_hf_on_connection_state_changed(&addr, PROFILE_STATE_CONNECTED, 0, 0);
}

static void zblue_hf_disconnected(struct bt_hfp_hf* hf)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    memcpy(&addr, &conn->addr, sizeof(bt_address_t));

    bt_list_remove(g_sal_hf_conn_list, conn);
    conn_list_unlock();

    hfp_hf_on_connection_state_changed(&addr, PROFILE_STATE_DISCONNECTING, 0, 0);
    hfp_hf_on_connection_state_changed(&addr, PROFILE_STATE_DISCONNECTED, 0, 0);
    bt_sal_cm_profile_disconnected_callback(&addr, PROFILE_HFP_HF, CONN_ID_DEFAULT);
}

static void zblue_on_sco_connected(struct bt_hfp_hf* hf, struct bt_conn* sco_conn)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection for SCO", __func__);
        return;
    }

    conn->sco_conn = sco_conn;
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_audio_connection_state_changed(&addr, HFP_AUDIO_STATE_CONNECTED, 0);
}

static void zblue_on_sco_disconnected(struct bt_conn* sco_conn, uint8_t reason)
{
    bt_address_t addr;
    (void)reason;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_sco(sco_conn);
    if (!conn) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find connection for SCO disconn", __func__);
        return;
    }

    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn->sco_conn = NULL;
    conn_list_unlock();

    hfp_hf_on_audio_connection_state_changed(&addr, HFP_AUDIO_STATE_DISCONNECTED, 0);
}

static void zblue_on_outgoing_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_call* call)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(hf);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    bt_hfp_hf_call_info_t* sal_call = find_or_create_call(sal_conn, call);
    if (!sal_call) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to track outgoing call", __func__);
        return;
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_DIALING);
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_call_setup_state_changed(&addr, HFP_CALLSETUP_OUTGOING);
}

static void zblue_on_incoming_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_call* call)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(hf);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    bt_hfp_hf_call_info_t* sal_call = find_or_create_call(sal_conn, call);
    if (!sal_call) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to track incoming call", __func__);
        return;
    }

    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_INCOMING);
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_call_setup_state_changed(&addr, HFP_CALLSETUP_INCOMING);
}

static void zblue_on_remote_ringing(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, &sal_call);

    if (!conn) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find connection for remote ringing", __func__);
        return;
    }

    if (sal_call) {
        set_call_state(conn, sal_call, HFP_HF_CALL_STATE_ALERTING);
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_call_setup_state_changed(&addr, HFP_CALLSETUP_ALERTING);
}

static void zblue_on_call_accept(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, &sal_call);

    if (!conn || !sal_call) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find call to accept", __func__);
        return;
    }

    set_call_state(conn, sal_call, HFP_HF_CALL_STATE_ACTIVE);
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_call_active_state_changed(&addr, HFP_CALL_CALLS_IN_PROGRESS);
    hfp_hf_on_call_setup_state_changed(&addr, HFP_CALLSETUP_NONE);
}

static void zblue_on_call_reject(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find call to reject", __func__);
        return;
    }

    remove_call(sal_conn, sal_call);
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_call_setup_state_changed(&addr, HFP_CALLSETUP_NONE);
}

static void zblue_on_call_terminate(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_address_t addr;
    hfp_hf_call_state_t prev_state;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find call to terminate", __func__);
        return;
    }

    prev_state = sal_call->state;
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    remove_call(sal_conn, sal_call);
    conn_list_unlock();

    if (prev_state == HFP_HF_CALL_STATE_ACTIVE) {
        hfp_hf_on_call_active_state_changed(&addr, HFP_CALL_NO_CALLS_IN_PROGRESS);
    } else if (prev_state == HFP_HF_CALL_STATE_HELD) {
        hfp_hf_on_call_held_state_changed(&addr, HFP_CALLHELD_NONE);
    } else {
        BT_LOGW("Unknow previous state %d.", prev_state);
    }
}

static void zblue_on_call_held(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_address_t addr;
    hfp_hf_call_state_t prev_state;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find call to hold", __func__);
        return;
    }

    prev_state = sal_call->state;
    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_HELD);
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_call_held_state_changed(&addr, HFP_CALLHELD_HELD);

    if (prev_state == HFP_HF_CALL_STATE_ACTIVE) {
        hfp_hf_on_call_active_state_changed(&addr, HFP_CALL_NO_CALLS_IN_PROGRESS);
    } else {
        BT_LOGW("Unexpected previous state %d.", prev_state);
    }
}

static void zblue_on_call_retrieve(struct bt_hfp_hf_call* call)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    bt_address_t addr;
    hfp_hf_call_state_t prev_state;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_call_context(call, &sal_call);

    if (!sal_conn || !sal_call) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find call to retrieve", __func__);
        return;
    }

    prev_state = sal_call->state;
    set_call_state(sal_conn, sal_call, HFP_HF_CALL_STATE_ACTIVE);
    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    if (prev_state == HFP_HF_CALL_STATE_HELD) {
        hfp_hf_on_call_held_state_changed(&addr, HFP_CALLHELD_NONE);
    } else {
        BT_LOGW("Unexpected previous state %d.", prev_state);
    }

    hfp_hf_on_call_active_state_changed(&addr, HFP_CALL_CALLS_IN_PROGRESS);
}

static void zblue_on_subscriber_number(struct bt_hfp_hf* hf, const char* number, uint8_t type, uint8_t service)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

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

    hfp_hf_on_subscriber_number_response(&addr, number, fw_service);
}

static void zblue_on_vgm(struct bt_hfp_hf* hf, uint8_t gain)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_volume_changed(&addr, HFP_VOLUME_TYPE_MIC, gain);
}

static void zblue_on_vgs(struct bt_hfp_hf* hf, uint8_t gain)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_volume_changed(&addr, HFP_VOLUME_TYPE_SPK, gain);
}

static void zblue_on_voice_recognition(struct bt_hfp_hf* hf, bool activate)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_voice_recognition_state_changed(&addr, activate);
}

static void zblue_on_ring_indication(struct bt_hfp_hf_call* call)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, NULL);
    if (!conn) {
        conn_list_unlock();
        BT_LOGW("%s, Failed to find connection for ring", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_ring_active_state_changed(&addr, true, HFP_IN_BAND_RINGTONE_NOT_PROVIDED);
}

static void zblue_on_clip(struct bt_hfp_hf_call* call, char* number, uint8_t type)
{
    bt_hfp_hf_call_info_t* sal_call = NULL;
    const char* num = number ? number : "";
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_call_context(call, &sal_call);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection for CLIP", __func__);
        return;
    }

    if (sal_call) {
        sal_call->type = type;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_clip(&addr, num, "");
}

static void zblue_on_vendor_specific(struct bt_hfp_hf* hf, const char* cmd, const char* value)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection for vendor specific response", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    if (!cmd || !value) {
        return;
    }

    size_t cmd_len = strlen(cmd);
    size_t val_len = strlen(value);
    size_t len = cmd_len + val_len + 2; /* '+' and ':' */

    char* rsp = malloc(len + 1);
    if (!rsp) {
        BT_LOGE("%s, Failed to allocate vendor response", __func__);
        return;
    }

    snprintf(rsp, len + 1, "+%s:%s", cmd, value);
    hfp_hf_on_received_at_cmd_resp(&addr, rsp, len);
    free(rsp);
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

static void zblue_on_at_cmd_complete(struct bt_hfp_hf* hf, enum bt_hfp_hf_at_cmd cmd,
    enum bt_at_result result, enum bt_at_cme err)
{
    bt_address_t addr;
    BT_LOGD("%s, AT cmd complete: cmd=%d, result=%d, err=%d", __func__, cmd, result, err);

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection for AT cmd complete", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    uint32_t result_code = result == BT_AT_RESULT_CME_ERROR ? err : result;

    hfp_hf_on_at_command_result_response(&addr, zblue_at_cmd_to_service_cmd(cmd), result_code);
}

static void zblue_on_codec_negotiate(struct bt_hfp_hf* hf, uint8_t id)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* conn = find_connection_by_hf(hf);
    if (!conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }
    memcpy(&addr, &conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    int ret = Z_API(bt_hfp_hf_select_codec)(hf, id);
    if (ret) {
        BT_LOGE("%s, bt_hfp_hf_select_codec failed: %d", __func__, ret);
    }

    hfp_codec_config_t cfg = { 0 };
    switch (id) {
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

    hfp_hf_on_codec_changed(&addr, &cfg);
}

static void zblue_on_current_call(struct bt_hfp_hf* hf, struct bt_hfp_hf_current_call* call)
{
    bt_address_t addr;

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_hf(hf);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return;
    }

    if (!call) {
        memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
        conn_list_unlock();
        BT_ADDR_LOG("CLCC finished from %s", &addr);
        hfp_hf_on_current_call_response(&addr, 0, 0, 0, 0, NULL, 0);
        return;
    }

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

    if (sal_call) {
        sal_call->index = idx;
        sal_call->state = status;
    }

    memcpy(&addr, &sal_conn->addr, sizeof(bt_address_t));
    conn_list_unlock();

    hfp_hf_on_current_call_response(&addr, idx, dir, status, mpty, call->number, call->type);
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
    pthread_mutexattr_t attr;

    (void)hf_features;
    (void)max_connection;
    int err;
    g_sal_hf_conn_list = bt_list_new(free_connection);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_sal_hf_conn_lock, &attr);
    pthread_mutexattr_destroy(&attr);

    err = Z_API(bt_hfp_hf_register)(&hf_callbacks);

    if (err) {
        bt_list_free(g_sal_hf_conn_list);
        g_sal_hf_conn_list = NULL;
        pthread_mutex_destroy(&g_sal_hf_conn_lock);
    }

    SAL_CHECK_RET(err, 0);
    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_hf_cleanup(void)
{
    if (Z_API(bt_hfp_hf_unregister)()) {
        BT_LOGE("%s, Failed to unregister HFP HF callbacks", __func__);
    }

    conn_list_lock();
    if (g_sal_hf_conn_list) {
        bt_list_free(g_sal_hf_conn_list);
        g_sal_hf_conn_list = NULL;
    }
    conn_list_unlock();
    pthread_mutex_destroy(&g_sal_hf_conn_lock);

    return;
}

bt_status_t bt_sal_hfp_hf_connect(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (sal_conn) {
        conn_list_unlock();
        BT_LOGW("%s, Connection already exists or in progress", __func__);
        return BT_STATUS_BUSY;
    }
    conn_list_unlock();

    return bt_sal_profile_connect_request(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT, 0, do_hf_sdp_discover, NULL);
}

bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }
    conn_list_unlock();

    return bt_sal_profile_disconnect_request(addr, PROFILE_HFP_HF, CONN_ID_DEFAULT, 0, do_hf_disconnect, NULL);
}

bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    struct bt_hfp_hf* hf;
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_FAIL;
    }

    if (!sal_conn->hf) {
        conn_list_unlock();
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, connection is initializing for address: %s", __func__, addr_str);
        return BT_STATUS_NOT_READY;
    }
    hf = sal_conn->hf;
    conn_list_unlock();

    if (!service_loop_work(hf, do_hf_sco_connect, NULL)) {
        BT_LOGE("%s, service loop work submit failed.", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t* addr)
{
    struct bt_conn* sco_conn;
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    if (!sal_conn->sco_conn) {
        conn_list_unlock();
        BT_LOGW("%s, SCO not connected", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    sco_conn = sal_conn->sco_conn;
    conn_list_unlock();

    if (!service_loop_work(sco_conn, do_hf_sco_disconnect, NULL)) {
        BT_LOGE("%s, service loop work submit failed.", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_simple_addr_param_t* params = zalloc(sizeof(bt_hfp_hf_simple_addr_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));

    if (!service_loop_work(params, do_hf_answer_call, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_reject_call(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }
    conn_list_unlock();

    bt_hfp_hf_simple_addr_param_t* params = zalloc(sizeof(bt_hfp_hf_simple_addr_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));

    if (!service_loop_work(params, do_hf_reject_call, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hold_call(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }
    conn_list_unlock();

    bt_hfp_hf_simple_addr_param_t* params = zalloc(sizeof(bt_hfp_hf_simple_addr_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));

    if (!service_loop_work(params, do_hf_hold_call, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_hangup_call(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_FAIL;
    }
    conn_list_unlock();

    bt_hfp_hf_simple_addr_param_t* params = zalloc(sizeof(bt_hfp_hf_simple_addr_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));

    if (!service_loop_work(params, do_hf_hangup_call, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_number(bt_address_t* addr, const char* number)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_dial_number_param_t* params = zalloc(sizeof(bt_hfp_hf_dial_number_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    if (number) {
        strlcpy(params->number, number, sizeof(params->number));
        params->has_number = true;
    } else {
        params->has_number = false;
    }

    if (!service_loop_work(params, do_hf_dial_number, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_dial_memory_param_t* params = zalloc(sizeof(bt_hfp_hf_dial_memory_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    snprintf(params->mem_in_str, sizeof(params->mem_in_str), "%" PRIu32, memory);

    if (!service_loop_work(params, do_hf_dial_memory, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_call_control(bt_address_t* addr, hfp_call_control_t chld, uint32_t index)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();
    if (!sal_conn) {
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_call_control_param_t* params = zalloc(sizeof(bt_hfp_hf_call_control_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->chld = chld;
    params->index = index;

    if (!service_loop_work(params, do_hf_call_control, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_simple_addr_param_t* params = zalloc(sizeof(bt_hfp_hf_simple_addr_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));

    if (!service_loop_work(params, do_hf_get_current_calls, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_set_volume(bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("%s, Failed to find connection for address: %s", __func__, addr_str);
        return BT_STATUS_PARM_INVALID;
    }

    if (!sal_conn->hf) {
        conn_list_unlock();
        BT_LOGE("%s, connection is not ready", __func__);
        return BT_STATUS_NOT_READY;
    }
    conn_list_unlock();

    bt_hfp_hf_set_volume_param_t* params = zalloc(sizeof(bt_hfp_hf_set_volume_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->type = type;
    params->gain = volume > 15 ? 15 : volume;

    if (!service_loop_work(params, do_hf_set_volume, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_start_voice_recognition(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_voice_recognition_param_t* params = zalloc(sizeof(bt_hfp_hf_voice_recognition_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->activate = true;

    if (!service_loop_work(params, do_hf_voice_recognition, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_stop_voice_recognition(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_voice_recognition_param_t* params = zalloc(sizeof(bt_hfp_hf_voice_recognition_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->activate = false;

    if (!service_loop_work(params, do_hf_voice_recognition, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_battery_level(bt_address_t* addr, uint8_t value)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_battery_level_param_t* params = zalloc(sizeof(bt_hfp_hf_battery_level_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->level = (value > 100) ? 100 : value;

    if (!service_loop_work(params, do_hf_send_battery_level, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_at_cmd(bt_address_t* addr, const char* cmd, uint16_t len)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    if (!cmd || len == 0) {
        BT_LOGE("%s, Invalid AT command", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    bt_hfp_hf_send_at_cmd_param_t* params = zalloc(sizeof(bt_hfp_hf_send_at_cmd_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    strlcpy(params->cmd, cmd, sizeof(params->cmd));

    BT_LOGD("%s, Sending AT command: %.*s", __func__, len, cmd);

    if (!service_loop_work(params, do_hf_send_at_cmd, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_send_dtmf(bt_address_t* addr, char dtmf)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_send_dtmf_param_t* params = zalloc(sizeof(bt_hfp_hf_send_dtmf_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));
    params->dtmf = dtmf;

    if (!service_loop_work(params, do_hf_send_dtmf, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_hfp_hf_get_subscriber_number(bt_address_t* addr)
{
    if (!addr) {
        BT_LOGE("%s, addr is NULL", __func__);
        return BT_STATUS_PARM_INVALID;
    }

    conn_list_lock();
    bt_hfp_hf_connection_t* sal_conn = find_connection_by_addr(addr);
    if (!sal_conn) {
        conn_list_unlock();
        BT_LOGE("%s, Failed to find connection", __func__);
        return BT_STATUS_PARM_INVALID;
    }
    conn_list_unlock();

    bt_hfp_hf_simple_addr_param_t* params = zalloc(sizeof(bt_hfp_hf_simple_addr_param_t));
    if (!params) {
        BT_LOGE("%s, Failed to allocate params", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&params->addr, addr, sizeof(bt_address_t));

    if (!service_loop_work(params, do_hf_get_subscriber_number, NULL)) {
        BT_LOGE("%s, service loop work submit failed", __func__);
        free(params);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}
