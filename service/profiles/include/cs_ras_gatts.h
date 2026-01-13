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
#ifndef _CS_RAS_GATTS_H_
#define _CS_RAS_GATTS_H_

#include "bt_addr.h"
#include "bt_gatt_defs.h"
#include "bt_status.h"

#define BT_UUID_RANGING_VAL 0x185B
#define BT_UUID_RANG_FEAT_VAL 0x2C14
#define BT_UUID_RANG_RTT_DT_VAL 0x2C15
#define BT_UUID_RANG_ON_DEM_DT_VAL 0x2C16
#define BT_UUID_RANG_RAS_CTR_POINT_VAL 0x2C17
#define BT_UUID_RANG_DT_RD_VAL 0x2C18
#define BT_UUID_RANG_DT_OV_WR_VAL 0x2C19

#define RAS_RTT_DATA_CCC_CFG_CHANGE_EVT (0)
#define RAS_ON_DEMAND_DATA_CCC_CFG_CHANGE_EVT (1)
#define RAS_CTR_PT_CCC_CFG_CHANGE_EVT (2)
#define RAS_DATA_READY_CCC_CFG_CHANGE_EVT (3)
#define RAS_OVER_WRITE_CCC_CFG_CHANGE_EVT (4)
typedef uint8_t ras_ccc_cfg_change_evt_t;

#define RAS_REAL_TIME_CHAR_SEND (0)
#define RAS_ON_DEMAND_CHAR_SEND (1)
#define RAS_DATA_READY_CHAR_SEND (2)
#define RAS_CONTROL_POINT_CHAR_SEND (3)
#define RAS_OVER_WRITE_CHAR_SEND (4)
typedef uint8_t ras_attr_notify_t;

enum {
    RAS_RANGING_SERVICE_ATTR_ID = 1,
    RAS_RANGING_FEATURE_ATTR_ID,
    RAS_RANGING_REAL_TIME_ATTR_ID,
    RAS_RANGING_REAL_TIME_CCC_ID,
    RAS_RANGING_ON_DEMAND_ATTR_ID,
    RAS_RANGING_ON_DEMAND_CCC_ID,
    RAS_RANGING_CONTROL_POINT_ATTR_ID,
    RAS_RANGING_CONTROL_POINT_CCC_ID,
    RAS_RANGING_DATA_READY_ATTR_ID,
    RAS_RANGING_DATA_READY_CCC_ID,
    RAS_RANGING_DATA_OVER_WRITE_ATTR_ID,
    RAS_RANGING_DATA_OVER_WRITE_CCC_ID
};

typedef void (*ras_ccc_cfg_cb_t)(bt_address_t* addr, ras_ccc_cfg_change_evt_t event,
    const uint8_t* value, uint16_t length);

typedef void (*ras_ctr_pt_write_cb_t)(bt_address_t* addr,
    const uint8_t* value, uint16_t length);

typedef void (*ras_feature_read_cb_t)(bt_address_t* addr, uint32_t req_handle);

typedef void (*ras_data_notify_cb_t)(ras_attr_notify_t attr, bt_address_t* addr);

typedef void (*ras_mtu_updated_cb_t)(bt_address_t* addr, uint32_t mtu);

typedef void (*ras_gatts_notify_complete_cb_t)(bt_address_t* addr, gatt_status_t status,
    ras_attr_notify_t attr);

typedef void (*ras_gatts_conn_cb_t)(bt_address_t* addr);
typedef void (*ras_gatts_disconn_cb_t)(bt_address_t* addr);

typedef struct {
    ras_ccc_cfg_cb_t cfg_cb;
    ras_ctr_pt_write_cb_t pt_write_cb;
    ras_feature_read_cb_t feature_read_cb;
    ras_mtu_updated_cb_t mtu_updated_cb;
    ras_gatts_notify_complete_cb_t notify_cb;
    ras_gatts_conn_cb_t conn_cb;
    ras_gatts_disconn_cb_t disconn_cb;
} ras_gatts_callbacks_t;

bt_status_t ras_gatts_data_send_notify(ras_attr_notify_t attr, bt_address_t* addr, uint8_t* value,
    uint16_t len, bool is_notify);
void ras_gatts_set_feature_value(uint32_t feature);
void bt_cs_ras_gatts_init(const ras_gatts_callbacks_t* callback);
bt_status_t ras_send_feature_read_rsp(bt_address_t* addr, uint32_t feature);
void ras_gatts_deinit(void);

#endif /* _CS_RAS_GATTS_H_ */