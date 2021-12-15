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
#ifndef _MGR_INC_BLUETOOTH_GATT_S_H
#define _MGR_INC_BLUETOOTH_GATT_S_H

#include <stddef.h>

#include "btm_manager.h"

typedef void (*btm_gatts_connection_state_changed_callback)(void* handle, bt_address remote_addr, PROFILE_CONNECTION_STATE state);
typedef void (*btm_gatts_opened_callback)(void* handle);
typedef void (*btm_gatts_closed_callback)(void* handle);
typedef void (*btm_gatts_service_added_callback)(void* handle, GATT_STATUS status, gatt_element_t* element,
    size_t size);
typedef void (*btm_gatts_service_removed_callback)(void* handle, GATT_STATUS status, gatt_element_t* element,
    size_t size);
typedef void (*btm_gatts_phy_read_callback)(void* handle, bt_address remote_addr, BLE_PHY_TYPE tx, BLE_PHY_TYPE rx);
typedef void (*btm_gatts_phy_update_callback)(void* handle, bt_address remote_addr, BLE_PHY_TYPE tx, BLE_PHY_TYPE rx, GATT_STATUS status);
typedef void (*btm_gatts_read_request_callback)(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element);
typedef void (*btm_gatts_write_request_callback)(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size);
typedef void (*btm_gatts_mtu_changed_callback)(void* handle, bt_address remote_addr, uint32_t mtu);
typedef void (*btm_gatts_notify_sent_callback)(void* handle, bt_address remote_addr, GATT_STATUS status);

typedef struct {
    btm_gatts_connection_state_changed_callback gatts_connection_state_changed_cb;
    btm_gatts_opened_callback gatts_server_opened_cb;
    btm_gatts_closed_callback gatts_server_closed_cb;
    btm_gatts_service_added_callback gatts_service_added_cb;
    btm_gatts_service_removed_callback gatts_service_removed_cb;
    btm_gatts_phy_read_callback gatts_phy_read_cb;
    btm_gatts_phy_update_callback gatts_phy_update_cb;
    btm_gatts_read_request_callback gatts_read_request_cb;
    btm_gatts_write_request_callback gatts_write_request_cb;
    btm_gatts_mtu_changed_callback gatts_mtu_changed_cb;
    btm_gatts_notify_sent_callback gatts_notify_sent_cb;
} btm_gatt_server_callbacks;

typedef struct {
    size_t size;

    BT_RESULT_CODE (*open)(void** handle, btm_gatt_server_callbacks* callbacks);
    BT_RESULT_CODE (*close)(void* handle);
    BT_RESULT_CODE (*connect)(void* handle, bt_address remote_addr, bool auto_connect);
    BT_RESULT_CODE (*disconnect)(void* handle, bt_address remote_addr);
    BT_RESULT_CODE (*add_service)(void* handle, gatt_element_t* element, uint16_t size);
    BT_RESULT_CODE (*remove_service)(void* handle, uint32_t* ids, uint16_t size);
    BT_RESULT_CODE (*read_phy)(void* handle, bt_address remote_addr);
    BT_RESULT_CODE (*update_phy)(void* handle, bt_address remote_addr, BLE_PHY_TYPE tx_type, BLE_PHY_TYPE rx_type);
    BT_RESULT_CODE (*send_notify)(void* handle, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    BT_RESULT_CODE (*send_indicate)(void* handle, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    BT_RESULT_CODE (*send_response)(void* handle, bt_address remote_addr, gatt_response_t* response);
} btm_gatt_server_interface_t;

btm_gatt_server_interface_t* get_btm_gatts_interface(void* bt_mgr_interface);

#endif