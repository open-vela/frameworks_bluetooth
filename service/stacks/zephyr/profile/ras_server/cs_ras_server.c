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

#include "cs_ras_server.h"
#include "bt_addr.h"
#include "cs_service.h"
#include "profiles/cs/cs_msg.h"
#include <common/bt_str.h>
#include <math.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#ifdef CONFIG_BLUETOOTH_LE_CS

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#define CS_CONFIG_ID 0
#define NUM_MODE_0_STEPS 1
#define RAS_SEG_HEADER_SIZE 4

static const char sample_str[] = "CS Sample111";
static const struct bt_data ad[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, "CS Sample111", sizeof(sample_str) - 1),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(0x185B)),
};

static sal_le_ras_srv_env_t* ras_srv;

/** @brief LE Audio Attribute User Data. */
struct bt_ras_attr_user_data {
    /** Attribute read callback */
    ssize_t (*read)(struct bt_conn* conn, const struct bt_gatt_attr* attr,
        void* buf, uint16_t len, uint16_t offset);

    /** Attribute write callback */
    ssize_t (*write)(struct bt_conn* conn, const struct bt_gatt_attr* attr,
        const void* buf, uint16_t len, uint16_t offset,
        uint8_t flags);

    /** Attribute user data */
    void* user_data;
};

static void split_real_time_segment(struct bt_conn* conn, uint8_t* buf, int len);
static ssize_t ras_feature_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    void* buf, uint16_t len, uint16_t offset);
static void range_rt_dt_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value);
static void range_on_dem_dt_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value);
ssize_t on_ras_ctr_pt_write_cb(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    const void* buf, uint16_t len, uint16_t offset,
    uint8_t flags);
static void range_dt_rd_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value);
static void range_dt_ov_wr_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value);
static void range_ctr_pt_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value);
static ras_rang_on_demand_t* ras_rang_on_demand_find_subevent(struct bt_conn* conn, uint16_t count);
static int ras_data_ready_send(struct bt_conn* conn, uint16_t count);
static int ras_on_demond_send_cmp_ranging_data_rsp(struct bt_conn* conn, uint16_t count);

#define BT_RAS_ATTR_USER_DATA_INIT(_read, _write, _user_data) \
    {                                                         \
        .read = _read,                                        \
        .write = _write,                                      \
        .user_data = _user_data,                              \
    }

/** Helper to define LE RAS characteristic. */
#define BT_RAS_CHRC(_uuid, _props, _perm, _read, _write, _user_data) \
    BT_GATT_CHARACTERISTIC(_uuid, _props, _perm, _read, _write,      \
        ((struct bt_ras_attr_user_data[]) {                          \
            BT_RAS_ATTR_USER_DATA_INIT(_read, _write, _user_data),   \
        }))

#define BT_RAS_CHRC_USER_DATA(_attr) \
    (((struct bt_ras_attr_user_data*)(_attr)->user_data)->user_data)

/** Helper to define LE Audio CCC descriptor. */
#define BT_RAS_CCC(_changed)                                                                         \
    BT_GATT_CCC_MANAGED(((struct _bt_gatt_ccc[]) { BT_GATT_CCC_INITIALIZER(_changed, NULL, NULL) }), \
        (BT_GATT_PERM_READ))

#define BT_RAS_SERVICE_DEFINITION()                                                                 \
    {                                                                                               \
        BT_GATT_PRIMARY_SERVICE(BT_UUID_RANGING),                                                   \
            BT_RAS_CHRC(BT_UUID_RANG_FEAT,                                                          \
                BT_GATT_CHRC_READ,                                                                  \
                BT_GATT_PERM_READ,                                                                  \
                ras_feature_read, NULL, NULL),                                                      \
            BT_RAS_CHRC(BT_UUID_RANG_RT_DT,                                                         \
                (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE),                                      \
                BT_GATT_PERM_READ,                                                                  \
                NULL, NULL, NULL),                                                                  \
            BT_GATT_CCC(range_rt_dt_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),     \
            BT_RAS_CHRC(BT_UUID_RANG_ON_DEM_DT,                                                     \
                (BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE),                                      \
                BT_GATT_PERM_READ,                                                                  \
                NULL, NULL, NULL),                                                                  \
            BT_GATT_CCC(range_on_dem_dt_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)), \
            BT_RAS_CHRC(BT_UUID_RANG_RAS_CTR_POINT,                                                 \
                (BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_INDICATE),                          \
                (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),                                           \
                NULL, on_ras_ctr_pt_write_cb, NULL),                                                \
            BT_GATT_CCC(range_ctr_pt_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),    \
            BT_RAS_CHRC(BT_UUID_RANG_DT_RD,                                                         \
                BT_GATT_CHRC_INDICATE,                                                              \
                BT_GATT_PERM_READ,                                                                  \
                NULL, NULL, NULL),                                                                  \
            BT_GATT_CCC(range_dt_rd_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),     \
            BT_RAS_CHRC(BT_UUID_RANG_DT_OV_WR,                                                      \
                BT_GATT_CHRC_INDICATE,                                                              \
                BT_GATT_PERM_READ,                                                                  \
                NULL, NULL, NULL),                                                                  \
            BT_GATT_CCC(range_dt_ov_wr_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),  \
    }

static struct bt_gatt_attr ras_attrs[RAS_IDX_MAX] = BT_RAS_SERVICE_DEFINITION();
static struct bt_gatt_service ras_svc = (struct bt_gatt_service)BT_GATT_SERVICE(ras_attrs);

static void ras_on_demand_notify_finished(struct bt_conn* conn, void* user_data)
{
    sys_snode_t* on_deman_pdu = (sys_snode_t*)user_data;

    if (!on_deman_pdu) {
        LOG_INF("Compelete segment data sent.");
        ras_on_demond_send_cmp_ranging_data_rsp(conn, 10);
        return;
    }

    ras_segment_t* seg = CONTAINER_OF(on_deman_pdu, ras_segment_t, seg_node);

    if (!seg) {
        LOG_WRN("Invalid segment.");
        return;
    }

    LOG_INF("seg:%p, seg->data(%d):%s", seg, seg->len, bt_hex(seg->data, seg->len));

    struct bt_gatt_notify_params notify_param = {
        .attr = &ras_attrs[SAL_LE_RAS_ON_DEM_CHAR_IDX],
        .func = ras_on_demand_notify_finished,
        .user_data = (void*)on_deman_pdu->next,
        .data = seg->data,
        .len = seg->len,
    };

    int err = BT_GATT_NOTIFY_CB(conn, &notify_param);
    if (err != 0) {
        LOG_ERR("On-demand ranging data notify fail, seg_idx(%d), err(%d).",
            seg->seg_idx, err);
        return;
    }

    return;
}

static void ras_on_demand_indicate_finished(struct bt_conn* conn,
    struct bt_gatt_indicate_params* params,
    uint8_t err)
{
    sys_snode_t* on_deman_pdu = (sys_snode_t*)ras_srv->on_deman_curr_node;

    if (!on_deman_pdu) {
        LOG_INF("Compelete segment data sent.");
        ras_on_demond_send_cmp_ranging_data_rsp(conn, 10);
        return;
    }

    ras_segment_t* seg = CONTAINER_OF(on_deman_pdu, ras_segment_t, seg_node);

    if (!seg) {
        LOG_WRN("Invalid segment.");
        return;
    }

    LOG_INF("seg:%p, seg->data(%d):%s", seg, seg->len, bt_hex(seg->data, seg->len));

    struct bt_gatt_indicate_params ind_params = {
        .attr = &ras_attrs[SAL_LE_RAS_ON_DEM_CHAR_IDX],
        .data = seg->data,
        .len = seg->len,
        .func = ras_on_demand_indicate_finished,
    };

    ras_srv->on_deman_curr_node = on_deman_pdu->next;

    err = BT_GATT_INDICATE(conn, &ind_params);
    if (err != 0) {
        LOG_ERR("On-demand ranging data indication fail, seg_idx(%d), err(%d).",
            seg->seg_idx, err);
        return;
    }

    return;
}

static int ras_ondemand_send_ranging_data(struct bt_conn* conn, uint16_t count)
{
    if (!atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) && !atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        LOG_ERR("RAS haven't enable the on-demond ranging data notify or indication state.");
        return -1;
    }

    int err = 0;

    ras_rang_on_demand_t* on_demand_sub = ras_rang_on_demand_find_subevent(conn, count);

    if (!on_demand_sub) {
        LOG_ERR("Haven't find subevent with count(%d).", count);
        return -1;
    }

    const sys_snode_t* on_deman_pdu = sys_slist_peek_head(&on_demand_sub->seg_list);
    struct ras_segment_t* seg = CONTAINER_OF(on_deman_pdu, ras_segment_t, seg_node);

    if (atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY)) {
        struct bt_gatt_notify_params notify_param = {
            .attr = &ras_attrs[SAL_LE_RAS_ON_DEM_CHAR_IDX],
            .func = ras_on_demand_notify_finished,
            .user_data = (void*)on_deman_pdu->next,
            .data = seg->data,
            .len = seg->len,
        };

        LOG_INF("seg:%p, seg->data(%d):%s", seg, seg->len, bt_hex(seg->data, seg->len));

        err = BT_GATT_NOTIFY_CB(conn, &notify_param);
        if (err != 0) {
            LOG_ERR("On-demand ranging data notify fail, err(%d).", err);
            return err;
        }
    } else if (atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        struct bt_gatt_indicate_params ind_params = {
            .attr = &ras_attrs[SAL_LE_RAS_ON_DEM_CHAR_IDX],
            .data = seg->data,
            .len = seg->len,
            .func = ras_on_demand_indicate_finished,
        };

        LOG_INF("seg:%p, seg->data(%d):%s", seg, seg->len, bt_hex(seg->data, seg->len));
        // Set the current node to the next.
        ras_srv->on_deman_curr_node = on_deman_pdu->next;
        err = BT_GATT_INDICATE(conn, &ind_params);
        if (err != 0) {
            LOG_ERR("On-demand ranging data indicate fail, err(%d).", err);
            return err;
        }
    }

    return err;
}

static int ras_on_demond_send_cmp_ranging_data_rsp(struct bt_conn* conn, uint16_t count)
{
    ras_rang_on_demand_t* on_demand_data = ras_rang_on_demand_find_subevent(conn, count);

    if (!on_demand_data) {
        LOG_WRN("Haven't find the demand data with the count(%d).", count);
    }

    uint8_t buf[3] = { 0 };
    int err;
    buf[0] = SAL_LE_RAS_CTL_OP_RSP_CMP_RANG_DATA;
    sys_put_le16(count, &buf[1]);

    if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        struct bt_gatt_notify_params notify_param = {
            .attr = &ras_attrs[SAL_LE_RAS_CTR_PT_CHAR_IDX],
            .func = NULL,
            .user_data = NULL,
            .data = buf,
            .len = sizeof(buf),
        };

        err = BT_GATT_NOTIFY_CB(conn, &notify_param);
        if (err != 0) {
            LOG_ERR("ranging data Control Point response ACK notify fail, err(%d).", err);
            return err;
        }
    } else if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        struct bt_gatt_indicate_params ind_params = {
            .attr = &ras_attrs[SAL_LE_RAS_CTR_PT_CHAR_IDX],
            .data = buf,
            .len = sizeof(buf),
            .func = NULL,
        };

        err = BT_GATT_INDICATE(conn, &ind_params);
        if (err != 0) {
            LOG_ERR("ranging data Control Point response ACK indicate fail, err(%d).", err);
            return err;
        }
    } else {
        LOG_ERR("The Ranging Control Point haven't set to notify or indication state.");
        return -1;
    }

    return 0;
}

static int ras_on_demond_send_code_rsp(struct bt_conn* conn, uint16_t count)
{
    ras_rang_on_demand_t* on_demand_data = ras_rang_on_demand_find_subevent(conn, count);

    if (!on_demand_data) {
        LOG_WRN("Haven't find the demand data with the count(%d).", count);
    }

    uint8_t buf[2] = { 0 };
    int err;
    buf[0] = SAL_LE_RAS_CTL_OP_RSP_CODE;
    buf[1] = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS;

    if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {

        struct bt_gatt_notify_params notify_param = {
            .attr = &ras_attrs[SAL_LE_RAS_CTR_PT_CHAR_IDX],
            .func = NULL,
            .user_data = NULL,
            .data = buf,
            .len = sizeof(buf),
        };

        err = BT_GATT_NOTIFY_CB(conn, &notify_param);
        if (err != 0) {
            LOG_ERR("ranging data Control Point response ACK notify fail, err(%d).", err);
            return err;
        }
    } else if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        struct bt_gatt_indicate_params ind_params = {
            .attr = &ras_attrs[SAL_LE_RAS_CTR_PT_CHAR_IDX],
            .data = buf,
            .len = sizeof(buf),
            .func = NULL,
        };

        err = BT_GATT_INDICATE(conn, &ind_params);
        if (err != 0) {
            LOG_ERR("ranging data Control Point response ACK indicate fail, err(%d).", err);
            return err;
        }
    }

    // Free the on-demand segment list when response the ack to the Client.
    ras_segment_t *seg_prev, *seg_next;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&on_demand_data->seg_list,
        seg_prev, seg_next, seg_node)
    {
        sys_slist_remove(&on_demand_data->seg_list, NULL, &seg_prev->seg_node);
        LOG_INF("seg_prev:%p", seg_prev);
        free(seg_prev);
    }

    LOG_INF("The on-demand data has been sent, Cancel on-demand timer.");
    /* The on-demand data has been sent, Cancel on-demand timer */
    k_work_cancel_delayable(&on_demand_data->on_demand_work);
    memset(on_demand_data, 0, sizeof(ras_rang_on_demand_t));
    return 0;
}

static int ras_on_demond_send_lost_ranging_data_cmp_rsp(struct bt_conn* conn, uint16_t count, uint8_t first, uint8_t last)
{
    uint8_t buf[5] = { 0 };

    buf[0] = 0x01;
    sys_put_le16(count, &buf[1]);
    buf[3] = first;
    buf[4] = last;

    int err;
    if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
        buf[0] = SAL_LE_RAS_CTL_OP_RSP_CODE;
        buf[1] = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS;
        struct bt_gatt_notify_params notify_param = {
            .attr = &ras_attrs[SAL_LE_RAS_CTR_PT_CHAR_IDX],
            .func = NULL,
            .user_data = NULL,
            .data = buf,
            .len = sizeof(buf),
        };

        err = BT_GATT_NOTIFY_CB(conn, &notify_param);
        if (err != 0) {
            LOG_ERR("ranging data Control Point response ACK notify fail, err(%d).", err);
            return err;
        }
    } else if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE)) {
        struct bt_gatt_indicate_params ind_params = {
            .attr = &ras_attrs[SAL_LE_RAS_CTR_PT_CHAR_IDX],
            .data = buf,
            .len = sizeof(buf),
            .func = NULL,
        };

        err = BT_GATT_INDICATE(conn, &ind_params);
        if (err != 0) {
            LOG_ERR("ranging data Control Point response ACK indicate fail, err(%d).", err);
            return err;
        }
    }

    return 0;
}

static int ras_on_demand_retrieve_send_lost_data(struct bt_conn* conn, uint16_t count,
    uint8_t first_seg, uint8_t last_seg)
{
    ras_rang_on_demand_t* on_demand_data = ras_rang_on_demand_find_subevent(conn, count);

    if (!atomic_test_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_RANGING_DATA_RSP)) {
        LOG_WRN("Invalid on_demand state.");
        return -1;
    }

    int err;
    // Free the on-demand segment list when response the ack to the Client.
    ras_segment_t *seg_prev, *seg_next;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&on_demand_data->seg_list,
        seg_prev, seg_next, seg_node)
    {
        if (seg_prev && (seg_prev->seg_idx >= first_seg) && (seg_prev->seg_idx <= last_seg)) {
            if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY)) {
                struct bt_gatt_notify_params notify_param = {
                    .attr = &ras_attrs[SAL_LE_RAS_ON_DEM_CHAR_IDX],
                    .func = ras_on_demand_notify_finished,
                    .user_data = (void*)seg_prev->seg_node.next,
                    .data = seg_prev->data,
                    .len = seg_prev->len,
                };

                err = BT_GATT_NOTIFY_CB(conn, &notify_param);
                if (err != 0) {
                    LOG_ERR("On-demand ranging data notify fail, err(%d).", err);
                    return err;
                }
            } else if (atomic_test_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE)) {
                struct bt_gatt_indicate_params ind_params = {
                    .attr = &ras_attrs[SAL_LE_RAS_ON_DEM_CHAR_IDX],
                    .data = seg_prev->data,
                    .len = seg_prev->len,
                    .func = ras_on_demand_indicate_finished,
                };

                err = BT_GATT_INDICATE(conn, &ind_params);
                if (err != 0) {
                    LOG_ERR("On-demand ranging data indicate fail, err(%d).", err);
                    return err;
                }
            }
        } else if (seg_prev && (seg_prev->seg_idx > last_seg)) {
            break;
        }
    }

    ras_on_demond_send_lost_ranging_data_cmp_rsp(conn, count, first_seg, last_seg);
    return 0;
}

static uint8_t ras_check_ranging_mode(struct bt_conn* conn)
{
    if (atomic_test_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY) || atomic_test_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE)) {
        LOG_INF("The real-time mode has been set.");
        return SAL_LE_RAS_RANGING_MODE_REAL_TIME;
    } else if (atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) || atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE)) {
        LOG_INF("The on-demand mode has been set.");
        return SAL_LE_RAS_RANGING_MODE_ON_DEMAND;
    }

    LOG_ERR("No mode have been set.");
    return SAL_LE_RAS_RANGING_MODE_UNDEFINED;
}

bool ras_is_filter_bit_set(uint16_t mode, uint16_t filter_bit)
{
    // Check if the mode is valid (within the range of SAL_LE_RAS_MODE_3_FILTER_MAX)
    if (mode >= SAL_LE_RAS_FILTER_MODE_MAX) {
        LOG_ERR("Error: Mode %d is out of valid range (0 to %d)\n",
            mode, SAL_LE_RAS_FILTER_MODE_MAX - 1);
        return false;
    }

    // Retrieve the current filter mask from the atomic array
    atomic_val_t current_filter_mask = atomic_get(&ras_srv->ras_filter[mode]);

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
        atomic_and(&ras_srv->ras_filter[mode], SAL_LE_RAS_FILTER_BIT_MASK); // Clear the filter bits (2-15)
        atomic_or(&ras_srv->ras_filter[mode], filter_mask); // Set the new filter mask (bits 2-15)
    } else {
        // Handle error case if mode is out of range
        LOG_ERR("Error: Mode %d is out of valid range (0 to %d)\n", mode, SAL_LE_RAS_FILTER_MODE_MAX - 1);
    }

    return;
}

void ras_handle_abort_operation(ras_control_point_t* control_point)
{
    if (control_point->is_processing) {
        LOG_DBG("Stopping current RAS Control Point processes...");
        control_point->is_processing = 0; // Stop current processing.

        if (control_point->is_data_pending) {
            LOG_DBG("Flushing pending Ranging Data segments...");
            control_point->is_data_pending = 0; // Discard pending data
        }

        control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS; // response success.
        LOG_DBG("Abort Operation completed successfully.");
    } else {
        control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED;
        LOG_DBG("Abort Operation was unsuccessful (no operation to abort).");
    }
}

void ras_handle_other_operation(ras_control_point_t* control_point)
{
    LOG_DBG("Handling other operations...");
    control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS; // response success.
}

void ras_write_opcode(ras_control_point_t* control_point, ras_opcode_t op_code)
{
    switch (op_code) {
    case ABORT_OPERATION:
        LOG_DBG("Received ABORT_OPERATION OpCode.");
        ras_handle_abort_operation(control_point);
        break;

    case OTHER_OPERATION:
        LOG_DBG("Received OTHER_OPERATION OpCode.");
        ras_handle_other_operation(control_point);
        break;

    default:
        control_point->response_code = SAL_LE_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED;
        LOG_WRN("OpCode not supported.");
        break;
    }
}

void rs_display_response(ras_control_point_t* control_point)
{
    switch (control_point->response_code) {
    case SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS:
        LOG_INF("Response: SUCCESS.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_PROCE_NOT_CMP:
        LOG_INF("Response: PROCEDURE NOT COMPLETED.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_PERSISTED:
        LOG_INF("Response: ABORT UNSUCCESSFUL.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_SERVER_BUSY:
        LOG_INF("Response: SERVER BUSY.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_INVALID_PARAMS:
        LOG_INF("Response: INVALID PARAMETER.");
        break;
    case SAL_LE_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED:
        LOG_INF("Response: OP CODE NOT SUPPORTED.");
        break;
    default:
        LOG_INF("Unknown response code.");
        break;
    }
}

ssize_t on_ras_ctr_pt_write_cb(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    const void* buf, uint16_t len, uint16_t offset,
    uint8_t flags)
{
    if (!buf || len == 0) {
        LOG_ERR("Invalid buf or len.");
        return 0;
    }

    uint8_t* buf_send = (uint8_t*)buf;

    uint8_t opcode = (uint8_t)buf_send[0];
    LOG_INF("RAS Control Point cb, opcode(%d)\n", opcode);
    // LOG_INF("opcode:%d, offset:%d, flags:%d, buf_send[%d]:%s",
    // 		opcode, offset, flags, len, bt_hex(buf_send, len));
    uint16_t count;

    switch (opcode) {
    case SAL_LE_RAS_CTL_OP_CMD_GET_RANG_DATA: {
        count = sys_get_le16((const uint8_t*)&buf_send[1]);
        ras_ondemand_send_ranging_data(conn, count);
        break;
    }
    case SAL_LE_RAS_CTL_OP_CMD_ACK_RANG_DATA: {
        count = sys_get_le16((const uint8_t*)&buf_send[1]);
        ras_on_demond_send_code_rsp(conn, count);
        break;
    }
    case SAL_LE_RAS_CTL_OP_CMD_RETRIEVE_LOST_RANG_DATA_SEG: {
        count = sys_get_le16((const uint8_t*)&buf_send[1]);
        uint8_t first_seg = (uint8_t)buf_send[3];
        uint8_t last_seg = (uint8_t)buf_send[4];
        ras_on_demand_retrieve_send_lost_data(conn, count, first_seg, last_seg);
    }
    case SAL_LE_RAS_CTL_OP_CMD_ABORT_OPERATION: {
        // TODO:
    }
    case SAL_LE_RAS_CTL_OP_CMD_SET_FILTER: {
        uint16_t filter_params = sys_get_le16((const uint8_t*)&buf_send[1]);
        ras_set_filter(filter_params);
    }
    default:
        break;
    }
    return len;
}

static ssize_t ras_feature_read(struct bt_conn* conn, const struct bt_gatt_attr* attr,
    void* buf, uint16_t len, uint16_t offset)
{
    LOG_INF("RAS feature read cb, ras_feature 0x%lx.\n", ras_srv->ras_feature);
    LOG_INF("offset:%d, buf[%d]:%s", offset, len, bt_hex(buf, len));
    return BT_GATT_ATTR_READ(conn, attr, buf, len, offset, (uint8_t*)&ras_srv->ras_feature,
        sizeof(ras_srv->ras_feature));
}

static void range_rt_dt_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    if (!ras_srv) {
        LOG_ERR("Invalid ras_srv value.");
        return;
    }

    // The Real time mode and the on-demand mode can't be set together.
    if ((value != 0) && (atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY) || atomic_test_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE))) {
        LOG_ERR("The on-demond mode has been setted, Please clear it before set to real-time mode.");
        return;
    }

    if (value == 0) {
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY);
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE);
    } else {
        atomic_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_RTT_DATA_NOTIFY : RAS_RTT_DATA_INDICATE);
    }

    LOG_INF("The range real-time data ccc value is change to (%d)\n", value);
    return;
}

static void range_on_dem_dt_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    if (!ras_srv) {
        LOG_ERR("Invalid ras_srv value.");
        return;
    }

    // The Real time mode and the on-demand mode can't be set together.
    if ((value != 0) && (atomic_test_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY) || atomic_test_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_INDICATE))) {
        LOG_ERR("The on-demond mode has been setted, Please clear it before set to real-time mode.");
        return;
    }

    if (value == 0) {
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY);
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
    } else {
        atomic_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_ON_DEMAND_DATA_NOTIFY : RAS_ON_DEMAND_DATA_INDICATE);
    }

    LOG_INF("The range on-dem data ccc value is change to (%d)\n", value);
    return;
}

static void range_ctr_pt_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    if (!ras_srv) {
        LOG_ERR("Invalid ras_srv value.");
        return;
    }

    if (value == 0) {
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_INDICATE);
    } else {
        atomic_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_CONTROL_POINT_NOTIFY : RAS_CONTROL_POINT_INDICATE);
    }

    LOG_INF("The range control point data ccc value is change to (%d)\n", value);
    return;
}

static void range_dt_rd_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    if (!ras_srv) {
        LOG_ERR("Invalid ras_srv value.");
        return;
    }

    if (value == 0) {
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_DATA_READY_INDICATE);
    } else {
        atomic_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_DATA_READY_NOTIFY : RAS_DATA_READY_INDICATE);
    }

    LOG_INF("The range data ready data ccc value is change to (%d)\n", value);
    return;
}

static void range_dt_ov_wr_ccc_cfg_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    if (!ras_srv) {
        LOG_ERR("Invalid ras_srv value.");
        return;
    }

    if (value == 0) {
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_OVER_WRITE_NOTIFY);
        atomic_clear_bit(&ras_srv->char_notify_state, RAS_OVER_WRITE_INDICATE);
    } else {
        atomic_set_bit(&ras_srv->char_notify_state,
            (value == SAL_LE_RAS_GATT_NOTIFY) ? RAS_OVER_WRITE_NOTIFY : RAS_OVER_WRITE_INDICATE);
    }

    LOG_INF("The range over write data ccc value is change to (%d)\n", value);
    return;
}

static void ras_dt_rd_indicate_cb(struct bt_conn* conn,
    struct bt_gatt_indicate_params* params, uint8_t err)
{
    LOG_INF("Indication %s\n", err != 0U ? "fail" : "success");
    if (ras_srv->remaining_len) {
        split_real_time_segment(conn, &ras_srv->latest_local_steps[ras_srv->ras_seg_offset], ras_srv->remaining_len);
    }

    return;
}

static void ras_dt_rd_indicate_destroy(struct bt_gatt_indicate_params* params)
{
    LOG_INF("Indication complete\n");
    ras_srv->ras_dt_rd_indicating = 0U;

    return;
}

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

static void split_real_time_segment(struct bt_conn* conn, uint8_t* buf, int len)
{
    if (ras_srv->remaining_len > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
        int curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
        LOG_INF("data send: offset:%lu, len:%d\n", ras_srv->ras_seg_offset, curr_seg_size);

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
        ras_srv->ras_dt_rd_ind_params.attr = &ras_attrs[SAL_LE_RAS_RT_DT_CHAR_IDX];
        ras_srv->ras_dt_rd_ind_params.func = ras_dt_rd_indicate_cb;
        ras_srv->ras_dt_rd_ind_params.destroy = ras_dt_rd_indicate_destroy;
        ras_srv->ras_dt_rd_ind_params.data = send_buf;
        ras_srv->ras_dt_rd_ind_params.len = curr_seg_size + 1;

        if ((send_buf[0] & 0x01) == 0x01) {
            LOG_INF("First seg, data(%d):%s\n", curr_seg_size + 1, bt_hex(send_buf, curr_seg_size + 1));
        } else {
            LOG_INF("The %d seg, data(%d):%s\n", send_buf[0] >> 2, curr_seg_size + 1, bt_hex(send_buf, curr_seg_size + 1));
        }

        if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_NOTIFY) {
            if (BT_GATT_NOTIFY(conn, &ras_attrs[SAL_LE_RAS_RT_DT_CHAR_IDX], send_buf, curr_seg_size + 1) == 0) {
                if (ras_srv->remaining_len) {
                    split_real_time_segment(conn, &ras_srv->latest_local_steps[ras_srv->ras_seg_offset], ras_srv->remaining_len);
                }
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                LOG_INF("ras data ready Notify fail.\n");
                free(send_buf);
                return;
            }
        } else if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_INDICATION) {
            if (BT_GATT_INDICATE(conn, &ras_srv->ras_dt_rd_ind_params) == 0) {
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                LOG_INF("ras data ready Indicate fail.\n");
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
        ras_srv->ras_dt_rd_ind_params.attr = &ras_attrs[SAL_LE_RAS_RT_DT_CHAR_IDX];
        ras_srv->ras_dt_rd_ind_params.func = ras_dt_rd_indicate_cb;
        ras_srv->ras_dt_rd_ind_params.destroy = ras_dt_rd_indicate_destroy;
        ras_srv->ras_dt_rd_ind_params.data = send_buf;
        ras_srv->ras_dt_rd_ind_params.len = curr_seg_size + 1;
        ras_srv->ras_dt_rd_indicating = 0U;

        ras_srv->ras_seg_idx = 0;
        ras_srv->ras_dt_rd_indicating = 0U;
        ras_srv->remaining_len = 0;
        ras_srv->ras_seg_offset = 0;
        LOG_INF("The last(%d) seg, data(%d):%s\n", send_buf[0] >> 2, curr_seg_size + 1, bt_hex(send_buf, curr_seg_size + 1));
        LOG_INF("ras_dt_rd_indicating:%d\n", ras_srv->ras_dt_rd_indicating);
        if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_NOTIFY) {
            if (BT_GATT_NOTIFY(conn, &ras_attrs[SAL_LE_RAS_RT_DT_CHAR_IDX], send_buf, curr_seg_size + 1) == 0) {
                free(send_buf);
            } else {
                LOG_INF("ras data ready Indicate fail.\n");
                free(send_buf);
                return;
            }
        } else if (ras_srv->rt_dt_ccc_cfg == SAL_LE_RAS_GATT_INDICATION) {
            if (BT_GATT_INDICATE(conn, &ras_srv->ras_dt_rd_ind_params) == 0) {
                ras_srv->ras_dt_rd_indicating = 1U;
                free(send_buf);
            } else {
                LOG_INF("ras data ready Indicate fail.\n");
                free(send_buf);
                return;
            }
        }
    }

    return;
}

static void ras_on_demand_data_send_timeout(struct k_work* work)
{
    ras_rang_on_demand_t* on_demand_subevent = RAS_ON_DEMAND_WORK_PICK(work);
    LOG_INF("On-demand data send timeout, remove the data in the list.");
    // Free the on-demand segment list when response the ack to the Client.
    ras_segment_t *seg_prev, *seg_next;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&on_demand_subevent->seg_list,
        seg_prev, seg_next, seg_node)
    {
        sys_slist_remove(&on_demand_subevent->seg_list, NULL, &seg_prev->seg_node);
        LOG_INF("seg_prev:%p", seg_prev);
        free(seg_prev);
    }

    memset(on_demand_subevent, 0, sizeof(ras_rang_on_demand_t));

    return;
}

static void split_on_demand_segment(struct bt_conn* conn, uint8_t* buf, int len,
    ras_rang_on_demand_t* subevent)
{
    int curr_seg_size = 0;
    uint16_t seg_index = 0;
    ras_srv->ras_seg_offset = 0;

    do {
        if (ras_srv->remaining_len > ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
            curr_seg_size = ras_srv->ras_mtu - RAS_SEG_HEADER_SIZE - 1;
            LOG_INF("data send: offset:%lu, len:%d\n", ras_srv->ras_seg_offset, curr_seg_size);
            subevent->seg = (ras_segment_t*)malloc(sizeof(ras_segment_t) + curr_seg_size + 1);

            if (!subevent->seg) {
                LOG_ERR("Malloc fail.");
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
                LOG_ERR("Malloc fail.");
                return;
            }

            memcpy(&subevent->seg->data[1], &buf[ras_srv->ras_seg_offset], curr_seg_size);

            subevent->seg->data[0] = (seg_index == 0) ? (0x01) : (seg_index << 2);
            subevent->seg->data[0] |= (0x01 << 1);
            ras_srv->remaining_len = 0;
            LOG_INF("ras_dt_rd_indicating:%d\n", ras_srv->ras_dt_rd_indicating);
        }

        if ((subevent->seg->data[0] & 0x01) == 0x01) {
            LOG_INF("First seg, data(%d):%s\n", curr_seg_size + 1, bt_hex(subevent->seg->data, curr_seg_size + 1));
        } else {
            LOG_INF("The %d seg, data(%d):%s\n", subevent->seg->data[0] >> 2, curr_seg_size + 1, bt_hex(subevent->seg->data, curr_seg_size + 1));
        }

        subevent->seg->seg_idx = seg_index++;
        subevent->seg->len = curr_seg_size + 1;
        sys_slist_append(&subevent->seg_list, &subevent->seg->seg_node);
        LOG_INF("subevent seg:%p, seg_node:%p.", subevent->seg, &subevent->seg->seg_node);
    } while (ras_srv->remaining_len > 0);

    k_work_init_delayable(&subevent->on_demand_work, ras_on_demand_data_send_timeout);
    /* Start a on demand data send timer. */
    k_work_schedule(&subevent->on_demand_work, RAS_RSP_TIMEOUT);

    return;
}

void on_cccd_changed(const struct bt_gatt_attr* attr, uint16_t value)
{
    return;
}

static void write_func(struct bt_conn* conn, uint8_t err, struct bt_gatt_write_params* params)
{
    if (err) {
        LOG_INF("Write failed (err %d)\n", err);

        return;
    }
}

int write_cs_reflector_step_data(void)
{
    int err;
    struct bt_gatt_write_params write_params;
    write_params.func = write_func;
    write_params.handle = ras_srv->step_data_attr_handle;
    write_params.length = SAL_LE_RAS_STEP_DATA_BUF_LEN;
    write_params.data = &ras_srv->latest_local_steps[0];
    write_params.offset = 0;

    err = bt_gatt_write(ras_srv->connection, &write_params);
    if (err) {
        LOG_INF("Write failed (err %d)\n", err);
        return 0;
    }

    return 0;
}

/**
 * Input: data points to the original buffer, data_len is the total length of the input
 * Output: buf stores the transformed result, function returns the length of the output buffer
 */
static size_t transform_step_data_to_ras_format_filtered(
    uint8_t* data, size_t data_len,
    uint8_t* buf, atomic_t* ras_filter, uint8_t role,
    uint8_t num_antenna_paths)
{
    size_t in_offset = 0;
    size_t out_offset = 0;

    while (in_offset + 3 <= data_len) {
        uint8_t step_mode = data[in_offset];
        uint8_t step_data_length = data[in_offset + 2];

        if (in_offset + 3 + step_data_length > data_len) {
            LOG_WRN("Incomplete data, exit early.");
            break;
        }

        uint8_t* step_data = &data[in_offset + 3];
        uint32_t filter_mask = atomic_get(&ras_filter[step_mode]);

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
            LOG_WRN("Unknown mode %d, copying raw data.", step_mode);
            memcpy(&buf[out_offset], step_data, step_data_length);
            out_offset += step_data_length;
            break;
        }

        in_offset += 3 + step_data_length;
    }

    return out_offset;
}

static ras_rang_on_demand_t* ras_rang_on_demand_subevent_pool_find(struct bt_conn* conn)
{
    if (!ras_srv) {
        LOG_ERR("Invalid ras_srv enviranment, init it first.");
        return NULL;
    }

    for (int i = 0; i < SAL_LE_RAS_STORE_PROCEDURE_NUM_MAX; i++) {
        if (ras_srv->subevent[i].proc_used == false) {
            memset(&ras_srv->subevent[i], 0, sizeof(ras_rang_on_demand_t));
            ras_srv->subevent[i].proc_used = true;
            return &ras_srv->subevent[i];
        }
    }

    LOG_INF("No subvent pool can be used.");
    return NULL;
}

static ras_rang_on_demand_t* ras_rang_on_demand_find_subevent(struct bt_conn* conn, uint16_t count)
{
    if (!ras_srv) {
        LOG_ERR("Invalid ras_srv enviranment, init it first.");
        return NULL;
    }

    for (int i = 0; i < SAL_LE_RAS_STORE_PROCEDURE_NUM_MAX; i++) {
        if (ras_srv->subevent[i].count == count && ras_srv->subevent[i].proc_used == true) {
            return &ras_srv->subevent[i];
        }
    }

    LOG_INF("No subvent find with count(%d).", count);
    return NULL;
}

static uint8_t* ras_subevent_data_conversion(struct bt_conn* conn, struct bt_conn_le_cs_subevent_result* result)
{
    static int i = 0;
    memset(ras_srv->latest_local_steps, 0, sizeof(ras_srv->latest_local_steps));

    if (result->step_data_buf) {
        if (result->step_data_buf->len <= SAL_LE_RAS_STEP_DATA_BUF_LEN) {
            memcpy(ras_srv->latest_local_steps, result->step_data_buf->data,
                result->step_data_buf->len);
            LOG_INF("step data[%d]:%s\n", i++, bt_hex(result->step_data_buf->data, result->step_data_buf->len));
        } else {
            LOG_INF("Not enough memory to store step data. (%d > %d)\n",
                result->step_data_buf->len, SAL_LE_RAS_STEP_DATA_BUF_LEN);
        }
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
     * Starting ACL connection event count for the results reported in the event.
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
    ras_srv->remaining_len = transform_step_data_to_ras_format_filtered(result->step_data_buf->data,
        result->step_data_buf->len, stream_buf + SAL_LE_RAS_SUB_PROCUDURE_HEAD,
        ras_srv->ras_filter, ras_srv->ras_role, result->header.num_antenna_paths & 0x0F);
    ras_srv->remaining_len += SAL_LE_RAS_SUB_PROCUDURE_HEAD;
    LOG_INF("stream head:%s.", bt_hex(stream_buf, SAL_LE_RAS_SUB_PROCUDURE_HEAD));
    LOG_INF("stream_buf(%ld):%s.\n", ras_srv->remaining_len, bt_hex(stream_buf + 12, ras_srv->remaining_len - SAL_LE_RAS_SUB_PROCUDURE_HEAD));
    LOG_INF("data ready indiacte count(%d)., uuid:0x%x, handle:%d\n",
        result->header.procedure_counter,
        BT_UUID_16(ras_attrs[SAL_LE_RAS_DT_RD_CHAR_VAL_IDX].uuid)->val,
        ras_attrs[SAL_LE_RAS_DT_RD_CHAR_VAL_IDX].handle);
    LOG_INF("procedure_counter:0x%x, config_id:0x%x, reference_power_level:0x%x",
        result->header.procedure_counter, result->header.config_id,
        result->header.reference_power_level);
    LOG_INF("num_antenna_paths:0x%x, start_acl_conn_event:0x%x, frequency_compensation:0x%x",
        result->header.num_antenna_paths, result->header.start_acl_conn_event,
        result->header.frequency_compensation);
    LOG_INF("procedure_done_status:0x%x, subevent_done_status:0x%x,   :0x%x",
        result->header.procedure_done_status, result->header.subevent_done_status,
        result->header.procedure_abort_reason);
    LOG_INF("subevent_abort_reason:0x%x, reference_power_level:0x%x, num_steps_reported:0x%x",
        result->header.subevent_abort_reason, result->header.reference_power_level,
        result->header.num_steps_reported);
    LOG_INF("mode:0x%x, channel:0x%x, len:0x%x", result->step_data_buf->data[0],
        result->step_data_buf->data[1],
        result->step_data_buf->data[2]);
    return stream_buf;
}

static void ras_process_real_time_ranging_data(struct bt_conn* conn, struct bt_conn_le_cs_subevent_result* result)
{
    uint8_t* stream_buf = ras_subevent_data_conversion(conn, result);
    split_real_time_segment(conn, stream_buf, ras_srv->remaining_len);
}

static void ras_process_on_demand_ranging_data(struct bt_conn* conn, struct bt_conn_le_cs_subevent_result* result)
{
    ras_rang_on_demand_t* subevent = ras_rang_on_demand_subevent_pool_find(conn);

    if (!subevent) {
        LOG_ERR("No subevent pool found.");
        return;
    }

    LOG_DBG("procedure counter:%d.", result->header.procedure_counter);

    subevent->count = result->header.procedure_counter;
    uint8_t* stream_buf = ras_subevent_data_conversion(conn, result);
    split_on_demand_segment(conn, stream_buf, ras_srv->remaining_len, subevent);
    if (atomic_test_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE)) {
        ras_data_ready_send(conn, subevent->count);
        // Set the on-demand state to ready.
        atomic_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_DATA_READY_INDICATE);
    }

    return;
}

static void subevent_result_cb(struct bt_conn* conn, struct bt_conn_le_cs_subevent_result* result)
{
    if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE && ras_check_ranging_mode(conn) == SAL_LE_RAS_RANGING_MODE_REAL_TIME) {
        LOG_DBG("Recv the real-time ranging data.");
        ras_process_real_time_ranging_data(conn, result);
        return;
    }

    if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE && ras_check_ranging_mode(conn) == SAL_LE_RAS_RANGING_MODE_ON_DEMAND) {
        LOG_DBG("Recv the on-demand ranging data.");
        ras_process_on_demand_ranging_data(conn, result);
        return;
    }

    LOG_ERR("No mode has been set, discard the subevent result.");
    return;
}

static int ras_data_ready_send(struct bt_conn* conn, uint16_t count)
{
    // if (!conn) {
    // 	LOG_ERR("Invalid conn handle.");
    // 	return -1;
    // }

    uint8_t buf[2];
    sys_put_le16(count, buf);
    if (atomic_test_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY)) {
        return BT_GATT_NOTIFY(conn, &ras_attrs[SAL_LE_RAS_DT_RD_CHAR_IDX], &count, sizeof(count));
    } else if (atomic_test_bit(&ras_srv->char_notify_state, RAS_DATA_READY_INDICATE)) {
        struct bt_gatt_indicate_params data_ready_indicate;
        data_ready_indicate.attr = &ras_attrs[SAL_LE_RAS_DT_RD_CHAR_IDX];
        data_ready_indicate.func = ras_dt_rd_indicate_cb;
        data_ready_indicate.destroy = ras_dt_rd_indicate_destroy;
        data_ready_indicate.data = buf;
        data_ready_indicate.len = sizeof(buf);
        return BT_GATT_INDICATE(conn, &data_ready_indicate);
    }

    LOG_ERR("Invalid data ready char indication state.");
    return -1;
}

static void ras_mtu_updated(struct bt_conn* conn, uint16_t tx, uint16_t rx)
{
    if (ras_srv) {
        ras_srv->ras_mtu = MIN(tx, rx);
    }

    LOG_INF("Updated MTU: TX: %d RX: %d bytes, ras_mtu: %lu\n", tx, rx, ras_srv->ras_mtu);
    return;
}

static void connected_cb(struct bt_conn* conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_address_t bt_addr = { 0 };

    (void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected to %s (err 0x%02X)\n", addr, err);

    memcpy(bt_addr.addr, addr, sizeof(bt_address_t));
    cs_msg_t* msg = cs_msg_new(CONNECTED_EVT, &bt_addr);
    bt_sal_cs_event_callback(msg);

    __ASSERT(ras_srv->connection == conn, "Unexpected connected callback");

    if (err) {
        bt_conn_unref(conn);
        ras_srv->connection = NULL;
    }

    LOG_INF("ras_srv:%p.", ras_srv);
    ras_srv->connection = bt_conn_ref(conn);

    const struct bt_le_cs_set_default_settings_param default_settings = {
        .enable_initiator_role = false,
        .enable_reflector_role = true,
        .cs_sync_antenna_selection = BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
        .max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
    };

    err = bt_le_cs_set_default_settings(ras_srv->connection, &default_settings);
    if (err) {
        LOG_INF("Failed to configure default CS settings (err %d)\n", err);
    }
}

static void disconnected_cb(struct bt_conn* conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_address_t bt_addr = { 0 };

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    memcpy(bt_addr.addr, addr, sizeof(bt_address_t));
    cs_msg_t* msg = cs_msg_new(DISCONNECTED_EVT, &bt_addr);
    bt_sal_cs_event_callback(msg);

    LOG_INF("Disconnected (reason 0x%02X)\n", reason);

    bt_conn_unref(conn);
    ras_srv->connection = NULL;

    // int err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
    //                               BT_GAP_ADV_FAST_INT_MAX_1, NULL),
    //     ad, ARRAY_SIZE(ad), NULL, 0);
    // if (err) {
    //     LOG_INF("Advertising failed to start (err %d)\n", err);
    //     return;
    // }

    LOG_INF("Advertising start again.\n");
}

static void remote_capabilities_cb(struct bt_conn* conn, struct bt_conn_le_cs_capabilities* params)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_address_t bt_addr = { 0 };

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    memcpy(bt_addr.addr, addr, sizeof(bt_address_t));
    cs_msg_t* msg = cs_msg_new(CAPBLITIES_RECEIVED_EVT, &bt_addr);
    bt_srv_conn_le_cs_capabilities_t capabilities = {};
    memcpy(&capabilities, params, sizeof(bt_srv_conn_le_cs_capabilities_t));
    msg->cs_data.data = &capabilities;
    bt_sal_cs_event_callback(msg);

    ARG_UNUSED(params);
    LOG_INF("CS capability exchange completed.\n");
    LOG_INF("num_config_supported:%d, max_consecutive_procedures_supported:%d",
        params->num_config_supported, params->max_consecutive_procedures_supported);
    LOG_INF("num_antennas_supported:%d, max_antenna_paths_supported:%d.",
        params->num_antennas_supported, params->max_antenna_paths_supported);
    LOG_INF("initiator_supported:%d, reflector_supported:%d",
        params->initiator_supported, params->reflector_supported);
    LOG_INF("mode_3_supported:%d, rtt_aa_only_precision:%d",
        params->mode_3_supported, params->rtt_aa_only_precision);
    LOG_INF("rtt_sounding_precision:%d, rtt_random_payload_precision:%d",
        params->rtt_sounding_precision, params->rtt_random_payload_precision);
    LOG_INF("rtt_aa_only_n:%d, rtt_sounding_n:%d, rtt_random_payload_n:%d",
        params->rtt_aa_only_n, params->rtt_sounding_n, params->rtt_random_payload_n);
    LOG_INF("phase_based_nadm_sounding_supported:%d, phase_based_nadm_random_supported:%d",
        params->phase_based_nadm_sounding_supported, params->phase_based_nadm_random_supported);
    LOG_INF("cs_sync_2m_phy_supported:%d, cs_sync_2m_2bt_phy_supported:%d",
        params->cs_sync_2m_phy_supported, params->cs_sync_2m_2bt_phy_supported);
    LOG_INF("cs_without_fae_supported:%d, chsel_alg_3c_supported:%d",
        params->cs_without_fae_supported, params->chsel_alg_3c_supported);
    LOG_INF("pbr_from_rtt_sounding_seq_supported:%d, t_ip1_times_supported:%d",
        params->pbr_from_rtt_sounding_seq_supported, params->t_ip1_times_supported);
    LOG_INF("t_ip2_times_supported:%d, t_fcs_times_supported:%d",
        params->t_ip2_times_supported, params->t_fcs_times_supported);
    LOG_INF("t_pm_times_supported:%d, t_sw_time:%d, tx_snr_capability:%d",
        params->t_pm_times_supported, params->t_sw_time,
        params->tx_snr_capability);
}

static void config_created_cb(struct bt_conn* conn, struct bt_conn_le_cs_config* config)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_address_t bt_addr = { 0 };

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    memcpy(bt_addr.addr, addr, sizeof(bt_address_t));
    cs_msg_t* msg = cs_msg_new(CONFIG_DONE_EVT, &bt_addr);
    bt_srv_conn_le_cs_config_t cs_config = {};
    memcpy(&cs_config, config, sizeof(bt_srv_conn_le_cs_config_t));
    msg->cs_data.data = (void*)&cs_config;
    bt_sal_cs_event_callback(msg);

    LOG_INF("CS config creation complete. ID: %d\n", config->id);
    LOG_INF("main_mode_type:%d, sub_mode_type:%d",
        config->main_mode_type, config->sub_mode_type);
    LOG_INF("min_main_mode_steps:%d, max_main_mode_steps:%d",
        config->min_main_mode_steps, config->max_main_mode_steps);
    LOG_INF("main_mode_repetition:%d, mode_0_steps:%d",
        config->main_mode_repetition, config->mode_0_steps);
    LOG_INF("role:%d, rtt_type:%d, cs_sync_phy:%d",
        config->role, config->rtt_type, config->cs_sync_phy);
    LOG_INF("channel_map_repetition:%d, channel_selection_type:%d",
        config->channel_map_repetition, config->channel_selection_type);
    LOG_INF("ch3c_shape:%d, ch3c_jump:%d", config->ch3c_shape, config->ch3c_jump);
    LOG_INF("t_ip1_time_us:%d, t_ip2_time_us:%d",
        config->t_ip1_time_us, config->t_ip2_time_us);
    LOG_INF("t_fcs_time_us:%d, t_pm_time_us:%d", config->t_fcs_time_us, config->t_pm_time_us);
    LOG_INF("channel_map:0x%x%x%x%x%x%x%x%x%x%x.",
        config->channel_map[0], config->channel_map[1], config->channel_map[2],
        config->channel_map[3], config->channel_map[4], config->channel_map[5],
        config->channel_map[6], config->channel_map[7], config->channel_map[8],
        config->channel_map[9]);
}

static void security_enabled_cb(struct bt_conn* conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_address_t bt_addr = { 0 };

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    memcpy(bt_addr.addr, addr, sizeof(bt_address_t));
    cs_msg_t* msg = cs_msg_new(SECURITY_DONE_EVT, &bt_addr);
    bt_sal_cs_event_callback(msg);
    LOG_INF("CS security enabled.\n");
}

static void procedure_enabled_cb(struct bt_conn* conn,
    struct bt_conn_le_cs_procedure_enable_complete* params)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_address_t bt_addr = { 0 };

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    memcpy(bt_addr.addr, addr, sizeof(bt_address_t));
    cs_msg_t* msg = cs_msg_new(PROCEDURE_DONE_EVT, &bt_addr);
    bt_srv_conn_le_cs_procedure_enable_complete_t procedure = {};
    memcpy(&procedure, params, sizeof(bt_srv_conn_le_cs_procedure_enable_complete_t));
    msg->cs_data.data = (void*)&procedure;
    bt_sal_cs_event_callback(msg);

    if (params->state == 1) {
        LOG_INF("CS procedures enabled.");
    } else {
        LOG_INF("CS procedures disabled.");
    }

    LOG_INF("config_id:%d, tone_antenna:%d, tx_power:%d, subevents_per_event:%d\n",
        params->config_id, params->tone_antenna_config_selection,
        params->selected_tx_power, params->subevents_per_event);
    LOG_INF("subevent_interval:%d, event_interval:%d, procedure_interval:%d, procedure_count:%d, max_procedure_len:%d\n",
        params->subevent_interval, params->event_interval, params->procedure_interval,
        params->procedure_count, params->max_procedure_len);

    return;
}

static struct bt_conn_cb conn_cbs = {
    .connected = connected_cb,
    .disconnected = disconnected_cb,
    .le_cs_remote_capabilities_available = remote_capabilities_cb,
    .le_cs_config_created = config_created_cb,
    .le_cs_security_enabled = security_enabled_cb,
    .le_cs_procedure_enabled = procedure_enabled_cb,
    .le_cs_subevent_data_available = subevent_result_cb,
};

static struct bt_gatt_cb ras_gatt_callbacks = {
    .att_mtu_updated = ras_mtu_updated
};

int le_cs_enable(void)
{
    int err;

    LOG_INF("Starting Channel Sounding Demo\n");

    ras_srv = (sal_le_ras_srv_env_t*)malloc(sizeof(sal_le_ras_srv_env_t));

    memset(ras_srv, 0, sizeof(sal_le_ras_srv_env_t));

    ras_srv->ras_feature = 0x07000007;

    for (int i = 0; i < SAL_LE_RAS_FILTER_MODE_MAX; i++) {
        ras_srv->ras_filter[i] = 0xFFFFFFFF;
    }

    err = bt_gatt_service_register(&ras_svc);
    if (err != 0) {
        LOG_INF("Failed to register Ranging Service in gatt DB");
        return err;
    }

    bt_conn_cb_register(&conn_cbs);

    bt_gatt_cb_register(&ras_gatt_callbacks);

    // err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
    //                           BT_GAP_ADV_FAST_INT_MAX_1, NULL),
    //     ad, ARRAY_SIZE(ad), NULL, 0);
    // if (err) {
    //     LOG_INF("Advertising failed to start (err %d)\n", err);
    //     return 0;
    // }

    LOG_INF("Advertising starting.\n");

    return 0;
}

// struct bt_conn conn_back = {0};

struct bt_gatt_attr* ras_get_gatt_attr(void)
{
    return ras_attrs;
}

int ras_subevent_recv_test(ras_rang_mode_t mode, ras_testcase_t test_case,
    struct bt_conn* conn, struct bt_conn_le_cs_subevent_result* result)
{
    if (mode == SAL_LE_RAS_RANGING_MODE_REAL_TIME) {
        atomic_set_bit(&ras_srv->char_notify_state, RAS_RTT_DATA_NOTIFY);
        LOG_INF("ras server test: set ranging to real-time mode.");
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
        atomic_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_NOTIFY);
        atomic_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        atomic_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE);
        atomic_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_INDICATE_VALID_RANG_DATA_004:
        ras_srv->ras_mtu = 253;
        atomic_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        atomic_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        atomic_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE);
        atomic_set_bit(&ras_srv->char_notify_state, RAS_CONTROL_POINT_NOTIFY);
        break;
    case RAS_TESTCASE_ON_DEMAND_WRITE_RANG_DATA_TIMWOUT_010:
        ras_srv->ras_mtu = 253;
        atomic_set_bit(&ras_srv->char_notify_state, RAS_ON_DEMAND_DATA_INDICATE);
        atomic_set_bit(&ras_srv->char_notify_state, RAS_DATA_READY_NOTIFY);
        atomic_set_bit(&ras_srv->on_demand_state, SAL_LE_RAS_ON_DEMAND_STATE_IDLE);
        break;
    default:
        LOG_ERR("Invalid test case number(%d).", test_case);
        return -1;
    }

    subevent_result_cb(conn, result);
    return 0;
}

int ras_ctrl_point_send_test(struct bt_conn* conn, struct bt_gatt_attr* attr,
    uint8_t* data, uint16_t len, uint16_t offset,
    uint8_t flags)
{
    on_ras_ctr_pt_write_cb(NULL, attr, (void*)data, len, offset, flags);
    return 0;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
