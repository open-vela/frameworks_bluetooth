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
 * @cond
 */

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
 * @endcond
 */

/**
 * @brief Callback for advertising started notification.
 *
 * This callback is used to notify the application of the adverter handle, ID, and
 * start status of the advertiser. It will be triggered in the following cases:
 * 1. Advertiser ID allocates failed or the return value of the function "bt_sal_le_start_adv"
 *    is not "BT_STATUS_SUCCESS" within the advertiser starting event.
 * 2. 1 second after the successful notification of the start of LE advertising to the
 *    Bluetooth protocol stack.
 *
 * @param adv - Notifies the allocated Advertiser handle.
 * @param adv_id - Notifies the allocated Advertiser ID.
 * @param status - Notifies the starting status of the advertiser. BT_ADV_STATUS_SUCCESS
 *                 indicates that the advertiser has started successfully.
 *
 * **Example:**
 * @code
void on_advertising_start(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status)
{
    printf("on_advertising_start, adv_id: %d, status: %d\n", adv_id, status);
}
 * @endcode
 */
typedef void (*on_advertising_start_cb_t)(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status);

/**
 * @brief Callback for advertising stopped notification.
 *
 * This callback is used to notify the application of the handle and ID corresponding to
 * the stopped advertising.
 *
 * @param adv - Advertiser handle.
 * @param adv_id - Advertiser ID.
 *
 * **Example:**
 * @code
void on_advertising_stopped(bt_advertiser_t* adv, uint8_t adv_id)
{
    printf("on_advertising_stopped, adv_id: %d\n", adv_id);
}
 * @endcode
 */
typedef void (*on_advertising_stopped_cb_t)(bt_advertiser_t* adv, uint8_t adv_id);

/**
 * @cond
 */

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
 * @endcond
 */

/**
 * @brief Initate LE advertising.
 *
 * Before using this function to initiate LE advertising, the Bluetooth application
 * should set appropriate LE advertising parameters, prepare advertising data and scan
 * response data, and an advertising callback function. When using this function, the
 * above content is passed as parameters, and an advertiser handle is returned to indicate
 * the initiated advertising. With this handle, the Bluetooth application can disable the
 * corresponding advertising at an appropriate time.
 *
 * @param ins - Bluetooth client instance.
 * @param params - Advertising parameter.
 * @param adv_data - Advertisement data.
 * @param adv_len - Length of advertisement data.
 * @param scan_rsp_data - Scan response data.
 * @param scan_rsp_len - Length of scan response data.
 * @param cbs - Advertiser callback functions.
 * @return bt_advertiser_t* - Advertiser handle.
 *
 * **Example:**
 * @code
bt_advertiser_t* adver;

void app_start_advertising(bt_instance_t* ins)
{
    ble_adv_params_t params;
    uint8_t adv_data[10];
    uint8_t scan_rsp_data[10];
    advertiser_callback_t cbs;


    params.adv_type = BT_LE_ADV_IND;
    params.own_addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    params.tx_power = -20;
    params.interval = 100;
    params.duration = 0;
    params.channel_map = 7;
    params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_ALL;

    memset(adv_data, 0, sizeof(adv_data));
    memset(scan_rsp_data, 0, sizeof(scan_rsp_data));

    cbs.size = sizeof(advertiser_callback_t);
    cbs.on_advertising_start = on_advertising_start;
    cbs.on_advertising_stopped = on_advertising_stopped;

    adver = bt_le_start_advertising(ins, &params, adv_data, sizeof(adv_data), scan_rsp_data, sizeof(scan_rsp_data), &cbs);
}
 * @endcode
 */
bt_advertiser_t* BTSYMBOLS(bt_le_start_advertising)(bt_instance_t* ins,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    advertiser_callback_t* cbs);

/**
 * @brief Stop LE advertising by advertiser handle.
 *
 * This function is used to stop the LE advertising indicated by the specified advertiser
 * handle. Therefore, before using this function, the Bluetooth application should know
 * the handle corresponding to the initiated advertising. When a Bluetooth application
 * initiates an advertising, it will be informed of the advertiser handle.
 *
 * @param ins - Bluetooth client instance.
 * @param adver - Advertiser handle.
 *
 * **Example:**
 * @code
void app_stop_advertising(bt_instance_t* ins)
{
    if(!adver)
        return;
    bt_le_stop_advertising(ins, adver);
}
 * @endcode
 */
void BTSYMBOLS(bt_le_stop_advertising)(bt_instance_t* ins, bt_advertiser_t* adver);

/**
 * @brief Stop LE advertising by advertiser ID.
 *
 * This function is used to stop the LE advertising indicated by the specified advertiser ID.
 * Therefore, before using this function, the Bluetooth application should know the ID
 * corresponding to the initiated advertising. When a Bluetooth application successfully initiates
 * an advertising, the advertiser ID will be informed through the "on_advertising_start_cb_t"
 * callback function provided when the advertising is initiated by the application.
 *
 * @param ins - Bluetooth client instance.
 * @param adv_id - Advertiser ID.
 *
 * **Example:**
 * @code
void app_stop_advertising(bt_instance_t* ins, uint8_t adv_id)
{
    bt_le_stop_advertising_id(ins, adv_id);
}
 * @endcode
 */
void BTSYMBOLS(bt_le_stop_advertising_id)(bt_instance_t* ins, uint8_t adv_id);

/**
 * @brief Check if LE advertising is supported.
 *
 * This function is used to check if the Bluetooth protocol stack supports LE advertising for
 * Bluetooth applications.
 *
 * @param ins - Bluetooth client instance.
 * @return bool - true represents support for LE advertising, while false represents non-support.
 *
 * **Example:**
 * @code
void app_check_advertising_support(bt_instance_t* ins)
{
    if(bt_le_advertising_is_supported(ins))
        printf("advertising is supported\n");
    else
        printf("advertising is not supported\n");
}
 * @endcode
 */
bool BTSYMBOLS(bt_le_advertising_is_supported)(bt_instance_t* ins);

#ifdef __cplusplus
}
#endif

#endif /* __BT_LE_ADVERTISER_H__ */
