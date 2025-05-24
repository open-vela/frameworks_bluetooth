/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/cs.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include "common.h"

#define CS_CONFIG_ID     0
#define NUM_MODE_0_STEPS 1
#define RAS_SEG_HEADER_SIZE    4

extern const char *bt_hex(const void *buf, size_t len);

static K_SEM_DEFINE(sem_remote_capabilities_obtained, 0, 1);
static K_SEM_DEFINE(sem_config_created, 0, 1);
static K_SEM_DEFINE(sem_cs_security_enabled, 0, 1);
static K_SEM_DEFINE(sem_procedure_done, 0, 1);
static K_SEM_DEFINE(sem_connected, 0, 1);
static K_SEM_DEFINE(sem_discovered, 0, 1);
static K_SEM_DEFINE(sem_written, 0, 1);

enum {
    RAS_SVC_IDX,

	RAS_FEAT_CHAR_IDX,
	RAS_FEAT_CHAR_VAL_IDX,

	RAS_RT_DT_CHAR_IDX,
	RAS_RT_DT_CHAR_VAL_IDX,
	RAS_RT_DT_CCC_CFG_IDX,

    RAS_ON_DEM_CHAR_IDX,
	RAS_ON_DEM_CHAR_VAL_IDX,
	RAS_ON_DEM_CCC_CFG_IDX,

	RAS_CTR_PT_CHAR_IDX,
	RAS_CTR_PT_CHAR_VAL_IDX,
	RAS_CTR_PT_CCC_CFG_IDX,

	RAS_DT_RD_CHAR_IDX,
	RAS_DT_RD_CHAR_VAL_IDX,
	RAS_DT_RD_CCC_CFG_IDX,

	RAS_DT_OV_WR_CHAR_IDX,
	RAS_DT_OV_WR_CHAR_VAL_IDX,
	RAS_DT_OV_WR_CCC_CFG_IDX,
	
	RAS_IDX_MAX
};

static uint16_t step_data_attr_handle;
static struct bt_conn *connection;
static uint8_t latest_local_steps[STEP_DATA_BUF_LEN];
static uint8_t ras_dt_rd_indicating;
static uint32_t ras_mtu = 0;
static uint32_t ras_seg_offset = 0;
static uint32_t remaining_len = 0;
static uint8_t ras_seg_idx = 0;
static struct bt_gatt_indicate_params ras_dt_rd_ind_params;

static const char sample_str[] = "CS Sample";
static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, "CS Sample", sizeof(sample_str) - 1),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(0x185B)),
};

static uint32_t ras_feature = 0x07000007;

// static ssize_t on_attr_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
// 				const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

// static ssize_t on_attr_ras_feature_read_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
// 				const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

/** @brief LE Audio Attribute User Data. */
struct bt_ras_attr_user_data {
	/** Attribute read callback */
	ssize_t (*read)(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset);

	/** Attribute write callback */
	ssize_t	(*write)(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags);

	/** Attribute user data */
	void *user_data;
};

static void split_segment(struct bt_conn *conn, uint8_t* buf, int len);

ssize_t	on_ras_ctr_pt_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
    printk("RAS Control Point cb.\n");
	printk("offset:%d, flags:%d, buf[%d]:%s", offset, flags, len, bt_hex(buf, len));
	return len;
}

ssize_t ras_feature_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	printk("RAS feature read cb.\n");
	printk("offset:%d, buf[%d]:%s", offset, len, bt_hex(buf, len));
	return bt_gatt_attr_read(conn, attr, buf, len, offset, (uint8_t *)&ras_feature, sizeof(ras_feature));
}

static void range_rt_dt_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printk("The range real-time data ccc value is change to (%d)\n", value);
}

static void range_on_dem_dt_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printk("The range on-dem data ccc value is change to (%d)\n", value);
}

static void range_ctr_pt_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printk("The range control point data ccc value is change to (%d)\n", value);
}

static void range_dt_rd_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printk("The range data ready data ccc value is change to (%d)\n", value);
}

static void range_dt_ov_wr_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	printk("The range over write data ccc value is change to (%d)\n", value);
}

#define BT_RAS_ATTR_USER_DATA_INIT(_read, _write, _user_data) \
{ \
	.read = _read, \
	.write = _write, \
	.user_data = _user_data, \
}

/** Helper to define LE RAS characteristic. */
#define BT_RAS_CHRC(_uuid, _props, _perm, _read, _write, _user_data) \
	BT_GATT_CHARACTERISTIC(_uuid, _props, _perm, _read, _write, \
			       ((struct bt_ras_attr_user_data[]) { \
				BT_RAS_ATTR_USER_DATA_INIT(_read, _write, _user_data), \
			       }))

#define BT_RAS_CHRC_USER_DATA(_attr) \
	(((struct bt_ras_attr_user_data *)(_attr)->user_data)->user_data)

/** Helper to define LE Audio CCC descriptor. */
#define BT_RAS_CCC(_changed)								\
	BT_GATT_CCC_MANAGED(((struct _bt_gatt_ccc[])					\
		{BT_GATT_CCC_INITIALIZER(_changed, NULL, NULL)}),	\
		(BT_GATT_PERM_READ))

#define BT_RAS_SERVICE_DEFINITION() { \
		BT_GATT_PRIMARY_SERVICE(BT_UUID_RANGING), \
		BT_RAS_CHRC(BT_UUID_RANG_FEAT, \
				BT_GATT_CHRC_READ, \
				BT_GATT_PERM_READ, \
				ras_feature_read, NULL, NULL), \
		BT_RAS_CHRC(BT_UUID_RANG_RT_DT, \
				(BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE), \
				BT_GATT_PERM_READ, \
				NULL, NULL, NULL), \
		BT_GATT_CCC(range_rt_dt_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)), \
		BT_RAS_CHRC(BT_UUID_RANG_ON_DEM_DT, \
				(BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE), \
				BT_GATT_PERM_READ, \
				NULL, on_ras_ctr_pt_write_cb, NULL), \
		BT_GATT_CCC(range_on_dem_dt_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)), \
		BT_RAS_CHRC(BT_UUID_RANG_RAS_CTR_POINT, \
				(BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_INDICATE), \
				(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), \
				NULL, NULL, NULL), \
		BT_GATT_CCC(range_ctr_pt_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)), \
		BT_RAS_CHRC(BT_UUID_RANG_DT_RD, \
				BT_GATT_CHRC_INDICATE, \
				BT_GATT_PERM_READ, \
				NULL, NULL, NULL), \
		BT_GATT_CCC(range_dt_rd_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)), \
		BT_RAS_CHRC(BT_UUID_RANG_DT_OV_WR, \
				BT_GATT_CHRC_INDICATE, \
				BT_GATT_PERM_READ, \
				NULL, NULL, NULL), \
		BT_GATT_CCC(range_dt_ov_wr_ccc_cfg_changed, (BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)),\
	}

#define ASCS_ASE_CHAR_ATTR_COUNT 3 /* declaration + value + cccd */

static void ras_dt_rd_indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
	if (remaining_len) {
		split_segment(conn, &latest_local_steps[ras_seg_offset], remaining_len);
	}
}

static void ras_dt_rd_indicate_destroy(struct bt_gatt_indicate_params *params)
{
	printk("Indication complete\n");
	ras_dt_rd_indicating = 0U;
}

static struct bt_gatt_attr ras_attrs[RAS_IDX_MAX] = BT_RAS_SERVICE_DEFINITION();
static struct bt_gatt_service ras_svc = (struct bt_gatt_service)BT_GATT_SERVICE(ras_attrs);

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

static void split_segment(struct bt_conn *conn, uint8_t* buf, int len)
{
	// remaining_len = len;

	if (remaining_len > ras_mtu - RAS_SEG_HEADER_SIZE - 1) {
		int curr_seg_size = ras_mtu - RAS_SEG_HEADER_SIZE - 1;
		printk("data send: offset:%lu, len:%d\n", ras_seg_offset, curr_seg_size);

		// update offset
		ras_seg_offset += curr_seg_size;
        uint8_t* send_buf = malloc(curr_seg_size + 1);
		
		remaining_len -= curr_seg_size;
		send_buf[0] = (ras_seg_idx == 0) ? (0x01) : (ras_seg_idx << 2);
		if (remaining_len == 0) {
			ras_seg_idx = 0;
			send_buf[0] |= (0x01 << 1);
			remaining_len = 0;
		    ras_seg_offset = 0;
		}
		
		ras_seg_idx++;
		memcpy(&send_buf[1], buf, curr_seg_size);
		ras_dt_rd_ind_params.attr = &ras_attrs[RAS_RT_DT_CHAR_IDX];
		ras_dt_rd_ind_params.func = ras_dt_rd_indicate_cb;
		ras_dt_rd_ind_params.destroy = ras_dt_rd_indicate_destroy;
		ras_dt_rd_ind_params.data = send_buf;
		ras_dt_rd_ind_params.len = curr_seg_size + 1;

		if ((send_buf[0] & 0x01) == 0x01) {
			printk("First seg, data(%d):%s\n", curr_seg_size + 1, bt_hex(send_buf, curr_seg_size + 1));
		} else {
			printk("The %d seg, data(%d):%s\n", send_buf[0] >> 2, curr_seg_size + 1, bt_hex(send_buf, curr_seg_size + 1));
		}

        
		if (bt_gatt_notify(NULL, &ras_attrs[RAS_RT_DT_CHAR_IDX], send_buf, curr_seg_size + 1) == 0) {
			if (remaining_len) {
				split_segment(conn, &latest_local_steps[ras_seg_offset], remaining_len);
			}
			ras_dt_rd_indicating = 1U;
			free(send_buf);
		} else {
			printk("ras data ready Indicate fail.\n");
			free(send_buf);
			return;
		}
	} else {
        int curr_seg_size = len;
		uint8_t* send_buf = malloc(curr_seg_size + 1);
		send_buf[0] = (ras_seg_idx == 0) ? (0x01) : (ras_seg_idx << 2);
        send_buf[0] |= (0x01 << 1);
        memcpy(&send_buf[1], buf, curr_seg_size);
		ras_dt_rd_ind_params.attr = &ras_attrs[RAS_RT_DT_CHAR_IDX];
		ras_dt_rd_ind_params.func = ras_dt_rd_indicate_cb;
		ras_dt_rd_ind_params.destroy = ras_dt_rd_indicate_destroy;
		ras_dt_rd_ind_params.data = send_buf;
		ras_dt_rd_ind_params.len = curr_seg_size + 1;
        ras_dt_rd_indicating = 0U;
        printk("The last(%d) seg, data(%d):%s\n", send_buf[0] >> 2, curr_seg_size + 1, bt_hex(send_buf, curr_seg_size + 1));
		printk("ras_dt_rd_indicating:%d\n", ras_dt_rd_indicating);
		if (bt_gatt_notify(NULL, &ras_attrs[RAS_RT_DT_CHAR_IDX], send_buf, curr_seg_size + 1) == 0) {
			free(send_buf);
			
		} else {
			printk("ras data ready Indicate fail.\n");
			free(send_buf);
			return;
		}

		ras_seg_idx = 0;
		ras_dt_rd_indicating = 0U;
		remaining_len = 0;
		ras_seg_offset = 0;
	}

	return;
}

// static struct bt_gatt_attr gatt_attributes[] = {
// 	BT_GATT_PRIMARY_SERVICE(&step_data_svc_uuid),
// 	BT_GATT_CHARACTERISTIC(&step_data_char_uuid.uuid, BT_GATT_CHRC_WRITE,
// 			       BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE, NULL,
// 			       on_attr_write_cb, NULL),
// };

void on_cccd_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	return;
}

// static ssize_t on_attr_ras_feature_read_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
// 				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
// {
// 	return len;
// }

// static ssize_t on_attr_ras_ctr_point_write_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
// 				const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
// {
// 	return len;
// }

static void write_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	if (err) {
		printk("Write failed (err %d)\n", err);

		return;
	}
}

static int write_cs_reflector_step_data(void)
{
	int err;
	struct bt_gatt_write_params write_params;
    write_params.func = write_func;
	write_params.handle = step_data_attr_handle;
	write_params.length = STEP_DATA_BUF_LEN;
	write_params.data = &latest_local_steps[0];
	write_params.offset = 0;

	err = bt_gatt_write(connection, &write_params);
	if (err) {
		printk("Write failed (err %d)\n", err);
		return 0;
	}

	return 0;
}

/**
 * Input: data points to the original buffer, data_len is the total length of the input
 * Output: buf stores the transformed result, function returns the length of the output buffer
 */
static size_t transfrom_step_data_to_ras_format(uint8_t* data, size_t data_len, uint8_t *buf)
{
    size_t in_offset = 0;    // Offset in the input buffer
	size_t out_offset = 0; // Offset in the Output buffer

	while (in_offset + 3 <= data_len) {
		uint8_t step_mode = data[in_offset];
		uint8_t step_channel = data[in_offset + 1];
		uint8_t step_data_length = data[in_offset + 2];

		// Check if there is enough data remaining for the full segment
		if (in_offset + 3 + step_data_length > data_len) {
            printk("Incomplete data, exiting early.\n");
			break;
		}

		// Copy Step_Mode
		buf[out_offset++] = step_mode;

		// Copy Step_Data
		memcpy(&buf[out_offset], &data[in_offset + 3], step_data_length);
		out_offset += step_data_length;

		// Move to the next data segment
		in_offset += 3 + step_data_length;
	}

	return out_offset;
}

#define SWAP_ADD_SUB(a, b) \
	do {				\
		(a) = (a) + (b);    \
		(b) = (a) - (b);    \
		(a) = (a) - (b);    \
	} while(0)

static void subevent_result_cb(struct bt_conn *conn, struct bt_conn_le_cs_subevent_result *result)
{
	static int i = 0;
	if (result->step_data_buf) {
		if (result->step_data_buf->len <= STEP_DATA_BUF_LEN) {
			memcpy(latest_local_steps, result->step_data_buf->data,
			       result->step_data_buf->len);
			printk("step data[%d]:%s\n", i++, bt_hex(result->step_data_buf->data, result->step_data_buf->len));
		} else {
			printk("Not enough memory to store step data. (%d > %d)\n",
			       result->step_data_buf->len, STEP_DATA_BUF_LEN);
		}
	}

	printk("The CS procedure state(%d), ras_dt_rd_indicating:%d\n", result->header.procedure_done_status, ras_dt_rd_indicating);
	memset(latest_local_steps, 0, sizeof(latest_local_steps));

	if (result->header.procedure_done_status == BT_CONN_LE_CS_PROCEDURE_COMPLETE) {
		uint8_t* stream_buf = latest_local_steps;
		int bit_offset = 0;
		
		ras_write_bits(stream_buf, &bit_offset, result->header.config_id, 4);
		ras_write_bits(stream_buf, &bit_offset, result->header.procedure_counter, 12);
		SWAP_ADD_SUB(stream_buf[0], stream_buf[1]);
		ras_write_bits(stream_buf, &bit_offset, result->header.reference_power_level, 8);
		ras_write_bits(stream_buf, &bit_offset, result->header.num_antenna_paths, 8);
		ras_write_bits(stream_buf, &bit_offset, result->header.start_acl_conn_event, 16);
		ras_write_bits(stream_buf, &bit_offset, result->header.frequency_compensation, 16);
		ras_write_bits(stream_buf, &bit_offset, result->header.procedure_done_status, 4);
		ras_write_bits(stream_buf, &bit_offset, result->header.subevent_done_status, 4);
		ras_write_bits(stream_buf, &bit_offset, result->header.procedure_abort_reason, 4);
		ras_write_bits(stream_buf, &bit_offset, result->header.subevent_abort_reason, 4);
		ras_write_bits(stream_buf, &bit_offset, result->header.reference_power_level, 8);
		ras_write_bits(stream_buf, &bit_offset, result->header.num_steps_reported, 8);

		ras_seg_offset = 0;
		remaining_len = transfrom_step_data_to_ras_format(result->step_data_buf->data, result->step_data_buf->len, stream_buf + 12);
		remaining_len += 12;
		printk("stream head:%s.", bt_hex(stream_buf, 12));
		printk("stream_buf(%ld):%s.\n", remaining_len, bt_hex(stream_buf + 12, remaining_len - 12));
		printk("data ready indiacte count(%d)., uuid:0x%x, handle:%d\n", 
			   result->header.procedure_counter, 
			   BT_UUID_16(ras_attrs[RAS_DT_RD_CHAR_VAL_IDX].uuid)->val, ras_attrs[RAS_DT_RD_CHAR_VAL_IDX].handle);
		printk("procedure_counter:0x%x, config_id:0x%x, reference_power_level:0x%x",
		    result->header.procedure_counter, result->header.config_id, result->header.reference_power_level);
		printk("num_antenna_paths:0x%x, start_acl_conn_event:0x%x, frequency_compensation:0x%x",
		    result->header.num_antenna_paths, result->header.start_acl_conn_event, result->header.frequency_compensation);
		printk("procedure_done_status:0x%x, subevent_done_status:0x%x,   :0x%x", 
		    result->header.procedure_done_status, result->header.subevent_done_status, result->header.procedure_abort_reason);
		printk("subevent_abort_reason:0x%x, reference_power_level:0x%x, num_steps_reported:0x%x",
		    result->header.subevent_abort_reason, result->header.reference_power_level, result->header.num_steps_reported);
		printk("mode:0x%x, channel:0x%x, len:0x%x", result->step_data_buf->data[0], result->step_data_buf->data[1],
		     result->step_data_buf->data[2]);

		split_segment(conn, stream_buf, remaining_len);
	}

	return;
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	ras_mtu =  bt_gatt_get_mtu(conn);
	printk("MTU exchange %s (%u)\n", err == 0U ? "success" : "failed", bt_gatt_get_mtu(conn));
}

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	printk("Connected to %s (err 0x%02X)\n", addr, err);

	__ASSERT(connection == conn, "Unexpected connected callback");

	if (err) {
		bt_conn_unref(conn);
		connection = NULL;
	}

	connection = bt_conn_ref(conn);

	static struct bt_gatt_exchange_params mtu_exchange_params = {.func = mtu_exchange_cb};

	err = bt_gatt_exchange_mtu(connection, &mtu_exchange_params);
	if (err) {
		printk("%s: MTU exchange failed (err %d)\n", __func__, err);
	}

	const struct bt_le_cs_set_default_settings_param default_settings = {
		.enable_initiator_role = false,
		.enable_reflector_role = true,
		.cs_sync_antenna_selection = BT_LE_CS_ANTENNA_SELECTION_OPT_REPETITIVE,
		.max_tx_power = BT_HCI_OP_LE_CS_MAX_MAX_TX_POWER,
	};

	err = bt_le_cs_set_default_settings(connection, &default_settings);
	if (err) {
		printk("Failed to configure default CS settings (err %d)\n", err);
	}
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02X)\n", reason);

	bt_conn_unref(conn);
	connection = NULL;

	int err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
					      BT_GAP_ADV_FAST_INT_MAX_1, NULL),
			      ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising start again.\n");
}

static void remote_capabilities_cb(struct bt_conn *conn, struct bt_conn_le_cs_capabilities *params)
{
	ARG_UNUSED(params);
	printk("CS capability exchange completed.\n");
	printk("num_config_supported:%d, max_consecutive_procedures_supported:%d", 
	params->num_config_supported, params->max_consecutive_procedures_supported);
	printk("num_antennas_supported:%d, max_antenna_paths_supported:%d.", 
		params->num_antennas_supported, params->max_antenna_paths_supported);
	printk("initiator_supported:%d, reflector_supported:%d", 
	   params->initiator_supported, params->reflector_supported);
	printk("mode_3_supported:%d, rtt_aa_only_precision:%d",
	   params->mode_3_supported, params->rtt_aa_only_precision);
	printk("rtt_sounding_precision:%d, rtt_random_payload_precision:%d",
	   params->rtt_sounding_precision, params->rtt_random_payload_precision);
	printk("rtt_aa_only_n:%d, rtt_sounding_n:%d, rtt_random_payload_n:%d",
	  params->rtt_aa_only_n, params->rtt_sounding_n, params->rtt_random_payload_n);
	printk("phase_based_nadm_sounding_supported:%d, phase_based_nadm_random_supported:%d",
	  params->phase_based_nadm_sounding_supported, params->phase_based_nadm_random_supported);
	printk("cs_sync_2m_phy_supported:%d, cs_sync_2m_2bt_phy_supported:%d",
	  params->cs_sync_2m_phy_supported, params->cs_sync_2m_2bt_phy_supported);
	printk("cs_without_fae_supported:%d, chsel_alg_3c_supported:%d",
	  params->cs_without_fae_supported, params->chsel_alg_3c_supported);
	printk("pbr_from_rtt_sounding_seq_supported:%d, t_ip1_times_supported:%d",
	  params->pbr_from_rtt_sounding_seq_supported, params->t_ip1_times_supported);
	printk("t_ip2_times_supported:%d, t_fcs_times_supported:%d",
	  params->t_ip2_times_supported, params->t_fcs_times_supported);
	printk("t_pm_times_supported:%d, t_sw_time:%d, tx_snr_capability:%d",
	  params->t_pm_times_supported, params->t_sw_time, 
	  params->tx_snr_capability);
	// k_sem_give(&sem_remote_capabilities_obtained);
}

static void config_created_cb(struct bt_conn *conn, struct bt_conn_le_cs_config *config)
{
	printk("CS config creation complete. ID: %d\n", config->id);
	printk("main_mode_type:%d, sub_mode_type:%d", 
	    config->main_mode_type, config->sub_mode_type);
	printk("min_main_mode_steps:%d, max_main_mode_steps:%d", 
	    config->min_main_mode_steps, config->max_main_mode_steps);
	printk("main_mode_repetition:%d, mode_0_steps:%d",
	    config->main_mode_repetition, config->mode_0_steps);
	printk("role:%d, rtt_type:%d, cs_sync_phy:%d", 
	    config->role, config->rtt_type, config->cs_sync_phy);
	printk("channel_map_repetition:%d, channel_selection_type:%d",
	    config->channel_map_repetition, config->channel_selection_type);
	printk("ch3c_shape:%d, ch3c_jump:%d", config->ch3c_shape, config->ch3c_jump);
	printk("t_ip1_time_us:%d, t_ip2_time_us:%d", 
	    config->t_ip1_time_us, config->t_ip2_time_us);
	printk("t_fcs_time_us:%d, t_pm_time_us:%d", config->t_fcs_time_us, config->t_pm_time_us);
    printk("channel_map:0x%x%x%x%x%x%x%x%x%x%x.", 
	    config->channel_map[0], config->channel_map[1], config->channel_map[2],
	    config->channel_map[3], config->channel_map[4], config->channel_map[5],
	    config->channel_map[6], config->channel_map[7], config->channel_map[8], 
		config->channel_map[9]);
	// k_sem_give(&sem_config_created);
}

static void security_enabled_cb(struct bt_conn *conn)
{
	printk("CS security enabled.\n");
	// k_sem_give(&sem_cs_security_enabled);
}

static void procedure_enabled_cb(struct bt_conn *conn,
				 struct bt_conn_le_cs_procedure_enable_complete *params)
{
	if (params->state == 1) {
		printk("CS procedures enabled.\n");
	} else {
		printk("CS procedures disabled.\n");
	}

	printk("config_id:%d, tone_antenna:%d, tx_power:%d, subevents_per_event:%d\n", 
		params->config_id, params->tone_antenna_config_selection, params->selected_tx_power, params->subevents_per_event);
	printk("subevent_interval:%d, event_interval:%d, procedure_interval:%d, procedure_count:%d, max_procedure_len:%d\n",
	    params->subevent_interval, params->event_interval, params->procedure_interval, params->procedure_count, params->max_procedure_len);

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

int vela_le_cs_enable(void)
{
	int err;

	printk("Starting Channel Sounding Demo\n");

	err = bt_gatt_service_register(&ras_svc);
	if (err != 0) {
		printk("Failed to register Ranging Service in gatt DB");
		return err;
	}

	bt_conn_cb_register(&conn_cbs);

	err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN, BT_GAP_ADV_FAST_INT_MIN_1,
					      BT_GAP_ADV_FAST_INT_MAX_1, NULL),
			      ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	printk("Advertising starting.\n");

	return 0;
}
