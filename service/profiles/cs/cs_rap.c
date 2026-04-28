/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#define LOG_TAG "cs_rap"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"
#include "cs_rap.h"
#include "cs_distance.h"
#include "cs_rap_gattc.h"
#include "cs_service.h"
#include "service_loop.h"
#include "utils/log.h"

/**
 * @brief Segment header bit definitions
 *
 * According to RAS specification:
 * - Bit 0: First segment flag (1 = first segment)
 * - Bit 1: Last segment flag (1 = last segment)
 * - Bits 2-7: Segment index (0-63)
 */
#define CS_RAP_SEG_FIRST_FLAG       0x01
#define CS_RAP_SEG_LAST_FLAG        0x02
#define CS_RAP_SEG_INDEX_MASK       0xFC
#define CS_RAP_SEG_INDEX_SHIFT      2

/**
 * @brief On-demand mode state definitions
 */
#define CS_RAP_ON_DEMAND_STATE_IDLE         0
#define CS_RAP_ON_DEMAND_STATE_RECEIVING    1
#define CS_RAP_ON_DEMAND_STATE_WAIT_ACK     2
#define CS_RAP_ON_DEMAND_STATE_COMPLETE     3

/**
 * @brief On-demand mode timeout (5 seconds)
 */
#define CS_RAP_ON_DEMAND_TIMEOUT            (5 * 1000)

/* Phase distance calibration offset (meters), empirically determined */
#define CS_RAP_PHASE_DISTANCE_OFFSET        0.7f

/**
 * @brief Segment reassembly context
 */
typedef struct {
    bool in_progress;               /**< Reassembly in progress */
    uint16_t ranging_counter;       /**< Current ranging counter */
    uint8_t expected_seg_idx;       /**< Expected next segment index */
    uint8_t last_seg_idx;           /**< Last received segment index */
    uint16_t total_len;             /**< Total reassembled data length */
    uint8_t data[CS_RAP_STEP_DATA_BUF_LEN]; /**< Reassembly buffer */
} cs_rap_segment_ctx_t;

/**
 * @brief CS RAP environment structure
 *
 * This structure maintains the runtime state for CS RAP operations,
 * similar to ras_srv_env_t in cs_ras.c
 */
typedef struct {
    bt_address_t* addr;                     /**< Current connection address */
    uint8_t role;                           /**< Local role (Initiator/Reflector) */
    uint8_t ranging_mode;                   /**< Current ranging mode */
    uint8_t on_demand_state;                /**< On-demand mode state */
    uint32_t remote_features;               /**< Remote RAS features */
    cs_rap_subevent_data_t* local_data;     /**< Local RAS subevent data */
    cs_rap_subevent_data_t* remote_data;    /**< Remote RAP subevent data */
    cs_rap_segment_ctx_t seg_ctx;           /**< Segment reassembly context */
    cs_rap_internal_distance_cb distance_cb; /**< Distance result callback */
    service_timer_t* on_demand_timer;       /**< On-demand mode timeout timer */
} cs_rap_env_t;

static cs_rap_env_t* g_cs_rap = NULL;

/* Forward declarations for functions with circular dependencies */
static void cs_rap_process_segment(bt_address_t* addr, uint8_t* data, uint16_t len);
static void cs_rap_on_reassembly_complete(bt_address_t* addr);
static uint16_t cs_rap_restore_step_len(uint8_t* src, uint16_t src_len, uint8_t* dst, uint16_t dst_max_len, uint8_t num_antenna_paths, uint8_t num_steps);

/**
 * @brief On-demand mode timeout callback
 *
 * Called when the on-demand timer expires (5 seconds).
 * Clears the pending data and resets the state.
 */
static void cs_rap_on_demand_timeout(service_timer_t* timer, void* data)
{
    BT_LOGW("On-demand mode timeout, clearing pending data");

    if (g_cs_rap) {
        g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_IDLE;
        g_cs_rap->seg_ctx.in_progress = false;
        memset(g_cs_rap->remote_data, 0, sizeof(cs_rap_subevent_data_t) + CS_RAP_STEP_DATA_BUF_LEN);
        g_cs_rap->on_demand_timer = NULL;
    }
}

/**
 * @brief Parse ranging data header from raw data
 *
 * The header format follows RAS specification Section 3.2.1
 */
static void cs_rap_parse_header(uint8_t* data, uint16_t len, cs_rap_ranging_header_t* header)
{
    if (len < CS_RAP_SUB_PROCEDURE_HEAD) {
        BT_LOGE("Data too short for header: %d", len);
        return;
    }

    /* Parse 12-byte header according to RAS specification */
    header->ranging_counter = (data[0] | (data[1] << 8)) & 0x0FFF;
    header->config_id = (data[1] >> 4) & 0x0F;
    header->selected_tx_power = (int8_t)data[2];
    header->antenna_paths_mask = data[3];
    header->start_acl_conn_event = data[4] | (data[5] << 8);
    header->frequency_compensation = (int16_t)(data[6] | (data[7] << 8));
    header->procedure_done_status = data[8] & 0x0F;
    header->subevent_done_status = (data[8] >> 4) & 0x0F;
    header->procedure_abort_reason = data[9] & 0x0F;
    header->subevent_abort_reason = (data[9] >> 4) & 0x0F;
    header->reference_power_level = (int8_t)data[10];
    header->num_steps_reported = data[11];

    /* Calculate number of antenna paths from mask */
    uint8_t mask = header->antenna_paths_mask & 0x0F;
    header->num_antenna_paths = 0;
    while (mask) {
        header->num_antenna_paths += (mask & 1);
        mask >>= 1;
    }
}

/**
 * @brief Process local subevent data
 *
 * Converts the subevent result to RAP format and stores it for
 * later combination with remote data.
 */
static void cs_rap_process_local_subevent(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    if (!g_cs_rap) {
        return;
    }

    cs_rap_subevent_data_t* local = g_cs_rap->local_data;

    /* Store header information */
    local->ranging_counter = result->header.procedure_counter & 0x0FFF;
    local->header.ranging_counter = local->ranging_counter;
    local->header.config_id = result->header.config_id;
    local->header.selected_tx_power = result->header.reference_power_level;
    local->header.antenna_paths_mask = result->header.num_antenna_paths;
    local->header.start_acl_conn_event = result->header.start_acl_conn_event_counter;
    local->header.frequency_compensation = result->header.frequency_compensation;
    local->header.procedure_done_status = result->header.procedure_done_status;
    local->header.subevent_done_status = result->header.subevent_done_status;
    local->header.procedure_abort_reason = result->header.procedure_abort_reason;
    local->header.subevent_abort_reason = result->header.subevent_abort_reason;
    local->header.reference_power_level = result->header.reference_power_level;
    local->header.num_steps_reported = result->header.num_steps_reported;
    local->header.num_antenna_paths = result->header.num_antenna_paths;

    /* Store step data */
    if (result->len > CS_RAP_STEP_DATA_BUF_LEN || result->len == 0) {
        BT_LOGE("Invalid step data length: %d (max %d)", result->len, CS_RAP_STEP_DATA_BUF_LEN);
        memset(local->step_data, 0, CS_RAP_STEP_DATA_BUF_LEN);
        local->valid = false;
        local->step_data_len = 0;
        return;
    }

    memcpy(local->step_data, result->step_data_buf, result->len);
    local->step_data_len = result->len;
    local->valid = true;

    BT_LOGD("Local data stored: counter=%d, steps=%d, len=%d",
        local->ranging_counter, local->header.num_steps_reported, local->step_data_len);

    /* Check if we can calculate distance */
    bool ready = cs_rap_is_data_ready(addr);
    BT_LOGD("Local subevent: after store, is_data_ready=%d", ready);
    if (ready) {
        cs_rap_calculate_distance(addr);
    }
}

/**
 * @brief CS subevent result callback
 *
 * This callback is registered with CS service to receive local ranging data.
 */
static void cs_rap_subevent_result_cb(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    if (!g_cs_rap || !addr || !result) {
        BT_LOGE("Invalid parameters in subevent callback");
        return;
    }

    BT_LOGD("Received local subevent: procedure_done=%d, subevent_done=%d, steps=%d",
        result->header.procedure_done_status,
        result->header.subevent_done_status,
        result->header.num_steps_reported);

    if (result->header.procedure_done_status != BT_LE_SRV_CS_PROCEDURE_COMPLETE) {
        BT_LOGD("Procedure not complete, waiting for more data");
        return;
    }

    cs_rap_process_local_subevent(addr, result);
}

/* RAP GATTC callback handlers */

static void cs_rap_on_gattc_connected(bt_address_t* addr, uint16_t conn_id)
{
    BT_LOGD("RAP GATTC connected: %s", bt_addr_str(addr));

    if (g_cs_rap && !g_cs_rap->addr) {
        g_cs_rap->addr = (bt_address_t*)malloc(sizeof(bt_address_t));
        if (g_cs_rap->addr) {
            memcpy(g_cs_rap->addr, addr, sizeof(bt_address_t));
        }
    }
}

static void cs_rap_on_gattc_disconnected(bt_address_t* addr, uint16_t conn_id)
{
    BT_LOGD("RAP GATTC disconnected: %s", bt_addr_str(addr));

    if (g_cs_rap) {
        /* Cancel on-demand timer if running */
        if (g_cs_rap->on_demand_timer) {
            service_loop_cancel_timer(g_cs_rap->on_demand_timer);
            g_cs_rap->on_demand_timer = NULL;
        }

        if (g_cs_rap->addr) {
            free(g_cs_rap->addr);
            g_cs_rap->addr = NULL;
        }
        cs_rap_clear_data(addr);
        g_cs_rap->seg_ctx.in_progress = false;
        g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_IDLE;
    }
}

static void cs_rap_on_gattc_discover_complete(bt_address_t* addr, bool success, cs_rap_gattc_handles_t* handles)
{
    BT_LOGD("RAP GATTC discover complete: success=%d", success);

    if (success && handles) {
        BT_LOGD("RAP GATTC handles: features=0x%04x, rt_data=0x%04x, od_data=0x%04x, cp=0x%04x, dr=0x%04x",
            handles->features_handle, handles->real_time_data_handle,
            handles->on_demand_data_handle, handles->control_point_handle,
            handles->data_ready_handle);
        /* Discovery done after encryption. Read features to auto-set ranging mode + CCC */
        cs_rap_gattc_read_features(addr);
    } else {
        BT_LOGE("RAP GATTC discover FAILED or no handles, cannot proceed with RAS");
    }
}

static void cs_rap_on_gattc_features_read(bt_address_t* addr, uint32_t features)
{
    BT_LOGD("RAP RAS features: 0x%08lx", features);

    if (g_cs_rap) {
        g_cs_rap->remote_features = features;

        bool real_time_supported = (features & RAS_FEATURE_REAL_TIME_DATA) != 0;
        bool on_demand_supported = (features & RAS_FEATURE_ON_DEMAND_DATA) != 0;

        BT_LOGD("Remote supports: real-time=%d, on-demand=%d",
            real_time_supported, on_demand_supported);

        /* Auto-select mode based on remote capabilities */
        if (g_cs_rap->ranging_mode == 0) {
            if (real_time_supported) {
                cs_rap_set_ranging_mode(addr, CS_RANGING_MODE_REAL_TIME);
            } else if (on_demand_supported) {
                cs_rap_set_ranging_mode(addr, CS_RANGING_MODE_ON_DEMAND);
            }
        }
    }
}

static void cs_rap_on_gattc_data_ready(bt_address_t* addr, uint16_t ranging_counter)
{
    BT_LOGD("RAP data ready: counter=%d", ranging_counter);

    if (g_cs_rap && g_cs_rap->ranging_mode == CS_RANGING_MODE_ON_DEMAND) {
        /* Store the ranging counter for segment reassembly */
        g_cs_rap->seg_ctx.ranging_counter = ranging_counter;

        /* Request the ranging data */
        cs_rap_gattc_get_ranging_data(addr, ranging_counter);
    }
}

static void cs_rap_on_gattc_ranging_data(bt_address_t* addr, uint8_t* data, uint16_t len)
{
    BT_LOGD("RAP ranging data received: len=%d, mode=%d", len,
        g_cs_rap ? g_cs_rap->ranging_mode : 0);

    if (!g_cs_rap) {
        BT_LOGE("CS RAP not initialized");
        return;
    }

    if (g_cs_rap->ranging_mode == CS_RANGING_MODE_REAL_TIME) {
        /* Real-time mode: process data immediately and calculate distance */
        cs_rap_process_remote_data(addr, data, len);

        /* For real-time mode, check if we can calculate distance immediately */
        bool ready = cs_rap_is_data_ready(addr);
        BT_LOGD("RAP real-time: after process_remote_data, is_data_ready=%d", ready);
        if (ready) {
            cs_rap_calculate_distance(addr);
        }
    } else if (g_cs_rap->ranging_mode == CS_RANGING_MODE_ON_DEMAND) {
        /* On-demand mode: cache data and wait for complete interaction */
        if (g_cs_rap->on_demand_state == CS_RAP_ON_DEMAND_STATE_IDLE) {
            g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_RECEIVING;

            /* Start the on-demand timer (5 seconds) */
            if (g_cs_rap->on_demand_timer) {
                service_loop_cancel_timer(g_cs_rap->on_demand_timer);
            }
            g_cs_rap->on_demand_timer = service_loop_timer(CS_RAP_ON_DEMAND_TIMEOUT, 0,
                cs_rap_on_demand_timeout, g_cs_rap);
        }

        /* Process and cache the segment data */
        cs_rap_process_remote_data(addr, data, len);

        /* Note: Distance calculation will be triggered after receiving
         * RAS_CP_RSP_RESPONSE_CODE in cs_rap_on_gattc_control_point_rsp */
    } else {
        BT_LOGW("Unknown ranging mode: %d", g_cs_rap->ranging_mode);
    }
}

static void cs_rap_on_gattc_control_point_rsp(bt_address_t* addr, uint8_t opcode, uint8_t* params, uint16_t len)
{
    BT_LOGD("RAP control point response: opcode=0x%02x, len=%d", opcode, len);

    switch (opcode) {
    case RAS_CP_RSP_COMPLETE_RANGING_DATA: {
        /* Complete Ranging Data Response: [ranging_counter(2)] */
        if (len < 2) {
            break;
        }
        uint16_t counter = params[0] | (params[1] << 8);
        BT_LOGD("Complete ranging data for counter=%d", counter);

        cs_rap_gattc_ack_ranging_data(addr, counter);

        if (g_cs_rap && g_cs_rap->ranging_mode == CS_RANGING_MODE_ON_DEMAND) {
            g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_WAIT_ACK;
        }
        break;
    }

    case RAS_CP_RSP_COMPLETE_LOST_DATA_SEG: {
        /* Complete Lost Ranging Data Segment Response: [counter(2), first(1), last(1)] */
        if (len < 4) {
            break;
        }
        uint16_t counter = params[0] | (params[1] << 8);
        uint8_t first_seg = params[2];
        uint8_t last_seg = params[3];
        BT_LOGD("Complete lost segments: counter=%d, first=%d, last=%d",
            counter, first_seg, last_seg);

        cs_rap_gattc_ack_ranging_data(addr, counter);

        if (g_cs_rap && g_cs_rap->ranging_mode == CS_RANGING_MODE_ON_DEMAND) {
            g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_WAIT_ACK;
        }
        break;
    }

    case RAS_CP_RSP_RESPONSE_CODE: {
        /* Response Code: [response_code(1)] */
        if (len < 1) {
            break;
        }
        uint8_t response_code = params[0];
        BT_LOGD("Response code: %d", response_code);

        if (g_cs_rap && g_cs_rap->ranging_mode == CS_RANGING_MODE_ON_DEMAND &&
            g_cs_rap->on_demand_state == CS_RAP_ON_DEMAND_STATE_WAIT_ACK) {

            if (g_cs_rap->on_demand_timer) {
                service_loop_cancel_timer(g_cs_rap->on_demand_timer);
                g_cs_rap->on_demand_timer = NULL;
            }

            g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_COMPLETE;

            if (response_code == RAS_RSP_SUCCESS) {
                if (cs_rap_is_data_ready(addr)) {
                    cs_rap_calculate_distance(addr);
                }
            } else {
                BT_LOGW("Response code indicates failure: %d, clearing data", response_code);
                cs_rap_clear_data(addr);
            }

            g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_IDLE;
        }
        break;
    }

    default:
        BT_LOGW("Unknown control point response: 0x%02x", opcode);
        break;
    }
}

static const cs_rap_gattc_callbacks_t g_cs_rap_gattc_cbs = {
    .on_connected = cs_rap_on_gattc_connected,
    .on_disconnected = cs_rap_on_gattc_disconnected,
    .on_discover_complete = cs_rap_on_gattc_discover_complete,
    .on_features_read = cs_rap_on_gattc_features_read,
    .on_data_ready = cs_rap_on_gattc_data_ready,
    .on_ranging_data = cs_rap_on_gattc_ranging_data,
    .on_control_point_rsp = cs_rap_on_gattc_control_point_rsp,
    .on_data_overwritten = NULL,
    .on_write_complete = NULL,
};

int cs_rap_init(cs_rap_internal_distance_cb cb)
{
    if (g_cs_rap) {
        BT_LOGW("CS RAP already initialized");
        return 0;
    }

    g_cs_rap = (cs_rap_env_t*)malloc(sizeof(cs_rap_env_t));
    if (!g_cs_rap) {
        BT_LOGE("Failed to allocate CS RAP environment");
        return -1;
    }

    memset(g_cs_rap, 0, sizeof(cs_rap_env_t));
    g_cs_rap->distance_cb = cb;
    g_cs_rap->role = CS_ROLE_INITIATOR;
    g_cs_rap->on_demand_state = CS_RAP_ON_DEMAND_STATE_IDLE;
    g_cs_rap->on_demand_timer = NULL;

    size_t subevent_alloc_size = sizeof(cs_rap_subevent_data_t) + CS_RAP_STEP_DATA_BUF_LEN;
    g_cs_rap->local_data = (cs_rap_subevent_data_t*)malloc(subevent_alloc_size);
    g_cs_rap->remote_data = (cs_rap_subevent_data_t*)malloc(subevent_alloc_size);
    if (!g_cs_rap->local_data || !g_cs_rap->remote_data) {
        BT_LOGE("Failed to allocate subevent data buffers");
        free(g_cs_rap->local_data);
        free(g_cs_rap->remote_data);
        free(g_cs_rap);
        g_cs_rap = NULL;
        return -1;
    }
    memset(g_cs_rap->local_data, 0, subevent_alloc_size);
    memset(g_cs_rap->remote_data, 0, subevent_alloc_size);

    /* Initialize RAP GATTC module */
    if (cs_rap_gattc_init(&g_cs_rap_gattc_cbs) != 0) {
        BT_LOGE("Failed to initialize RAP GATTC");
        free(g_cs_rap->local_data);
        free(g_cs_rap->remote_data);
        free(g_cs_rap);
        g_cs_rap = NULL;
        return -1;
    }

    /* Register subevent callback with CS service to receive local data */
    bt_cs_register_subevent_cb(cs_rap_subevent_result_cb);

    BT_LOGD("CS RAP initialized");
    return 0;
}

void cs_rap_deinit(void)
{
    if (!g_cs_rap) {
        return;
    }

    /* Cancel on-demand timer if running */
    if (g_cs_rap->on_demand_timer) {
        service_loop_cancel_timer(g_cs_rap->on_demand_timer);
        g_cs_rap->on_demand_timer = NULL;
    }

    /* Unregister subevent callback */
    bt_cs_register_subevent_cb(NULL);

    /* Deinitialize RAP GATTC module */
    cs_rap_gattc_deinit();

    if (g_cs_rap->addr) {
        free(g_cs_rap->addr);
    }

    free(g_cs_rap->local_data);
    free(g_cs_rap->remote_data);
    free(g_cs_rap);
    g_cs_rap = NULL;

    BT_LOGD("CS RAP deinitialized");
}

void cs_rap_set_role(uint8_t role)
{
    if (g_cs_rap) {
        g_cs_rap->role = role;
        BT_LOGD("CS RAP role set to %s",
            role == CS_ROLE_INITIATOR ? "Initiator" : "Reflector");
    }
}

uint8_t cs_rap_get_role(void)
{
    return g_cs_rap ? g_cs_rap->role : CS_ROLE_INITIATOR;
}

bt_status_t cs_rap_set_ranging_mode(bt_address_t* addr, uint8_t mode)
{
    if (!g_cs_rap || !addr) {
        return BT_STATUS_PARM_INVALID;
    }

    /*
     * According to RAS specification:
     * - Real-time mode: Subscribe to Real-time Ranging Data characteristic
     * - On-demand mode: Subscribe to On-demand Ranging Data characteristic
     *
     * The server shall operate in either Real-time or On-demand mode,
     * but not both simultaneously.
     */
    if (mode == CS_RANGING_MODE_REAL_TIME) {
        /* Enable real-time data notifications */
        cs_rap_gattc_enable_real_time_data(addr, true);
        cs_rap_gattc_enable_on_demand_data(addr, false);
    } else if (mode == CS_RANGING_MODE_ON_DEMAND) {
        /* Enable on-demand data notifications and data ready */
        cs_rap_gattc_enable_on_demand_data(addr, true);
        cs_rap_gattc_enable_real_time_data(addr, false);
        cs_rap_gattc_enable_data_ready(addr, true);
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    g_cs_rap->ranging_mode = mode;
    BT_LOGD("Ranging mode set to %s",
        mode == CS_RANGING_MODE_REAL_TIME ? "Real-time" : "On-demand");

    return BT_STATUS_SUCCESS;
}

uint8_t cs_rap_get_ranging_mode(bt_address_t* addr)
{
    return g_cs_rap ? g_cs_rap->ranging_mode : 0;
}

bt_status_t cs_rap_process_remote_data(bt_address_t* addr, uint8_t* data, uint16_t len)
{
    if (!g_cs_rap || !addr || !data || len < 1) {
        return BT_STATUS_PARM_INVALID;
    }

    BT_LOGD("Processing remote RAP data: len=%d", len);

    /* Process segmented data */
    cs_rap_process_segment(addr, data, len);

    return BT_STATUS_SUCCESS;
}

/**
 * @brief Process a single segment of ranging data
 *
 * Segments are formatted according to RAS specification:
 * - Byte 0: Segment header (first/last flags + segment index)
 * - Bytes 1-N: Segment payload
 */
static void cs_rap_process_segment(bt_address_t* addr, uint8_t* data, uint16_t len)
{
    if (len < 2) {
        BT_LOGE("Segment too short: %d", len);
        return;
    }

    uint8_t seg_header = data[0];
    bool is_first = (seg_header & CS_RAP_SEG_FIRST_FLAG) != 0;
    bool is_last = (seg_header & CS_RAP_SEG_LAST_FLAG) != 0;
    uint8_t seg_idx = (seg_header & CS_RAP_SEG_INDEX_MASK) >> CS_RAP_SEG_INDEX_SHIFT;

    uint8_t* payload = &data[1];
    uint16_t payload_len = len - 1;

    BT_LOGD("Segment: first=%d, last=%d, idx=%d, payload_len=%d",
        is_first, is_last, seg_idx, payload_len);

    cs_rap_segment_ctx_t* ctx = &g_cs_rap->seg_ctx;

    if (is_first) {
        /* Start new reassembly */
        ctx->in_progress = true;
        ctx->expected_seg_idx = 0;
        ctx->total_len = 0;
        memset(ctx->data, 0, sizeof(ctx->data));
    }

    if (!ctx->in_progress) {
        BT_LOGW("Received segment without first flag, discarding");
        return;
    }

    /* Check segment index */
    if (seg_idx != ctx->expected_seg_idx) {
        BT_LOGW("Segment index mismatch: expected=%d, got=%d",
            ctx->expected_seg_idx, seg_idx);

        /* Request lost segments */
        if (g_cs_rap->ranging_mode == CS_RANGING_MODE_ON_DEMAND) {
            cs_rap_retrieve_lost_segments(addr, ctx->ranging_counter,
                ctx->expected_seg_idx, seg_idx - 1);
        }
        return;
    }

    /* Append payload to reassembly buffer */
    if (ctx->total_len + payload_len > CS_RAP_STEP_DATA_BUF_LEN) {
        BT_LOGE("Reassembly buffer overflow");
        ctx->in_progress = false;
        return;
    }

    memcpy(&ctx->data[ctx->total_len], payload, payload_len);
    ctx->total_len += payload_len;
    ctx->expected_seg_idx++;
    ctx->last_seg_idx = seg_idx;

    if (is_last) {
        /* Reassembly complete */
        BT_LOGD("Segment reassembly complete: total_len=%d", ctx->total_len);
        ctx->in_progress = false;
        cs_rap_on_reassembly_complete(addr);
    }
}

/**
 * @brief Called when segment reassembly is complete
 *
 * Parses the reassembled data and stores it as remote ranging data.
 */
static void cs_rap_on_reassembly_complete(bt_address_t* addr)
{
    cs_rap_segment_ctx_t* ctx = &g_cs_rap->seg_ctx;
    cs_rap_subevent_data_t* remote = g_cs_rap->remote_data;

    if (ctx->total_len < CS_RAP_SUB_PROCEDURE_HEAD) {
        BT_LOGE("Reassembled data too short: %d", ctx->total_len);
        return;
    }

    /* Parse header from reassembled data */
    cs_rap_parse_header(ctx->data, ctx->total_len, &remote->header);
    remote->ranging_counter = remote->header.ranging_counter;

    BT_LOGD("Remote data reassembled: counter=%d, steps=%d, antenna_paths=%d",
        remote->ranging_counter, remote->header.num_steps_reported,
        remote->header.num_antenna_paths);

    /* Dump header bytes first for debugging */
    BT_LOGD("Header bytes: "
        "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
        ctx->data[0], ctx->data[1], ctx->data[2], ctx->data[3],
        ctx->data[4], ctx->data[5], ctx->data[6], ctx->data[7],
        ctx->data[8], ctx->data[9], ctx->data[10], ctx->data[11]);

    /* Dump step data after header */
    uint16_t dbg_step_len = ctx->total_len - CS_RAP_SUB_PROCEDURE_HEAD;
    if (dbg_step_len > 0) {
        uint8_t* p = &ctx->data[CS_RAP_SUB_PROCEDURE_HEAD];
        uint16_t dump_len = dbg_step_len < 20 ? dbg_step_len : 20;
        char hex_buf[20 * 3 + 1] = { 0 };
        uint16_t pos = 0;

        for (uint16_t i = 0; i < dump_len && pos < sizeof(hex_buf) - 4; i++) {
            pos += snprintf(&hex_buf[pos], sizeof(hex_buf) - pos, "%02x ", p[i]);
        }
        BT_LOGD("Step data (%d bytes, first %d): %s", dbg_step_len, dump_len, hex_buf);
    }

    /* Store step data (skip header) */
    uint16_t step_data_len = ctx->total_len - CS_RAP_SUB_PROCEDURE_HEAD;
    if (step_data_len > CS_RAP_STEP_DATA_BUF_LEN) {
        BT_LOGE("Step data too large: %d", step_data_len);
        return;
    }

    /*
     * According to RAS specification, the received data from RAS server
     * has Step_len[i] removed. The format is:
     *   Step_Mode[i], Step_Data[i] (without Step_len[i])
     *
     * We need to restore it to the standard format:
     *   Step_Mode[i], Step_len[i], Step_Data[i]
     *
     * This is required for proper distance calculation.
     */
    uint8_t* raw_step_data = &ctx->data[CS_RAP_SUB_PROCEDURE_HEAD];
    uint16_t restored_len = cs_rap_restore_step_len(raw_step_data, step_data_len,
        remote->step_data, CS_RAP_STEP_DATA_BUF_LEN, remote->header.num_antenna_paths,
        remote->header.num_steps_reported);

    if (restored_len == 0) {
        BT_LOGE("Failed to restore step length fields");
        return;
    }

    remote->step_data_len = restored_len;
    remote->valid = true;

    BT_LOGD("Step data restored: original_len=%d, restored_len=%d, remote->valid=%d",
        step_data_len, restored_len, remote->valid);

    /* For on-demand mode, send ACK after reassembly complete */
    if (g_cs_rap->ranging_mode == CS_RANGING_MODE_ON_DEMAND) {
        /* ACK will be sent after receiving Complete Ranging Data Response */
        BT_LOGD("On-demand mode: waiting for Complete Ranging Data Response");
    } else {
        /* For real-time mode, check if we can calculate distance */
        if (cs_rap_is_data_ready(addr)) {
            cs_rap_calculate_distance(addr);
        }
    }
}

/**
 * @brief Restore Step_len field to step data
 *
 * According to RAS specification, the RAS server removes Step_len[i] from
 * the step data before transmission. This function restores the Step_len[i]
 * field based on the Step_Mode and antenna paths configuration.
 *
 * Input format:  Step_Mode[i], Step_Data[i] (repeated)
 * Output format: Step_Mode[i], Step_Channel[i], Step_len[i], Step_Data[i] (repeated)
 *
 * Note: The input from RAS also doesn't include Step_Channel, but we need to
 * handle the format properly. Based on the RAS spec, the format received is:
 *   Step_Mode[i], Step_Data[i]
 *
 * @param src           Source buffer (without Step_len)
 * @param src_len       Source buffer length
 * @param dst           Destination buffer (with Step_len restored)
 * @param dst_max_len   Maximum destination buffer length
 * @param num_antenna_paths Number of antenna paths
 * @return              Length of restored data, 0 on error
 */
/**
 * @brief Get the expected step data size for a given mode
 *
 * The RAS server's transform_step_data_to_ras_format_filtered removes
 * Step_Channel and Step_len, outputting: Step_Mode(1) + Step_Data.
 *
 * Mode 0 size is deterministic based on remote role.
 * Mode 1/2/3 sizes depend on HCI controller's actual output, which may
 * not include PCT fields. We use a pre-scan approach: first count Mode 0
 * steps (whose size is known), then derive Mode 1 size from the remaining
 * data length.
 *
 * @param step_mode         Step mode (0-3)
 * @param num_antenna_paths Number of antenna paths
 * @param mode1_data_size   Detected Mode 1 data size (0 = use default 14)
 * @return                  Expected step data size in bytes
 */
static uint16_t cs_rap_get_step_data_size(uint8_t step_mode, uint8_t num_antenna_paths,
    uint16_t mode1_data_size)
{
    uint8_t remote_role;
    if (g_cs_rap->role == CS_ROLE_INITIATOR) {
        remote_role = CS_ROLE_REFLECTOR;
    } else {
        remote_role = CS_ROLE_INITIATOR;
    }

    switch (step_mode) {
    case 0:
        return (remote_role == CS_ROLE_INITIATOR) ? 5 : 3;
    case 1:
        return mode1_data_size ? mode1_data_size : 14;
    case 2:
        return 1 + (num_antenna_paths + 1) * 4;
    case 3: {
        uint16_t m1_part = mode1_data_size ? mode1_data_size : 14;
        return m1_part + 1 + (num_antenna_paths + 1) * 4;
    }
    default:
        return 0;
    }
}

/**
 * @brief Pre-scan step data to detect actual Mode 1 data size
 *
 * The HCI controller may report Mode 1 step data with fewer fields than
 * the full 14 bytes (e.g., 6 bytes without PCT1/PCT2). Since Mode 0 size
 * is deterministic, we can count Mode 0 steps and their total bytes, then
 * derive Mode 1 size from the remaining data.
 *
 * Algorithm:
 *   1. Scan through data using known Mode 0 size
 *   2. Count Mode 0 steps and Mode 1 steps
 *   3. mode1_data_size = (src_len - mode0_count*(1+mode0_size) - mode1_count*1) / mode1_count
 *
 * @param src               Source buffer
 * @param src_len           Source buffer length
 * @param num_antenna_paths Number of antenna paths
 * @param num_steps         Expected number of steps (from header)
 * @return                  Detected Mode 1 data size, or 14 as default
 */
static uint16_t cs_rap_detect_mode1_size(uint8_t* src, uint16_t src_len,
    uint8_t num_antenna_paths, uint8_t num_steps)
{
    uint16_t mode0_size;
    if (g_cs_rap->role == CS_ROLE_INITIATOR) {
        mode0_size = 3; /* Remote is Reflector */
    } else {
        mode0_size = 5; /* Remote is Initiator */
    }

    /*
     * Quick scan: walk through data using known Mode 0 size.
     * When we hit a Mode 1 byte, we don't know its size yet,
     * so we count how many Mode 0 vs non-Mode-0 steps there are.
     */
    int mode0_count = 0;

    /* First pass: count Mode 0 steps by walking with known size */
    uint16_t temp_offset = 0;
    while (temp_offset < src_len) {
        uint8_t mode = src[temp_offset];
        if (mode == 0) {
            mode0_count++;
            temp_offset += 1 + mode0_size;
        } else if (mode <= 3) {
            /* Found a non-Mode-0 step, stop counting Mode 0 */
            break;
        } else {
            /* Invalid mode byte, stop */
            break;
        }
    }

    /* Now we know how many leading Mode 0 steps there are.
     * The remaining data contains non-Mode-0 steps mixed with more Mode 0 steps.
     * Use a different approach: calculate from total length and step count.
     *
     * For the typical CS pattern (Mode 0 + Mode 1 repeating):
     * total_bytes = mode0_total * (1 + mode0_size) + mode1_total * (1 + mode1_size)
     * where mode0_total + mode1_total = num_steps
     *
     * But we need to know mode0_total and mode1_total first.
     * Since Mode 0 bytes are 0x00 and Mode 1 bytes are 0x01, we can count
     * all step_mode bytes if we know the step sizes.
     *
     * Alternative: try candidate Mode 1 sizes and see which one parses all steps.
     */
    uint16_t candidates[] = {6, 14, 4, 8, 10, 12};
    int num_candidates = sizeof(candidates) / sizeof(candidates[0]);

    for (int c = 0; c < num_candidates; c++) {
        uint16_t try_m1_size = candidates[c];
        uint16_t try_offset = 0;
        int try_steps = 0;
        bool valid = true;

        while (try_offset < src_len && try_steps < num_steps) {
            uint8_t mode = src[try_offset];
            if (mode > 3) {
                valid = false;
                break;
            }

            uint16_t data_size;
            switch (mode) {
            case 0:
                data_size = mode0_size;
                break;
            case 1:
                data_size = try_m1_size;
                break;
            case 2:
                data_size = 1 + (num_antenna_paths + 1) * 4;
                break;
            case 3:
                data_size = try_m1_size + 1 + (num_antenna_paths + 1) * 4;
                break;
            default:
                valid = false;
                data_size = 0;
                break;
            }

            if (!valid) break;

            if (try_offset + 1 + data_size > src_len) {
                valid = false;
                break;
            }

            try_offset += 1 + data_size;
            try_steps++;
        }

        if (valid && try_steps == num_steps && try_offset == src_len) {
            BT_LOGD("Detected Mode 1 data size = %d (parsed %d steps, %d bytes)",
                try_m1_size, try_steps, try_offset);
            return try_m1_size;
        }
    }

    /* Fallback: try to find any size that works */
    for (uint16_t try_m1_size = 1; try_m1_size <= 20; try_m1_size++) {
        uint16_t try_offset = 0;
        int try_steps = 0;
        bool valid = true;

        while (try_offset < src_len && try_steps < num_steps) {
            uint8_t mode = src[try_offset];
            if (mode > 3) { valid = false; break; }

            uint16_t data_size;
            switch (mode) {
            case 0: data_size = mode0_size; break;
            case 1: data_size = try_m1_size; break;
            case 2: data_size = 1 + (num_antenna_paths + 1) * 4; break;
            case 3: data_size = try_m1_size + 1 + (num_antenna_paths + 1) * 4; break;
            default: valid = false; data_size = 0; break;
            }
            if (!valid) break;
            if (try_offset + 1 + data_size > src_len) { valid = false; break; }

            try_offset += 1 + data_size;
            try_steps++;
        }

        if (valid && try_steps == num_steps && try_offset == src_len) {
            BT_LOGD("Detected Mode 1 data size = %d (fallback, parsed %d steps, %d bytes)",
                try_m1_size, try_steps, try_offset);
            return try_m1_size;
        }
    }

    BT_LOGD("Could not detect Mode 1 size, using default 14");
    return 14;
}

static uint16_t cs_rap_restore_step_len(uint8_t* src, uint16_t src_len,
    uint8_t* dst, uint16_t dst_max_len, uint8_t num_antenna_paths,
    uint8_t num_steps)
{
    uint16_t src_offset = 0;
    uint16_t dst_offset = 0;
    int step_count = 0;

    BT_LOGD("cs_rap_restore_step_len: src_len=%d, num_antenna_paths=%d, local_role=%d, num_steps=%d",
        src_len, num_antenna_paths, g_cs_rap->role, num_steps);

    /* Detect actual Mode 1 data size by trial parsing */
    uint16_t mode1_size = cs_rap_detect_mode1_size(src, src_len, num_antenna_paths, num_steps);

    while (src_offset < src_len) {
        uint8_t step_mode = src[src_offset];

        /* Validate step_mode */
        if (step_mode > 3) {
            BT_LOGE("Invalid step mode %d at offset %d (step #%d), stopping",
                step_mode, src_offset, step_count);
            break;
        }

        /* Get step data size using detected Mode 1 size */
        uint16_t step_data_size = cs_rap_get_step_data_size(step_mode, num_antenna_paths, mode1_size);

        if (step_data_size == 0) {
            BT_LOGE("Unknown step mode %d, cannot determine size", step_mode);
            break;
        }

        /* Check if we have enough source data */
        if (src_offset + 1 + step_data_size > src_len) {
            BT_LOGD("Incomplete step data: mode=%d, expected=%d, available=%d",
                step_mode, step_data_size, src_len - src_offset - 1);
            break;
        }

        /* Check if we have enough destination space */
        /* Output format: Step_Mode(1) + Step_Channel(1) + Step_len(1) + Step_Data */
        if (dst_offset + 3 + step_data_size > dst_max_len) {
            BT_LOGE("Destination buffer overflow");
            return 0;
        }

        /* Write Step_Mode */
        dst[dst_offset++] = step_mode;

        /* Write Step_Channel (placeholder, set to 0 as it's not in the received data) */
        dst[dst_offset++] = 0;

        /* Write Step_len */
        dst[dst_offset++] = (uint8_t)step_data_size;

        /* Copy Step_Data */
        memcpy(&dst[dst_offset], &src[src_offset + 1], step_data_size);
        dst_offset += step_data_size;

        /* Move to next step */
        src_offset += 1 + step_data_size;
        step_count++;
    }

    BT_LOGD("cs_rap_restore_step_len: processed %d/%d steps, output_len=%d",
        step_count, num_steps, dst_offset);
    return dst_offset;
}

bt_status_t cs_rap_retrieve_lost_segments(bt_address_t* addr, uint16_t ranging_counter,
    uint8_t first_seg_idx, uint8_t last_seg_idx)
{
    if (!addr) {
        return BT_STATUS_PARM_INVALID;
    }

    BT_LOGD("Retrieving lost segments: counter=%d, first=%d, last=%d",
        ranging_counter, first_seg_idx, last_seg_idx);

    /* Send Retrieve_Lost_Ranging_Data_Segments command via GATTC */
    return cs_rap_gattc_retrieve_lost_segments(addr, ranging_counter, first_seg_idx, last_seg_idx);
}

void cs_rap_clear_data(bt_address_t* addr)
{
    if (!g_cs_rap) {
        return;
    }

    memset(g_cs_rap->local_data, 0, sizeof(cs_rap_subevent_data_t) + CS_RAP_STEP_DATA_BUF_LEN);
    memset(g_cs_rap->remote_data, 0, sizeof(cs_rap_subevent_data_t) + CS_RAP_STEP_DATA_BUF_LEN);

    BT_LOGD("CS RAP data cleared");
}

bool cs_rap_is_data_ready(bt_address_t* addr)
{
    if (!g_cs_rap) {
        return false;
    }

    cs_rap_subevent_data_t* local = g_cs_rap->local_data;
    cs_rap_subevent_data_t* remote = g_cs_rap->remote_data;

    BT_LOGD("is_data_ready: local valid=%d counter=%d, remote valid=%d counter=%d",
        local->valid, local->ranging_counter, remote->valid, remote->ranging_counter);

    /* Check if both local and remote data are valid */
    if (!local->valid || !remote->valid) {
        BT_LOGD("is_data_ready: NOT ready (local_valid=%d, remote_valid=%d)",
            local->valid, remote->valid);
        return false;
    }

    /* Check if ranging counters match */
    if (local->ranging_counter != remote->ranging_counter) {
        BT_LOGD("Ranging counter mismatch: local=%d, remote=%d",
            local->ranging_counter, remote->ranging_counter);
        return false;
    }

    BT_LOGD("is_data_ready: READY, counter=%d", local->ranging_counter);
    return true;
}

bt_status_t cs_rap_calculate_distance(bt_address_t* addr)
{
    if (!g_cs_rap || !addr) {
        return BT_STATUS_PARM_INVALID;
    }

    cs_rap_subevent_data_t* local = g_cs_rap->local_data;
    cs_rap_subevent_data_t* remote = g_cs_rap->remote_data;

    if (!local->valid || !remote->valid) {
        BT_LOGE("Data not ready for distance calculation");
        return BT_STATUS_NOT_READY;
    }

    BT_LOGD("Calculating distance: local_len=%d, remote_len=%d, counter=%d",
        local->step_data_len, remote->step_data_len, local->ranging_counter);

    /* Use cs_distance module to calculate distance */
    cs_distance_result_t dist_result;
    int ret = cs_distance_calculate(
        local->step_data, local->step_data_len,
        remote->step_data, remote->step_data_len,
        local->header.num_antenna_paths,
        g_cs_rap->role,
        &dist_result);

    if (ret != 0) {
        BT_LOGE("Distance calculation failed: %d", ret);
        return BT_STATUS_FAIL;
    }

    /* Log results */
    if (dist_result.rtt_valid) {
        BT_LOGI("RTT Distance: %.2f meters (%d samples)",
            dist_result.rtt_distance, dist_result.mode1_samples);
    }
    if (dist_result.phase_valid) {
        if (dist_result.phase_distance > CS_RAP_PHASE_DISTANCE_OFFSET) {
            dist_result.phase_distance -= CS_RAP_PHASE_DISTANCE_OFFSET;
        }
        BT_LOGI("Phase Distance: %.2f meters (%d samples)",
            dist_result.phase_distance, dist_result.mode2_samples);
    }

    if (!dist_result.rtt_valid && !dist_result.phase_valid) {
        BT_LOGW("Could not compute reliable distance estimate");
    }

    /* Notify application via callback */
    if (g_cs_rap->distance_cb) {
        cs_rap_internal_distance_result_t result;
        memcpy(&result.addr, addr, sizeof(bt_address_t));
        result.ranging_counter = local->ranging_counter;
        result.rtt_distance = dist_result.rtt_distance;
        result.phase_distance = dist_result.phase_distance;
        result.mode1_samples = dist_result.mode1_samples;
        result.mode2_samples = dist_result.mode2_samples;
        result.rtt_valid = dist_result.rtt_valid;
        result.phase_valid = dist_result.phase_valid;

        g_cs_rap->distance_cb(addr, &result);
    }

    /* Clear processed data */
    cs_rap_clear_data(addr);

    return BT_STATUS_SUCCESS;
}
