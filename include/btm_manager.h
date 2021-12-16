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
#include "btm_common_define.h"
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


/**
 * @brief bt state changed callback, in response to enable or disable interface.
* @param[out]  state    current bt state of stack.
*/
typedef void (*bt_manager_state_changed_callback)(btm_bt_state state);

/**
 * @brief  state changed callback, in response to enable or disable interface.
* @param[out]  state    current ble state of stack.
*/
typedef void (*bt_manager_ble_state_changed_callback)(btm_ble_state state);

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
    btm_bt_state (*bt_get_state)(void* handle);
    /** get state of ble module
     * @param[in]  handle  unique handle for every app.
    * @return  current state of ble module.
    */
    btm_ble_state (*ble_get_state)(void* handle);
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
