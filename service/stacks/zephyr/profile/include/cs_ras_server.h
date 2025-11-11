/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef _CS_RAS_SERVER_H_
#define _CS_RAS_SERVER_H_

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include "cs_ras_test.h"

#define CONFIG_BT_CS_TEST 1

#ifndef BIT
#define BIT(n)        (1 << n)
#endif /* BIT */

#define RAS_RSP_TIMEOUT                             K_SECONDS(5)

#define RAS_EMPTY_ARRAY                             (0)

#define SAL_LE_RAS_ROLE_INITIATOR                   (0)
#define SAL_LE_RAS_ROLE_REFLECTOR                   (1)

#define SAL_LE_RAS_SUBEVENT_STEP_MODE_0             (0)
#define SAL_LE_RAS_SUBEVENT_STEP_MODE_1             (1)
#define SAL_LE_RAS_SUBEVENT_STEP_MODE_2             (2)
#define SAL_LE_RAS_SUBEVENT_STEP_MODE_3             (3)

#define SAL_LE_RAS_GATT_NOTIFY           (1)
#define SAL_LE_RAS_GATT_INDICATION       (2)

#define SAL_LE_RAS_SUB_PROCUDURE_HEAD    (12)

#define SAL_LE_RAS_CCCD_NOT_IMPR_CONFIG_ERR    (0xFD)
#define SAL_LE_RAS_CCCD_WR_REQ_REJECT          (0xFC)

#define SAL_LE_RAS_RANGING_MODE_REAL_TIME      (0x01)
#define SAL_LE_RAS_RANGING_MODE_ON_DEMAND      (0x02)
#define SAL_LE_RAS_RANGING_MODE_UNDEFINED      (0xFF)
typedef uint8_t ras_rang_mode_t;

#define SAL_LE_RAS_STORE_PROCEDURE_NUM_MAX     (10)

/**
 * RAS Features format
 * The RAS Features characteristic bit formats are listed in
 * +--------+-----------------------------------------------+
 * | Bit(s) | Definition                                    |
 * +--------+-----------------------------------------------+
 * | 0      | Real-time Ranging Data                        |
 * | 1      | Retrieve Lost Ranging Data Segments           |
 * | 2      | Abort Operation                               |
 * | 3      | Filter Ranging Data                           |
 * | 4–31   | Reserved for Future Use (RFU)                 |
 * +--------+-----------------------------------------------+
 */
#define SAL_LE_RAS_REAL_TIME_RANG_DATA_SUPPROTED    (1 << 0)
#define SAL_LE_RAS_RETRI_LOST_RANG_DATA_SEG         (1 << 1)
#define SAL_LE_RAS_ABORT_OPRATION                   (1 << 2)
#define SAL_LE_RAS_FILTER_RANG_DATA                 (1 << 3)

#define SAL_LE_RAS_CTL_OP_CMD_GET_RANG_DATA                  (0x00)
#define SAL_LE_RAS_CTL_OP_CMD_ACK_RANG_DATA                  (0x01)
#define SAL_LE_RAS_CTL_OP_CMD_RETRIEVE_LOST_RANG_DATA_SEG    (0x02)
#define SAL_LE_RAS_CTL_OP_CMD_ABORT_OPERATION                (0x03)
#define SAL_LE_RAS_CTL_OP_CMD_SET_FILTER                     (0x04)

#define SAL_LE_RAS_CTL_OP_RSP_CMP_RANG_DATA                  (0x00)
#define SAL_LE_RAS_CTL_OP_RSP_CMP_LOST_RANG_DATA_SEG         (0x01)
#define SAL_LE_RAS_CTL_OP_RSP_CODE                           (0x02)

#define SAL_LE_RAS_CTL_OP_RSP_CODE_RESERVED                  (0x00)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_SUCCESS                   (0x01)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED             (0x02)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_INVALID_PARAMS            (0x03)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_PERSISTED                 (0x04)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_ABORT                     (0x05)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_PROCE_NOT_CMP             (0x06)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_SERVER_BUSY               (0x07)
#define SAL_LE_RAS_CTL_OP_RSP_CODE_NO_RECORD_FOUND           (0x08)

#define SAL_LE_RAS_FILTER_MODE_MAX                           (0x04)
#define SAL_LE_RAS_FILTER_MODE_MASK                          (0x03)   // Mask for Mode bits (0-1)
#define SAL_LE_RAS_FILTER_BIT_MASK                           (0xFFFC) // Mask for Filter bits (2-15)

#define SAL_LE_RAS_STEP_DATA_BUF_LEN 2048 /* Maximum GATT characteristic length */
#define SAL_LE_RAS_SWAP_ADD_SUB(a, b) \
	do {				\
		(a) = (a) + (b);    \
		(b) = (a) - (b);    \
		(a) = (a) - (b);    \
	} while(0)

#define COPY_FIELD_IF_ENABLED(_size, _filter_bit)                     \
    do {                                                              \
        if (remaining >= (_size)) {                                   \
            if (filter_mask & (_filter_bit)) {                        \
                memcpy(&buf[out_offset], p, (_size));                 \
                out_offset += (_size);                                \
            }                                                         \
            p += (_size);                                             \
            remaining -= (_size);                                     \
        } else {                                                      \
            LOG_WRN("Field truncated, size=%d", (int)(_size));        \
            remaining = 0;                                            \
        }                                                             \
    } while (0)

enum cs_ras_notify_enable_bit {
	RAS_RTT_DATA_NOTIFY,
	RAS_RTT_DATA_INDICATE,
	RAS_CONTROL_POINT_NOTIFY,
	RAS_CONTROL_POINT_INDICATE,
	RAS_ON_DEMAND_DATA_NOTIFY,
	RAS_ON_DEMAND_DATA_INDICATE,
	RAS_DATA_READY_NOTIFY,
	RAS_DATA_READY_INDICATE,
	RAS_OVER_WRITE_NOTIFY,
	RAS_OVER_WRITE_INDICATE
};

enum {
    SAL_LE_RAS_SVC_IDX,

	SAL_LE_RAS_FEAT_CHAR_IDX,
	SAL_LE_RAS_FEAT_CHAR_VAL_IDX,

	SAL_LE_RAS_RT_DT_CHAR_IDX,
	SAL_LE_RAS_RT_DT_CHAR_VAL_IDX,
	SAL_LE_RAS_RT_DT_CCC_CFG_IDX,

    SAL_LE_RAS_ON_DEM_CHAR_IDX,
	SAL_LE_RAS_ON_DEM_CHAR_VAL_IDX,
	SAL_LE_RAS_ON_DEM_CCC_CFG_IDX,

	SAL_LE_RAS_CTR_PT_CHAR_IDX,
	SAL_LE_RAS_CTR_PT_CHAR_VAL_IDX,
	SAL_LE_RAS_CTR_PT_CCC_CFG_IDX,

	SAL_LE_RAS_DT_RD_CHAR_IDX,
	SAL_LE_RAS_DT_RD_CHAR_VAL_IDX,
	SAL_LE_RAS_DT_RD_CCC_CFG_IDX,

	SAL_LE_RAS_DT_OV_WR_CHAR_IDX,
	SAL_LE_RAS_DT_OV_WR_CHAR_VAL_IDX,
	SAL_LE_RAS_DT_OV_WR_CCC_CFG_IDX,

	RAS_IDX_MAX
};

/*******************************************************************************************
 *
 *  FILTER BIT MAPPING TABLE
 *  ------------------------------------------------------------
 *  Each field can be individually filtered out using the
 *  atomic_t ras_filter[SAL_LE_RAS_FILTER_MODE_MAX] bitmask.
 *
 *  +----------------------------+-------------------------------+
 *  | Field Name                 | Bit Definition                |
 *  +----------------------------+-------------------------------+
 *  | Packet_Quality             | RAS_FILTER_BIT_PKT_QUALITY     (BIT(0))  |
 *  | Packet_NADM                | RAS_FILTER_BIT_PKT_NADM        (BIT(1))  |
 *  | Packet_RSSI                | RAS_FILTER_BIT_PKT_RSSI        (BIT(2))  |
 *  | Packet_Antenna             | RAS_FILTER_BIT_PKT_ANTENNA     (BIT(3))  |
 *  | Packet_PCT1                | RAS_FILTER_BIT_PKT_PCT1        (BIT(4))  |
 *  | Packet_PCT2                | RAS_FILTER_BIT_PKT_PCT2        (BIT(5))  |
 *  | Measured_Freq_Offset       | RAS_FILTER_BIT_FREQ_OFFSET     (BIT(6))  |
 *  | ToA_ToD_Initiator          | RAS_FILTER_BIT_TOA_TOD         (BIT(7))  |
 *  | ToD_ToA_Reflector          | RAS_FILTER_BIT_TOD_TOA         (BIT(8))  |
 *  | Antenna_Permutation_Index  | RAS_FILTER_BIT_ANT_PERM_IDX    (BIT(9))  |
 *  | Tone_PCT[k]                | RAS_FILTER_BIT_TONE_PCT        (BIT(10)) |
 *  | Tone_Quality_Indicator[k]  | RAS_FILTER_BIT_TONE_QUALITY    (BIT(11)) |
 *  +----------------------------+-------------------------------+
 *
 *******************************************************************************************/
typedef enum {
    RAS_FILTER_BIT_PKT_QUALITY      = BIT(0),
    RAS_FILTER_BIT_PKT_NADM         = BIT(1),
    RAS_FILTER_BIT_PKT_RSSI         = BIT(2),
    RAS_FILTER_BIT_PKT_ANTENNA      = BIT(3),
    RAS_FILTER_BIT_PKT_PCT1         = BIT(4),
    RAS_FILTER_BIT_PKT_PCT2         = BIT(5),
    RAS_FILTER_BIT_FREQ_OFFSET      = BIT(6),
    RAS_FILTER_BIT_TOA_TOD          = BIT(7),
    RAS_FILTER_BIT_TOD_TOA          = BIT(8),
    RAS_FILTER_BIT_ANT_PERM_IDX     = BIT(9),
    RAS_FILTER_BIT_TONE_PCT         = BIT(10),
    RAS_FILTER_BIT_TONE_QUALITY     = BIT(11),
} ras_filter_bits_t;

enum {
	SAL_LE_RAS_MODE_0_FILTER_PACKET_QUALITY,
	SAL_LE_RAS_MODE_0_FILTER_PACKET_RSSI,
	SAL_LE_RAS_MODE_0_FILTER_PACKET_ANTENNA,
	SAL_LE_RAS_MODE_0_FILTER_MEASURED_FREQ_OFFSET,
	SAL_LE_RAS_MODE_0_FILTER_MAX,
};

enum {
	SAL_LE_RAS_MODE_1_FILTER_PACKET_QUALITY,
	SAL_LE_RAS_MODE_1_FILTER_PACKET_NADM,
	SAL_LE_RAS_MODE_1_FILTER_PACKET_RSSI,
	SAL_LE_RAS_MODE_1_FILTER_TOD_TOA,
	SAL_LE_RAS_MODE_1_FILTER_PACKET_ANTENNA,
	SAL_LE_RAS_MODE_1_FILTER_PACKET_PCT_1,
	SAL_LE_RAS_MODE_1_FILTER_PACKET_PCT_2,
	SAL_LE_RAS_MODE_1_FILTER_MAX,
};

enum {
	SAL_LE_RAS_MODE_2_FILTER_ANTENNA_PERMUTATION_INDEX,
	SAL_LE_RAS_MODE_2_FILTER_TONE_PCT,
	SAL_LE_RAS_MODE_2_FILTER_TONE_QUALITY_INDICATOR,
	SAL_LE_RAS_MODE_2_FILTER_ANTENNA_PATH_1,
	SAL_LE_RAS_MODE_2_FILTER_ANTENNA_PATH_2,
	SAL_LE_RAS_MODE_2_FILTER_ANTENNA_PATH_3,
	SAL_LE_RAS_MODE_2_FILTER_ANTENNA_PATH_4,
	SAL_LE_RAS_MODE_2_FILTER_MAX,
};

enum {
	SAL_LE_RAS_MODE_3_FILTER_PACKET_QUALITY,
	SAL_LE_RAS_MODE_3_FILTER_PACKET_NADM,
	SAL_LE_RAS_MODE_3_FILTER_PACKET_RSSI,
	SAL_LE_RAS_MODE_3_FILTER_TOD_TOA,
	SAL_LE_RAS_MODE_3_FILTER_PACKET_ANTENNA,
	SAL_LE_RAS_MODE_3_FILTER_PACKET_PCT_1,
	SAL_LE_RAS_MODE_3_FILTER_PACKET_PCT_2,
	SAL_LE_RAS_MODE_3_FILTER_ANTENNA_PERMUTATION_INDEX,
	SAL_LE_RAS_MODE_3_FILTER_TONE_PCT,
    SAL_LE_RAS_MODE_3_FILTER_TONE_QUALITY_INDICATOR,
	SAL_LE_RAS_MODE_3_FILTER_ANTENNA_PATH_1,
	SAL_LE_RAS_MODE_3_FILTER_ANTENNA_PATH_2,
	SAL_LE_RAS_MODE_3_FILTER_ANTENNA_PATH_3,
	SAL_LE_RAS_MODE_3_FILTER_ANTENNA_PATH_4,
	SAL_LE_RAS_MODE_3_FILTER_MAX,
};

enum {
	SAL_LE_RAS_ON_DEMAND_STATE_IDLE,
	SAL_LE_RAS_ON_DEMAND_STATE_DATA_READY_INDICATE,
	SAL_LE_RAS_ON_DEMAND_STATE_GET_RANGING_DATA_RSP,
	SAL_LE_RAS_ON_DEMAND_STATE_RANGING_DATA_SENDING,
	SAL_LE_RAS_ON_DEMAND_STATE_RANGING_DATA_COMPLETE_SEND,
	SAL_LE_RAS_ON_DEMAND_STATE_RANGING_DATA_ACK_RECV,
	SAL_LE_RAS_ON_DEMAND_STATE_RANGING_DATA_RSP,
	SAL_LE_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_RECV,
	SAL_LE_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_RSP,
	SAL_LE_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_COMPLETE,
	SAL_LE_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_ACK,
	SAL_LE_RAS_ON_DEMAND_STATE_NUMS,
};

typedef enum {
    ABORT_OPERATION = 0,
    OTHER_OPERATION = 1
} ras_opcode_t;

typedef struct ras_segment_t {
	sys_snode_t seg_node;
    uint16_t seg_idx;  // Segment index
	uint16_t len; // Segment data len
    uint8_t data[RAS_EMPTY_ARRAY];    // Segment data
} ras_segment_t;

typedef struct ras_rang_on_demand_t {
	bool proc_used;
    uint16_t count;
	/* RAS On-demand Response Timeout expired (timer */
	struct k_work_delayable on_demand_work;
	sys_slist_t seg_list;
	ras_segment_t *seg;
} ras_rang_on_demand_t;

typedef struct {
    ras_opcode_t opcode;        // current Opcode
    int is_processing;            // Is processing or not
    int is_data_pending;           // Is data pending or not
    uint8_t response_code;   // response code
} ras_control_point_t;

typedef struct {
    uint16_t step_data_attr_handle;
    struct bt_conn *connection;
    uint8_t latest_local_steps[SAL_LE_RAS_STEP_DATA_BUF_LEN];
	uint8_t rt_dt_ccc_cfg;
    uint8_t ras_dt_rd_indicating;
	uint8_t ras_role;
    uint32_t ras_mtu;
    uint32_t ras_seg_offset;
    uint32_t remaining_len;
    uint8_t ras_seg_idx;
    struct bt_gatt_indicate_params ras_dt_rd_ind_params;
    uint32_t ras_feature;
	atomic_t char_notify_state;
	atomic_t ras_filter[SAL_LE_RAS_FILTER_MODE_MAX];
	atomic_t on_demand_state;
	ras_rang_on_demand_t subevent[SAL_LE_RAS_STORE_PROCEDURE_NUM_MAX];
	ras_control_point_t control_point;
	sys_snode_t *on_deman_curr_node;
} sal_le_ras_srv_env_t;

#define RAS_ON_DEMAND_WORK_PICK(_w) CONTAINER_OF(k_work_delayable_from_work(_w), \
				 struct ras_rang_on_demand_t, on_demand_work)

int write_cs_reflector_step_data(void);
int le_cs_enable(void);

#ifdef CONFIG_BT_CS_TEST
#define BT_GATT_NOTIFY_CB(conn, params)    bt_gatt_notify_cb_test(conn, params)
#define BT_GATT_ATTR_READ(conn, attr, buf, buf_len, offset, value, value_len)    \
						bt_gatt_attr_read_test(conn, attr, buf, buf_len, offset, value, value_len)
#define BT_GATT_NOTIFY(conn, attr, data, len)    bt_gatt_notify_test(conn, attr, data, len)
#define BT_GATT_INDICATE(conn, params)     bt_gatt_indicate_test(conn, params)
#else
#define BT_GATT_NOTIFY_CB(conn, params)    bt_gatt_notify_cb(conn, params)
#define BT_GATT_ATTR_READ(conn, attr, buf, buf_len, offset, value, value_len)    \
						bt_gatt_attr_read(conn, attr, buf, buf_len, offset, value, value_len)
#define BT_GATT_NOTIFY(conn, attr, data, len)    bt_gatt_notify(conn, attr, data, len)
#define BT_GATT_INDICATE(conn, params)     bt_gatt_indicate(conn, params)
#endif /* CONFIG_BT_CS_TEST */

struct bt_gatt_attr *ras_get_gatt_attr(void);

int ras_subevent_recv_test(ras_rang_mode_t mode, ras_testcase_t test_case,
	                        struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result);
int ras_ctrl_point_send_test(struct bt_conn *conn, struct bt_gatt_attr *attr,
	                    uint8_t* data, uint16_t len, uint16_t offset,
			 			uint8_t flags);
#endif /* _CS_RAS_SERVER_H_ */
