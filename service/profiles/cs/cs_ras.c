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

#include <inttypes.h>

#include "bt_status.h"
#include "bt_utils.h"
#include "cs_ras.h"
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

static void cs_ras_split_real_time_segment(bt_address_t* addr, ras_rt_queued_data_t* item);
static void cs_ras_rt_try_send_next(bt_address_t* addr);
static ras_od_procedure_t* cs_ras_od_find_procedure(bt_address_t* addr, uint16_t count);
static bt_status_t cs_ras_data_ready_send(bt_address_t* addr, uint16_t count);
static bt_status_t cs_ras_on_demand_send_cmp_ranging_data_rsp(bt_address_t* addr, uint16_t count);
static ssize_t on_ras_ctr_pt_write_cb(bt_address_t* addr, const void* buf, uint16_t len);

static uint32_t cs_ras_rt_queue_len(void)
{
    uint32_t count = 0;
    cs_node_t* cur;
    CS_GENLIST_FOR_EACH_NODE(list, &ras_srv->rt_queue, cur)
    {
        count++;
    }
    return count;
}

static bool cs_ras_rt_queue_remove_old(void)
{
    /* Remove one non-processing item to make room.
     * Returns true if an item was removed, false if nothing could be removed. */
    cs_node_t* prev = NULL;
    cs_node_t* cur;
    CS_GENLIST_FOR_EACH_NODE(list, &ras_srv->rt_queue, cur)
    {
        ras_rt_queued_data_t* item = container_of(cur, ras_rt_queued_data_t, node);
        if (!item->processing) {
            cs_list_remove(&ras_srv->rt_queue, prev, cur);
            BT_LOGD("RT remove_old: removed item");
            free(item);
            return true;
        }
        prev = cur;
    }

    BT_LOGW("RT remove_old: all items are processing, cannot remove.");
    return false;
}

static void cs_ras_rt_queue_free_all(void)
{
    ras_rt_queued_data_t *cur, *next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&ras_srv->rt_queue, cur, next, node)
    {
        cs_list_remove(&ras_srv->rt_queue, NULL, &cur->node);
        free(cur);
    }
}

static uint32_t cs_ras_od_list_len(void)
{
    uint32_t count = 0;
    cs_node_t* cur;
    CS_GENLIST_FOR_EACH_NODE(list, &ras_srv->od_procedure_list, cur)
    {
        count++;
    }
    return count;
}

static void cs_ras_od_free_procedure(ras_od_procedure_t* proc)
{
    if (!proc)
        return;

    /* Free all segments in the procedure */
    ras_segment_t *seg_cur, *seg_next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&proc->seg_list, seg_cur, seg_next, seg_node)
    {
        cs_list_remove(&proc->seg_list, NULL, &seg_cur->seg_node);
        free(seg_cur);
    }

    if (proc->on_demand_timer) {
        service_loop_cancel_timer(proc->on_demand_timer);
        proc->on_demand_timer = NULL;
    }

    free(proc);
}

static bool cs_ras_od_remove_old(void)
{
    /* Remove one non-processing procedure to make room.
     * Returns true if an item was removed, false if nothing could be removed. */
    cs_node_t* prev = NULL;
    cs_node_t* cur;
    CS_GENLIST_FOR_EACH_NODE(list, &ras_srv->od_procedure_list, cur)
    {
        ras_od_procedure_t* proc = container_of(cur, ras_od_procedure_t, node);
        if (!proc->processing) {
            cs_list_remove(&ras_srv->od_procedure_list, prev, cur);
            BT_LOGD("OD remove_old: removed procedure count=%d", proc->count);
            cs_ras_od_free_procedure(proc);
            return true;
        }
        prev = cur;
    }

    BT_LOGW("OD remove_old: all procedures are processing, cannot remove.");
    return false;
}

static void cs_ras_od_list_free_all(void)
{
    ras_od_procedure_t *cur, *next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&ras_srv->od_procedure_list, cur, next, node)
    {
        cs_list_remove(&ras_srv->od_procedure_list, NULL, &cur->node);
        cs_ras_od_free_procedure(cur);
    }
}

static void cs_ras_on_demand_notify_finished(bt_address_t* addr)
{
    cs_node_t* on_demand_pdu = (cs_node_t*)ras_srv->on_demand_curr_node;

    if (!on_demand_pdu) {
        BT_LOGD("Complete segment data sent.");
        /* Reset processing flag on the procedure that just finished */
        ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, ras_srv->procedure_count);
        if (!od_proc) {
            BT_LOGE("On-demand notify finished: procedure not found for count(%d), abort.",
                ras_srv->procedure_count);
            return;
        }
        od_proc->processing = false;
        cs_ras_on_demand_send_cmp_ranging_data_rsp(addr, ras_srv->procedure_count);
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
        /* Reset processing flag on failure so the procedure can be removed */
        ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, ras_srv->procedure_count);
        if (!od_proc) {
            BT_LOGE("On-demand notify error: procedure not found for count(%d), abort.",
                ras_srv->procedure_count);
            ras_srv->on_demand_curr_node = NULL;
            return;
        }
        od_proc->processing = false;
        return;
    }

    return;
}

static void ras_on_demand_indicate_finished(bt_address_t* addr)
{
    cs_node_t* on_demand_pdu = (cs_node_t*)ras_srv->on_demand_curr_node;

    if (!on_demand_pdu) {
        BT_LOGD("Complete segment data sent.");
        /* Reset processing flag on the procedure that just finished */
        ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, ras_srv->procedure_count);
        if (!od_proc) {
            BT_LOGE("On-demand indicate finished: procedure not found for count(%d), abort.",
                ras_srv->procedure_count);
            return;
        }
        od_proc->processing = false;
        cs_ras_on_demand_send_cmp_ranging_data_rsp(addr, ras_srv->procedure_count);
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
        /* Reset processing flag on failure so the procedure can be removed */
        ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, ras_srv->procedure_count);
        if (!od_proc) {
            BT_LOGE("On-demand indicate error: procedure not found for count(%d), abort.",
                ras_srv->procedure_count);
            ras_srv->on_demand_curr_node = NULL;
            return;
        }
        od_proc->processing = false;
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

    ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, count);

    if (!od_proc) {
        BT_LOGE("Haven't find procedure with count(%d).", count);
        return BT_STATUS_PARM_INVALID;
    }

    od_proc->processing = true;

    const cs_node_t* on_demand_pdu = cs_list_peek_head(&od_proc->seg_list);
    if (!on_demand_pdu) {
        BT_LOGE("Procedure count(%d) has no segments.", count);
        od_proc->processing = false;
        return BT_STATUS_FAIL;
    }

    struct ras_segment_t* seg = container_of(on_demand_pdu, ras_segment_t, seg_node);

    BT_LOGD("seg:%p, seg->data(%d)", seg, seg->len);
    BT_DUMPBUFFER("seg->data", seg->data, seg->len);

    if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY)) {
        status = BT_GATT_NOTIFY_CB(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("On-demand ranging data notify fail, status(%d).", status);
            od_proc->processing = false;
            return status;
        }

    } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        status = BT_GATT_INDICATE(RAS_ON_DEMAND_CHAR_SEND, addr, seg->data, seg->len);
        if (status != 0) {
            BT_LOGE("On-demand ranging data indicate fail, err(%d).", status);
            od_proc->processing = false;
            return status;
        }
    }

    // Set the current node to the next.
    ras_srv->on_demand_curr_node = on_demand_pdu->next;
    ras_srv->procedure_count = count;

    return status;
}

static bt_status_t cs_ras_on_demand_send_cmp_ranging_data_rsp(bt_address_t* addr, uint16_t count)
{
    ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, count);

    if (!od_proc) {
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

    ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_COMPLETE;

    return BT_STATUS_SUCCESS;
}

static bt_status_t ras_on_demand_send_code_rsp(bt_address_t* addr, uint16_t count)
{
    ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, count);

    if (!od_proc) {
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

    /* Free the on-demand procedure and remove from list */
    if (od_proc) {
        if (od_proc->processing) {
            BT_LOGW("ACK for procedure count=%d but it is still processing, defer free.", count);
        } else {
            cs_node_t* prev = NULL;
            cs_node_t* cur;
            CS_GENLIST_FOR_EACH_NODE(list, &ras_srv->od_procedure_list, cur)
            {
                if (cur == &od_proc->node) {
                    cs_list_remove(&ras_srv->od_procedure_list, prev, cur);
                    break;
                }
                prev = cur;
            }
            cs_ras_od_free_procedure(od_proc);
        }
    }

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
    ras_od_procedure_t* od_proc = cs_ras_od_find_procedure(addr, count);
    if (!od_proc) {
        BT_LOGE("On-demand retrieve lost data failed: procedure not found for count(%u).", count);
        return BT_STATUS_PARM_INVALID;
    }

    bt_status_t status;

    ras_segment_t *seg_prev, *seg_next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&od_proc->seg_list,
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
    uint16_t current_filter_mask = ras_srv->ras_filter[mode];

    // Check if the specific filter bit is set
    // We shift the filter_bit into the correct position and mask it to check
    return (current_filter_mask & (1 << filter_bit)) != 0;
}

static void ras_set_filter(uint16_t filter_value)
{
    // Extract the mode (bits 0-1)
    uint16_t mode = filter_value & CS_RAS_FILTER_MODE_MASK;

    // Use the mode directly as an index into ras_filter
    if (mode < CS_RAS_FILTER_MODE_MAX) {
        ras_srv->ras_filter[mode] = filter_value;
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

    /* Find the current item being sent from the head of rt_queue */
    cs_node_t* head = cs_list_peek_head(&ras_srv->rt_queue);
    if (!head) {
        BT_LOGD("RT queue empty after indication.");
        return;
    }

    ras_rt_queued_data_t* item = container_of(head, ras_rt_queued_data_t, node);
    if (item->remaining > 0) {
        /* Continue sending the current item */
        cs_ras_split_real_time_segment(addr, item);
    } else {
        /* Current item fully sent, free it and try next */
        cs_list_remove(&ras_srv->rt_queue, NULL, head);
        free(item);
        cs_ras_rt_try_send_next(addr);
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

static void cs_ras_split_real_time_segment(bt_address_t* addr, ras_rt_queued_data_t* item)
{
    if (ras_srv->ras_mtu <= RAS_SEG_HEADER_SIZE + 1) {
        BT_LOGE("%s, ras_mtu(%" PRIu32 ") too small, minimum required: %d",
            __func__, ras_srv->ras_mtu, RAS_SEG_HEADER_SIZE + 2);
        return;
    }

    while (item->remaining > 0) {
        uint8_t* buf = &item->data[item->seg_offset];

        if (item->remaining > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
            int curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
            BT_LOGD("data send: offset:%" PRIu32 ", len:%d", item->seg_offset, curr_seg_size);

            item->seg_offset += curr_seg_size;
            uint8_t* send_buf = zalloc(curr_seg_size + 1);
            if (!send_buf) {
                BT_LOGE("Malloc send_buf fail, size=%d.", curr_seg_size + 1);
                return;
            }

            item->remaining -= curr_seg_size;
            send_buf[0] = (item->seg_idx == 0) ? (0x01) : (item->seg_idx << 2);

            item->seg_idx++;
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
                    free(send_buf);
                    /* Notify mode: loop continues to send next segment */
                    continue;
                } else {
                    BT_LOGD("ras data ready Notify fail.");
                    free(send_buf);
                    return;
                }
            } else if (ras_srv->rt_dt_ccc_cfg == CS_RAS_GATT_INDICATION) {
                if (BT_GATT_INDICATE(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                    ras_srv->ras_dt_rd_indicating = 1U;
                    free(send_buf);
                    /* Indication mode: wait for ras_dt_rd_indicate_cb to continue */
                    return;
                } else {
                    BT_LOGD("ras data ready Indicate fail.");
                    free(send_buf);
                    return;
                }
            } else {
                BT_LOGE("Invalid range data ccc config state:0x%x", ras_srv->rt_dt_ccc_cfg);
                free(send_buf);
                return;
            }
        } else {
            /* Last segment */
            int curr_seg_size = item->remaining;
            uint8_t* send_buf = zalloc(curr_seg_size + 1);
            if (!send_buf) {
                BT_LOGE("Malloc send_buf fail, size=%d.", curr_seg_size + 1);
                return;
            }
            send_buf[0] = (item->seg_idx == 0) ? (0x01) : (item->seg_idx << 2);
            send_buf[0] |= (0x01 << 1); /* Mark as last segment */
            memcpy(&send_buf[1], buf, curr_seg_size);

            item->seg_idx = 0;
            item->remaining = 0;
            item->seg_offset = 0;
            ras_srv->ras_dt_rd_indicating = 0U;
            BT_LOGD("The last(%d) seg, data(%d)", send_buf[0] >> 2, curr_seg_size + 1);
            BT_DUMPBUFFER("seg->data", send_buf, curr_seg_size + 1);
            BT_LOGD("ras_dt_rd_indicating:%d", ras_srv->ras_dt_rd_indicating);
            if (ras_srv->rt_dt_ccc_cfg == CS_RAS_GATT_NOTIFY) {
                if (BT_GATT_NOTIFY(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                    free(send_buf);
                    /* Notify mode: cleanup is handled by ras_notify_cb */
                } else {
                    BT_LOGD("ras data ready Notify fail.");
                    free(send_buf);
                    return;
                }
            } else if (ras_srv->rt_dt_ccc_cfg == CS_RAS_GATT_INDICATION) {
                if (BT_GATT_INDICATE(RAS_REAL_TIME_CHAR_SEND, addr, send_buf, curr_seg_size + 1) == 0) {
                    ras_srv->ras_dt_rd_indicating = 1U;
                    free(send_buf);
                    /* Indication mode: ras_dt_rd_indicate_cb will handle completion */
                } else {
                    BT_LOGD("ras data ready Indicate fail.");
                    free(send_buf);
                    return;
                }
            } else {
                BT_LOGE("Invalid range data ccc config state:0x%x", ras_srv->rt_dt_ccc_cfg);
                free(send_buf);
            }
            /* Last segment sent, loop exits since remaining == 0 */
        }
    }
}

/**
 * @brief Try to start sending the next item in the real-time queue.
 *
 * Called after the current item finishes sending (all segments done).
 */
static void cs_ras_rt_try_send_next(bt_address_t* addr)
{
    cs_node_t* head = cs_list_peek_head(&ras_srv->rt_queue);
    if (!head)
        return;

    ras_rt_queued_data_t* item = container_of(head, ras_rt_queued_data_t, node);
    if (item->processing)
        return;

    item->processing = true;
    BT_LOGD("RT: start sending next queued item, data_len=%" PRIu32, item->data_len);
    cs_ras_split_real_time_segment(addr, item);
}

static void ras_on_demand_data_send_timeout(service_timer_t* timer, void* data)
{
    ras_od_procedure_t* od_proc = (ras_od_procedure_t*)timer->userdata;
    BT_LOGD("On-demand data send timeout, remove the data in the list.");

    if (!od_proc)
        return;

    /* Do not free a procedure that is currently being sent */
    if (od_proc->processing) {
        BT_LOGW("On-demand timeout: procedure count=%d is processing, skip free.", od_proc->count);
        od_proc->on_demand_timer = NULL;
        return;
    }

    /* Remove from the od_procedure_list */
    cs_node_t* prev = NULL;
    cs_node_t* cur;
    CS_GENLIST_FOR_EACH_NODE(list, &ras_srv->od_procedure_list, cur)
    {
        if (cur == &od_proc->node) {
            cs_list_remove(&ras_srv->od_procedure_list, prev, cur);
            break;
        }
        prev = cur;
    }

    od_proc->on_demand_timer = NULL; /* Prevent double-cancel in free */
    cs_ras_od_free_procedure(od_proc);
    return;
}

static void cs_ras_split_on_demand_segment(bt_address_t* addr, uint8_t* buf, int len,
    ras_od_procedure_t* proc, bool is_last_subevent)
{
    if (ras_srv->ras_mtu <= RAS_SEG_HEADER_SIZE + 1) {
        BT_LOGE("%s, ras_mtu(%" PRIu32 ") too small, minimum required: %d",
            __func__, ras_srv->ras_mtu, RAS_SEG_HEADER_SIZE + 2);
        return;
    }

    int curr_seg_size = 0;
    uint32_t seg_offset = 0;
    uint32_t remaining = len;

    do {
        if (remaining > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
            curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
            BT_LOGD("data send: offset:%" PRIu32 ", len:%d", seg_offset, curr_seg_size);
            proc->seg = (ras_segment_t*)malloc(sizeof(ras_segment_t) + curr_seg_size + 1);

            if (!proc->seg) {
                BT_LOGE("Malloc fail.");
                return;
            }

            memcpy(&proc->seg->data[1], &buf[seg_offset], curr_seg_size);
            seg_offset += curr_seg_size;
            remaining -= curr_seg_size;

            /* Non-last segment: set first flag only for the very first segment of the procedure */
            proc->seg->data[0] = (proc->next_seg_index == 0) ? (0x01) : (proc->next_seg_index << 2);
        } else {
            curr_seg_size = remaining;
            proc->seg = (ras_segment_t*)malloc(sizeof(ras_segment_t) + curr_seg_size + 1);

            if (!proc->seg) {
                BT_LOGE("Malloc fail.");
                return;
            }

            memcpy(&proc->seg->data[1], &buf[seg_offset], curr_seg_size);

            proc->seg->data[0] = (proc->next_seg_index == 0) ? (0x01) : (proc->next_seg_index << 2);
            /* Only set last flag when this is the final subevent of the procedure */
            if (is_last_subevent) {
                proc->seg->data[0] |= (0x01 << 1);
            }
            remaining = 0;
            BT_LOGD("ras_dt_rd_indicating:%d", ras_srv->ras_dt_rd_indicating);
        }

        if ((proc->seg->data[0] & 0x01) == 0x01) {
            BT_LOGD("First seg, data(%d)", curr_seg_size + 1);
            BT_DUMPBUFFER("seg->data", proc->seg->data, curr_seg_size + 1);
        } else {
            BT_LOGD("The %d seg, data(%d)", proc->seg->data[0] >> 2, curr_seg_size + 1);
            BT_DUMPBUFFER("seg->data", proc->seg->data, curr_seg_size + 1);
        }

        proc->seg->seg_idx = proc->next_seg_index++;
        proc->seg->len = curr_seg_size + 1;
        cs_list_append(&proc->seg_list, &proc->seg->seg_node);
        BT_LOGD("proc seg:%p, seg_node:%p.", proc->seg, &proc->seg->seg_node);
    } while (remaining > 0);

    /* Only start the timeout timer when the procedure is complete */
    if (is_last_subevent) {
        if (!proc->on_demand_timer) {
            proc->on_demand_timer = service_loop_timer(RAS_RSP_TIMEOUT, false, ras_on_demand_data_send_timeout, proc);
        } else {
            BT_LOGE("The on demand timer already exit. procedure count:%d.", proc->count);
        }
    }
    return;
}

/**
 * Input: data points to the original buffer, data_len is the total length of the input
 * Output: buf stores the transformed result, function returns the length of the output buffer
 */
static size_t transform_step_data_to_ras_format_filtered(
    uint8_t* data, size_t data_len,
    uint8_t* buf, uint16_t* ras_filter, uint8_t role,
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
        uint16_t filter = ras_filter[step_mode];

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
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_0_FILTER_PACKET_QUALITY_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_0_FILTER_PACKET_RSSI_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_0_FILTER_PACKET_ANTENNA_BIT);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_MODE_0_FILTER_MEASURED_FREQ_OFFSET_BIT);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_0_FILTER_PACKET_QUALITY_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_0_FILTER_PACKET_RSSI_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_0_FILTER_PACKET_ANTENNA_BIT);
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
            if (role == CS_RAS_ROLE_INITIATOR) {
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_QUALITY_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_NADM_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_RSSI_BIT);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_MODE_1_FILTER_TOD_TOA_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_ANTENNA_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_1_FILTER_PACKET_PCT_1_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_1_FILTER_PACKET_PCT_2_BIT);
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_QUALITY_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_NADM_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_RSSI_BIT);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_MODE_1_FILTER_TOD_TOA_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_1_FILTER_PACKET_ANTENNA_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_1_FILTER_PACKET_PCT_1_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_1_FILTER_PACKET_PCT_2_BIT);
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
            COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_2_FILTER_ANTENNA_PERMUTATION_INDEX_BIT);

            while (remaining > 0) {
                COPY_FIELD_IF_ENABLED(3, CS_RAS_MODE_2_FILTER_TONE_PCT_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_2_FILTER_TONE_QUALITY_INDICATOR_BIT);
            }
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
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_QUALITY_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_NADM_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_RSSI_BIT);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_MODE_3_FILTER_TOD_TOA_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_ANTENNA_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_3_FILTER_PACKET_PCT_1_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_3_FILTER_PACKET_PCT_2_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_ANTENNA_PERMUTATION_INDEX_BIT);
                while (remaining > 0) {
                    COPY_FIELD_IF_ENABLED(3, CS_RAS_MODE_3_FILTER_TONE_PCT_BIT);
                    COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_TONE_QUALITY_INDICATOR_BIT);
                }
            } else { // Reflector
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_QUALITY_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_NADM_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_RSSI_BIT);
                COPY_FIELD_IF_ENABLED(2, CS_RAS_MODE_3_FILTER_TOD_TOA_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_PACKET_ANTENNA_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_3_FILTER_PACKET_PCT_1_BIT);
                COPY_FIELD_IF_ENABLED(4, CS_RAS_MODE_3_FILTER_PACKET_PCT_2_BIT);
                COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_ANTENNA_PERMUTATION_INDEX_BIT);
                while (remaining > 0) {
                    COPY_FIELD_IF_ENABLED(3, CS_RAS_MODE_3_FILTER_TONE_PCT_BIT);
                    COPY_FIELD_IF_ENABLED(1, CS_RAS_MODE_3_FILTER_TONE_QUALITY_INDICATOR_BIT);
                }
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

static ras_od_procedure_t* cs_ras_od_find_procedure(bt_address_t* addr, uint16_t count)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv environment, init it first.");
        return NULL;
    }

    ras_od_procedure_t *cur, *next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&ras_srv->od_procedure_list, cur, next, node)
    {
        if (cur->count == count) {
            return cur;
        }
    }

    BT_LOGD("No procedure find with count(%d).", count);
    return NULL;
}

static ras_od_procedure_t* cs_ras_od_alloc_procedure(bt_address_t* addr, uint16_t count)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv environment, init it first.");
        return NULL;
    }

    /* Check if we already have a procedure with this count */
    ras_od_procedure_t* existing = cs_ras_od_find_procedure(addr, count);
    if (existing)
        return existing;

    /* Remove old if at max capacity */
    while (cs_ras_od_list_len() >= RAS_OD_PROCEDURE_MAX) {
        if (!cs_ras_od_remove_old()) {
            BT_LOGE("Cannot remove any procedure (all processing), dropping new data.");
            return NULL;
        }
    }

    ras_od_procedure_t* proc = (ras_od_procedure_t*)zalloc(sizeof(ras_od_procedure_t));
    if (!proc) {
        BT_LOGE("Malloc ras_od_procedure_t fail.");
        return NULL;
    }

    proc->count = count;
    cs_list_init(&proc->seg_list);
    cs_list_append(&ras_srv->od_procedure_list, &proc->node);

    return proc;
}

static size_t ras_subevent_data_conversion(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result,
    uint8_t* stream_buf, size_t buf_size)
{
    memset(stream_buf, 0, buf_size);

    int bit_offset = 0;

    /**
     * Ranging Counter.
     * Ranging Counter is lower 12-bits of CS Procedure_Counter Provided by the Core Controller.
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
     * Antenna Paths Mask (8 bits)
     * Bit0: 1 if Antenna Path_1 included; 0 if not.
     * Bit1: 1 if Antenna Path_2 included; 0 if not.
     * Bit2: 1 if Antenna Path_3 included; 0 if not.
     * Bit3: 1 if Antenna Path_4 included; 0 if not.
     * Bits 4-7: RFU
     *
     * Note: HCI provides num_antenna_paths as a count (1-4) of antenna paths used.
     * The RAS Antenna_Paths_Mask sets bits 0..(N-1) for N antenna paths.
     * Example: num_antenna_paths=4 means paths 1-4 used, mask=0x0F (bits 0-3)
     */
    uint8_t antenna_paths_mask = 0;
    if (result->header.num_antenna_paths >= 1 && result->header.num_antenna_paths <= 4) {
        // Convert count to bitmask: count=1->0x01, count=2->0x03, count=3->0x07, count=4->0x0F
        antenna_paths_mask = (1 << result->header.num_antenna_paths) - 1;
    }
    ras_write_bits(stream_buf, &bit_offset, antenna_paths_mask, 8);
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

    size_t data_len = transform_step_data_to_ras_format_filtered(result->step_data_buf,
        result->len, stream_buf + CS_RAS_SUB_PROCUDURE_HEAD,
        ras_srv->ras_filter, ras_srv->ras_role, result->header.num_antenna_paths);
    data_len += CS_RAS_SUB_PROCUDURE_HEAD;

    /* Debug print */
    BT_LOGD("stream head");
    BT_DUMPBUFFER("head", stream_buf, CS_RAS_SUB_PROCUDURE_HEAD);
    BT_LOGD("stream_buf(%zu):", data_len);
    BT_DUMPBUFFER("buf", stream_buf + 12, data_len - CS_RAS_SUB_PROCUDURE_HEAD);
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

    return data_len;
}

static void cs_ras_process_real_time_ranging_data(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    /* Allocate buffer based on input size: header + step data */
    size_t buf_size = result->len + CS_RAS_SUB_PROCUDURE_HEAD;
    uint8_t* tmp_buf = (uint8_t*)zalloc(buf_size);
    if (!tmp_buf) {
        BT_LOGE("Malloc tmp_buf fail, size=%zu.", buf_size);
        return;
    }

    size_t data_len = ras_subevent_data_conversion(addr, result, tmp_buf, buf_size);

    /* Remove old if queue is full */
    while (cs_ras_rt_queue_len() >= RAS_RT_QUEUE_MAX) {
        BT_LOGD("RT queue full (%" PRIu32 "), removing old.", cs_ras_rt_queue_len());
        if (!cs_ras_rt_queue_remove_old()) {
            BT_LOGE("Cannot remove any RT item (all processing), dropping new data.");
            free(tmp_buf);
            return;
        }
    }

    /* Malloc a queued item with the data copy */
    ras_rt_queued_data_t* item = (ras_rt_queued_data_t*)zalloc(sizeof(ras_rt_queued_data_t) + data_len);
    if (!item) {
        BT_LOGE("Malloc ras_rt_queued_data_t fail, len=%zu.", data_len);
        free(tmp_buf);
        return;
    }

    item->data_len = data_len;
    item->remaining = data_len;
    item->seg_offset = 0;
    item->seg_idx = 0;
    item->processing = false;
    memcpy(item->data, tmp_buf, data_len);
    free(tmp_buf);

    /* Append to queue */
    cs_list_append(&ras_srv->rt_queue, &item->node);
    BT_LOGD("RT: enqueued item, data_len=%zu", data_len);

    /* If not currently sending, start sending from head */
    cs_ras_rt_try_send_next(addr);
}

static void cs_ras_process_on_demand_ranging_data(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result, bool procedure_complete)
{
    ras_od_procedure_t* proc = cs_ras_od_alloc_procedure(addr, result->header.procedure_counter);

    if (!proc) {
        BT_LOGE("No on-demand procedure slot available.");
        return;
    }

    BT_LOGD("procedure counter:%d.", result->header.procedure_counter);

    /* Allocate buffer based on input size: header + step data */
    size_t buf_size = result->len + CS_RAS_SUB_PROCUDURE_HEAD;
    uint8_t* tmp_buf = (uint8_t*)zalloc(buf_size);
    if (!tmp_buf) {
        BT_LOGE("Malloc tmp_buf fail, size=%zu.", buf_size);
        return;
    }

    size_t data_len = ras_subevent_data_conversion(addr, result, tmp_buf, buf_size);

    cs_ras_split_on_demand_segment(addr, tmp_buf, data_len, proc, procedure_complete);
    free(tmp_buf);

    /* Only notify data ready when the procedure is complete */
    if (procedure_complete && ras_srv->on_demand_state == CS_RAS_ON_DEMAND_STATE_IDLE) {
        cs_ras_data_ready_send(addr, proc->count);
        ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_BUSY;
    }

    return;
}

static void cs_ras_subevent_result_cb(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result)
{
    if (result->header.subevent_done_status == BT_LE_SRV_CS_SUBEVENT_COMPLETE && ras_check_ranging_mode(addr) == CS_RAS_RANGING_MODE_REAL_TIME) {
        BT_LOGD("Recv the real-time ranging data.");
        cs_ras_process_real_time_ranging_data(addr, result);
        return;
    }

    if (ras_check_ranging_mode(addr) == CS_RAS_RANGING_MODE_ON_DEMAND) {
        bool proc_complete = (result->header.procedure_done_status == BT_LE_SRV_CS_PROCEDURE_COMPLETE);
        if (proc_complete) {
            BT_LOGD("Recv the on-demand ranging data (procedure complete).");
        } else {
            BT_LOGD("Recv on-demand subevent (procedure partial), storing.");
        }
        cs_ras_process_on_demand_ranging_data(addr, result, proc_complete);
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
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv value.");
        return;
    }

    ras_srv->ras_mtu = mtu;
    BT_LOGD("Updated MTU, ras_mtu: %" PRIu32, ras_srv->ras_mtu);
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
    BT_LOGI("feature read, req_handle:%" PRIu32, req_handle);
    if (!ras_srv) {
        BT_LOGE("RAS haven't init.");
        return;
    }

    ras_send_feature_read_rsp(addr, ras_srv->ras_feature, req_handle);
    return;
}

static void ras_rang_on_demand_send_ready(bt_address_t* addr)
{
    if (!ras_srv) {
        BT_LOGE("Invalid ras_srv environment, init it first.");
        return;
    }

    /* Find the procedure with the smallest count (oldest) */
    uint16_t min_count = 0xFFFF;
    ras_od_procedure_t *cur, *next;
    CS_LIST_FOR_EACH_CONTAINER_SAFE(&ras_srv->od_procedure_list, cur, next, node)
    {
        if (cur->count < min_count) {
            min_count = cur->count;
        }
    }

    if (min_count < 0xFFFF && cs_ras_data_ready_send(addr, min_count) == BT_STATUS_SUCCESS) {
        ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_BUSY;
        return;
    }
}

static void ras_notify_cb(bt_address_t* addr, gatt_status_t status, ras_attr_notify_t attr)
{
    if (!ras_srv) {
        BT_LOGE("ras_srv is NULL, service may have been disabled.");
        return;
    }

    if (status != GATT_STATUS_SUCCESS) {
        BT_LOGE("Notify fail, status(%d)", status);
        return;
    }

    BT_LOGI("ras notify cb, attr:%d", attr);
    switch (attr) {
    case RAS_REAL_TIME_CHAR_SEND: {
        if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE)) {
            ras_dt_rd_indicate_cb(addr);
        } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY)) {
            /* Notify mode: check if current item is done, dequeue next */
            cs_node_t* head = cs_list_peek_head(&ras_srv->rt_queue);
            if (head) {
                ras_rt_queued_data_t* item = container_of(head, ras_rt_queued_data_t, node);
                if (item->remaining == 0) {
                    cs_list_remove(&ras_srv->rt_queue, NULL, head);
                    free(item);
                    cs_ras_rt_try_send_next(addr);
                }
            }
        }
    } break;
    case RAS_ON_DEMAND_CHAR_SEND: {
        if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY)) {
            cs_ras_on_demand_notify_finished(addr);
        } else if (ras_state_get_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
            ras_on_demand_indicate_finished(addr);
        }
    } break;
    case RAS_CONTROL_POINT_CHAR_SEND: {
        if (ras_srv->on_demand_state == CS_RAS_ON_DEMAND_STATE_COMPLETE) {
            ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_IDLE;
            ras_rang_on_demand_send_ready(addr);
        }
    } break;
    default:
        break;
    }

    return;
}

static void ras_conn_cb(bt_address_t* addr)
{
    if (!ras_srv) {
        BT_LOGE("ras_srv is NULL, service may have been disabled.");
        return;
    }

    if (ras_srv->addr) {
        BT_LOGE("The connection has been created.");
        return;
    }

    ras_srv->addr = (bt_address_t*)malloc(sizeof(bt_address_t));
    if (!ras_srv->addr) {
        BT_LOGE("Failed to allocate address.");
        return;
    }

    memcpy(ras_srv->addr, addr, sizeof(bt_address_t));
    BT_LOGD("RAS Conn to address:%s", bt_addr_str(addr));
    ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_IDLE;
    return;
}

static void ras_disconn_cb(bt_address_t* addr)
{
    if (!ras_srv) {
        BT_LOGE("ras_srv is NULL, service may have been disabled.");
        return;
    }

    if (ras_srv->addr == NULL) {
        BT_LOGE("The connection has been release.");
        return;
    }

    BT_LOGD("RAS disconn to address:%s", bt_addr_str(addr));

    /* Clean up real-time queue */
    cs_ras_rt_queue_free_all();

    /* Clean up on-demand procedure list */
    cs_ras_od_list_free_all();

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
        ras_srv->ras_filter[i] = 0xFFFF; // All 16 bits set to 1 (all fields enabled)
    }

    /* Initialize real-time queue */
    cs_list_init(&ras_srv->rt_queue);

    /* Initialize on-demand procedure list */
    cs_list_init(&ras_srv->od_procedure_list);

    bt_cs_ras_gatts_init(&ras_cb);
    bt_cs_register_subevent_cb(cs_ras_subevent_result_cb);

    return 0;
}

int bt_cs_ras_disable(void)
{
    BT_LOGD("Disable Channel Sounding RAS Profile.");
    if (ras_srv) {
        cs_ras_rt_queue_free_all();
        cs_ras_od_list_free_all();
        if (ras_srv->addr) {
            free(ras_srv->addr);
            ras_srv->addr = NULL;
        }
        free(ras_srv);
        ras_srv = NULL;
    }

    ras_gatts_deinit();
    return 0;
}

bt_status_t bt_cs_ras_set_feature(uint32_t feature)
{
    if (!ras_srv) {
        BT_LOGE("RAS server not initialized.");
        return BT_STATUS_NOT_READY;
    }

    ras_srv->ras_feature = feature;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_cs_ras_set_role(uint8_t role)
{
    if (!ras_srv) {
        BT_LOGE("RAS server not initialized.");
        return BT_STATUS_NOT_READY;
    }

    // Convert from bt_cs_set_params_t role format to RAS role
    // role bit 0: initiator, bit 1: reflector
    if (role & 0x01) {
        ras_srv->ras_role = CS_RAS_ROLE_INITIATOR;
    } else if (role & 0x02) {
        ras_srv->ras_role = CS_RAS_ROLE_REFLECTOR;
    } else {
        // Default to initiator if no role specified
        ras_srv->ras_role = CS_RAS_ROLE_REFLECTOR;
    }

    BT_LOGD("RAS role set to: %s", ras_srv->ras_role == CS_RAS_ROLE_INITIATOR ? "Initiator" : "Reflector");

    return BT_STATUS_SUCCESS;
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
        ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_IDLE;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_INDICATE_VALID_RANG_DATA_004:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_IDLE;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_WRITE_RANG_DATA_TIMEOUT_010:
        ras_srv->ras_mtu = 253;
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        ras_state_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        ras_srv->on_demand_state = CS_RAS_ON_DEMAND_STATE_IDLE;
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
