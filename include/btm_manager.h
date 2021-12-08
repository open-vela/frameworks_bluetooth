/**@file  btm_manager.h
* @brief       bluetooth adapter for bluetooth service.
* @details   including get all profile interface
* @date        2021-11-10
* @version     V1.0
*/
/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
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
#pragma once

/*! Importation of librairies*/
#include <stdbool.h>
#include <stddef.h>
/****************************************************************************
 * Included Files
 ****************************************************************************/
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#include "stack_adapter_common.h"
#include "stack_adapter_gatt.h"

/** Bluetooth profile name */
/*!
* \def BT_PROFILE_GAP
* Description
*/
#define BT_PROFILE_GAP "gap"
/*!
* \def BT_PROFILE_HANDSFREE_AG
* Description
*/
#define BT_PROFILE_HANDSFREE_AG "handsfree_ag"
/*!
* \def BT_PROFILE_HANDSFREE_HF
* Description
*/
#define BT_PROFILE_HANDSFREE_HF "handsfree_hf"
/*!
* \def BT_PROFILE_ADVANCED_AUDIO_SOURCE
* Description
*/
#define BT_PROFILE_ADVANCED_AUDIO_SOURCE "a2dp_source"
/*!
* \def BT_PROFILE_ADVANCED_AUDIO_SINK
* Description
*/
#define BT_PROFILE_ADVANCED_AUDIO_SINK "a2dp_sink"
/*!
* \def BT_PROFILE_HIDHOST
* Description
*/
#define BT_PROFILE_HIDHOST "hidhost"
/*!
* \def BT_PROFILE_HIDDEV
* Description
*/
#define BT_PROFILE_HIDDEV "hiddev"
/*!
* \def BT_PROFILE_GATT
* Description
*/
#define BT_PROFILE_GATT "gatt"
/*!
* \def BT_PROFILE_AV_RC_TARGET
* Description
*/
#define BT_PROFILE_AV_RC_TARGET "avrcp_target"
/*!
* \def BT_PROFILE_AV_RC_CTRL
* Description
*/
#define BT_PROFILE_AV_RC_CTRL "avrcp_ctrl"
/*!
* \def BT_PROFILE_SPP
* Description
*/
#define BT_PROFILE_SPP "spp"
/*!
* \def BT_PROFILE_LE_AUDIO
* Description
*/
#define BT_PROFILE_LE_AUDIO "le_audio"

/** 
 * @brief Bluetooth address length 
 */
/*!
* \def BT_ADDR_LENGTH
* Description
*/
#define BT_ADDR_LENGTH (6) /*!< define the address length*/
#define UUID_SIZE 16
#define MAX_UUID_NUM 10

#ifdef BD_NAME_MAX_SIZE
#undef BD_NAME_MAX_SIZE
#endif
/*!
* \def BD_NAME_MAX_SIZE
* Description
*/
#define BD_NAME_MAX_SIZE (20)

typedef SERVICE_BLE_KEYS_S ble_keys_t;
typedef SERVICE_LE_CONNECT_PARAMS_S ble_connect_params_t;
typedef SERVICE_SSP_REPLY_DATA_S spp_reply_data_t;
typedef SERVICE_BT_TEST_MODE bt_test_mode;

typedef SERVICE_SCAN_PARAMS_S scan_params_t;
typedef SERVICE_BLE_SCAN_FILTER_S ble_scan_filter_t;
typedef SERVICE_SCAN_RESULT_DATA_S scan_result_t;
typedef BD_ADDR bt_address;
typedef BT_UUID_T bt_uuid_t;

typedef SERVICE_PROFILE_CONNECTION_STATE profile_state_t;
typedef SERVICE_SCAN_ADV_PARAMS_S advertise_param_t;
typedef SERVICE_GATT_ELEMENT_S gatt_element_t;
typedef SERVICE_GATT_RESPONSE_S gatt_response_t;
typedef GATT_SERVER_CALLBACKS_S stack_gatt_server_callbacks;
typedef GATT_CLIENT_CALLBACKS_S stack_gatt_client_callbacks;

typedef SERVICE_GATT_STATUS gatt_status_t;
typedef SERVICE_BLE_PHY_TYPE ble_phy_type_t;
typedef SERVICE_BT_STACK_STATE stack_state_t;

typedef SERVICE_PIN_REQUEST_DATA_S pin_request_data_t;
typedef SERVICE_BT_DISCOVERY_STATE discovery_state;

typedef SERVICE_SSP_REQUEST_DATA_S ssp_request_data_t;
typedef SERVICE_BT_BOND_STATE bt_bonde_state;
typedef SERVICE_ACL_STATE_PARAM_S acl_state_params_t;
typedef SERVICE_BT_LINK_ROLE bt_link_role;
typedef SERVICE_BT_SCAN_MODE bt_scan_mode;
typedef SERVICE_BT_LINK_MODE bt_link_mode;
typedef SERVICE_BT_LINK_POLICY bt_link_policy;
typedef SERVICE_BT_HCI_EVENT_S hci_event_t;
typedef SERVICE_REMOTE_DEVICE_S remote_device_t;
typedef SERVICE_BR_SERVICE_S br_service_t;
typedef SERVICE_BT_STATUS bt_status;
typedef SERVICE_BLE_ADDR_TYPE ble_addr_type;
typedef BT_COMMON_KEY bt_common_key;

typedef SERVICE_BTHD_APP_STATE hid_app_state_t;
typedef SERVICE_HID_SERVICE_INFO_S bt_hidd_sdp_settings_t;
typedef SERVICE_HID_QOS_PARAM_S bt_hidd_qos_settings_t;

/**@enum bt_result_code
* @brief Result code of bluetooth manager
*/
typedef enum {
    BT_RESULT_STATE_ALLREADY_ON = -7,
    BT_RESULT_STATE_ALLREADY_OFF = -6,
    BT_RESULT_STATE_NOT_ON = -5,
    BT_RESULT_ALLOC_BUFFER_FAILED = -4,
    BT_RESULT_CALLBACK_ALREADY_EXSIT = -3,
    BT_RESULT_PARAMETER_ERROR = -2,
    BT_RESULT_FAILED = -1, ///< genernal error code.
    BT_RESULT_SUCCESS = 0, ///< success code.
    BT_RESULT_WAITING_FOR_INIT_STATUS_CHANGED = 1,
    BT_RESULT_ENABLE_ALLREADY_ON_GOING = 2,
} bt_result_code;

/** State of bluetooth manager*/
typedef enum {
    BT_MANAGER_STATE_OFF = 0,
    BT_MANAGER_STATE_TURNING_OFF,
    BT_MANAGER_STATE_TURNING_ON,
    BT_MANAGER_STATE_ON
} bt_manager_bt_state;

/** Bluetooth connection state*/
typedef enum {
    STATE_DISCONNECTED = 0,
    STATE_DISCONNECTING,
    STATE_CONNECTING,
    STATE_CONNECTED,
} bt_connection_state;

/** State of bluetooth manager*/
typedef enum {
    STATE_BLE_OFF = 0,
    STATE_BLE_TURNING_OFF,
    STATE_BLE_TURNING_ON,
    STATE_BLE_ON
} bt_manager_ble_state;

/** Bluetooth profile interface IDs */
typedef enum {
    BT_PROFILE_COMMON_ID = 1,
    BT_PROFILE_GAP_ID,
    BT_PROFILE_HANDSFREE_AG_ID,
    BT_PROFILE_HANDSFREE_HF_ID,
    BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID,
    BT_PROFILE_ADVANCED_AUDIO_SINK_ID,
    BT_PROFILE_HIDHOST_ID,
    BT_PROFILE_HIDDEV_ID,
    BT_PROFILE_LESCAN_ID,
    BT_PROFILE_GATTC_ID,
    BT_PROFILE_LEADV_ID,
    BT_PROFILE_GATTS_ID,
    BT_PROFILE_AV_RC_TARGET_ID,
    BT_PROFILE_AV_RC_CTRL_ID,
    BT_PROFILE_SPP_ID,
    BT_PROFILE_LE_AUDIO_ID,
    BT_PROFILE_MAX_ID,
} bt_profile_id;

typedef enum {
    BT_DEVTYPE_BREDR,
    BT_DEVTYPE_BLE,
    BT_DEVTYPE_DUAL,
} bt_device_type;

/* * BLE address type */
typedef enum {
    BLE_ADDRESS_PUBLIC,
    BLE_ADDRESS_RANDOM,
    BLE_ADDRESS_PUBLIC_ID,
    BLE_ADDRESS_RANDOM_ID,
    BLE_ADDRESS_ANONYMOUS,
} ble_address_type;

typedef struct
{
    bt_address addr;
    bt_device_type device_type;
    ble_address_type addr_type;
    char *name;
    int name_length;
    int rssi;
    uint32_t cod;
    bt_uuid_t uuids[MAX_UUID_NUM];
} bt_device_t;

/**
 * @brief bt state changed callback, in response to enable or disable interface.
* @param[out]  state    current bt state of stack.
*/
typedef void (*bt_manager_state_changed_callback)(bt_manager_bt_state state);

/**
 * @brief  state changed callback, in response to enable or disable interface.
* @param[out]  state    current ble state of stack.
*/
typedef void (*bt_manager_ble_state_changed_callback)(bt_manager_ble_state state);

//typedef void (*init_status_changed_callback)(bt_result_code status);

/**
 * @brief callback for bluetooth service changed
*/
typedef struct {
    /** set to sizeof(bt_callbacks_t) */
    size_t size;
    bt_manager_state_changed_callback bt_manager_state_changed_callback_cb;
    //init_status_changed_callback init_status_changed_callback_cb;
    bt_manager_ble_state_changed_callback bt_manager_ble_state_changed_callback_cb;
} bt_mgr_callback_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**@struct btm_interface_t
 *  @brief All the interface for BT manager.
 *
 */
typedef struct {
    size_t size;
    /** create manager handle for app, and registe callback for manager. 
     * @brief
     * @param[out]  handle  create handle in bluetooth service, upper layer must save it until handle is clean up.
     * @param[in]  callbacks  callback for bluetooth manager
     * @return  init success or failed.
    */
    bt_result_code (*init)(void** handle, const bt_mgr_callback_t* callbacks);
    /** enable bt module,  responsed by bt_manager_state_changed_callback. 
     * @param[in]  handle  unique handle for every app.
     * @return  interface called success or failed.
    */
    bt_result_code (*enable)(void* handle);
    /** disable bt module ,  responsed by bt_manager_state_changed_callback. 
     * @param[in]  handle  unique handle for every app.
    * @return  interface called success or failed.
    */
    bt_result_code (*disable)(void* handle);
    /** enable ble module,  responsed by ble_manager_state_changed_callback. 
     * @param[in]  handle  unique handle for every app.
    * @return  interface called success or failed.
    */
    bt_result_code (*enable_ble)(void* handle);
    /** disable ble module,  responsed by ble_manager_state_changed_callback. 
     * @param[in]  handle  unique handle for every app.
    * @return interface called success or failed.
    */
    bt_result_code (*disable_ble)(void* handle);
    /** get state of bt module
     * @param[in]  handle  unique handle for every app.
    * @return  current state of bt module.
    */
    bt_manager_bt_state (*bt_get_state)(void* handle);
    /** get state of ble module
     * @param[in]  handle  unique handle for every app.
    * @return  current state of ble module.
    */
    bt_manager_ble_state (*ble_get_state)(void* handle);
    /** clean manager interface handle.
     * @param[in]  handle  unique handle for every app.
    */
    void (*cleanup)(void* handle);
    /** get profile interface for profile id.
     * @param[in]  profile_id  profile id info.
    * @return interface of the profile.
    */
    const void* (*get_profile_interface)(const char* profile_id);
} btm_interface_t;
/**@brief get bluetooth manager interface. 
 * each app need call first.
 * 
 * @return  all functions in btm_interface_t struct.
 */
btm_interface_t* get_bt_manager_interface(void);

/*!
* \def BT_CBACK(P_CB,
* Description
*/
#define BT_CBACK(P_CB, P_CBACK, ...)                             \
    do {                                                         \
        if ((P_CB) && (P_CB)->P_CBACK) {                         \
            BT_LOGD("%s: BT %s->%s", __func__, #P_CB, #P_CBACK); \
            (P_CB)->P_CBACK(__VA_ARGS__);                        \
        } else {                                                 \
            BT_LOGE("%s Callback is NULL", __func__);            \
        }                                                        \
    } while (0)

/*!
* \def CHECK_PTR(handle)
* Description
*/
#define CHECK_PTR(handle)           \
    do {                            \
        if (!handle) {              \
            BT_LOGE("handle NULL"); \
            return;                 \
        }                           \
    } while (0)

/*!
* \def CHECK_PTR_RETURN(handle,
* Description
*/
#define CHECK_PTR_RETURN(handle, ret) \
    do {                              \
        if (!handle) {                \
            BT_LOGE("handle NULL");   \
            return ret;               \
        }                             \
    } while (0)
