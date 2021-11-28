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
#include "bts_common.h"

typedef void (*bts_gatts_connection_state_changed_callback)(void* handle, bd_addr_t remote_addr, profile_state_t state);
typedef void (*bts_gatts_opened_callback)(void* handle, uint8_t server_if);
typedef void (*bts_gatts_closed_callback)(void* handle);

typedef void (*bts_gatts_element_added_callback)(void* handle, gatt_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*bts_gatts_element_removed_callback)(void* handle, gatt_status_t status, gatt_element_t* element,
    size_t size);
typedef void (*bts_gatts_phy_read_callback)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*bts_gatts_phy_update_callback)(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx, gatt_status_t status);
typedef void (*bts_gatts_read_request_callback)(void* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element);
typedef void (*bts_gatts_write_request_callback)(void* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size);
typedef void (*bts_gatts_mtu_changed_callback)(void* handle, bd_addr_t remote_addr, uint32_t mtu);
typedef void (*bts_gatts_notify_sent_callback)(void* handle, bd_addr_t remote_addr, gatt_status_t status);

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

    bt_result_code (*init)(void);
    void (*clean_up)(void);

    bt_result_code (*open_server)(bts_gatts_hdl_t handle);
    bt_result_code (*close_server)(uint8_t server_if);
    bt_result_code (*connect)(uint8_t server_if, bd_addr_t addr, bool auto_connect);
    bt_result_code (*disconnect)(uint8_t server_if, bd_addr_t addr);
    bt_result_code (*add_element)(uint8_t server_if, gatt_element_t* element, uint16_t size);
    bt_result_code (*remove_element)(uint8_t server_if, uint32_t* ids, uint16_t size);
    bt_result_code (*read_phy)(uint8_t server_if, bd_addr_t addr);
    bt_result_code (*update_phy)(uint8_t server_if, bd_addr_t addr, ble_phy_type_t tx_type, ble_phy_type_t rx_type);
    bt_result_code (*send_notify)(uint8_t server_if, bd_addr_t addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_indicate)(uint8_t server_if, bd_addr_t addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);
    bt_result_code (*send_response)(uint8_t server_if, bd_addr_t remote_addr, gatt_response_t* response)

} bts_gatts_interface_t;

const bts_gatts_interface_t* get_bts_gatts_instance(void);
#endif