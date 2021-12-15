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
#ifndef _SRV_INC_GATT_SERVER_MANAGER_H
#define _SRV_INC_GATT_SERVER_MANAGER_H

#include <nuttx/list.h>
#include <stddef.h>

#include "btm_manager.h"

typedef void (*bts_gatts_connection_state_changed_callback)(void* handle, bt_address remote_addr, PROFILE_CONNECTION_STATE state);
typedef void (*bts_gatts_opened_callback)(void* handle, uint8_t server_if);
typedef void (*bts_gatts_closed_callback)(void* handle);

typedef void (*bts_gatts_element_added_callback)(void* handle, GATT_STATUS status, gatt_element_t* element,
    size_t size);
typedef void (*bts_gatts_element_removed_callback)(void* handle, GATT_STATUS status, gatt_element_t* element,
    size_t size);
typedef void (*bts_gatts_phy_read_callback)(void* handle, bt_address remote_addr, BLE_PHY_TYPE tx, BLE_PHY_TYPE rx);
typedef void (*bts_gatts_phy_update_callback)(void* handle, bt_address remote_addr, BLE_PHY_TYPE tx, BLE_PHY_TYPE rx, GATT_STATUS status);
typedef void (*bts_gatts_read_request_callback)(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element);
typedef void (*bts_gatts_write_request_callback)(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size);
typedef void (*bts_gatts_mtu_changed_callback)(void* handle, bt_address remote_addr, uint32_t mtu);
typedef void (*bts_gatts_notify_sent_callback)(void* handle, bt_address remote_addr, GATT_STATUS status);

typedef struct {
    bts_gatts_connection_state_changed_callback bts_gatts_connection_state_changed_cb;
    bts_gatts_opened_callback bts_gatts_server_opened_cb;
    bts_gatts_closed_callback bts_gatts_server_closed_cb;
    bts_gatts_element_added_callback bts_gatts_elements_added_cb;
    bts_gatts_element_removed_callback bts_gatts_elements_removed_cb;
    bts_gatts_phy_read_callback bts_gatts_phy_read_cb;
    bts_gatts_phy_update_callback bts_gatts_phy_update_cb;
    bts_gatts_read_request_callback bts_gatts_read_request_cb;
    bts_gatts_write_request_callback bts_gatts_write_request_cb;
    bts_gatts_mtu_changed_callback bts_gatts_mtu_changed_cb;
    bts_gatts_notify_sent_callback bts_gatts_notify_sent_cb;
} bts_gatt_server_callbacks;

typedef struct {
    struct list_node node;

    uint8_t server_if;
    const bts_gatt_server_callbacks* callbacks;
    void* btm_handle;
} bts_gatts_hdl_t;

typedef struct {
    size_t size;

    BT_RESULT_CODE (*init)(void);
    void (*clean_up)(void);

    BT_RESULT_CODE (*open_server)(bts_gatts_hdl_t handle);
    BT_RESULT_CODE (*close_server)(uint8_t server_if);
    BT_RESULT_CODE (*connect)(uint8_t server_if, bt_address addr, bool auto_connect);
    BT_RESULT_CODE (*disconnect)(uint8_t server_if, bt_address addr);
    BT_RESULT_CODE (*add_element)(uint8_t server_if, gatt_element_t* element, uint16_t size);
    BT_RESULT_CODE (*remove_element)(uint8_t server_if, uint32_t* ids, uint16_t size);
    BT_RESULT_CODE (*read_phy)(uint8_t server_if, bt_address addr);
    BT_RESULT_CODE (*update_phy)(uint8_t server_if, bt_address addr, BLE_PHY_TYPE tx_type, BLE_PHY_TYPE rx_type);
    BT_RESULT_CODE (*send_notify)(uint8_t server_if, bt_address addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    BT_RESULT_CODE (*send_indicate)(uint8_t server_if, bt_address addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    BT_RESULT_CODE (*send_response)(uint8_t server_if, bt_address remote_addr, gatt_response_t* response);
} bts_gatts_interface_t;

const bts_gatts_interface_t* get_bts_gatts_instance(void);
#endif