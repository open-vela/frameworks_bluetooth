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
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

#define SAL_LE_RAS_GATT_NOTIFY           (1)
#define SAL_LE_RAS_GATT_INDICATION       (2)

#define SAL_LE_RAS_SUB_PROCUDURE_HEAD    (12)

#define SAL_LE_RAS_CCCD_NOT_IMPR_CONFIG_ERR    (0xFD)
#define SAL_LE_RAS_CCCD_WR_REQ_REJECT          (0xFC)

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




#define SAL_LE_RAS_STEP_DATA_BUF_LEN 2048 /* Maximum GATT characteristic length */
#define SAL_LE_RAS_SWAP_ADD_SUB(a, b) \
	do {				\
		(a) = (a) + (b);    \
		(b) = (a) - (b);    \
		(a) = (a) - (b);    \
	} while(0)

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

typedef struct {
    uint16_t step_data_attr_handle;
    struct bt_conn *connection;
    uint8_t latest_local_steps[SAL_LE_RAS_STEP_DATA_BUF_LEN];
	uint8_t rt_dt_ccc_cfg;
    uint8_t ras_dt_rd_indicating;
    uint32_t ras_mtu;
    uint32_t ras_seg_offset;
    uint32_t remaining_len;
    uint8_t ras_seg_idx;
    struct bt_gatt_indicate_params ras_dt_rd_ind_params;
    uint32_t ras_feature;
} sal_le_ras_srv_env_t;

int write_cs_reflector_step_data(void);
int le_cs_enable(void);

#endif /* _CS_RAS_SERVER_H_ */
