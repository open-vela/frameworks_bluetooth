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
#ifndef _SDK_INC_BLUETOOTH_HID_DEVICE_H
#define _SDK_INC_BLUETOOTH_HID_DEVICE_H

#include <stddef.h>

#include "btm_manager.h"

/**
 * @brief: HID device result callback for chaning HID device function state.
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {hid_app_state} registered -  application  register  state
 * @return {*}
 */
typedef void (*btm_hidd_device_state_changed_callback)(void* handle, hid_app_state registered);

/**
 * @brief: HID device callback for HID Device connection state change.
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {bool} le_hid - True for HOGP, false for HID
 * @param {profile_connection_state} state - connection state
 * @return {*}
 */
typedef void (*btm_hidd_connection_state_changed_callback)(void* handle, bt_address remote_addr, bool le_hid, profile_connection_state state);

typedef struct {
    btm_hidd_device_state_changed_callback hidd_app_state_changed_cb;
    btm_hidd_connection_state_changed_callback hidd_connection_state_changed_cb;
} bt_hid_device_callbacks;

typedef struct {
    size_t size;

    /**
     * @brief: HID device register HID device application
     * @note: handle must be create before this funciton.
     * @param {void**} handle
     * @param {bt_hidd_sdp_settings_t} sdp - HID service information
     * @param {bt_hidd_qos_settings_t} tx_qos - TX QoS configuration.
     * @param {bt_hidd_qos_settings_t} rx_qos - RX QoS configuration.
     * @param {bt_hid_device_callbacks*} callbacks - callbacks
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*register_device)(void** handle, bt_hidd_sdp_settings_t sdp, bt_hidd_qos_settings_t tx_qos,
        bt_hidd_qos_settings_t rx_qos, bt_hid_device_callbacks* callbacks);

    /**
     * @brief: HID device unregister HID device application
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*unregister_device)(void* handle);

    /**
     * @brief: HID device reconnect to a virtual cable plugged BREDR HID Host.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*connect)(void* handle, bt_address remote_addr);

    /**
     * @brief: HID device disconnect from currently connected HID host
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*disconnect)(void* handle, bt_address remote_addr);

    /**
     * @brief: HID device send input report to HID device over interrupt channel.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @param {uint8_t} report_id -  ID of the input report to send data. It shall be one of SERVICE_HID_BOOT_MODE_REPORT_ID
     *                       in boot mode.
     * @param {uint8_t*} buffer - Report data. It shall include the ReportID as the first octect in Report Protocol
     *                       Mode when any Report ID Global items are declared in the report descriptor, and in
     *                       Boot Procool Mode.
     * @param {size_t} size - Size of the report data
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*send_report)(void* handle, bt_address remote_addr, uint8_t report_id, uint8_t* buffer, size_t size);

    /**
     * @brief: HID device virtual UnPlug the current connected HID Host.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*unplug)(void* handle, bt_address remote_addr);
} btm_hid_device_interface_t;

/**
 * @brief: HID device get hid device interface from GAP interface
 * @note:
 * @param {void*} bt_mgr_interface
 * @return {btm_hid_device_interface_t} hid device interface
 */
btm_hid_device_interface_t* get_btm_hid_device_interface(void* bt_mgr_interface);

#endif