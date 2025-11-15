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

#ifndef _CS_RAS_CLIENT_H_
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

enum ras_svc_disc_state_bit {
	DISC_RAS_RANG_RT_DT,
	DISC_RAS_NAME,
	DISC_RAS_TYPE,
	DISC_RAS_SIZE,
	DISC_RAS_ID,
};

typedef struct {
	uint16_t rang_realtime_data_handle;
	uint16_t rang_feature_handle;
	uint16_t rang_on_demond_data_handle;
	uint16_t rang_ras_ctr_point_handle;
	uint16_t rang_data_ready_handle;
	uint16_t rang_data_ov_wr_handle;
    uint16_t step_data_attr_handle;
    struct bt_conn *connection;
    uint32_t ras_mtu;
    uint32_t ras_seg_offset;
    uint32_t remaining_len;
    uint8_t ras_seg_idx;
    struct bt_gatt_indicate_params ras_dt_rd_ind_params;
    uint32_t ras_feature;
    atomic_t disc_state;
	uint8_t *ras_svc_data;
	uint8_t *ras_cli_data;
} sal_le_ras_cli_env_t;

#endif /* _CS_RAS_CLIENT_H_ */