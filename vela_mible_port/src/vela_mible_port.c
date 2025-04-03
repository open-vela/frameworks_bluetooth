/***********************************************************************
 *
 * Copyright 2025 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#include "vela_mible_port.h"
#include "vela_mible_manage.h"
#include "bt_adapter.h"
#include "bt_le_advertiser.h"
#include "bt_le_scan.h"
#include "bt_gattc.h"
#include "bt_gatts.h"
#include "utils/log.h"

#define VELA_MIBLE_UINT8_TO_BYTE_STREAM(p, u8) \
    {                                     \
        *(p)++ = (uint8_t)(u8);           \
    }

#define VELA_MIBLE_ARRAY_TO_BYTE_STREAM(p, a, len) \
{                                         \
    int ijk;                              \
    for (ijk = 0; ijk < (len); ijk++)     \
        *(p)++ = (uint8_t)(a)[ijk];       \
}

static void* le_gap_handle;

typedef struct {
    bt_advertiser_t* le_advertiser;
} vela_mible_adv_t;

static vela_mible_adv_t le_adv_ins;

typedef struct {
    bt_scanner_t* scanner_handle;
    bool is_scanning;
} vela_mible_scan_t;

static vela_mible_scan_t le_scan_ins = { 0 };

static scanner_callbacks_t le_scan_cbs = {
    .on_scan_result = NULL,
    .on_scan_start_status = NULL,
    .on_scan_stopped = NULL,
};

#define VELA_MIBLE_ADV_DATA_LEN_MAX 31

#define VELA_MIBLE_ADV_TLV_TO_STRAM(byte_stream, type, len, data)         \
    {                                                            \
        uint8_t* p_val = (uint8_t*)data;                        \
        VELA_MIBLE_UINT8_TO_BYTE_STREAM(byte_stream, type);            \
        VELA_MIBLE_UINT8_TO_BYTE_STREAM(byte_stream, len);            \
        if (p_val != NULL) {                                     \
            VELA_MIBLE_ARRAY_TO_BYTE_STREAM(byte_stream, p_val, len); \
        }                                                        \
    }

static void on_adv_start_cb(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status)
{
    if (status == BT_STATUS_SUCCESS) {
        BT_LOGI("ADV start success: handle=%p, adv_id=%d, status=%d", adv, adv_id, status);
        return;
    }

    if (adv == le_adv_ins.le_advertiser) {
        BT_LOGE("Failed to start ADV: handle=%p, adv_id=%d, status=%d", adv, adv_id, status);
        le_adv_ins.le_advertiser = NULL;
    }

    return;
}

static void on_adv_stopped_cb(bt_advertiser_t* adv, uint8_t adv_id)
{
    BT_LOGI("handle:%p, adv_id:%d", adv, adv_id);
    if (adv == le_adv_ins.le_advertiser) {
        le_adv_ins.le_advertiser = NULL;
        BT_LOGI("New advertising allowed");
    }

    return;
}

static advertiser_callback_t vela_le_adv_cbs
    = { sizeof(vela_le_adv_cbs), on_adv_start_cb, on_adv_stopped_cb };

static void on_scan_start_cb(bt_scanner_t* scanner, uint8_t status)
{
    if (status == BT_STATUS_SUCCESS) {
        BT_LOGD("BLE scan started successfully, status=%d", status);
    } else {
        BT_LOGE("BLE scan start failed, status=%d", status);
        if (le_scan_ins.scanner_handle == scanner) {
            le_scan_ins.scanner_handle = NULL;
        }
    }

    le_scan_ins.is_scanning = (status == 0) ? true : false;
}

static void on_scan_stopped_cb(bt_scanner_t* scanner)
{
    BT_LOGI("ble scan stopped");
    le_scan_ins.is_scanning = false;
}

static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr,
    bt_transport_t transport, connection_state_t state)
{
    if (transport == BT_TRANSPORT_BLE) {
        // TODO:
    }

    return;
}

static adapter_callbacks_t gap_cbs = {
    .on_connection_state_changed = on_connection_state_changed_cb,
};

int vela_mible_adv_start(const struct bt_le_adv_param *param, int32_t duration,
			   const struct bt_data *ad, size_t ad_len,
			   const struct bt_data *sd, size_t sd_len)
{
    if (!param || !ad || ad_len == 0) {
        BT_LOGW("Invalid params.");
        return -1;
    }
    uint8_t adv_data[VELA_MIBLE_ADV_DATA_LEN_MAX];
    uint8_t scan_rsp_data[VELA_MIBLE_ADV_DATA_LEN_MAX];
    uint8_t* stream = adv_data;
    if (ad && ad_len) {
        for (int i = 0; i < ad_len; i++) {
            VELA_MIBLE_ADV_TLV_TO_STRAM(stream, ad[i].type, ad[i].data_len, ad[i].data);
        }
    }

    BT_LOGE("vela_mible_adv_start");
    // Parsing ADV data
    size_t adv_len = stream - adv_data;
    stream = scan_rsp_data;
    if (sd && sd_len) {
        for (int i = 0; i < sd_len; i++) {
            VELA_MIBLE_ADV_TLV_TO_STRAM(stream, sd[i].type, sd[i].data_len, sd[i].data);
        }
    }

    ble_adv_params_t adv_params = { 0 };
    adv_params.adv_type = BT_LE_LEGACY_ADV_IND;
    /* We can choice only white lists can connect in. */
    adv_params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE;
    adv_params.own_addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    adv_params.channel_map = BT_LE_ADV_CHANNEL_DEFAULT;
    adv_params.interval = param->interval_min;
    adv_params.tx_power = -10;

    // Parsing SCAN rsp data
    size_t scan_rsp_len = stream - scan_rsp_data;

    le_adv_ins.le_advertiser = bt_le_start_advertising(
        (bt_instance_t*)vela_mible_svc_ins_get(), &adv_params, adv_data, adv_len,
        scan_rsp_data, scan_rsp_len, &vela_le_adv_cbs);
    return 0;
}

int vela_mible_adv_stop(void)
{
    if (!le_adv_ins.le_advertiser) {
        BT_LOGE("The ADV hasn't started");
        return -1;
    }
    bt_le_stop_advertising((bt_instance_t*)vela_mible_svc_ins_get(),
        INT2PTR(bt_advertiser_t*)le_adv_ins.le_advertiser);
    return 0;
}

int vela_mible_scan_start(const struct bt_le_scan_param *param, bt_le_scan_cb_t cb)
{
    if (le_scan_ins.scanner_handle) {
        BT_LOGI("already in scanning");
        return 0;
    }

    if (!le_scan_cbs.on_scan_start_status) {
        le_scan_cbs.on_scan_start_status = on_scan_start_cb;
    }

    if (!le_scan_cbs.on_scan_stopped) {
        le_scan_cbs.on_scan_stopped = on_scan_stopped_cb;
    }

    // TODO:
    le_scan_ins.scanner_handle = bt_le_start_scan_with_filters(
        (bt_instance_t*)vela_mible_svc_ins_get(), NULL, NULL,
        (const scanner_callbacks_t*)&le_scan_cbs);

    if (!le_scan_ins.scanner_handle) {
        BT_LOGE("Failed to start BLE scan.");
        vela_mible_scan_stop();
        return -1;
    }

    return 0;
}

int vela_mible_scan_stop(void)
{
    if (!le_scan_ins.scanner_handle) {
        BT_LOGE("BLE Scan stop failed, BLE scan hasn't started.");
        return -1;
    }

    bt_le_stop_scan((bt_instance_t*)vela_mible_svc_ins_get(), le_scan_ins.scanner_handle);
    le_scan_ins.scanner_handle = NULL;

    BT_LOGI("BLE scanning stopped.");

    return 0;
}

int vela_mible_gap_init(void)
{
    if (le_gap_handle) {
        BT_LOGE("bt adapter already register vela ble gap callback");
        return -1;
    }

    le_gap_handle = bt_adapter_register_callback((bt_instance_t*)vela_mible_svc_ins_get(), &gap_cbs);

    if (!le_gap_handle) {
        BT_LOGE("bt adapter register vela ble gap callback failed");
        return -1;
    }

    return 0;
}

int vela_mible_gap_deinit(void)
{
    if (!le_gap_handle) {
        BT_LOGE("bt adapter have't register vela ble gap callback");
        return -1;
    }

    bt_adapter_unregister_callback((bt_instance_t*)vela_mible_svc_ins_get(), le_gap_handle);
    le_gap_handle = NULL;

    return 0;
}

