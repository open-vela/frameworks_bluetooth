/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef __BT_LE_ADVERTISER_H__
#define __BT_LE_ADVERTISER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"
#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief Advertising type
 *
 */
typedef enum {
    /* Auto selection, for compatibility */
    BT_LE_ADV_IND,
    BT_LE_ADV_DIRECT_IND,
    BT_LE_ADV_SCAN_IND,
    BT_LE_ADV_NONCONN_IND,
    BT_LE_SCAN_RSP,

    /* Legacy mode */
    BT_LE_LEGACY_ADV_IND,
    BT_LE_LEGACY_ADV_DIRECT_IND,
    BT_LE_LEGACY_ADV_SCAN_IND,
    BT_LE_LEGACY_ADV_NONCONN_IND,
    BT_LE_LEGACY_SCAN_RSP,

    /* None-legacy mode */
    BT_LE_EXT_ADV_IND,
    BT_LE_EXT_ADV_DIRECT_IND,
    BT_LE_EXT_ADV_SCAN_IND,
    BT_LE_EXT_ADV_NONCONN_IND,
    BT_LE_EXT_SCAN_RSP,
} ble_adv_type_t;

/**
 * @brief Advertising channels
 *
 */
typedef enum {
    BT_LE_ADV_CHANNEL_DEFAULT,
    BT_LE_ADV_CHANNEL_37_ONLY,
    BT_LE_ADV_CHANNEL_38_ONLY,
    BT_LE_ADV_CHANNEL_39_ONLY
} ble_adv_channel_t;

/**
 * @brief Advertising filter policy
 *
 */
typedef enum {
    BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE, /* Scan and Connection requests from ANY devices */
    BT_LE_ADV_FILTER_WHITE_LIST_FOR_SCAN, /* Connection requests from ANY devices; Scan requests from devices in the White List */
    BT_LE_ADV_FILTER_WHITE_LIST_FOR_CONNECTION, /* Scan request form ANY devices; Connection requests from devices in the White List */
    BT_LE_ADV_FILTER_WHITE_LIST_FOR_ALL /* Scan and Connection reqeusts from devices in the White List */
} ble_adv_filter_policy_t;

/**
 * @brief Start advertising status code
 *
 */
enum {
    BT_ADV_STATUS_SUCCESS,
    BT_ADV_STATUS_START_NOMEM,
    BT_ADV_STATUS_START_TIMEOUT,
    BT_ADV_STATUS_STACK_ERR,
};

typedef void bt_advertiser_t;

/**
 * @brief Advertising start status callback, invoke on adverting start success or failure
 *
 * @param adv - advertiser handle.
 * @param adv_id - advertiser ID.
 * @param status - advertiser start status, BT_ADV_STATUS_SUCCESS on success.
 *
 */
typedef void (*on_advertising_start_cb_t)(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status);

/**
 * @brief Advertising stopped notification
 *
 * @param adv - advertiser handle
 * @param adv_id - advertiser ID.
 */
typedef void (*on_advertising_stopped_cb_t)(bt_advertiser_t* adv, uint8_t adv_id);

/**
 * @brief Advertising callback functions structure
 *
 */
typedef struct {
    uint32_t size;
    on_advertising_start_cb_t on_advertising_start;
    on_advertising_stopped_cb_t on_advertising_stopped;
} advertiser_callback_t;

/* * BLE ADV Parameters */
typedef struct {
    bt_address_t peer_addr; /* For directed advertising only */
    uint8_t adv_type; /* ble_adv_type_t */
    uint8_t peer_addr_type; /* ble_addr_type_t, For directed advertising only */
    bt_address_t own_addr; /* Mandatory if own_addr_type is BT_LE_ADDR_TYPE_RANDOM. Ignored otherwise */
    uint8_t own_addr_type; /* ble_addr_type_t, One of BT_LE_ADDR_TYPE_PUBLIC, BT_LE_ADDR_TYPE_RANDOM and BT_LE_ADDR_TYPE_UNKNOWN */
    int8_t tx_power; /* *Range:-20~10 */
    uint32_t interval;
    uint32_t duration;
    uint8_t channel_map; /* ble_adv_channel_t */
    uint8_t filter_policy; /* ble_adv_filter_policy_t */
} ble_adv_params_t;

/**
 * @brief Start LE advertising
 *
 * @param ins - bluetooth client instance.
 * @param params - advertising parameter.
 * @param adv_data - advertisement data.
 * @param adv_len - length of advertisement data.
 * @param scan_rsp_data - scan response data.
 * @param scan_rsp_len - length of scan response data.
 * @param cbs - advertiser callback functions.
 * @return bt_advertiser_t* - advertiser handle.
 */
bt_advertiser_t* BTSYMBOLS(bt_le_start_advertising)(bt_instance_t* ins,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    advertiser_callback_t* cbs);

/**
 * @brief Stop LE advertising by advertiser handle
 *
 * @param ins - bluetooth client instance.
 * @param adver - advertiser handle.
 */
void BTSYMBOLS(bt_le_stop_advertising)(bt_instance_t* ins, bt_advertiser_t* adver);

/**
 * @brief Stop LE advertising by adver id
 *
 * @param ins - bluetooth client instance.
 * @param adv_id - advertiser ID.
 */
void BTSYMBOLS(bt_le_stop_advertising_id)(bt_instance_t* ins, uint8_t adv_id);

/**
 * @brief Check is advertising supported
 *
 * @param ins - bluetooth client instance.
 * @return true - support.
 * @return false - not support.
 */
bool BTSYMBOLS(bt_le_advertising_is_supported)(bt_instance_t* ins);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LE_ADVERTISER_H__ */
