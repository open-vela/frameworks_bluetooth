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

/**
 * @brief: gatt server connection state changed callback
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {profile_connection_state} state - connection state, profile conenction state, PROFILE_DISCONNECTED = 0, PROFILE_CONNECTING =1, PROFILE_CONNECTED = 2, PROFILE_DISCONNECTING = 3
 * @return {*}
 */
typedef void (*btm_gatts_connection_state_changed_callback)(void* handle, bt_address remote_addr, profile_connection_state state);

/**
 * @brief: gatt server open callback
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @return {*}
 */
typedef void (*btm_gatts_opened_callback)(void* handle);

/**
 * @brief: gatt server close callback
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @return {*}
 */
typedef void (*btm_gatts_closed_callback)(void* handle);

/**
 * @brief: gatt server  indicates whether a local service has been added successfully
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {gatt_status} status - gatt status
 * @param {gatt_element_t*} element - added elements. It shall be the same buffer as the first
 *                         parameter of  gatt server add elements.
 * @param {size_t} size - elements size
 * @return {*}
 */
typedef void (*btm_gatts_service_added_callback)(void* handle, gatt_status status, gatt_element_t* element,
    size_t size);

/**
 * @brief: gatt server indicates whether a local service has been removed successfully
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {gatt_status} status - gatt status
 * @param {gatt_element_t*} element -  Full elements of the removed service.
 *                         It shall be the same buffer as the first parameter of
 *                          gatt server add remove elements.
 * @param {size_t} size -  elements size
 * @return {*}
 */
typedef void (*btm_gatts_service_removed_callback)(void* handle, gatt_status status, gatt_element_t* element,
    size_t size);

/**
 * @brief: gatt server PHY read callback
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {ble_phy_type} tx - transmitter PHY
 * @param {ble_phy_type} rx - recevicer PHY
 * @return {*}
 */
typedef void (*btm_gatts_phy_read_callback)(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx);

/**
 * @brief: gatt server PHY changed callback
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {ble_phy_type} tx - transmitter PHY
 * @param {ble_phy_type} rx - recevicer PHY
 * @param {gatt_status} status
 * @return {*}
 */
typedef void (*btm_gatts_phy_update_callback)(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx, gatt_status status);

/**
 * @brief: a remote client has requested to read a local characteristic or descriptor
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {uint32_t} request_id -  request id
 * @param {gatt_element_t*} element -  characteristic or descriptor
 * @return {*}
 */
typedef void (*btm_gatts_read_request_callback)(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element);

/**
 * @brief: gatt server a remote client has requested to write a local characteristic or descriptor
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {uint32_t} request_id - request id
 * @param {gatt_element_t*} element - characteristic or descriptor
 * @param {uint8_t*} value - buffer
 * @param {uint16_t} offset - offset of the value to write
 * @param {uint16_t} size -  buffer length
 * @return {*}
 */
typedef void (*btm_gatts_write_request_callback)(void* handle, bt_address remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size);

/**
 * @brief: gatt server  mtu changed callback
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {uint32_t} mtu - The new (ATT_MTU-3) value negotiated.
 *                                    The default mtu value is (23-3).
 *                                    3 is the ATT PDU header size.
 * @return {*}
 */
typedef void (*btm_gatts_mtu_changed_callback)(void* handle, bt_address remote_addr, uint32_t mtu);

/**
 * @brief: gatt server a notification or indication has been sent to a remote device
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} remote_addr - remote address
 * @param {gatt_status} status - GATT status
 * @return {*}
 */
typedef void (*btm_gatts_notify_sent_callback)(void* handle, bt_address remote_addr, gatt_element_t* element, gatt_status status);

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

    /**
     * @brief: gatt server open
     * @note: handle would be created
     * @param {void**} handle
     * @param {btm_gatt_server_callbacks*} callbacks - callback function struct
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*open)(void** handle, btm_gatt_server_callbacks* callbacks);

    /**
     * @brief: gatt server close
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*close)(void* handle);

    /**
     * @brief: gatt server initiate a connection to a Bluetooth GATT capable device
     * @note:  handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @param {bool} auto_connect - auto reconnect when disconnect timeout
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*connect)(void* handle, bt_address remote_addr, bool auto_connect);

    /**
     * @brief: gatt server disconnect remote connection
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*disconnect)(void* handle, bt_address remote_addr);

    /**
     * @brief: gatt server add a service to the list of services to be hosted.
     * The first element shall be the service (primary or secondary).
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {gatt_element_t*} element - ervice,[include],[characteristic, descriptor],
     *                       [characteristic, descriptor]. The elements buffer
     *                       shall keep valid until it is returned to the application
     *                       using gatt_server_elements_removed_cb.
     * @param {uint16_t} size - elements size
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*add_service)(void* handle, gatt_element_t* element, uint16_t size);

    /**
     * @brief: gatt server remove specified service(s) from the list of services hosted.
     * If a service included by other service(s) is removed, all the host services
     * shall also be removed manually.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {uint32_t*} ids - Each item in the ids specifies the ID of the service to remove.
     *                  If ids is NULL, all services are removed.
     * @param {uint16_t} size -  Number of IDs in the ids.
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*remove_service)(void* handle, uint32_t* ids, uint16_t size);

    /**
     * @brief: gatt server read the current transmitter PHY and receiver PHY of the connection.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*read_phy)(void* handle, bt_address remote_addr);

    /**
     * @brief: gatt server set the current transmitter PHY and receiver PHY of the connection.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr
     * @param {ble_phy_type} tx_type
     * @param {ble_phy_type} rx_type
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*update_phy)(void* handle, bt_address remote_addr, ble_phy_type tx_type, ble_phy_type rx_type);

    /**
     * @brief: gatt server  set Client Characteristic Configuration descriptor value for the specified
     * bonded device.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote device
     * @param {gatt_element_t*} characteristic -  characteristic. It shall be within the elements
     *                       parameter of service_adapter_gatt_server_add_elements.
     * @param {uint8_t*} value - buffer
     * @param {size_t} size - buffer length
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*send_notify)(void* handle, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);

    /**
     * @brief: gatt server send an indication that a local characteristic has been updated.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @param {gatt_element_t*} characteristic -  characteristic. It shall be within the elements
     *                        parameter of service_adapter_gatt_server_add_elements.
     * @param {uint8_t*} value
     * @param {size_t} size
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*send_indicate)(void* handle, bt_address remote_addr, gatt_element_t* characteristic, uint8_t* value,
        size_t size);

    /**
     * @brief: gatt server send a response to a read or write request to a remote device.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} remote_addr - remote address
     * @param {gatt_response_t*} response - response
     * @return {bt_result_code} GATT Error status code (0- Success)
     */
    bt_result_code (*send_response)(void* handle, bt_address remote_addr, gatt_response_t* response);
} btm_gatt_server_interface_t;

/**
 * @brief: gatt client get client interface from GAP interface
 * @note:
 * @param {void*} bt_mgr_interface
 * @return {btm_gatt_server_interface_t} gatt server interface
 */
btm_gatt_server_interface_t* get_btm_gatts_interface(void* bt_mgr_interface);

#endif