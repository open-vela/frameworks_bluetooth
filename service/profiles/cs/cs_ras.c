/*
 * Copyright (C) 2025 Xiaomi Corporation. All rights reserved.
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
 *
 */

#include "cs_ras.h"
#include "bt_status.h"
#include "bt_utils.h"
#include "cs_ras_gatts.h"
#include "cs_ras_test.h"
#include "cs_ras_util.h"
#include "utils/log.h"

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#define CS_CONFIG_ID 0
#define NUM_MODE_0_STEPS 1
#define RAS_SEG_HEADER_SIZE 4
#define CS_RAS_GET_RANG_DATA_LEN 3
#define CS_RAS_ACK_RANG_DATA_LEN 3
#define CS_RAS_RETRIEVE_LOST_RANG_DATA_LEN 5
#define CS_RAS_SET_FILTER_RANG_DATA_LEN 5

static ras_srv_env_t* ras_srv;

static void cs_ras_split_real_time_segment(bt_address_t* addr, uint8_t* buf, int len);
static ras_rang_on_demand_t* cs_ras_rang_on_demand_find_subevent(bt_address_t* addr, uint16_t count);
static bt_status_t cs_ras_data_ready_send(bt_address_t* addr, uint16_t count);
static bt_status_t cs_ras_on_demand_send_cmp_ranging_data_rsp(bt_address_t* addr, uint16_t count);
static ssize_t on_ras_ctr_pt_write_cb(bt_address_t* addr, const void* buf, uint16_t len);

static void cs_ras_on_demand_notify_finished(bt_address_t* addr)
{
    cs_node_t* on_demand_pdu = (cs_node_t*)ras_srv->on_demand_curr_node;

    if (!on_demand_pdu) {
        BT_LOGD("Complete segment data sent.");
        cs_ras_on_demand_send_cmp_ranging_data_rsp(addr, 10);
        return;
    }

    ras_segment_t* seg = container_of(on_demand_pdu, ras_segment_t, seg_node);

    BT_LOGD("seg:%p, seg->data(%d)", seg, seg->len);
    BT_DUMPBUFFER("seg->data", seg->data, seg->len);

    ras_srv->on_demand_curr_node = on_demand_pdu->next;

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
    cs_node_t* on_demand_pdu = (cs_node_t*)ras_srv->on_demand_curr_node;

    if (!on_demand_pdu) {
        BT_LOGD("Complete segment data sent.");
        cs_ras_on_demand_send_cmp_ranging_data_rsp(addr, 10);
        return;
    }

    ras_segment_t* seg = container_of(on_demand_pdu, ras_segment_t, seg_node);

    BT_LOGD("seg:%p, seg->data(%d)", seg, seg->len);
    BT_DUMPBUFFER("seg->data", seg->data, seg->len);
    ras_srv->on_demand_curr_node = on_demand_pdu->next;

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
        BT_LOGE("RAS haven't enable the on-demand ranging data notify or indication state.");
        return BT_STATUS_FAIL;
    }

    bt_status_t status = BT_STATUS_SUCCESS;

    ras_rang_on_demand_t* on_demand_sub = cs_ras_rang_on_demand_find_subevent(addr, count);

    if (!on_demand_sub) {
        BT_LOGE("Haven't find subevent with count(%d).", count);
        return BT_STATUS_PARM_INVALID;
    }

    const cs_node_t* on_demand_pdu = cs_list_peek_head(&on_demand_sub->seg_list);
    struct ras_segment_t* seg = container_of(on_demand_pdu, ras_segment_t, seg_node);

    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY)) {
        BT_LOGD("seg:%p, seg->data(%d)", seg, seg->len);
        BT_DUMPBUFFER("seg->data", seg->data, seg->len);

        status = BT_GATT_NOTIFY_CB(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);

        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("On-demand ranging data notify fail, status(%d).", status);
            return status;
        }

    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        BT_LOGD("seg:%p, seg->data(%d)", seg, seg->len);
        BT_DUMPBUFFER("seg->data", seg->data, seg->len);
        // Set the current node to the next.
        ras_srv->on_demand_curr_node = on_demand_pdu->next;
        status = BT_GATT_INDICATE(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);

        if (status != 0) {
            BT_LOGE("On-demand ranging data indicate fail, err(%d).", status);
            return status;
        }
    }

    return status;
}

static bt_status_t cs_ras_on_demand_send_cmp_ranging_data_rsp(bt_address_t* addr, uint16_t count)
{
    ras_rang_on_demand_t* on_demand_data = cs_ras_rang_on_demand_find_subevent(addr, count);

    if (!on_demand_data) {
        BT_LOGW("Haven't find the demand data with the count(%d).", count);
    }

    uint8_t buf[CS_RAS_CONTROL_POINT_DATA_LEN] = { 0 };
    bt_status_t status;
    buf[0] = CS_RAS_CTL_OP_RSP_CMP_RANG_DATA;
    ras_put_uint16_to_ptr(count, &buf[1]);

    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        status = BT_GATT_NOTIFY_CB(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK notify fail, err(%d).", status);
            return status;
        }
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE)) {
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

static bt_status_t ras_on_demand_send_code_rsp(bt_address_t* addr, uint16_t count)
{
    ras_rang_on_demand_t* on_demand_data = cs_ras_rang_on_demand_find_subevent(addr, count);

    if (!on_demand_data) {
        BT_LOGW("Haven't find the demand data with the count(%d).", count);
    }

    uint8_t buf[CS_RAS_CTL_OP_RSP_CODE_DATA_LEN] = { 0 };
    bt_status_t status;
    buf[0] = CS_RAS_CTL_OP_RSP_CODE;
    buf[1] = CS_RAS_CTL_OP_RSP_CODE_SUCCESS;

    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        status = BT_GATT_NOTIFY_CB(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK notify fail, err(%d).", status);
            return status;
        }
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE)) {
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

static bt_status_t ras_on_demand_send_lost_ranging_data_cmp_rsp(bt_address_t* addr, uint16_t count, uint8_t first, uint8_t last)
{
    /**
     * Complete Lost Ranging Data Segment Response (0x01)
     *
     * Reference: Section C.1
     *
     * +--------+---------------------+------+
     * | Offset | Field               | Size |
     * +--------+---------------------+------+
     * | 0      | Opcode              | 1    | uint8
     * | 1      | Ranging Counter     | 2    | uint16
     * | 3      | First Segment Index | 1    | uint8
     * | 4      | Last Segment Index  | 1    | uint8
     * +--------+---------------------+------+
     * Total: 5 bytes
     */
    uint8_t buf[5] = { 0 };
    buf[0] = CS_RAS_CTL_OP_RSP_CODE;
    ras_put_uint16_to_ptr(count, &buf[1]);
    buf[3] = first;
    buf[4] = last;

    bt_status_t status;
    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        status = BT_GATT_NOTIFY_CB(RAS_CONTROL_POINT_CHAR_SEND, addr, buf, sizeof(buf));
        if (status != 0) {
            BT_LOGE("ranging data Control Point response ACK notify fail, err(%d).", status);
            return status;
        }
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE)) {
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
    ras_rang_on_demand_t* on_demand_data = cs_ras_rang_on_demand_find_subevent(addr, count);
    if (!on_demand_data) {
        BT_LOGE("On-demand retrieve lost data failed: subevent not found for count(%u).", count);
        return BT_STATUS_PARM_INVALID;
    }

    if (!ras_state_get_bit(&ras_srv->on_demand_state, CS_RAS_ON_DEMAND_STATE_RANGING_DATA_RSP)) {
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

    ras_on_demand_send_lost_ranging_data_cmp_rsp(addr, count, first_seg, last_seg);
    return BT_STATUS_SUCCESS;
}

static uint8_t ras_check_ranging_mode(bt_address_t* addr)
{
    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY) || ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE)) {
        BT_LOGD("The real-time mode has been set.");
        return CS_RAS_RANGING_MODE_REAL_TIME;
    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) || ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        BT_LOGD("The on-demand mode has been set.");
        return CS_RAS_RANGING_MODE_ON_DEMAND;
    }

    BT_LOGE("No mode have been set.");
    return CS_RAS_RANGING_MODE_UNDEFINED;
}

bool ras_is_filter_bit_set(uint16_t mode, uint16_t filter_bit)
{
    // Check if the mode is valid (within the range of CS_RAS_MODE_3_FILTER_MAX)
    if (mode >= CS_RAS_FILTER_MODE_MAX) {
        BT_LOGE("Error: Mode %d is out of valid range (0 to %d)",
            mode, CS_RAS_FILTER_MODE_MAX - 1);
        return false;
    }

    // Retrieve the current filter mask
    uint32_t current_filter_mask = ras_srv->ras_filter[mode];

    // Check if the specific filter bit is set
    // We shift the filter_bit into the correct position and mask it to check
    return (current_filter_mask & (1 << filter_bit)) != 0;
}

static void ras_set_filter(uint16_t filter_value)
{
    // Extract the mode (bits 0-1)
    uint16_t mode = filter_value & CS_RAS_FILTER_MODE_MASK;

    // Extract the filter mask (bits 2-15)
    uint16_t filter_mask = filter_value & ~CS_RAS_FILTER_MODE_MASK;

    // Use the mode directly as an index into ras_filter
    if (mode < CS_RAS_FILTER_MODE_MAX) {
        // Clear the filter bits (2-15)
        ras_srv->ras_filter[mode] &= CS_RAS_FILTER_MODE_MASK;

        // Set the new filter mask (bits 2-15)
        ras_srv->ras_filter[mode] |= filter_mask;
    } else {
        // Handle error case if mode is out of range
        BT_LOGE("Error: Mode %d is out of valid range (0 to %d)", mode, CS_RAS_FILTER_MODE_MAX - 1);
    }

    return;
}

static void ras_handle_abort_operation(ras_control_point_t* control_point)
{
    if (control_point->is_processing) {
        BT_LOGD("Stopping current RAS Control Point processes...");
        control_point->is_processing = 0; // Stop current processing.

        if (control_point->is_data_pending) {
            BT_LOGD("Flushing pending Ranging Data segments...");
            control_point->is_data_pending = 0; // Discard pending data
        }

        control_point->response_code = CS_RAS_CTL_OP_RSP_CODE_SUCCESS; // response success.
        BT_LOGD("Abort Operation completed successfully.");
    } else {
        control_point->response_code = CS_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED;
        BT_LOGD("Abort Operation was unsuccessful (no operation to abort).");
    }
}

static void ras_handle_other_operation(ras_control_point_t* control_point)
{
    BT_LOGD("Handling other operations...");
    control_point->response_code = CS_RAS_CTL_OP_RSP_CODE_SUCCESS; // response success.
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
        control_point->response_code = CS_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED;
        BT_LOGW("OpCode not supported.");
        break;
    }
}

void rs_display_response(ras_control_point_t* control_point)
{
    switch (control_point->response_code) {
    case CS_RAS_CTL_OP_RSP_CODE_SUCCESS:
        BT_LOGD("Response: SUCCESS.");
        break;
    case CS_RAS_CTL_OP_RSP_CODE_PROCE_NOT_CMP:
        BT_LOGD("Response: PROCEDURE NOT COMPLETED.");
        break;
    case CS_RAS_CTL_OP_RSP_CODE_PERSISTED:
        BT_LOGD("Response: PERSISTED UNSUCCESSFUL.");
        break;
    case CS_RAS_CTL_OP_RSP_CODE_SERVER_BUSY:
        BT_LOGD("Response: SERVER BUSY.");
        break;
    case CS_RAS_CTL_OP_RSP_CODE_INVALID_PARAMS:
        BT_LOGD("Response: INVALID PARAMETER.");
        break;
    case CS_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED:
        BT_LOGD("Response: OP CODE NOT SUPPORTED.");
        break;
    default:
        BT_LOGD("Unknown response code.");
        break;
    }
}

static ssize_t on_ras_ctr_pt_write_cb(bt_address_t* addr, const void* buf, uint16_t len)
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
    case CS_RAS_CTL_OP_CMD_GET_RANG_DATA: {
        if (len != CS_RAS_GET_RANG_DATA_LEN) {
            BT_LOGE("Invalid get ranging data len(%d).", len);
            return 0;
        }
        count = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        ras_ondemand_send_ranging_data(addr, count);
        break;
    }
    case CS_RAS_CTL_OP_CMD_ACK_RANG_DATA: {
        if (len != CS_RAS_ACK_RANG_DATA_LEN) {
            BT_LOGE("Invalid ack ranging data len(%d).", len);
            return 0;
        }
        count = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        ras_on_demand_send_code_rsp(addr, count);
        break;
    }
    case CS_RAS_CTL_OP_CMD_RETRIEVE_LOST_RANG_DATA_SEG: {
        if (len != CS_RAS_RETRIEVE_LOST_RANG_DATA_LEN) {
            BT_LOGE("Invalid retrieve lost ranging data len(%d).", len);
            return 0;
        }
        count = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        uint8_t first_seg = (uint8_t)buf_send[3];
        uint8_t last_seg = (uint8_t)buf_send[4];
        ras_on_demand_retrieve_send_lost_data(addr, count, first_seg, last_seg);
        break;
    }
    case CS_RAS_CTL_OP_CMD_ABORT_OPERATION: {
        break;
    }
    case CS_RAS_CTL_OP_CMD_SET_FILTER: {
        if (len != CS_RAS_SET_FILTER_RANG_DATA_LEN) {
            BT_LOGE("Invalid retrieve lost ranging data len(%d).", len);
            return 0;
        }
        uint16_t filter_params = ras_get_uint16_from_ptr((const uint8_t*)&buf_send[1]);
        ras_set_filter(filter_params);
        break;
    }
    default:
        break;
    }
    return len;
}

static void range_rtt_dt_ccc_cfg_changed(bt_address_t* addr, uint16_t value)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv value.");
        return;
    }

    // The Real time mode and the on-demand mode can't be set together.
    if ((value != 0) && (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) || ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE))) {
        BT_LOGE("The on-demand mode has been set, Please clear it before set to real-time mode.");
        return;
    }

    if (value == 0) {
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY);
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE);
    } else {
        ras_state_set_bit(&ras_srv->char_notify_state,
            (value == CS_RAS_GATT_NOTIFY) ? RAS_RTT_DATA_NOTIFY : RAS_RTT_DATA_INDICATE);
    }

    ras_srv->rt_dt_ccc_cfg = value;

    BT_LOGD("The range real-time data ccc value is change to (%d)", value);
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
        BT_LOGE("The real-time mode has been set, please clear it before set to on-demand mode.");
        return;
    }

    if (value == 0) {
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY);
        ras_state_clear_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
    } else {
        ras_state_set_bit(&ras_srv->char_notify_state,
            (value == CS_RAS_GATT_NOTIFY) ? RAS_ON_DEMAND_DATA_NOTIFY : RAS_ON_DEMAND_DATA_INDICATE);
    }

    BT_LOGD("The range on-demand data ccc value is change to (%d)", value);
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
            (value == CS_RAS_GATT_NOTIFY) ? RAS_CONTROL_POINT_NOTIFY : RAS_CONTROL_POINT_INDICATE);
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
            (value == CS_RAS_GATT_NOTIFY) ? RAS_DATA_READY_NOTIFY : RAS_DATA_READY_INDICATE);
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
            (value == CS_RAS_GATT_NOTIFY) ? RAS_OVER_WRITE_NOTIFY : RAS_OVER_WRITE_INDICATE);
    }

    BT_LOGD("The range over write data ccc value is change to (%d)", value);
    return;
}

static void ras_dt_rd_indicate_cb(bt_address_t* addr)
{
    if (!ras_srv) {
        BT_LOGE("The CS RAS moudle haven't init, ras_srv is NULL.");
        return;
    }

    BT_LOGD("Indication finish");
    if (ras_srv->remaining_len) {
        cs_ras_split_real_time_segment(addr, &ras_srv->latest_local_steps[ras_srv->ras_seg_offset], ras_srv->remaining_len);
    }

    return;
}

static void ras_write_bits(uint8_t* buf, int* bit_offset, uint32_t value, int bit_count)
{
    int byte_index;
    int bit_index;

    if (!buf || !bit_offset) {
        BT_LOGE("Invalid buf or bit_offset.");
        return;
    }

    for (int i = 0; i < bit_count; i++) {
        byte_index = *bit_offset / 8;
        bit_index = *bit_offset % 8;
        uint8_t bit = (value >> i) & 0x01;

        if (bit) {
            buf[byte_index] |= (1 << bit_index);
        } else {
            buf[byte_index] &= ~(1 << bit_index);
        }

        (*bit_offset)++;
    }
}

static void cs_ras_split_real_time_segment(bt_address_t* addr, uint8_t* buf, int len)
{
    if (ras_srv->remaining_len > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
        int curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
        BT_LOGD("data send: offset:%lu, len:%d", ras_srv->ras_seg_offset, curr_seg_size);

        // update offset
        ras_srv->ras_seg_offset += curr_seg_size;
        uint8_t* send_buf = zalloc(curr_seg_size + 1);

        ras_srv->remaining_len -= curr_seg_size;
        send_buf[0] = (ras_srv->ras_seg_idx == 0) ? (0x01) : (ras_srv->ras_seg_idx << 2);

        ras_srv->ras_seg_idx++;
        memcpy(&send_buf[1], buf, curr_seg_size);

        if ((send_buf[0] & 0x01) == 0x01) {
            BT_LOGD("First seg, data(%d)", curr_seg_size + 1);
            BT_DUMPBUFFER("seg->data", send_buf, curr_seg_size + 1);
        } else {
            BT_LOGD("The %d seg, data(%d)", send_buf[0] >> 2, curr_seg_size + 1);
            BT_DUMPBUFFER("seg->data", send_buf, curr_seg_size + 1);
        }

        if (ras_srv->rt_dt_ccc_cfg == CS_RAS_GATT_NOTIFY) {
            if (BT_GATT_NOTIFY(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                if (ras_srv->remaining_len) {
                    cs_ras_split_real_time_segment(addr, &ras_srv->latest_local_steps[ras_srv->ras_seg_offset], ras_srv->remaining_len);
                }
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Notify fail.");
                free(send_buf);
                return;
            }
        } else if (ras_srv->rt_dt_ccc_cfg == CS_RAS_GATT_INDICATION) {
            if (BT_GATT_INDICATE(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Indicate fail.");
                free(send_buf);
                return;
            }
        } else {
            BT_LOGE("Invalid range data ccc config state:0x%x", ras_srv->rt_dt_ccc_cfg);
            free(send_buf);
        }
    } else {
        int curr_seg_size = len;
        uint8_t* send_buf = zalloc(curr_seg_size + 1);
        send_buf[0] = (ras_srv->ras_seg_idx == 0) ? (0x01) : (ras_srv->ras_seg_idx << 2);
        send_buf[0] |= (0x01 << 1);
        memcpy(&send_buf[1], buf, curr_seg_size);

        ras_srv->ras_seg_idx = 0;
        ras_srv->ras_dt_rd_indicating = 0U;
        ras_srv->remaining_len = 0;
        ras_srv->ras_seg_offset = 0;
        BT_LOGD("The last(%d) seg, data(%d)", send_buf[0] >> 2, curr_seg_size + 1);
        BT_DUMPBUFFER("seg->data", send_buf, curr_seg_size + 1);
        BT_LOGD("ras_dt_rd_indicating:%d", ras_srv->ras_dt_rd_indicating);
        if (ras_srv->rt_dt_ccc_cfg == CS_RAS_GATT_NOTIFY) {
            if (BT_GATT_NOTIFY(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Notify fail.");
                free(send_buf);
                return;
            }
        } else if (ras_srv->rt_dt_ccc_cfg == CS_RAS_GATT_INDICATION) {
            if (BT_GATT_INDICATE(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                BT_LOGD("ras data ready Indicate fail.");
                free(send_buf);
                return;
            }
        } else {
            BT_LOGE("Invalid range data ccc config state:0x%x", ras_srv->rt_dt_ccc_cfg);
            free(send_buf);
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
    timer->userdata = NULL;
    on_demand_subevent->on_demand_timer = NULL;
    return;
}

static void cs_ras_split_on_demand_segment(bt_address_t* addr, uint8_t* buf, int len,
    ras_rang_on_demand_t* subevent)
{
    int curr_seg_size = 0;
    uint16_t seg_index = 0;
    ras_srv->ras_seg_offset = 0;

    do {
        if (ras_srv->remaining_len > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
            curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
            BT_LOGD("data send: offset:%lu, len:%d", ras_srv->ras_seg_offset, curr_seg_size);
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
            BT_LOGD("ras_dt_rd_indicating:%d", ras_srv->ras_dt_rd_indicating);
        }

        if ((subevent->seg->data[0] & 0x01) == 0x01) {
            BT_LOGD("First seg, data(%d)", curr_seg_size + 1);
            BT_DUMPBUFFER("seg->data", subevent->seg->data, curr_seg_size + 1);
        } else {
            BT_LOGD("The %d seg, data(%d)", subevent->seg->data[0] >> 2, curr_seg_size + 1);
            BT_DUMPBUFFER("seg->data", subevent->seg->data, curr_seg_size + 1);
        }

        subevent->seg->seg_idx = seg_index++;
        subevent->seg->len = curr_seg_size + 1;
        cs_list_append(&subevent->seg_list, &subevent->seg->seg_node);
        BT_LOGD("subevent seg:%p, seg_node:%p.", subevent->seg, &subevent->seg->seg_node);
    } while (ras_srv->remaining_len > 0);

    if (!subevent->on_demand_timer) {
        subevent->on_demand_timer = service_loop_timer(RAS_RSP_TIMEOUT, false, ras_on_demand_data_send_timeout, subevent);
    } else {
        BT_LOGE("The on demand timer already exit. subevent count:%d.", subevent->count);
    }
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
        case CS_RAS_SUBEVENT_STEP_MODE_0:
            if (role == CS_RAS_ROLE_INITIATOR) {
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_FILTER_BIT_FREQ_OFFSET);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_ANTENNA);
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
        case CS_RAS_SUBEVENT_STEP_MODE_1:
            if (role == CS_RAS_ROLE_REFLECTOR) {
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_FILTER_BIT_TOA_TOD);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT2);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_FILTER_BIT_TOD_TOA);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT2);
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
        case CS_RAS_SUBEVENT_STEP_MODE_2:
            // Initiator and Reflector are the same.
            COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_ANT_PERM_IDX);

            // Tone_PCT[k]
            // Actual size = (Num_Antenna_Paths + 1) × 3 octets
            // COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, CS_RAS_FILTER_BIT_TONE_PCT);
            COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, CS_RAS_FILTER_BIT_TONE_PCT);
            // Tone_Quality_Indicator[k]
            // COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, CS_RAS_FILTER_BIT_TONE_QUALITY);
            COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, CS_RAS_FILTER_BIT_TONE_QUALITY);
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
        case CS_RAS_SUBEVENT_STEP_MODE_3:
            if (role == CS_RAS_ROLE_INITIATOR) {
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_FILTER_BIT_TOA_TOD);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT2);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_ANT_PERM_IDX);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, CS_RAS_FILTER_BIT_TONE_PCT);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, CS_RAS_FILTER_BIT_TONE_QUALITY);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_QUALITY);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_NADM);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_RSSI);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_FILTER_BIT_TOD_TOA);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_PKT_ANTENNA);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT1);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_FILTER_BIT_PKT_PCT2);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_FILTER_BIT_ANT_PERM_IDX);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 3, CS_RAS_FILTER_BIT_TONE_PCT);
                COPY_FIELD_IF_ENABLED((num_antenna_paths + 1) * 1, CS_RAS_FILTER_BIT_TONE_QUALITY);
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
        BT_LOGE("Invalid ras_srv environment, init it first.");
        return NULL;
    }

    for (int i = 0; i < CS_RAS_STORE_PROCEDURE_NUM_MAX; i++) {
        if (ras_srv->subevent[i].proc_used == false) {
            memset(&ras_srv->subevent[i], 0, sizeof(ras_rang_on_demand_t));
            ras_srv->subevent[i].proc_used = true;
            return &ras_srv->subevent[i];
        }
    }

    BT_LOGD("No subvent pool can be used.");
    return NULL;
}

static ras_rang_on_demand_t* cs_ras_rang_on_demand_find_subevent(bt_address_t* addr, uint16_t count)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv environment, init it first.");
        return NULL;
    }

    for (int i = 0; i < CS_RAS_STORE_PROCEDURE_NUM_MAX; i++) {
        if (ras_srv->subevent[i].count == count && ras_srv->subevent[i].proc_used == true) {
            return &ras_srv->subevent[i];
        }
    }

    BT_LOGD("No subvent find with count(%d).", count);
    return NULL;
}

static void ras_subevent_debug_info_print(bt_srv_conn_le_cs_subevent_result_t* result, uint8_t* stream_buf)
{
    if (!result) {
        BT_LOGE("result is NULL");
        return;
    }

    BT_LOGD("stream head");
    BT_DUMPBUFFER("head", stream_buf, CS_RAS_SUB_PROCUDURE_HEAD);
    BT_LOGD("stream_buf(%ld):", ras_srv->remaining_len);
    BT_DUMPBUFFER("buf", stream_buf + 12, ras_srv->remaining_len - CS_RAS_SUB_PROCUDURE_HEAD);
    BT_LOGD("data ready indicate count(%d)", result->header.procedure_counter);
    BT_LOGD("procedure_counter:0x%x, config_id:0x%x, reference_power_level:0x%x",
        result->header.procedure_counter, result->header.config_id,
        result->header.reference_power_level);
    BT_LOGD("num_antenna_paths:0x%x, start_acl_conn_event:0x%x, frequency_compensation:0x%x",
        result->header.num_antenna_paths, result->header.start_acl_conn_event_counter,
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
}

static uint8_t* ras_subevent_data_conversion(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    memset(ras_srv->latest_local_steps, 0, sizeof(ras_srv->latest_local_steps));

    if (result->len <= CS_RAS_STEP_DATA_BUF_LEN) {
        memcpy(ras_srv->latest_local_steps, result->step_data_buf,
            result->len);
        BT_LOGD("step data[%d]", result->len);
        BT_DUMPBUFFER("step data", result->step_data_buf, result->len);
    } else {
        BT_LOGD("Not enough memory to store step data. (%d > %d)",
            result->len, CS_RAS_STEP_DATA_BUF_LEN);
    }

    uint8_t* stream_buf = ras_srv->latest_local_steps;
    int bit_offset = 0;

     /**
     * Rangging Counter.
     * Rangging Counter is lower 12-bits of CS Procedure_Counter Provided by the Core Controller.
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.procedure_counter, 12);

     /**
     * Configuration ID.
     * Range: 0 to 3
     * CS configuration identifier.
     */
    ras_write_bits(stream_buf, &bit_offset, result->header.config_id, 4);

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
    ras_write_bits(stream_buf, &bit_offset, result->header.start_acl_conn_event_counter, 16);
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
        result->len, stream_buf + CS_RAS_SUB_PROCUDURE_HEAD,
        ras_srv->ras_filter, ras_srv->ras_role, result->header.num_antenna_paths & 0x0F);
    ras_srv->remaining_len += CS_RAS_SUB_PROCUDURE_HEAD;
    ras_subevent_debug_info_print(result, stream_buf);
    return stream_buf;
}

static void cs_ras_process_real_time_ranging_data(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    uint8_t* stream_buf = ras_subevent_data_conversion(addr, result);
    cs_ras_split_real_time_segment(addr, stream_buf, ras_srv->remaining_len);
}

static void cs_ras_process_on_demand_ranging_data(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    ras_rang_on_demand_t* subevent = ras_rang_on_demand_subevent_pool_find(addr);

    if (!subevent) {
        BT_LOGE("No subevent pool found.");
        return;
    }

    BT_LOGD("procedure counter:%d.", result->header.procedure_counter);

    subevent->count = result->header.procedure_counter;
    uint8_t* stream_buf = ras_subevent_data_conversion(addr, result);
    cs_ras_split_on_demand_segment(addr, stream_buf, ras_srv->remaining_len, subevent);
    if (ras_state_get_bit(&ras_srv->on_demand_state, CS_RAS_ON_DEMAND_STATE_IDLE)) {
        cs_ras_data_ready_send(addr, subevent->count);
        // Set the on-demand state to ready.
        ras_state_set_bit(&ras_srv->on_demand_state, CS_RAS_ON_DEMAND_STATE_DATA_READY_INDICATE);
    }

    return;
}

static void cs_ras_subevent_result_cb(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    if (result->header.procedure_done_status == BT_LE_SRV_CS_PROCEDURE_COMPLETE && ras_check_ranging_mode(addr) == CS_RAS_RANGING_MODE_REAL_TIME) {
        BT_LOGD("Recv the real-time ranging data.");
        cs_ras_process_real_time_ranging_data(addr, result);
        return;
    }

    if (result->header.procedure_done_status == BT_LE_SRV_CS_PROCEDURE_COMPLETE && ras_check_ranging_mode(addr) == CS_RAS_RANGING_MODE_ON_DEMAND) {
        BT_LOGD("Recv the on-demand ranging data.");
        cs_ras_process_on_demand_ranging_data(addr, result);
        return;
    }

    BT_LOGE("No mode has been set, discard the subevent result.");
    return;
}

static bt_status_t cs_ras_data_ready_send(bt_address_t* addr, uint16_t count)
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

static void cs_ras_mtu_updated_cb(bt_address_t* addr, uint32_t mtu)
{
    if (ras_srv) {
        ras_srv->ras_mtu = mtu;
    }

    BT_LOGD("Updated MTU, ras_mtu: %lu", ras_srv->ras_mtu);
    return;
}

static void cs_ras_gatts_ccc_cfg_cb(bt_address_t* addr, ras_ccc_cfg_change_evt_t event,
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

        STREAM_TO_UINT16(rtt_cfg_val, value);
        range_rtt_dt_ccc_cfg_changed(addr, rtt_cfg_val);
    } break;
    case RAS_ON_DEMAND_DATA_CCC_CFG_CHANGE_EVT: {
        uint16_t on_demand_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS On-demand CCC data config value length(%d).", length);
            return;
        }

        STREAM_TO_UINT16(on_demand_cfg_val, value);
        range_on_dem_dt_ccc_cfg_changed(addr, on_demand_cfg_val);
    } break;
    case RAS_CTR_PT_CCC_CFG_CHANGE_EVT: {
        uint16_t ctr_point_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS Control Point CCC data config value length(%d).", length);
            return;
        }

        STREAM_TO_UINT16(ctr_point_cfg_val, value);
        range_ctr_pt_ccc_cfg_changed(addr, ctr_point_cfg_val);
    } break;
    case RAS_DATA_READY_CCC_CFG_CHANGE_EVT: {
        uint16_t data_ready_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS data ready CCC data config value length(%d).", length);
            return;
        }

        STREAM_TO_UINT16(data_ready_cfg_val, value);
        range_dt_rd_ccc_cfg_changed(addr, data_ready_cfg_val);
    } break;
    case RAS_OVER_WRITE_CCC_CFG_CHANGE_EVT: {
        uint16_t over_write_cfg_val;

        if (length != 2) {
            BT_LOGE("Invalid RAS Over Write CCC data config value length(%d).", length);
            return;
        }

        STREAM_TO_UINT16(over_write_cfg_val, value);
        range_dt_ov_wr_ccc_cfg_changed(addr, over_write_cfg_val);
    } break;
    }
    return;
}

static void cs_ras_gatts_ctr_pt_write_cb(bt_address_t* addr,
    const uint8_t* value, uint16_t length)
{
    on_ras_ctr_pt_write_cb(addr, value, length);
    return;
}

static void cs_ras_gatts_feature_read_cb(bt_address_t* addr, uint32_t req_handle)
{
    BT_LOGI("feature read, req_handle:%lu", req_handle);
    if (!ras_srv) {
        BT_LOGE("RAS haven't init.");
        return;
    }

    ras_send_feature_read_rsp(addr, ras_srv->ras_feature, req_handle);
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
        if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE)) {
            ras_dt_rd_indicate_cb(addr);
        }
    } break;
    case RAS_ON_DEMAND_CHAR_SEND: {
        if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY)) {
            cs_ras_on_demand_notify_finished(addr);
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
    .cfg_cb = cs_ras_gatts_ccc_cfg_cb,
    .pt_write_cb = cs_ras_gatts_ctr_pt_write_cb,
    .feature_read_cb = cs_ras_gatts_feature_read_cb,
    .mtu_updated_cb = cs_ras_mtu_updated_cb,
    .notify_cb = ras_notify_cb,
    .conn_cb = ras_conn_cb,
    .disconn_cb = ras_disconn_cb,
};

int bt_cs_ras_enable(void)
{
    if (ras_srv) {
        BT_LOGW("CS RAS Profile already eanble.");
        return 0;
    }

    BT_LOGD("Enable Channel Sounding RAS Profile.");

    ras_srv = (ras_srv_env_t*)malloc(sizeof(ras_srv_env_t));

    if (!ras_srv) {
        BT_LOGE("ras_srv malloc failed.");
        return -1;
    }

    memset(ras_srv, 0, sizeof(ras_srv_env_t));

    ras_srv->ras_feature = 0x07000007;

    for (int i = 0; i < CS_RAS_FILTER_MODE_MAX; i++) {
        ras_srv->ras_filter[i] = 0xFFFFFFFF;
    }

    bt_cs_ras_gatts_init(&ras_cb);
    bt_cs_register_subevent_cb(cs_ras_subevent_result_cb);

    return 0;
}

int bt_cs_ras_disable(void)
{
    BT_LOGD("Disable Channel Sounding RAS Profile.");
    if (ras_srv) {
        free(ras_srv);
        ras_srv = NULL;
    }

    ras_gatts_deinit();
    return 0;
}

#ifdef CONFIG_BT_CS_RAS_TEST
int ras_subevent_recv_test(ras_rang_mode_t mode, ras_testcase_t test_case,
    bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    if (mode == CS_RAS_RANGING_MODE_REAL_TIME) {
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY);
        BT_LOGD("RAS Server test: set ranging to real-time mode.");
    }

    switch (test_case) {
    case RAS_TESTCASE_REAL_TIME_NOTIFY_VALID_RANGE_DATA_001:
        ras_srv->ras_mtu = 253;
        ras_srv->rt_dt_ccc_cfg = CS_RAS_GATT_NOTIFY;
        break;
    case RAS_TESTCASE_REAL_TIME_INDICATE_VALID_RANG_DATA_002:
        ras_srv->ras_mtu = 253;
        ras_srv->rt_dt_ccc_cfg = CS_RAS_GATT_INDICATION;
        break;
    case RAS_TESTCASE_ON_DEMAND_NOTIFY_VALID_RANG_DATA_003:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_state_set_bit(&ras_srv->on_demand_state, CS_RAS_ON_DEMAND_STATE_IDLE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_INDICATE_VALID_RANG_DATA_004:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_state_set_bit(&ras_srv->on_demand_state, CS_RAS_ON_DEMAND_STATE_IDLE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_WRITE_RANG_DATA_TIMEOUT_010:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_state_set_bit(&ras_srv->on_demand_state, CS_RAS_ON_DEMAND_STATE_IDLE);
        break;
    default:
        BT_LOGE("Invalid test case number(%d).", test_case);
        return -1;
    }

    cs_ras_subevent_result_cb(addr, result);
    return 0;
}

int bt_cs_ras_ctrl_point_send_test(bt_address_t* addr, uint8_t* data, uint16_t len)
{
    on_ras_ctr_pt_write_cb(addr, (void*)data, len);
    return 0;
}

void bt_cs_ras_on_demand_notify_finish_test(bt_address_t* addr)
{
    cs_ras_on_demand_notify_finished(addr);
}

void bt_cs_ras_on_demand_indicate_finish_test(bt_address_t* addr, ras_attr_notify_t attr)
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
#endif /* CONFIG_BT_CS_RAS_TEST */
