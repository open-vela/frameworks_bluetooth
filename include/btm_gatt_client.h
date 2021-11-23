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
#ifndef _MGR_INC_BLUETOOTH_GATT_C_H
#define _MGR_INC_BLUETOOTH_GATT_C_H

#include <stddef.h>

#include "btm_manager.h"

typedef void (*btm_gattc_connection_state_changed_callback)(void* handle, bt_address remote_addr, profile_state_t state);
typedef void (*btm_gattc_service_discovered_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, uint16_t size);
typedef void (*btm_gattc_read_result_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_status_t status);
typedef void (*btm_gattc_write_result_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, gatt_status_t status);
typedef void (*btm_gattc_nofity_request_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size);
typedef void (*btm_gattc_rssi_read_callback)(void* handle, bt_address remote_addr, int32_t rssi, gatt_status_t status);
typedef void (*btm_gattc_phy_read_callback)(void* handle, bt_address remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*btm_gattc_phy_update_callback)(void* handle, bt_address remote_addr, ble_phy_type_t tx, ble_phy_type_t rx);
typedef void (*btm_gattc_mtu_changed_callback)(void* handle, bt_address remote_addr, uint32_t mtu);

typedef struct {
    btm_gattc_connection_state_changed_callback gattc_connection_state_changed_cb;
    btm_gattc_service_discovered_callback gattc_service_discovered_cb;
    btm_gattc_read_result_callback gattc_read_result_cb;
    btm_gattc_write_result_callback gattc_write_result_cb;
    btm_gattc_nofity_request_callback gattc_nofity_request_cb;
    btm_gattc_rssi_read_callback gattc_rssi_read_cb;
    btm_gattc_phy_read_callback gattc_phy_read_cb;
    btm_gattc_phy_update_callback gattc_phy_update_cb;
    btm_gattc_mtu_changed_callback gattc_mtu_changed_cb;
} btm_gatt_client_callbacks;

typedef struct {
    size_t size;

    bt_result_code (*connect)(void** handle, bt_address remote_addr, btm_gatt_client_callbacks* callbacks);
    bt_result_code (*disconnect)(void* handle);
    bt_result_code (*discover_services)(void* handle, bt_uuid_t uuid);
    bt_result_code (*read_request)(void* handle, gatt_element_t* element);
    bt_result_code (*write_request)(void* handle, gatt_element_t* element, uint8_t* value, uint16_t length);
    bt_result_code (*register_notification)(void* handle, gatt_element_t* element, bool enable);
    bt_result_code (*read_rssi)(void* handle);
    bt_result_code (*read_phy)(void* handle);
    bt_result_code (*update_phy)(void* handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_result_code (*update_mtu)(void* handle, uint32_t mtu);
    bt_result_code (*update_connection_parameter)(void* handle, uint32_t min_interval, uint32_t max_interval,
        uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
} btm_gatt_client_interface_t;

btm_gatt_client_interface_t* get_btm_gattc_interface(void* bt_mgr_interface);

#endif