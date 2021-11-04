/**@file  btm_manager.h
* @brief       bluetooth adapter for bluetooth service.
* @details   including get all profile interface
* @date        2021-11-10
* @version     V1.0
* @copyright     
* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
**********************************************************************************
*
**********************************************************************************
*/
#pragma once

/*! Importation of librairies*/
#include <stddef.h>
#include <stdbool.h>
#include "bts_common.h"
/****************************************************************************
 * Included Files
 ****************************************************************************/
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

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
#define BT_ADDR_LENGTH (6)        /*!< define the address length*/
#define UUID_SIZE 16

#ifdef BD_NAME_MAX_SIZE
#undef BD_NAME_MAX_SIZE
#endif
/*!
* \def BD_NAME_MAX_SIZE
* Description
*/
#define BD_NAME_MAX_SIZE (20)

/** Bluetooth address type*/
typedef uint8_t bt_address[BT_ADDR_LENGTH];
typedef uint8_t bt_uuid_t [UUID_SIZE];

/** 
 * Result code of bluetooth manager
 */
typedef enum { 
    BT_RESULT_STATE_ALLREADY_ON = -7,
    BT_RESULT_STATE_ALLREADY_OFF = -6,
    BT_RESULT_STATE_NOT_ON = -5,
    BT_RESULT_ALLOC_BUFFER_FAILED = -4,
    BT_RESULT_CALLBACK_ALREADY_EXSIT = -3, 
    BT_RESULT_PARAMETER_ERROR = -2, 
    BT_RESULT_FAILED = -1,
    BT_RESULT_SUCCESS = 0, 
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
    BT_PROFILE_GATT_ID,
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
}bt_device_type;

/* * BLE address type */
typedef enum {
    BLE_ADDRESS_PUBLIC,
    BLE_ADDRESS_RANDOM,
    BLE_ADDRESS_PUBLIC_ID,
    BLE_ADDRESS_RANDOM_ID,
    BLE_ADDRESS_ANONYMOUS,
}ble_address_type;

typedef struct
{
    bt_address addr;
    bt_device_type device_type;
    ble_address_type addr_type;
    int rssi;
}bt_device_t;

// typedef void (*btm_process_func) (void * handle, char * context, size_t data_size);


/**@brief stack state changed callback
* @param[out]  state    current state of stack
*/
typedef void (*bt_manager_state_changed_callback)(bt_manager_bt_state state);

typedef void (*bt_manager_ble_state_changed_callback)(bt_manager_ble_state state);

 typedef void (*init_status_changed_callback)(bt_result_code status);

/**@brief callback for bluetooth service changed
*/
typedef struct {
    /** set to sizeof(bt_callbacks_t) */
    size_t size;
    bt_manager_state_changed_callback bt_manager_state_changed_callback_cb;
    init_status_changed_callback init_status_changed_callback_cb;
    bt_manager_ble_state_changed_callback bt_manager_ble_state_changed_callback_cb;
} bt_mgr_callback_t;


/****************************************************************************
 * Public Functions
 ****************************************************************************/


/**
 *@brief  btm_interface_t
 *  Description:
 *  All the interface for  BT manager .
 *
 * 
 *
 ****************************************************************************/
typedef struct {
    size_t size;
    bt_result_code (*init)(void ** handle, const bt_mgr_callback_t* callbacks);
    bt_result_code (*enable)(void * handle);
    bt_result_code (*disable)(void * handle);
    bt_result_code (*enable_ble)(void * handle);
    bt_result_code (*disable_ble)(void * handle);
    bt_manager_bt_state  (*bt_get_state)(void * handle);
    bt_manager_ble_state  (*ble_get_state)(void * handle);
    void (*cleanup)(void * handle);
    const void* (*get_profile_interface)(const char* profile_id);
} btm_interface_t;

btm_interface_t* get_bt_manager_interface(void);

/*!
* \def BT_CBACK(P_CB,
* Description
*/
#define BT_CBACK(P_CB, P_CBACK, ...)                           \
    do {                                                       \
        if ((P_CB) && (P_CB)->P_CBACK) {                       \
            BT_LOGD("%s: BT %s->%s", __func__, #P_CB, #P_CBACK); \
            (P_CB)->P_CBACK(__VA_ARGS__);                      \
        } else {                                               \
            BT_LOGE("%s Callback is NULL", __func__);            \
        }                                                      \
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