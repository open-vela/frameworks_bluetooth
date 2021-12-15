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
#ifndef _SRV_INC_GATT_CLIENT_MANAGER_H
#define _SRV_INC_GATT_CLIENT_MANAGER_H

#include <nuttx/list.h>
#include <stddef.h>

#include "btm_manager.h"

typedef void (*bts_gattc_connect_state_changed_callback)(void* handle, PROFILE_CONNECTION_STATE state);
typedef void (*bts_gattc_services_discovered_callback)(void* handle, gatt_element_t* element, uint16_t size);
typedef void (*bts_gattc_read_rsp_callback)(void* handle, gatt_element_t* element, uint8_t* value, uint16_t size, GATT_STATUS status);
typedef void (*bts_gattc_write_req_callback)(void* handle, gatt_element_t* element, GATT_STATUS status);
typedef void (*bts_gattc_nofity_req_callback)(void* handle, gatt_element_t* element, uint8_t* value, uint16_t size);
typedef void (*bts_gattc_read_rssi_callback)(void* handle, int32_t rssi, GATT_STATUS status);
typedef void (*bts_gattc_read_phy_callback)(void* handle, BLE_PHY_TYPE tx, BLE_PHY_TYPE rx);
typedef void (*bts_gattc_update_phy_callback)(void* handle, BLE_PHY_TYPE tx, BLE_PHY_TYPE rx);
typedef void (*bts_gattc_mtu_updated_callback)(void* handle, uint32_t mtu);

typedef struct {
    bts_gattc_connect_state_changed_callback bts_gattc_connection_state_changed_cb;
    bts_gattc_services_discovered_callback bts_gattc_service_discovered_cb;
    bts_gattc_read_rsp_callback bts_gattc_read_result_cb;
    bts_gattc_write_req_callback bts_gattc_write_result_cb;
    bts_gattc_nofity_req_callback bts_gattc_nofity_request_cb;
    bts_gattc_read_rssi_callback bts_gattc_rssi_read_cb;
    bts_gattc_read_phy_callback bts_gattc_phy_read_cb;
    bts_gattc_update_phy_callback bts_gattc_phy_update_cb;
    bts_gattc_mtu_updated_callback bts_gattc_mtu_changed_cb;
} bts_gatt_client_callbacks;

typedef struct
{
    struct list_node node;

    const bts_gatt_client_callbacks* callbacks;
    bt_address remote_addr;
    void* btm_handle;
} bts_gattc_hdl_t;

typedef struct {
    size_t size;

    BT_RESULT_CODE (*connect)(bts_gattc_hdl_t handle);
    BT_RESULT_CODE (*disconnect)(bt_address addr);
    BT_RESULT_CODE (*discover_services)(bt_address addr, bt_uuid_t uuid);
    BT_RESULT_CODE (*read_request)(bt_address addr, gatt_element_t* element);
    BT_RESULT_CODE (*write_request)(bt_address addr, gatt_element_t* element, uint8_t* value, uint16_t length);
    BT_RESULT_CODE (*register_notification)(bt_address addr, gatt_element_t* element, bool enable);
    BT_RESULT_CODE (*read_rssi)(bt_address addr);
    BT_RESULT_CODE (*read_phy)(bt_address addr);
    BT_RESULT_CODE (*update_phy)(bt_address addr, BLE_PHY_TYPE tx_phy, BLE_PHY_TYPE rx_phy);
    BT_RESULT_CODE (*update_mtu)(bt_address addr, uint32_t mtu);
    BT_RESULT_CODE (*update_connection_parameter)(bt_address addr, uint32_t min_interval, uint32_t max_interval,
        uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
} bts_gattc_interface_t;

const bts_gattc_interface_t* get_bts_gattc_instance(void);

#endif