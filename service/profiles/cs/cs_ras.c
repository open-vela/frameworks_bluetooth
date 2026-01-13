/****************************************************************************
 *
 *   Copyright (C) 2025 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include "cs_ras.h"
#include "bt_status.h"
#include "cs_ras_gatts.h"
#include "cs_ras_test.h"
#include "cs_ras_util.h"
#include "utils/log.h"
#include <stdatomic.h>

#ifdef CONFIG_BLUETOOTH_LE_CS

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#define CS_CONFIG_ID 0
#define NUM_MODE_0_STEPS 1
#define RAS_SEG_HEADER_SIZE 4

static ras_srv_env_t* ras_srv;

static void split_real_time_segment(bt_address_t* addr, uint8_t* buf, int len);
static ssize_t ras_feature_read(bt_address_t* addr, void* buf, uint16_t len, uint16_t offset);
static ras_rang_on_demand_t* ras_rang_on_demand_find_subevent(bt_address_t* addr, uint16_t count);
static bt_status_t ras_data_ready_send(bt_address_t* addr, uint16_t count);
static bt_status_t ras_on_demond_send_cmp_ranging_data_rsp(bt_address_t* addr, uint16_t count);

static void ras_on_demand_notify_finished(bt_address_t* addr)
{
    cs_node_t* on_deman_pdu = (cs_node_t*)ras_srv->on_deman_curr_node;

    if (!on_deman_pdu) {
        BT_LOGD("Compelete segment data sent.");
        ras_on_demond_send_cmp_ranging_data_rsp(addr, 10);
        return;
    }

    ras_segment_t* seg = CS_CONTAINER_OF(on_deman_pdu, ras_segment_t, seg_node);

    if (!seg) {
        BT_LOGW("Invalid segment.");
        return;
    }

    BT_LOGD("seg:%p, seg->data(%d):%s", seg, seg->len, cs_log_to_hex_str(seg->data, seg->len));

    ras_srv->on_deman_curr_node = on_deman_pdu->next;

    bt_status_t status = BT_GATT_NOTIFY_CB(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);
    if (status != 0) {
        BT_LOGE("On-demand ranging data notify fail, seg_idx(%d), err(%d).",
            seg->seg_idx, status);
        return;
    }

    return;
}

static void ras_on_demand_indicate_finished(bt_address_t* addr)
{
    cs_node_t* on_deman_pdu = (cs_node_t*)ras_srv->on_deman_curr_node;

    if (!on_deman_pdu) {
        BT_LOGD("Compelete segment data sent.");
        ras_on_demond_send_cmp_ranging_data_rsp(addr, 10);
        return;
    }

    ras_segment_t* seg = CS_CONTAINER_OF(on_deman_pdu, ras_segment_t, seg_node);

    if (!seg) {
        BT_LOGW("Invalid segment.");
        return;
    }

    BT_LOGD("seg:%p, seg->data(%d):%s", seg, seg->len, cs_log_to_hex_str(seg->data, seg->len));

    ras_srv->on_deman_curr_node = on_deman_pdu->next;

    bt_status_t status = BT_GATT_INDICATE(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);
    if (status != 0) {
        BT_LOGE("On-demand ranging data indication fail, seg_idx(%d), err(%d).",
            seg->seg_idx, status);
        return;
    }

    return;
}

static bt_status_t ras_ondemand_send_ranging_data(bt_address_t* addr, uint16_t count)
{
    if (!ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) && !ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        BT_LOGE("RAS haven't enable the on-demond ranging data notify or indication state.");
        return BT_STATUS_FAIL;
    }

    bt_status_t status = BT_STATUS_SUCCESS;

    ras_rang_on_demand_t* on_demand_sub = ras_rang_on_demand_find_subevent(addr, count);

    if (!on_demand_sub) {
        BT_LOGE("Haven't find subevent with count(%d).", count);
        return BT_STATUS_PARM_INVALID;
    }

    const cs_node_t* on_deman_pdu = cs_list_peek_head(&on_demand_sub->seg_list);
    struct ras_segment_t* seg = CS_CONTAINER_OF(on_deman_pdu, ras_segment_t, seg_node);

    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY)) {
        BT_LOGD("seg:%p, seg->data(%d):%s", seg, seg->len, cs_log_to_hex_str(seg->data, seg->len));

        status = BT_GATT_NOTIFY_CB(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);

        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("On-demand ranging data notify fail, status(%d).", status);
            return status;
        }

    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        BT_LOGD("seg:%p, seg->data(%d):%s", seg, seg->len, cs_log_to_hex_str(seg->data, seg->len));
        // Set the current node to the next.
        ras_srv->on_deman_curr_node = on_deman_pdu->next;
        status = BT_GATT_INDICATE(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);

        if (status != 0) {
            BT_LOGE("On-demand ranging data indicate fail, err(%d).", status);
            return status;
        }
    }

    return status;
}

static bt_status_t ras_on_demond_send_cmp_ranging_data_rsp(bt_address_t* addr, uint16_t count)
{
    ras_rang_on_demand_t* on_demand_data = ras_rang_on_demand_find_subevent(addr, count);

    if (!on_demand_data) {
        BT_LOGW("Haven't find the demand data with the count(%d).", count);
    }

    uint8_t buf[3] = { 0 };
    bt_status_t status;
    buf[0] = SAL_LE_RAS_CTL_OP_RSP_CMP_RANG_DATA;
    ras_put_uint16_to_ptr(count, &buf[1]);

    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        status = BT_GATT_NOTIFY_CB(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK notify fail, err(%d).", status);
            return status;
        }
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        status = BT_GATT_INDICATE(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK indicate fail, err(%d).", status);
            return status;
        }
    } else {
        BT_LOGE("The Ranging Control Point haven't set to notify or indication state.");
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t ras_on_demond_send_code_rsp(bt_address_t* addr, uint16_t count)
{
    ras_rang_on_demand_t* on_demand_data = ras_rang_on_demand_find_subevent(addr, count);

    if (!on_demand_data) {
        BT_LOGW("Haven't find the demand data with the count(%d).", count);
    }

    uint8_t buf[2] = { 0 };
    bt_status_t status;
    buf[0] = SAL_LE_RAS_CTL_OP_RSP_CODE;
    buf[1] = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS;

    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        status = BT_GATT_NOTIFY_CB(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK notify fail, err(%d).", status);
            return status;
        }
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        status = BT_GATT_INDICATE(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK indicate fail, err(%d).", status);
            return status;
        }
    }

    // Free the on-demand segment list when response the ack to the Client.
    ras_segment_t *seg_prev, *seg_next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&on_demand_data->seg_list,
        seg_prev, seg_next, seg_node)
    {
        cs_list_remove(&on_demand_data->seg_list, NULL, &seg_prev->seg_node);
        BT_LOGD("seg_prev:%p", seg_prev);
        free(seg_prev);
    }

    BT_LOGD("The on-demand data has been sent, Cancel on-demand timer.");
    /* The on-demand data has been sent, Cancel on-demand timer */
    service_loop_cancel_timer(on_demand_data->on_demand_timer);
    memset(on_demand_data, 0, sizeof(ras_rang_on_demand_t));
    return BT_STATUS_SUCCESS;
}

static bt_status_t ras_on_demond_send_lost_ranging_data_cmp_rsp(bt_address_t* addr, uint16_t count, uint8_t first, uint8_t last)
{
    uint8_t buf[5] = { 0 };

    buf[0] = 0x01;
    ras_put_uint16_to_ptr(count, &buf[1]);
    buf[3] = first;
    buf[4] = last;

    bt_status_t status;
    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        buf[0] = SAL_LE_RAS_CTL_OP_RSP_CODE;
        buf[1] = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS;

        status = BT_GATT_NOTIFY_CB(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK notify fail, err(%d).", status);
            return status;
        }
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE)) {
        buf[0] = SAL_LE_RAS_CTL_OP_RSP_CODE;
        buf[1] = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS;

        status = BT_GATT_INDICATE(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK indicate fail, err(%d).", status);
            return status;
        }
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t ras_on_demand_retrieve_send_lost_data(bt_address_t* addr, uint16_t count,
    uint8_t first_seg, uint8_t last_seg)
{
    ras_rang_on_demand_t* on_demand_data = ras_rang_on_demand_find_subevent(addr, count);

    if (!ras_state_get_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_RANGING_DATA_RSP)) {
        BT_LOGW("Invalid on_demand state.");
        return -1;
    }

    bt_status_t status;
    // Free the on-demand segment list when response the ack to the Client.
    ras_segment_t *seg_prev, *seg_next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&on_demand_data->seg_list,
        seg_prev, seg_next, seg_node)
    {
        if (seg_prev && (seg_prev->seg_idx >= first_seg) && (seg_prev->seg_idx <= last_seg)) {
            if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
                status = BT_GATT_NOTIFY_CB(RAS_ON_DEMAND_CHAR_SEND, addr, seg_prev->data, seg_prev->len);
                if (status != 0) {
                    BT_LOGE("On-demand ranging data notify fail, err(%d).", status);
                    return status;
                }
            } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE)) {
                status = BT_GATT_INDICATE(RAS_ON_DEMAND_CHAR_SEND, addr, seg_prev->data, seg_prev->len);
                if (status != 0) {
                    BT_LOGE("On-demand ranging data indicate fail, err(%d).", status);
                    return status;
                }
            }
        } else if (seg_prev && (seg_prev->seg_idx > last_seg)) {
            break;
        }
    }

    ras_on_demond_send_lost_ranging_data_cmp_rsp(addr, count, first_seg, last_seg);
    return BT_STATUS_SUCCESS;
}

static uint8_t ras_check_ranging_mode(bt_address_t* addr)
{
    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY) || ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE)) {
        BT_LOGD("The real-time mode has been set.");
        return SAL_LE_RAS_RANGING_MODE_REAL_TIME;
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) || ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        BT_LOGD("The on-demand mode has been set.");
        return SAL_LE_RAS_RANGING_MODE_ON_DEMAND;
    }

    BT_LOGE("No mode have been set.");
    return SAL_LE_RAS_RANGING_MODE_UNDEFINED;
}

bool ras_is_filter_bit_set(uint16_t mode, uint16_t filter_bit)
{
    // Check if the mode is valid (within the range of SAL_LE_RAS_MODE_3_FILTER_MAX)
    if (mode >= SAL_LE_RAS_FILTER_MODE_MAX) {
        BT_LOGE("Error: Mode %d is out of valid range (0 to %d)",
            mode, SAL_LE_RAS_FILTER_MODE_MAX - 1);
        return false;
    }

    // Retrieve the current filter mask from the atomic array
    uint32_t current_filter_mask = __atomic_load_n(&ras_srv->ras_filter[mode], __ATOMIC_SEQ_CST);

    // Check if the specific filter bit is set
    // We shift the filter_bit into the correct position and mask it to check
    return (current_filter_mask & (1 << filter_bit)) != 0;
}

static void ras_set_filter(uint16_t filter_value)
{
    // Extract the mode (bits 0-1)
    uint16_t mode = filter_value & SAL_LE_RAS_FILTER_MODE_MASK;

    // Extract the filter mask (bits 2-15)
    uint16_t filter_mask = filter_value & ~SAL_LE_RAS_FILTER_MODE_MASK;

    // Use the mode directly as an index into ras_filter
    if (mode < SAL_LE_RAS_FILTER_MODE_MAX) {
        // Atomic operations to modify the filter mask
        // Clear the filter bits (2-15)
        __atomic_fetch_add(&ras_srv->ras_filter[mode], SAL_LE_RAS_FILTER_BIT_MASK, __ATOMIC_SEQ_CST);
        // Set the new filter mask (bits 2-15)
        __atomic_fetch_or(&ras_srv->ras_filter[mode], filter_mask, __ATOMIC_SEQ_CST);
    } else {
        // Handle error case if mode is out of range
        BT_LOGE("Error: Mode %d is out of valid range (0 to %d)\n", mode, SAL_LE_RAS_FILTER_MODE_MAX - 1);
    }

    return;
}

void ras_handle_abort_operation(ras_control_point_t* control_point)
{
    if (control_point->is_processing) {
        BT_LOGD("Stopping current RAS Control Point processes...");
        control_point->is_processing = 0; // Stop current processing.

        if (control_point->is_data_pending) {
            BT_LOGD("Flushing pending Ranging Data segments...");
            control_point->is_data_pending = 0; // Discard pending data
        }

        control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS; // response success.
        BT_LOGD("Abort Operation completed successfully.");
    } else {
        control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED;
        BT_LOGD("Abort Operation was unsuccessful (no operation to abort).");
    }
}

void ras_handle_other_operation(ras_control_point_t* control_point)
{
    BT_LOGD("Handling other operations...");
    control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS; // response success.
}

void ras_write_opcode(ras_control_point_t* control_point, ras_opcode_t op_code)
{
    switch (op_code) {
    case ABORT_OPERATION:
        BT_LOGD("Received ABORT_OPERATION OpCode.");
        ras_handle_abort_operation(control_point);
        break;

    case OTHER_OPERATION:
        BT_LOGD("Received OTHER_OPERATION OpCode.");
        ras_handle_other_operation(control_point);
        break;

    default:
        control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED;
        BT_LOGW("OpCode not supported.");
        break;
    }
}

void rs_display_response(ras_control_point_t* control_point)
{
    switch (control_point->response_code) {
    case SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS:
        BT_LOGD("Response: SUCCESS.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_PROCE_NOT_CMP:
        BT_LOGD("Response: PROCEDURE NOT COMPLETED.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_PERSISTED:
        BT_LOGD("Response: ABORT UNSUCCESSFUL.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_SERVER_BUSY:
        BT_LOGD("Response: SERVER BUSY.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_INVALID_PARAMS:
        BT_LOGD("Response: INVALID PARAMETER.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED:
        BT_LOGD("Response: OP CODE NOT SUPPORTED.");
        break;
    default:
        BT_LOGD("Unknown response code.");
        break;
    }
}

ssize_t on_ras_ctr_pt_write_cb(bt_address_t* addr, const void* buf, uint16_t len)
{
    if (!buf || len == 0) {
        BT_LOGE("Invalid buf or len.");
        return 0;
    }

    uint8_t* buf_send = (uint8_t*)buf;

    uint8_t opcode = (uint8_t)buf_send[0];
    BT_LOGD("RAS Control Point cb, opcode(%d)", opcode);

    uint16_t count;

    switch (opcode) {
    case SAL_LE_RAS_CTL_OP_CMD_GET_RANG_DATA: {
        count = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        ras_ondemand_send_ranging_data(addr, count);
        break;
    }
    case SAL_LE_RAS_CTL_OP_CMD_ACK_RANG_DATA: {
        count = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        ras_on_demond_send_code_rsp(addr, count);
        break;
    }
    case SAL_LE_RAS_CTL_OP_CMD_RETRIEVE_LOST_RANG_DATA_SEG: {
        count = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        uint8_t first_seg = (uint8_t)buf_send[3];
        uint8_t last_seg = (uint8_t)buf_send[4];
        ras_on_demand_retrieve_send_lost_data(addr, count, first_seg, last_seg);
    }
    case SAL_LE_RAS_CTL_OP_CMD_ABORT_OPERATION: {
    }
    case SAL_LE_RAS_CTL_OP_CMD_SET_FILTER: {
        uint16_t filter_params = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        ras_set_filter(filter_params);
    }
    default:
        break;
    }
    return len;
}

static ssize_t ras_feature_read(bt_address_t* addr, void* buf, uint16_t len, uint16_t offset)
{
    BT_LOGD("RAS feature read cb, ras_feature 0x%lx.\n", ras_srv->ras_feature);
    BT_LOGD("offset:%d, buf[%d]:%s", offset, len, cs_log_to_hex_str(buf, len));
    return 0;
}

static void range_rtt_dt_ccc_cfg_changed(bt_address_t* addr, uint16_t value)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv value.");
        return;
    }

    // The Real time mode and the on-demand mode can't be set together.
    if ((value != 0) && (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) || ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE))) {
        BT_LOGE("The on-demond mode has been setted, Please clear it before set to real-time mode.");
        return;
    }

    if (value == 0) {
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY);
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE);
    } else {
        ras_state_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_RTT_DATA_NOTIFY : RAS_RTT_DATA_INDICATE);
    }

    BT_LOGD("The range real-time data ccc value is change to (%d)\n", value);
    return;
}

static void range_on_dem_dt_ccc_cfg_changed(bt_address_t* addr, uint16_t value)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv value.");
        return;
    }

    // The Real time mode and the on-demand mode can't be set together.
    if ((value != 0) && (ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY) || ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE))) {
        BT_LOGE("The on-demond mode has been setted, please clear it before set to real-time mode.");
        return;
    }

    if (value == 0) {
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY);
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
    } else {
        ras_state_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_ON_DEMAND_DATA_NOTIFY : RAS_ON_DEMAND_DATA_INDICATE);
    }

    BT_LOGD("The range on-dem data ccc value is change to (%d)\n", value);
    return;
}

static void range_ctr_pt_ccc_cfg_changed(bt_address_t* addr, uint16_t value)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv value.");
        return;
    }

    if (value == 0) {
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE);
    } else {
        ras_state_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_CONTROL_POINT_NOTIFY : RAS_CONTROL_POINT_INDICATE);
    }

    BT_LOGD("The range control point data ccc value is change to (%d)", value);
    return;
}

static void range_dt_rd_ccc_cfg_changed(bt_address_t* addr, uint16_t value)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv value.");
        return;
    }

    if (value == 0) {
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_DATA_READY_INDICATE);
    } else {
        ras_state_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_DATA_READY_NOTIFY : RAS_DATA_READY_INDICATE);
    }

    BT_LOGD("The range data ready data ccc value is change to (%d)", value);
    return;
}

static void range_dt_ov_wr_ccc_cfg_changed(bt_address_t* addr, uint16_t value)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv value.");
        return;
    }

    if (value == 0) {
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_OVER_WRITE_NOTIFY);
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_OVER_WRITE_INDICATE);
    } else {
        ras_state_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_OVER_WRITE_NOTIFY : RAS_OVER_WRITE_INDICATE);
    }

    BT_LOGD("The range over write data ccc value is change to (%d)\n", value);
    return;
}

static void ras_dt_rd_indicate_cb(bt_address_t* addr)
{
    BT_LOGD("Indication finish");
    if (ras_srv->remaining_len) {
        split_real_time_segment(addr, &ras_srv->latest_local_steps[ras_srv->ras_seg_offset], ras_srv->remaining_len);
    }

    return;
}

// static void ras_dt_rd_indicate_destroy(struct bt_gatt_indicate_params* params)
// {
//     BT_LOGD("Indication complete\n");
//     ras_srv->ras_dt_rd_indicating = 0U;

//     return;
// }

static void ras_write_bits(uint8_t* buf, int* bit_offset, uint32_t value, int bit_count)
{
    int byte_index;
    int bit_index;
    for (int i = bit_count - 1; i >= 0; i--) {
        byte_index = *bit_offset / 8;
        bit_index = 7 - (*bit_offset % 8);
        uint8_t bit = (value >> i) & 0x01;

        if (bit) {
            buf[byte_index] |= (1 << bit_index);
        } else {
            buf[byte_index] &= ~(1 << bit_index);
        }

        (*bit_offset)++;
    }

    return;
}

static void split_real_time_segment(bt_address_t* addr, uint8_t* buf, int len)
{
    if (ras_srv->remaining_len > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
        int curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
        BT_LOGD("data send: offset:%lu, len:%d\n", ras_srv->ras_seg_offset, curr_seg_size);

        // update offset
        ras_srv->ras_seg_offset += curr_seg_size;
        uint8_t* send_buf = malloc(curr_seg_size + 1);

        ras_srv->remaining_len -= curr_seg_size;
        send_buf[0] = (ras_srv->ras_seg_idx == 0) ? (0x01) : (ras_srv->ras_seg_idx << 2);
        if (ras_srv->remaining_len == 0) {
            ras_srv->ras_seg_idx = 0;
            send_buf[0] |= (0x01 << 1);
            ras_srv->remaining_len = 0;
            ras_srv->ras_seg_offset = 0;
        }

        ras_srv->ras_seg_idx++;
        memcpy(&send_buf[1], buf, curr_seg_size);

        if ((send_buf[0] & 0x01) == 0x01) {
            BT_LOGD("First seg, data(%d):%s\n", curr_seg_size + 1, cs_log_to_hex_str(send_buf, curr_seg_size + 1));
        } else {
            BT_LOGD("The %d seg, data(%d):%s\n", send_buf[0] >> 2, curr_seg_size + 1, cs_log_to_hex_str(send_buf, curr_seg_size + 1));
        }

        if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_NOTIFY) {
            if (BT_GATT_NOTIFY(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                if (ras_srv->remaining_len) {
                    split_real_time_segment(addr, &ras_srv->latest_local_steps[ras_srv->ras_seg_offset], ras_srv->remaining_len);
                }
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Notify fail.\n");
                free(send_buf);
                return;
            }
        } else if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_INDICATION) {
            if (BT_GATT_INDICATE(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Indicate fail.\n");
                free(send_buf);
                return;
            }
        }
    } else {
        int curr_seg_size = len;
        uint8_t* send_buf = malloc(curr_seg_size + 1);
        send_buf[0] = (ras_srv->ras_seg_idx == 0) ? (0x01) : (ras_srv->ras_seg_idx << 2);
        send_buf[0] |= (0x01 << 1);
        memcpy(&send_buf[1], buf, curr_seg_size);
        ras_srv->ras_dt_rd_indicating = 0U;

        ras_srv->ras_seg_idx = 0;
        ras_srv->ras_dt_rd_indicating = 0U;
        ras_srv->remaining_len = 0;
        ras_srv->ras_seg_offset = 0;
        BT_LOGD("The last(%d) seg, data(%d):%s\n", send_buf[0] >> 2, curr_seg_size + 1, cs_log_to_hex_str(send_buf, curr_seg_size + 1));
        BT_LOGD("ras_dt_rd_indicating:%d\n", ras_srv->ras_dt_rd_indicating);
        if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_NOTIFY) {
            if (BT_GATT_NOTIFY(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Indicate fail.\n");
                free(send_buf);
                return;
            }
        } else if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_INDICATION) {
            if (BT_GATT_INDICATE(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Indicate fail.\n");
                free(send_buf);
                return;
            }
        }
    }

    return;
}

static void ras_on_demand_data_send_timeout(service_timer_t* timer, void* data)
{
    ras_rang_on_demand_t* on_demand_subevent = (ras_rang_on_demand_t*)timer->userdata;
    BT_LOGD("On-demand data send timeout, remove the data in the list.");
    // Free the on-demand segment list when response the ack to the Client.
    ras_segment_t *seg_prev, *seg_next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&on_demand_subevent->seg_list,
        seg_prev, seg_next, seg_node)
    {
        cs_list_remove(&on_demand_subevent->seg_list, NULL, &seg_prev->seg_node);
        BT_LOGD("seg_prev:%p", seg_prev);
        free(seg_prev);
    }

    memset(on_demand_subevent, 0, sizeof(ras_rang_on_demand_t));

    return;
}

static void split_on_demand_segment(bt_address_t* addr, uint8_t* buf, int len,
    ras_rang_on_demand_t* subevent)
{
    int curr_seg_size = 0;
    uint16_t seg_index = 0;
    ras_srv->ras_seg_offset = 0;

    do {
        if (ras_srv->remaining_len > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
            curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
            BT_LOGD("data send: offset:%lu, len:%d\n", ras_srv->ras_seg_offset, curr_seg_size);
            subevent->seg = (ras_segment_t*)malloc(sizeof(ras_segment_t) + curr_seg_size + 1);

            if (!subevent->seg) {
                BT_LOGE("Malloc fail.");
                return;
            }

            memcpy(&subevent->seg->data[1], &buf[ras_srv->ras_seg_offset], curr_seg_size);
            // update offset
            ras_srv->ras_seg_offset += curr_seg_size;
            ras_srv->remaining_len -= curr_seg_size;

            subevent->seg->data[0] = (seg_index == 0) ? (0x01) : (seg_index << 2);
            if (ras_srv->remaining_len == 0) {
                subevent->seg->data[0] |= (0x01 << 1);
                ras_srv->remaining_len = 0;
                ras_srv->ras_seg_offset = 0;
            }

        } else {
            curr_seg_size = ras_srv->remaining_len;
            subevent->seg = (ras_segment_t*)malloc(sizeof(ras_segment_t) + curr_seg_size + 1);

            if (!subevent->seg) {
                BT_LOGE("Malloc fail.");
                return;
            }

            memcpy(&subevent->seg->data[1], &buf[ras_srv->ras_seg_offset], curr_seg_size);

            subevent->seg->data[0] = (seg_index == 0) ? (0x01) : (seg_index << 2);
            subevent->seg->data[0] |= (0x01 << 1);
            ras_srv->remaining_len = 0;
            BT_LOGD("ras_dt_rd_indicating:%d\n", ras_srv->ras_dt_rd_indicating);
        }

        if ((subevent->seg->data[0] & 0x01) == 0x01) {
            BT_LOGD("First seg, data(%d):%s\n", curr_seg_size + 1, cs_log_to_hex_str(subevent->seg->data, curr_seg_size + 1));
        } else {
            BT_LOGD("The %d seg, data(%d):%s\n", subevent->seg->data[0] >> 2, curr_seg_size + 1,
                cs_log_to_hex_str(subevent->seg->data, curr_seg_size + 1));
        }

        subevent->seg->seg_idx = seg_index++;
        subevent->seg->len = curr_seg_size + 1;
        cs_list_append(&subevent->seg_list, &subevent->seg->seg_node);
        BT_LOGD("subevent seg:%p, seg_node:%p.", subevent->seg, &subevent->seg->seg_node);
    } while (ras_srv->remaining_len > 0);

    subevent->on_demand_timer = service_loop_timer(RAS_RSP_TIMEOUT, false, ras_on_demand_data_send_timeout, subevent);
    return;
}

/**
 * Input: data points to the original buffer, data_len is the total length of the input
 * Output: buf stores the transformed result, function returns the length of the output buffer
 */
static size_t transform_step_data_to_ras_format_filtered(
    uint8_t* data, size_t data_len,
    uint8_t* buf, uint32_t* ras_filter, uint8_t role,
    uint8_t num_antenna_paths)
{
    size_t in_offset = 0;
    size_t out_offset = 0;

    while (in_offset + 3 <= data_len) {
        uint8_t step_mode = data[in_offset];
        uint8_t step_data_length = data[in_offset + 2];

        if (in_offset + 3 + step_data_length > data_len) {
            BT_LOGW("Incomplete data, exit early.");
            break;
        }

        uint8_t* step_data = &data[in_offset + 3];
        uint32_t filter_mask = ras_filter[step_mode];

        // Write Step_Mode to the output
        buf[out_offset++] = step_mode;

        const uint8_t* p = step_data;
        size_t remaining = step_data_length;

        switch (step_mode) {
            /*
             *  MODE 0 — Basic CS Packet Mode
             *  --------------------------------------------------------
             *  Initiator:
             *      1. Packet_Quality          (1)
             *      2. Packet_RSSI             (1)
             *      3. Packet_Antenna          (1)
             *      4. Measured_Freq_Offset    (2)
             *      --> Total: 5 bytes
             *
             *  Reflector:
             *      1. Packet_Quality          (1)
             *      2. Packet_RSSI             (1)
             *      3. Packet_Antenna          (1)
             *      --> Total: 3 bytes
             *
             ************************************************************/
        case SAL_LE_RAS_SUBEVENT_STEP_MODE_0:
            if (role == SAL_LE_RAS_ROLE_INITIATOR) {
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(2, RAS_FILTER_BIT_FREQ_OFFSET);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_ANTENNA);
            }
            break;
        /*  MODE 1 — Time-of-Flight (ToF) Mode
         *  ------------------------------------------------------------
         *  Initiator:
         *      1. Packet_Quality          (1)
         *      2. Packet_NADM             (1)
         *      3. Packet_RSSI             (1)
         *      4. ToA_ToD_Initiator       (2)
         *      5. Packet_Antenna          (1)
         *      6. Packet_PCT1             (4)
         *      7. Packet_PCT2             (4)
         *      --> Total: 14 bytes
         *
         *  Reflector:
         *      1. Packet_Quality          (1)
         *      2. Packet_NADM             (1)
         *      3. Packet_RSSI             (1)
         *      4. ToD_ToA_Reflector       (2)
         *      5. Packet_Antenna          (1)
         *      6. Packet_PCT1             (4)
         *      7. Packet_PCT2             (4)
         *      --> Total: 14 bytes
         *
         **************************************************************/
        case SAL_LE_RAS_SUBEVENT_STEP_MODE_1:
            if (role == SAL_LE_RAS_ROLE_REFLECTOR) {
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, RAS_FILTER_BIT_TOA_TOD);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT2);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, RAS_FILTER_BIT_TOD_TOA);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT2);
            }
            break;
        /*****************************************************************
         *  MODE 2 — Tone-based Phase Measurement Mode
         *  ------------------------------------------------------------
         *  (Same layout for Initiator and Reflector)
         *      1. Antenna_Permutation_Index  (1)
         *      2. Tone_PCT[k]                ((Num_Antenna_Paths + 1) × 3)
         *      3. Tone_Quality_Indicator[k]  ((Num_Antenna_Paths + 1) × 1)
         *      --> Example total (Num_Antenna_Paths=3): 17 bytes
         *
         *******************************************************************/
        case SAL_LE_RAS_SUBEVENT_STEP_MODE_2:
            // Initiator and Reflector are the same.
            COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_ANT_PERM_IDX);

            // Tone_PCT[k]
            // Actual size = (Num_Antenna_Paths + 1) × 3 octets
            // COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, RAS_FILTER_BIT_TONE_PCT);
            COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, RAS_FILTER_BIT_TONE_PCT);
            // Tone_Quality_Indicator[k]
            // COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, RAS_FILTER_BIT_TONE_QUALITY);
            COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, RAS_FILTER_BIT_TONE_QUALITY);
            break;
        /*  MODE 3 — Combined (ToF + Tone) Mode
         *  ------------------------------------------------------------
         *  Initiator:
         *      1.  Packet_Quality            (1)
         *      2.  Packet_NADM               (1)
         *      3.  Packet_RSSI               (1)
         *      4.  ToA_ToD_Initiator         (2)
         *      5.  Packet_Antenna            (1)
         *      6.  Packet_PCT1               (4)
         *      7.  Packet_PCT2               (4)
         *      8.  Antenna_Permutation_Index (1)
         *      9.  Tone_PCT[k]               ((Num_Antenna_Paths + 1) × 3)
         *      10. Tone_Quality_Indicator[k] ((Num_Antenna_Paths + 1) × 1)
         *      --> Example total (Num_Antenna_Paths=3): 31 bytes
         *
         *  Reflector:
         *      1.  Packet_Quality            (1)
         *      2.  Packet_NADM               (1)
         *      3.  Packet_RSSI               (1)
         *      4.  ToD_ToA_Reflector         (2)
         *      5.  Packet_Antenna            (1)
         *      6.  Packet_PCT1               (4)
         *      7.  Packet_PCT2               (4)
         *      8.  Antenna_Permutation_Index (1)
         *      9.  Tone_PCT[k]               ((Num_Antenna_Paths + 1) × 3)
         *      10. Tone_Quality_Indicator[k] ((Num_Antenna_Paths + 1) × 1)
         *      --> Example total (Num_Antenna_Paths=3): 31 bytes
         *
         ********************************************************************/
        case SAL_LE_RAS_SUBEVENT_STEP_MODE_3:
            if (role == SAL_LE_RAS_ROLE_INITIATOR) {
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, RAS_FILTER_BIT_TOA_TOD);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT2);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_ANT_PERM_IDX);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, RAS_FILTER_BIT_TONE_PCT);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, RAS_FILTER_BIT_TONE_QUALITY);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, RAS_FILTER_BIT_TOD_TOA);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, RAS_FILTER_BIT_PKT_PCT2);
                COPY_FIELD_IF_ENABLED(1, RAS_FILTER_BIT_ANT_PERM_IDX);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, RAS_FILTER_BIT_TONE_PCT);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, RAS_FILTER_BIT_TONE_QUALITY);
            }
            break;
        default:
            BT_LOGW("Unknown mode %d, copying raw data.", step_mode);
            memcpy(&buf[out_offset], step_data, step_data_length);
            out_offset += step_data_length;
            break;
        }

        in_offset += 3 + step_data_length;
    }

    return out_offset;
}

static ras_rang_on_demand_t* ras_rang_on_demand_subevent_pool_find(bt_address_t* addr)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv enviranment, init it first.");
        return NULL;
    }

    for (int i = 0; i < SAL_LE_RAS_STORE_PROCEDURE_NUM_MAX; i++) {
        if (ras_srv->subevent[i].proc_used == false) {
            memset(&ras_srv->subevent[i], 0, sizeof(ras_rang_on_demand_t));
            ras_srv->subevent[i].proc_used = true;
            return &ras_srv->subevent[i];
        }
    }

    BT_LOGD("No subvent pool can be used.");
    return NULL;
}

static ras_rang_on_demand_t* ras_rang_on_demand_find_subevent(bt_address_t* addr, uint16_t count)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv enviranment, init it first.");
        return NULL;
    }

    for (int i = 0; i < SAL_LE_RAS_STORE_PROCEDURE_NUM_MAX; i++) {
        if (ras_srv->subevent[i].count == count && ras_srv->subevent[i].proc_used == true) {
            return &ras_srv->subevent[i];
        }
    }

    BT_LOGD("No subvent find with count(%d).", count);
    return NULL;
}

static uint8_t* ras_subevent_data_conversion(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    memset(ras_srv->latest_local_steps, 0, sizeof(ras_srv->latest_local_steps));

    if (result->len <= SAL_LE_RAS_STEP_DATA_BUF_LEN) {
        memcpy(ras_srv->latest_local_steps, result->step_data_buf,
            result->len);
        BT_LOGD("step data[%d]:%s\n", result->len,
            cs_log_to_hex_str(result->step_data_buf, result->len));
    } else {
        BT_LOGD("Not enough memory to store step data. (%d > %d)\n",
            result->len, SAL_LE_RAS_STEP_DATA_BUF_LEN);
    }

    uint8_t* stream_buf = ras_srv->latest_local_steps;
    int bit_offset = 0;
    uint16_t count_id = ((result->header.procedure_counter & 0x0FFF) << 12) | (result->header.config_id & 0x0F);
    /**
     * CS configuration identifier.
     * Range: 0 to 3
     * Rangging Counter is lower 12-bits of CS Procedure_Counter Provided by the Core Controller
     */
    ras_write_bits(stream_buf, &bit_offset, count_id, 16);
    /**
     * Transmit power level used for the CS Procedure.
     * Range: -127 to 20
     * Units: decibels referenced to 1 milliwatt(dBm)
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.reference_power_level, 8);
    /**
     * Antenna paths that are reported:
     * Bit0: 1 if Antenna Path_1 included; 0 if not.
     * Bit1: 1 if Antenna Path_2 included; 0 if not.
     * Bit2: 1 if Antenna Path_3 included; 0 if not.
     * Bit3: 1 if Antenna Path_4 included; 0 if not.
     * Bits 4-7: RFU
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.num_antenna_paths, 8);
    /**
     * Starting ACL addrection event count for the results reported in the event.
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.start_acl_conn_event, 16);
    /**
     * Frequency compensation value in units of 0.01 parts permillion(ppm)(15-bit signed integer).
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.frequency_compensation, 16);
    /**
     * 0x0: All results complete for the CS Procedure
     * 0x1: Partial results with more to follow for the CS Procedure
     * 0xF: All subsequent CS Procedures aborted
     * All other values: RFU
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.procedure_done_status, 4);
    /**
     * 0x0: All results complete for the CS Subevent
     * 0xF: Current CS Subevent aborted
     * All other values: RFU
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.subevent_done_status, 4);
    /**
     * Indicates the abort reason when the Procedure_Done Status received from the Core
     * Controller is set to 0xF; otherwise, the value is set to zero.
     * 0x0: Report with no abort
     * 0x1: Abort because of local Host or remote request
     * 0x2: Abort because filtered channel map has less than 15 channels
     * 0x3: Abort because the channel map update instant has passed
     * 0xF: Abort because of unspecified reasons
     * All other values: RFU
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.procedure_abort_reason, 4);
    /**
     * Indicates the abort reason when the Subevent_Done_Status received from the Core Controller
     * is set to 0xF; otherwise, the default value is set to zero.
     * 0x0: Report with no abort
     * 0x1: Abort because of local Host or remote request
     * 0x2: Abort because no CS_SYNC(mode 0) was received
     * 0x3: Abort because of scheduling conflicts or limited resources
     * 0xF: Abort because of unspecified reasons
     * All other values: RFU
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.subevent_abort_reason, 4);
    /**
     * Reference power level.
     * Range: -127 to 20
     * Units: dBm
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.reference_power_level, 8);
    /**
     * Number of steps in the CS Subevent for which results are reported. If the Subevent is
     * aborted, then the Number of Steps Reported can be set to zero.
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.num_steps_reported, 8);

    ras_srv->ras_seg_offset = 0;
    ras_srv->remaining_len = transform_step_data_to_ras_format_filtered(result->step_data_buf,
        result->len, stream_buf + SAL_LE_RAS_SUB_PROCUDURE_HEAD,
        ras_srv->ras_filter, ras_srv->ras_role, result->header.num_antenna_paths & 0x0F);
    ras_srv->remaining_len += SAL_LE_RAS_SUB_PROCUDURE_HEAD;
    BT_LOGD("stream head:%s.", cs_log_to_hex_str(stream_buf, SAL_LE_RAS_SUB_PROCUDURE_HEAD));
    BT_LOGD("stream_buf(%ld):%s.\n", ras_srv->remaining_len,
        cs_log_to_hex_str(stream_buf + 12, ras_srv->remaining_len - SAL_LE_RAS_SUB_PROCUDURE_HEAD));
    BT_LOGD("data ready indiacte count(%d)", result->header.procedure_counter);
    BT_LOGD("procedure_counter:0x%x, config_id:0x%x, reference_power_level:0x%x",
        result->header.procedure_counter, result->header.config_id,
        result->header.reference_power_level);
    BT_LOGD("num_antenna_paths:0x%x, start_acl_conn_event:0x%x, frequency_compensation:0x%x",
        result->header.num_antenna_paths, result->header.start_acl_conn_event,
        result->header.frequency_compensation);
    BT_LOGD("procedure_done_status:0x%x, subevent_done_status:0x%x, procedure_abort_reason:0x%x",
        result->header.procedure_done_status, result->header.subevent_done_status,
        result->header.procedure_abort_reason);
    BT_LOGD("subevent_abort_reason:0x%x, reference_power_level:0x%x, num_steps_reported:0x%x",
        result->header.subevent_abort_reason, result->header.reference_power_level,
        result->header.num_steps_reported);
    BT_LOGD("mode:0x%x, channel:0x%x, len:0x%x", result->step_data_buf[0],
        result->step_data_buf[1],
        result->step_data_buf[2]);
    return stream_buf;
}

static void ras_process_real_time_ranging_data(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    uint8_t* stream_buf = ras_subevent_data_conversion(addr, result);
    split_real_time_segment(addr, stream_buf, ras_srv->remaining_len);
}

static void ras_process_on_demand_ranging_data(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    ras_rang_on_demand_t* subevent = ras_rang_on_demand_subevent_pool_find(addr);

    if (!subevent) {
        BT_LOGE("No subevent pool found.");
        return;
    }

    BT_LOGD("procedure counter:%d.", result->header.procedure_counter);

    subevent->count = result->header.procedure_counter;
    uint8_t* stream_buf = ras_subevent_data_conversion(addr, result);
    split_on_demand_segment(addr, stream_buf, ras_srv->remaining_len, subevent);
    if (ras_state_get_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE)) {
        ras_data_ready_send(addr, subevent->count);
        // Set the on-demand state to ready.
        ras_state_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_DATA_READY_INDICATE);
    }

    return;
}

static void subevent_result_cb(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    if (result->header.procedure_done_status == BT_LE_SRV_CS_PROCEDURE_COMPLETE && ras_check_ranging_mode(addr) == SAL_LE_RAS_RANGING_MODE_REAL_TIME) {
        BT_LOGD("Recv the real-time ranging data.");
        ras_process_real_time_ranging_data(addr, result);
        return;
    }

    if (result->header.procedure_done_status == BT_LE_SRV_CS_PROCEDURE_COMPLETE && ras_check_ranging_mode(addr) == SAL_LE_RAS_RANGING_MODE_ON_DEMAND) {
        BT_LOGD("Recv the on-demand ranging data.");
        ras_process_on_demand_ranging_data(addr, result);
        return;
    }

    BT_LOGE("No mode has been set, discard the subevent result.");
    return;
}

static bt_status_t ras_data_ready_send(bt_address_t* addr, uint16_t count)
{
    uint8_t buf[2];
    ras_put_uint16_to_ptr(count, buf);
    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY)) {
        return BT_GATT_NOTIFY(RAS_DATA_READY_CHAR_SEND, addr, (uint8_t*)&count, sizeof(count));
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_DATA_READY_INDICATE)) {
        return BT_GATT_INDICATE(RAS_DATA_READY_CHAR_SEND, addr, (uint8_t*)&count, sizeof(count));
    }

    BT_LOGE("Invalid data ready char indication state.");
    return BT_STATUS_FAIL;
}

static void ras_mtu_updated_cb(bt_address_t* addr, uint32_t mtu)
{
    if (ras_srv) {
        ras_srv->ras_mtu = mtu;
    }

    BT_LOGD("Updated MTU, ras_mtu: %lu\n", ras_srv->ras_mtu);
    return;
}

static void ras_gatts_ccc_cfg_cb(bt_address_t* addr, ras_ccc_cfg_change_evt_t event,
    const uint8_t* value, uint16_t length)
{
    if (!addr) {
        BT_LOGE("Invalid BT address.");
        return;
    }

    switch (event) {
    case RAS_RTT_DATA_CCC_CFG_CHANGE_EVT: {
        uint16_t rtt_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS RTT CCC data config value length(%d).", length);
            return;
        }

        CS_BYTE_STREAM_TO_UINT16(rtt_cfg_val, value);
        range_rtt_dt_ccc_cfg_changed(addr, rtt_cfg_val);
    } break;
    case RAS_ON_DEMAND_DATA_CCC_CFG_CHANGE_EVT: {
        uint16_t on_demand_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS On-demand CCC data config value length(%d).", length);
            return;
        }

        CS_BYTE_STREAM_TO_UINT16(on_demand_cfg_val, value);
        range_on_dem_dt_ccc_cfg_changed(addr, on_demand_cfg_val);
    } break;
    case RAS_CTR_PT_CCC_CFG_CHANGE_EVT: {
        uint16_t ctr_point_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS Control Point CCC data config value length(%d).", length);
            return;
        }

        CS_BYTE_STREAM_TO_UINT16(ctr_point_cfg_val, value);
        range_ctr_pt_ccc_cfg_changed(addr, ctr_point_cfg_val);
    } break;
    case RAS_DATA_READY_CCC_CFG_CHANGE_EVT: {
        uint16_t data_ready_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS data ready CCC data config value length(%d).", length);
            return;
        }

        CS_BYTE_STREAM_TO_UINT16(data_ready_cfg_val, value);
        range_dt_rd_ccc_cfg_changed(addr, data_ready_cfg_val);
    } break;
    case RAS_OVER_WRITE_CCC_CFG_CHANGE_EVT: {
        uint16_t over_write_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS Over Write CCC data config value length(%d).", length);
            return;
        }

        CS_BYTE_STREAM_TO_UINT16(over_write_cfg_val, value);
        range_dt_ov_wr_ccc_cfg_changed(addr, over_write_cfg_val);
    } break;
    }
    return;
}

static void ras_gatts_ctr_pt_write_cb(bt_address_t* addr,
    const uint8_t* value, uint16_t length)
{
    on_ras_ctr_pt_write_cb(addr, value, length);
    return;
}

static void ras_gatts_feature_read_cb(bt_address_t* addr, uint32_t req_handle)
{
    BT_LOGI("feature read, req_handle:%lu", req_handle);
    if (!ras_srv) {
        BT_LOGE("RAS haven't init.");
        return;
    }

    ras_send_feature_read_rsp(addr, ras_srv->ras_feature);
    return;
}

static void ras_notify_cb(bt_address_t* addr, gatt_status_t status, ras_attr_notify_t attr)
{
    if (status != GATT_STATUS_SUCCESS) {
        BT_LOGE("Notify fail, status(%d)", status);
        return;
    }

    BT_LOGI("ras notify cb, attr:%d", attr);
    switch (attr) {
    case RAS_REAL_TIME_CHAR_SEND: {
        if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
            ras_dt_rd_indicate_cb(addr);
        }
    } break;
    case RAS_ON_DEMAND_CHAR_SEND: {
        if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY)) {
            ras_on_demand_notify_finished(addr);
        } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
            ras_on_demand_indicate_finished(addr);
        }
    } break;
    default:
        break;
    }

    return;
}

static void ras_conn_cb(bt_address_t* addr)
{
    if (ras_srv->addr) {
        BT_LOGE("The connection has been created.");
        return;
    }

    ras_srv->addr = (bt_address_t*)malloc(sizeof(bt_address_t));

    memcpy(ras_srv->addr, addr, sizeof(bt_address_t));
    BT_LOGD("RAS Conn to address:%s", bt_addr_str(addr));
    return;
}

static void ras_disconn_cb(bt_address_t* addr)
{
    if (ras_srv->addr == NULL) {
        BT_LOGE("The connection has been release.");
        return;
    }

    BT_LOGD("RAS disconn to address:%s", bt_addr_str(addr));
    free(ras_srv->addr);
    ras_srv->addr = NULL;
    return;
}

static const ras_gatts_callbacks_t ras_cb = {
    .cfg_cb = ras_gatts_ccc_cfg_cb,
    .pt_write_cb = ras_gatts_ctr_pt_write_cb,
    .feature_read_cb = ras_gatts_feature_read_cb,
    .mtu_updated_cb = ras_mtu_updated_cb,
    .notify_cb = ras_notify_cb,
    .conn_cb = ras_conn_cb,
    .disconn_cb = ras_disconn_cb,
};

int bt_cs_ras_enable(void)
{
    BT_LOGD("Enable Channel Sounding.");

    ras_srv = (ras_srv_env_t*)malloc(sizeof(ras_srv_env_t));

    memset(ras_srv, 0, sizeof(ras_srv_env_t));

    ras_srv->ras_feature = 0x07000007;

    for (int i = 0; i < SAL_LE_RAS_FILTER_MODE_MAX; i++) {
        ras_srv->ras_filter[i] = 0xFFFFFFFF;
    }

    bt_cs_ras_gatts_init(&ras_cb);
    bt_cs_register_subevent_cb(subevent_result_cb);

    return 0;
}

int ras_subevent_recv_test(ras_rang_mode_t mode, ras_testcase_t test_case,
    bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    if (mode == SAL_LE_RAS_RANGING_MODE_REAL_TIME) {
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY);
        BT_LOGD("RAS Server test: set ranging to real-time mode.");
    }

    switch (test_case) {
    case RAS_TESTCASE_REAL_TIME_NOTIFY_VALID_RANG_DATA_001:
        ras_srv->ras_mtu = 253;
        ras_srv->rt_dt_ccc_cfg = SAL_LE_RAS_GATT_NOTIFY;
        break;
    case RAS_TESTCASE_REAL_TIME_INDICATE_VALID_RANG_DATA_002:
        ras_srv->ras_mtu = 253;
        ras_srv->rt_dt_ccc_cfg = SAL_LE_RAS_GATT_INDICATION;
        break;
    case RAS_TESTCASE_ON_DEMAND_NOTIFY_VALID_RANG_DATA_003:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_state_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_INDICATE_VALID_RANG_DATA_004:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_state_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_WRITE_RANG_DATA_TIMWOUT_010:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_state_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE);
        break;
    default:
        BT_LOGE("Invalid test case number(%d).", test_case);
        return -1;
    }

    subevent_result_cb(addr, result);
    return 0;
}

int ras_ctrl_point_send_test(bt_address_t* addr, uint8_t* data, uint16_t len)
{
    on_ras_ctr_pt_write_cb(addr, (void*)data, len);
    return 0;
}

void ras_on_demand_notify_finish_test(bt_address_t* addr)
{
    ras_on_demand_notify_finished(addr);
}

void ras_on_demand_indicate_finish_test(bt_address_t* addr, ras_attr_notify_t attr)
{
    switch (attr) {
    case RAS_REAL_TIME_CHAR_SEND:
        ras_dt_rd_indicate_cb(addr);
        break;
    case RAS_ON_DEMAND_CHAR_SEND:
        ras_on_demand_indicate_finished(addr);
        break;
    default:
        break;
    }
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
