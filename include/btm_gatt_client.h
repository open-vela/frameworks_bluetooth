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

/**
 * @brief:  gatt client conection state changed callback
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {profile_connection_state} state - profile conenction state, PROFILE_DISCONNECTED = 0, PROFILE_CONNECTING =1, PROFILE_CONNECTED = 2, PROFILE_DISCONNECTING = 3
 * @return {bt_result_code} GATT Error status code (0- Success)
 */
typedef void (*btm_gattc_connection_state_changed_callback)(void* handle, bt_address remote_addr, profile_connection_state state);

/**
 * @brief: the list of remote services, characteristics and descriptors
 * for the remote device have been updated
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {gatt_element_t*} element - discovery elements
 * @param {uint16_t} size - number of discovered elements
 *  @return {*}
 */
typedef void (*btm_gattc_service_discovered_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, uint16_t size);

/**
 * @brief: Callback reporting the result of a characteristic or descriptor read operation
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {gatt_element_t*} element - discovery elements
 * @param {uint8_t*} value - value to read
 * @param {uint16_t} size - size to read
 * @param {gatt_status} status - status code (0- Success)
 * @return {*}
 */
typedef void (*btm_gattc_read_result_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_status status);

/**
 * @brief: Callback reporting the result of a characteristic or descriptor write operation
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {gatt_element_t*} element - write elements
 * @param {gatt_status} status - status code (0- Success)
 * @return {*}
 */
typedef void (*btm_gattc_write_result_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, gatt_status status);

/**
 * @brief:  Callback triggered as a result of a remote characteristic notification
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {gatt_element_t*} element - notify elements
 * @param {uint8_t*} value -  notification value
 * @param {uint16_t} size - notification size
 * @return {*}
 */
typedef void (*btm_gattc_nofity_request_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size);

/**
 * @brief: Callback reporting the RSSI for a remote device connection
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {int32_t} rssi - RSSI 
 * @param {gatt_status} status - status code (0- Success)
 * @return {*}
 */
typedef void (*btm_gattc_rssi_read_callback)(void* handle, bt_address remote_addr, int32_t rssi, gatt_status status);

/**
 * @brief: Callback reporting the PHY change for a remote device connection
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {ble_phy_type} tx - transfer PHY type
 * @param {ble_phy_type} rx - receive PHY type
 * @return {*}
 */
typedef void (*btm_gattc_phy_read_callback)(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx);

/**
 * @brief: gatt client request update remote PHY callback
 * @note:  handle, must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {ble_phy_type} tx - transfer PHY type
 * @param {ble_phy_type} rx - receive PHY type
 * @return {*}
 */
typedef void (*btm_gattc_phy_update_callback)(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx);

/**
 * @brief: Callback reporting the MTU change for a remote device connection
 * @note:
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {uint32_t} mtu - Max Transfer Unit
 * @return {*}
 */
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

    /**
     * @brief: gatt client connect remote device
     * @note:  handle would be created.
     * @param {void**} handle
     * @param {bt_address} remote_addr - remote device address
     * @param {btm_gatt_client_callbacks*} callbacks - gatt client callbacks
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*connect)(void** handle, bt_address remote_addr, btm_gatt_client_callbacks* callbacks);

    /**
     * @brief: Disconnects an established connection
     * @note:  handle, must be create before this funciton.
     * @param {void*} handle
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*disconnect)(void* handle);

    /**
     * @brief: gatt client  discovers services offered by a remote device
     * as well as their characteristics and descriptors.
     * @note:  handle, must be create before this funciton.
     * @param {void*} handle
     * @param {bt_uuid_t} uuid -  Service uuid.
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*discover_services)(void* handle, bt_uuid_t uuid);

    /**
     * @brief: gatt client reads the value for characteristic or descriptor from the associated remote device.
     * @note:  handle, must be create before this funciton.
     * @param {void*} handle
     * @param {gatt_element_t*} element - characteristic or descriptor
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*read_request)(void* handle, gatt_element_t* element);

    /**
     * @brief: gatt client writes the value for characteristic or descriptor from the associated remote device.
     * @note:  handle, must be create before this funciton.
     * @param {void*} handle
     * @param {gatt_element_t*} element - characteristic or descriptor
     * @param {uint8_t*} value - buffer
     * @param {uint16_t} length - buffer length
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*write_request)(void* handle, gatt_element_t* element, uint8_t* value, uint16_t length);

    /**
     * @brief: gatt client  enable or disable notifications/indications for a given characteristic
     * @note: handle, must be create before this funciton.
     * @param {void*} handle
     * @param {gatt_element_t*} element - characteristic
     * @param {bool} enable - Set to true to enable notifications/indications
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*register_notification)(void* handle, gatt_element_t* element, bool enable);

    /**
     * @brief: gatt client read the RSSI for a connected remote device.
     * @note: handle, must be create before this funciton.
     * @param {void*} handle
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*read_rssi)(void* handle);

    /**
     * @brief: gatt client read the current transmitter PHY and receiver PHY of the connection.
     * @note: handle, must be create before this funciton.
     * @param {void*} handle
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*read_phy)(void* handle);

    /**
     * @brief: gatt client set the current transmitter PHY and receiver PHY of the connection.
     * @note: handle, must be create before this funciton.
     * @param {void*} handle
     * @param {ble_phy_type} tx_phy -  transmitter PHY
     * @param {ble_phy_type} rx_phy - receiver PHY
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*update_phy)(void* handle, ble_phy_type tx_phy, ble_phy_type rx_phy);

    /**
     * @brief: gatt client request set mtu size
     * @note: handle, must be create before this funciton.
     * @param {void*} handle
     * @param {uint32_t} mtu - The new mtu value to set. It shall be (ATT_MTU-3).
     *                         The ATT_MTU is the actual MTU value to set using ATT
     *                         Exchange MTU Request. 3 is the ATT PDU header size.
     *                         The minimum mtu value is (23-3). 23 is the default
     *                         ATT_MTU.
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*update_mtu)(void* handle, uint32_t mtu);

    /**
     * @brief:  gatt client request update connection parameter
     * @note: handle, must be create before this funciton.
     * @param {void*} handle
     * @param {uint32_t} min_interval
     * @param {uint32_t} max_interval
     * @param {uint32_t} latency
     * @param {uint32_t} timeout
     * @param {uint32_t} min_connection_event_length
     * @param {uint32_t} max_connection_event_length
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*update_connection_parameter)(void* handle, uint32_t min_interval, uint32_t max_interval,
        uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length);
} btm_gatt_client_interface_t;

/**
 * @brief: gatt client get client interface from GAP interface
 * @note:
 * @param {void*} bt_mgr_interface
 * @return {btm_gatt_client_interface_t} gatt client handle
 */
btm_gatt_client_interface_t* get_btm_gattc_interface(void* bt_mgr_interface);

#endif