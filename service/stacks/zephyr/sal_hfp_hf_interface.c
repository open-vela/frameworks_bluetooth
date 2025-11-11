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
#include <stdio.h>
#include <string.h>

#undef BT_UUID_DECLARE_16
#undef BT_UUID_DECLARE_32
#undef BT_UUID_DECLARE_128

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/sdp.h>

static bt_list_t* g_sal_hf_conn_list = NULL;

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

static __attribute__((unused)) void free_connection(void* data)
{
    bt_hfp_hf_connection_t* sal_conn = (bt_hfp_hf_connection_t*)data;
    if (sal_conn->calls) {
        bt_list_free(sal_conn->calls);
    }
    free(sal_conn);
    return;
}

static __attribute__((unused)) void free_call(void* data)
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

static __attribute__((unused)) bt_hfp_hf_connection_t* find_connection_by_call_context(
    struct bt_hfp_hf_call* z_context,
    bt_hfp_hf_call_info_t** call_info)
{
    if (!g_sal_hf_conn_list || !z_context) {
        return NULL;
    }

    bt_list_node_t* node;

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

static inline __attribute__((unused)) bt_hfp_hf_connection_t* find_connection_by_addr(bt_address_t* addr)
{
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_addr_cmp, addr);
}

static inline __attribute__((unused)) bt_hfp_hf_connection_t* find_connection_by_hf(struct bt_hfp_hf* hf)
{
    return (bt_hfp_hf_connection_t*)bt_list_find(g_sal_hf_conn_list, sal_conn_hf_cmp, hf);
}

static __attribute__((unused)) bt_hfp_hf_connection_t* new_hf_connection(struct bt_conn* conn, struct bt_hfp_hf* hf)
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

static __attribute__((unused)) struct bt_hfp_hf_cb hf_callbacks = {
    .connected = NULL,
    .disconnected = NULL,
    .sco_connected = NULL,
    .sco_disconnected = NULL,
    .service = NULL,
    .outgoing = NULL,
    .remote_ringing = NULL,
    .incoming = NULL,
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
    .dialing = NULL,
    .clip = NULL,
    .vgm = NULL,
    .vgs = NULL,
    .inband_ring = NULL,
    .operator = NULL,
    .codec_negotiate = NULL,
    .ecnr_turn_off = NULL,
    .call_waiting = NULL,
    .voice_recognition = NULL,
    .vre_state = NULL,
    .textual_representation = NULL,
    .request_phone_number = NULL,
    .subscriber_number = NULL,
    .query_call = NULL,
};

bt_status_t bt_sal_hfp_hf_init(uint32_t hf_features, uint8_t p_max_connection)
{
    return BT_STATUS_SUCCESS;
}

void bt_sal_hfp_hf_cleanup(void)
{
    return;
}

bt_status_t pre_hfp_hf_connect()
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_connect(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_disconnect(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_connect_audio(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_disconnect_audio(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_answer_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_reject_call(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
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
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_dial_memory(bt_address_t* addr, uint32_t memory)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_call_control(bt_address_t* addr, hfp_call_control_t chld, uint32_t index)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_hfp_hf_get_current_calls(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
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
    return BT_STATUS_UNSUPPORTED;
}
