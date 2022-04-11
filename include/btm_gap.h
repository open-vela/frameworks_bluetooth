/**@file  btm_gap.h
* @brief       bluetooth adapter for GAP service.
* @details   including get all GAP profile interface
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
#ifndef __BTM_GAP_H__
#define __BTM_GAP_H__

#include "btm_manager.h"

#define MAX_PAIR_DEVICE 10
#define MAX_CONNECTED_DEVICE 2
/* * Bluetooth discovery state */


/** ssp request data */
/**
 *@brief details of HCI event from controller
 */

/**
 *@brief HCI event callback (only for the raw HCI command sent by upper layer)
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] hci_event, include evt_code and parameters
 * @return   void
 */

typedef void (*hci_event_callback)(void* handle, hci_event_t* hci_event);

// HCI command struct
typedef struct {
    uint8_t ogf; ///< OpCode Group Field
    uint16_t ocf; ///< OpCode Command Field
    hci_event_callback cb; // callback
    uint8_t length; ///< length of the params
    char params[1]; // parameters
} bt_hci_command_t;

//typedef void (*btm_ble_state_changed_callback)(void* handle, btm_ble_state state);
/**
 *@brief connection changed state callback.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device, remote device info.
 * @param[in] state, current connection state of remote device.
 * @return   void
 */
typedef void (*bt_connection_state_changed_callback)(void* handle, bt_device_t* device, bt_connection_state state);

/**
 * @brief BR/EDR device found callback, invoked in response to bt_start_discovery()
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] addr - newly found device
 * @return   void
 */
typedef void (*device_found_callback)(void* handle, bt_device_t* device);

/**
 *@brief BR/EDR device's name updated callback, invoked in response to bt_start_discovery()/bt_get_remote_name()
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] bt_name - remote device name
 * @param[in] length  - buffer length
 * @return   void
 */
typedef void (*received_remote_name_callback)(void* handle, bt_address bd_addr, char* bt_name, uint8_t length);

/**
 * @brief discoveryStateChangedCallback
 *              Discovery state change callback, invoked in response to bt_set_scan_mode()
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] state - newly state
 * @return   void
 */
typedef void (*discovery_state_changed_callback)(void* handle, bt_discovery_state state);

/**
 * @brief  SSP pairing reqeust callback - Just Works & Numeric Comparison
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] request_data - request data from callback
 * @return   void
 */
typedef void (*ssp_request_callback)(void* handle, ssp_request_data_t* request_data);

/**
* @brief Bonding state change callback - invoked in response to bt_create_bond(), bt_cancel_bond(),
 *              bt_remove_bond()
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device - remote BT address
 * @param[in] state - bond state
 * @return   void
 */
typedef void (*bond_state_changed_callback)(void* handle, bt_device_t* device, bt_bond_state state);

/**
 * @brief Local name callback - invoked in response to bt_set_local_name()
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] bt_name - local name
 * @param[in] length - lenth
 * @return   void
 */
typedef void (*local_name_callback)(void* handle, char* bt_name, uint8_t length);

typedef void (*local_address_callback)(void* handle, bt_device_t* device);

/**  
 * @brief Get local device class.
 * @param[in] handle, gap handle, must be create before this funciton.
 * @return  init success or failed.
*/
typedef void (*local_device_class_callback)(void* handle, uint32_t device_class);
/**
 * @brief connection state changed callback.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device - device info of connection state changed.
 * @param[in] state - newly state.
 * @return   void
 */
typedef void (*connection_state_callback)(void* handle, bt_device_t* device, bt_connection_state state);

/**
 * @brief smp request callback.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] request_data - request data info.
 * @return   void
 */
typedef void (*smp_request_callback)(void* gap_handle, ssp_request_data_t* request_data);
/**
 * @brief ble phy update callback.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] tx_phy - tx phy type updated.
 * @param[in] rx_phy - rx phy type updated.
 * @param[in] status - phy update success or failed.
 * @return   void
 */
typedef void (*ble_phy_update_callback)(void* gap_handle, bt_address remote_addr, ble_phy_type tx_phy, ble_phy_type rx_phy, bt_status  status);
/**
 * @brief ble address update callback- invoked in response to ble_set_address.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] ble_addr - newly ble address .
 * @param[in] ble_addr_type - rx phy type updated.
 * @return   void
 */
typedef void (*ble_address_callback)(void* gap_handle, bt_address ble_addr, ble_addr_type ble_addr_type);
/**
 * @brief ble address update callback- invoked in response to ble_set_address.
 * @param[in] handle -gap handle, must be create before this funciton.
 * @param[in] local_initiate - Local initiates the pairing or not.
 * @param[in] is_bondable - Local is bondable or not.
 * @return   void
 */
typedef void (*pairing_request_callback)(void* gap_handle, bt_address remote_addr, bool local_initiate, bool is_bondable);

typedef void (*ble_irk_callback)(void* gap_handle, bt_common_key irk, bt_address ble_addr, ble_addr_type ble_addr_type);
typedef void (*delete_linkey_callback)(void* gap_handle, bt_address remote_addr, bt_status reason);
typedef void (*link_connect_request_callback)(void* gap_handle, bt_address remote_addr);

typedef struct {
    /** set to sizeof(bt_callbacks_t) */
    size_t size;
    bt_connection_state_changed_callback bt_connection_state_changed_callback_cb;
    device_found_callback device_found_callback_cb;
    received_remote_name_callback received_remote_name_callback_cb;
    discovery_state_changed_callback discovery_state_changed_callback_cb;
    ssp_request_callback ssp_request_callback_cb;
    bond_state_changed_callback bond_state_changed_callback_cb;
    hci_event_callback hci_event_callback_cb;
    local_name_callback local_name_callback_cb;
    local_device_class_callback local_device_class_callback_cb;
    connection_state_callback connection_state_callback_cb;
    local_address_callback local_address_callback_cb;
    smp_request_callback smp_requeset_cb;
    ble_phy_update_callback ble_phy_update_cb;
    ble_address_callback ble_address_cb;
    pairing_request_callback pairing_request_cb;
    ble_irk_callback ble_irk_cb;
    delete_linkey_callback delete_linkey_cb;
    link_connect_request_callback link_connect_request_cb;
} btm_gap_callbacks_t;

/*gap interface*/

typedef struct {
    size_t size;
    bt_result_code (*gap_register_callbacks)(void* manager_handle, void** gap_handle, const btm_gap_callbacks_t* callbacks);
    /**
 *@brief  Clean up Bluetooth stack.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @return   Bluetooth Error status code (0- Success)
 */
    bt_result_code (*gap_cleanup)(void* gap_handle);
    /*Local property*/
    /**
 *@brief  Get local BT address.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @return   local BT address.
 */
    bt_result_code (*bt_get_local_address)(void* handle, bt_address addr);
    /**
 *@brief  Set local IO capability.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] io_capability - local IO capability. 
 * @return   Bluetooth Error status code (0- Success).
 */
    bt_result_code (*bt_set_local_io_capability)(void* handle, bt_io_capability io_capability);
    /**
 *@brief  Set local name.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] bt_name - local BT name, ended with 0. 
 * @param[in] len - length of bt_name.  
 * @return   Bluetooth Error status code (0- Success).
 */
    bt_result_code (*bt_set_local_name)(void* handle, char* bt_name, uint8_t len);
    /**
 *@brief  Get local name.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @return   Local name.
 */
    char* (*bt_get_local_name)(void* handle);
    /*Remote device*/
    /**
 *@brief  Get remote name, will be respose in received_remote_name_callback.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device - reomte device info.
 * @return   Bluetooth Error status code (0- Success).
 */
    bt_result_code (*bt_get_remote_name)(void* handle, bt_device_t* device);
    /**
 *@brief  Reply to pairing request.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device - reomte device info.
 * @param[in] accept - 0 is accept.
 * @return   Bluetooth Error status code (0- Success).
 */
    bt_result_code (*bt_reply_pair_request)(void* handle, bt_device_t* device, int accept);
    /**
 *@brief   * Create bonding with remote deivce, including ACL link creation
 *  and required pairing (bonding) procedure and SDP procedure.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device - reomte device info.
 * @return   Bluetooth Error status code (0- Success).
 */
   bt_result_code (*bt_ssp_reply)(void* gap_handle, spp_reply_data_t* reply_data);

    bt_result_code (*bt_create_bond)(void* handle, bt_device_t* device);
    /**
 *@brief   *  Cancel ongoing bonding procedure which previous executed by
 * bt_create_bond().
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device - reomte device info.
 * @return   Bluetooth Error status code (0- Success).
 */
    bt_result_code (*bt_cancel_bond)(void* handle, bt_device_t* device);
    /**
 *@brief   * Remove remote device from bonding history.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in] device - reomte device info.
 * @return   Bluetooth Error status code (0- Success).
 */
    bt_result_code (*bt_remove_bond)(void* handle, bt_device_t* device);
    /**
 *@brief   * Get remote device from bonding history, reponsed by get_bonded_device_list_callback
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[out] device_list - device list array. Max number of device list array is .
 * @return   num of bonded device got.
 */
    int (*bt_get_bonded_devices)(void* handle, bt_device_t* device_list, int max_out);

    // /*Connection*/
    /**
 *@brief   * Get remote device from connected device, reponsed by get_connected_device_list_callback
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[out] device_list - device list array.Max number of device list array is MAX_CONNTEDE_DEVICE
 * @return   num of bonded device got.
 * @return   Bluetooth Error status code (0- Success).
 */
    int (*bt_get_connected_devices)(void* handle, bt_device_t* device_list, int max_out);

    /*Discovery*/
    /**
 *@brief  Set BT scan mode.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in]    scan_mode - BT scan mode (connectable, discoverable)
 * @param[in]    bondable - Bondable mode (0 - none bondable; 1 - bondable) 
 * @return   Bluetooth Error status code (0- Success)
 */
    bt_result_code (*bt_set_scan_mode)(void* handle, bt_scan_mode scanMode, bool bondable);
    /**
 *@brief  Start BR/EDR device discovery.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param    timeout - duration for the discovery procedure
 * @return   Bluetooth Error status code (0- Success)
 */
    bt_result_code (*bt_start_discovery)(void* handle, uint32_t timeout);
    /**
 *@brief  Cancel BR/EDR device discovery.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @return   Bluetooth Error status code (0- Success)
 */
    bt_result_code (*bt_stop_discovery)(void* handle);

    /*VSC command*/
    /**
 *@brief   Send HCI command for testing purpose. These command should use the HCI_Command_Complete_Event
 * or HCI_Vendor_Specific_Event as the complete event. If the complete event is none of the
 * HCI_Command_Complete_Event and HCI_Vendor_Specific_Event, event_type shall be set to
 * SERVICE_HCI_COMMAND_COMPLETED_BY_NONE, and the application shall use the gap_hci_event_cb
 * to receive any vendor extended HCI events created due to execution of this command.
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param[in]  command  hci command
 * @param[in]  event_type  type of the complete event for the command
 * @return   Bluetooth Error status code (0- Success)
 */
#ifdef HCI_VSC_COMMAND
    bt_result_code (*bt_send_hci_command)(void* handle, bt_hci_command_t* command, hci_command_complete_event event_type);
#endif
    /*service discovery*/
    /**
 * @brief  Start BR/EDR service discovery
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param    device - Remote BT address
 * @param    uuid - Type of the service to discovered. If NULL is set, return all.
 * @return   Bluetooth Error status code (0- Success)
 */
    bt_result_code (*bt_start_service_discovery)(void* gap_handle, bt_device_t* device, bt_uuid_t uuid);
    /**
 * @brief  Stop BR/EDR service discovery
 * @param[in] handle - gap handle, must be create before this funciton.
 * @param    device - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
    bt_result_code (*bt_stop_service_discovery)(void* gap_handle, bt_device_t* device);

    int (*bt_get_remote_services)(void* gap_handle, bt_device_t* remote_addr, bt_uuid_t* service_list, uint8_t count_in);

    bt_result_code (*ble_set_static_identity)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_set_public_identity)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_get_current_irk)(void* gap_handle);
    bt_result_code (*ble_set_address)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_get_address)(void* gap_handle);
    bt_result_code (*ble_set_bonded_devices)(void* gap_handle, ble_keys_t* bonded_device_list, uint8_t count_in);
    bt_result_code (*ble_connect)(void* gap_handle, ble_connect_params_t* conn_param);
    bt_result_code (*ble_disconnect)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_smp_reply)(void* gap_handle, spp_reply_data_t* reply_data);
    bt_result_code (*ble_add_white_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_remove_white_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_add_resolving_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_remove_resolving_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_set_phy)(void* gap_handle, bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy);
    bt_result_code (*ble_add_private_channel)(void* gap_handle, uint16_t private_cid);
    bt_result_code (*ble_send_packet)(void* gap_handle, bt_device_t* device, uint16_t private_cid, uint8_t* packet, uint16_t packet_size);
    int (*ble_get_bonded_devices)(void* gap_handle, bt_device_t* device_list, int max_out);
    int (*ble_get_connected_devices)(void* gap_handle, bt_device_t* device_list, int max_out);
    int (*ble_get_whitelist_devices)(void* gap_handle, bt_device_t* device_list, int max_out);
    int (*ble_get_resolvinglist_devices)(void* gap_handle, bt_device_t* device_list, int max_out);
    /**
     * Send HCI command for testing purpose
     * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
     * @return Bluetooth Error status code (0- Success)
     */
    bt_result_code (*enter_bluetooth_test_mode)(void* gap_handle, test_mode mode);
    bt_result_code (*bt_set_local_device_class)(void* handle, uint32_t class_of_device);
    uint32_t (*bt_get_local_device_class)(void* handle);
    bt_result_code (*bt_set_local_address)(void* handle, bt_device_t* device);
    bt_result_code (*bt_set_inquiry_scan_parameters)(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window);
    bt_result_code (*bt_set_page_scan_parameters)(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window);
    bt_result_code (*bt_reply_link_request)(void* gap_handle, bt_address remote_addr, bool accept);
} btm_gap_interface_t;

btm_gap_interface_t* get_gap_instance(void);

#endif
