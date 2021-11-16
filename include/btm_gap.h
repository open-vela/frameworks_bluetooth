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
#include "bts_common.h"

/* * Bluetooth discovery state */
typedef enum { 
    BT_DISCOVERY_STATE_STOPPED = 0,
    BT_DISCOVERY_STATE_STARTED 
} bt_discovery_state;

/* * Bluetooth Bond state */
typedef enum {
    BT_BOND_STATE_NONE = 0,
    BT_BOND_STATE_BONDING,
    BT_BOND_STATE_BONDED,
    BT_BOND_STATE_SDP_DONE,
    BT_BOND_STATE_BLE_NONE,
    BT_BOND_STATE_BLE_BONDING,
    BT_BOND_STATE_BLE_BONDED
} bt_bond_state;

/* * Local IO capability, shall be the same value defined in HCI Specification. */
typedef enum {
    BT_IO_CAPABILITY_DISPLAYONLY = 0,
    BT_IO_CAPABILITY_DISPLAYYESNO,
    BT_IO_CAPABILITY_KEYBOARDONLY,
    BT_IO_CAPABILITY_NOINPUTNOOUTPUT,
    BT_IO_CAPABILITY_KEYBOARDDISPLAY
} bt_io_capability;

/* * Bluetooth Scan Mode */
// typedef enum { 
//     BT_SCAN_MODE_NONE = 0, 
//     BT_SCAN_MODE_CONNECTABLE, 
//     BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE 
// } bt_scan_mode;

/** Bluetooth link mode */
// typedef enum { 
//     BT_LINK_MODE_ACTIVE = 0, 
//     BT_LINK_MODE_SNIFF 
// } bt_link_mode;

// Type of the event created by the ctroller when a command is completed
typedef enum {
	HCI_COMMAND_COMPLETED_BY_NONE = 0, /* None of the following complete event is created for this command */
	HCI_COMMAND_COMPLETED_BY_COMMAND_COMPLETE_EVENT, /* HCI Command Complete event completes this command */
	HCI_COMMAND_COMPLETED_BY_VENDOR_SPECIFIC_EVENT, /* A HCI Vendor Specific event completes this command */
	HCI_COMMAND_COMPLETED_EVENT_TYPE_END /* End of definition, new event type shall be added before it */
} bt_service_hci_command_complete_event;

/** ssp type data */
typedef enum {
    SPP_TYPE_PASSKEY_CONFIRMATION = 0,
    SPP_TYPE_PASSKEY_ENTRY,
    SPP_TYPE_CONSENT,
    SPP_TYPE_PASSKEY_NOTIFICATION
} ssp_type;

/** ssp request data */
typedef struct {
    bt_address remote_addr;
    uint32_t cod;
    ssp_type ssp_type;
    uint32_t pass_key;
    char bt_name[BD_NAME_MAX_SIZE];
} bt_ssp_request_data_t;

// details of HCI event from controller
typedef struct {
    uint8_t evt_code;  // HCI event code
    uint8_t length;    // length of the params
    char params[0];    // parameters
} bt_hci_event_t;

/**
 * HCI event callback (only for the raw HCI command sent by upper layer)
 * @param[in] hci_event, include evt_code and parameters
 * @return   void
 */
typedef void (*hci_event_callback)(bt_hci_event_t *hci_event);

// HCI command struct
typedef struct {
    uint8_t ogf;                // OpCode Group Field
    uint16_t ocf;               // OpCode Command Field
    hci_event_callback cb;  // callback
    uint8_t length;             // length of the params
    char params[1];             // parameters
} bt_hci_command_t;

typedef void (*bt_manager_ble_state_changed_callback)(bt_manager_ble_state state);

typedef void (*bt_connection_state_changed_callback)(bt_device_t* device, bt_connection_state state);

/**
 * BR/EDR device found callback, invoked in response to btStartDiscovery()
 * @param[in] addr - newly found device
 * @return   void
 */
typedef void (*device_found_callback)(bt_device_t* device);
/**
 * BR/EDR device's name updated callback, invoked in response to btStartDiscovery()/btGetRemoteName()
 * @param[in] bt_name - remote device name
 * @param[in] length  - buffer length
 * @return   void
 */
typedef void (*received_remote_name_callback)(bt_address bd_addr, char *btName, uint8_t length);

/**
 * @name: discoveryStateChangedCallback
 * Discovery state change callback, invoked in response to btSetScanMode()
 * @param[in] state - newly state
 * @return   void
 */
typedef void (*discovery_state_changed_callback)(bt_discovery_state state);

/**
 * SSP pairing reqeust callback - Just Works & Numeric Comparison
 * @param[in] request_data - request data from callback
 * @return   void
 */
typedef void (*ssp_request_callback)(bt_ssp_request_data_t *request_data);

/**
 * Bonding state change callback - invoked in response to btCreateBond(), service_adapter_cancel_bond(),
 * service_adapter_remove_bond()
 * @param[in] remoteAddr - remote BT address
 * @param[in] state - bond state
 * @return   void
 */
typedef void (*bond_state_changed_callback)(bt_device_t* device, bt_bond_state state);

/**
 * Get local name callback - invoked in response to btGetLocalName()
 * service_adapter_remove_bond()
 * @param[in] bt_name - local name
 * @param[in] maxLen - lenth
 * @return   void
 */
typedef void (*local_name_callback)(char *bt_name, uint8_t length);

typedef void (*local_address_callback)(bt_device_t* device);

typedef void (*local_device_class_callback)(uint32_t device_class);

typedef void (*connected_state_callback)(bt_device_t* device, bt_connection_state state);

typedef void (*get_bonded_device_list_callback)(bt_address*bonded_device_list, uint8_t number);

typedef void (*connected_device_list_callback)(bt_address*connected_device_list, uint8_t number);

typedef void (*btm_adapter_state_changed_callback)(stack_state_t state);

typedef struct {
    /** set to sizeof(bt_callbacks_t) */
    size_t size;
    btm_adapter_state_changed_callback state_changed_cb;
    bt_connection_state_changed_callback   bt_connection_state_changed_callback_cb;
    device_found_callback device_found_callback_cb;
    received_remote_name_callback received_remote_name_callback_cb;
    discovery_state_changed_callback discovery_state_changed_callback_cb;
    ssp_request_callback ssp_request_callback_cb;
    bond_state_changed_callback bond_state_changed_callback_cb;
    hci_event_callback hci_event_callback_cb;
    get_bonded_device_list_callback get_bonded_device_list_callback_cb;
    local_name_callback local_name_callback_cb;
    local_device_class_callback local_device_class_callback_cb;
    connected_state_callback connected_state_callback_cb;
    connected_device_list_callback connected_device_list_callback_cb;
    local_address_callback local_address_callback_cb;
} btm_gap_callbacks_t;

/*gap interface*/

typedef bt_result_code (*bt_gap_register_callbacks)(void * manager_handle, void ** gap_handle, const btm_gap_callbacks_t* callbacks);
typedef void (*bt_gap_unregister_callbacks)(void * gap_handle);

/*Local property*/
typedef bt_result_code (*bt_set_local_address)(void * handle, bt_device_t* device);
typedef bt_address* (*bt_get_local_address)(void * handle);
typedef bt_result_code (*bt_set_local_io_capability)(void * handle, bt_io_capability io_capability);
typedef bt_result_code (*bt_set_local_name)(void * handle, char *bt_name, uint8_t len);
typedef char* (*bt_get_local_name)(void * handle);
typedef bt_result_code (*bt_set_local_device_class)(void * handle, uint32_t class_of_device);
typedef bt_result_code (*bt_get_local_device_class)(void * handle);

/*Remote device*/
typedef bt_result_code (*bt_get_remote_name)(void * handle, bt_device_t* device);
typedef bt_result_code (*bt_get_connection_state)(void * handle, bt_device_t* device);

/*Bond*/
typedef bt_bond_state (*bt_get_bond_state)(void * handle, bt_device_t* device);
typedef bt_result_code (*bt_reply_pair_request)(void * handle, bt_device_t* device, bool accept);
typedef bt_result_code (*bt_create_bond)(void * handle, bt_device_t* device);
typedef bt_result_code (*bt_cancel_bond)(void * handle, bt_device_t* device);
typedef bt_result_code (*bt_remove_bond)(void * handle, bt_device_t* device);
typedef bt_result_code (*bt_get_bonded_devices)(void * handle);

/*Connection*/
typedef bt_result_code (*bt_connect_all)(void * handle, bt_device_t* device);
typedef bt_result_code (*bt_disonnect_all)(void * handle, bt_device_t* device);
typedef bt_result_code (*bt_get_connected_devices)(void * handle);

/*Discovery*/
typedef bt_result_code (*bt_set_scan_mode)(void * handle, bt_scan_mode scanMode, bool bondable);
//bt_result_code btGapSetLinkMode(bt_address remote_addr, BTLinkMode link_mode);
typedef bt_result_code (*bt_start_discovery)(void * handle, uint32_t timeout);
typedef bt_result_code (*bt_stop_discovery)(void * handle);

/*VSC command*/
typedef bt_result_code (*bt_send_hci_command_v1)(void * handle, bt_hci_command_t *command, bt_service_hci_command_complete_event event_type);

/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
typedef bt_result_code (*bt_send_hci_command)(void * handle, uint8_t *hci_cmd_packet, hci_event_callback cb);


typedef struct {
    size_t size;
    bt_gap_register_callbacks register_callbacks;
    bt_gap_unregister_callbacks unregister_callbacks;
    bt_set_local_address set_address;
    bt_get_local_address get_address;
    bt_set_local_io_capability set_iocapability;
    bt_get_local_name get_name;
    bt_set_local_name set_name;
    bt_set_local_address set_class;
    bt_get_local_device_class get_class;
    bt_get_remote_name get_remote_name;
    bt_get_connection_state get_connetion_state;
    bt_get_bond_state get_bond_state;
    bt_reply_pair_request replay_pair_request;
    bt_create_bond create_bond;
    bt_remove_bond remove_bond;
    bt_cancel_bond cancel_bond;
    bt_get_bonded_devices get_bonded_devices;
    bt_connect_all connect_all;
    bt_disonnect_all disconnect_all;
    bt_get_connected_devices get_connected_devices;
    bt_set_scan_mode set_scan_mode;
    bt_start_discovery start_discovery;
    bt_stop_discovery stop_discovery;
    bt_send_hci_command_v1 send_hci_command_v1;
    bt_send_hci_command send_hci_command;

    void (*cleanup)(void);
    bt_result_code (*enable)(void);
    bt_result_code (*disable)(bool normal_disable);
    stack_state_t (*gap_get_stack_state)(void);

   /*ble interface*/
    bt_result_code (*set_ble_scan_parameters)(void* gap_handle, scan_params_t *scan_param);
    bt_result_code (*set_ble_scan_filter)(void* gap_handle, ble_scan_filter_t *scan_filter);
    bt_result_code (*start_ble_scan)(void* gap_handle);
    bt_result_code (*stop_ble_scan)(void* gap_handle);
    bt_result_code (*start_ble_adv)(void* gap_handle, advertise_param_t *scan_params);
    bt_result_code (*stop_ble_adv)(void* gap_handle, uint8_t adv_id);
    bt_result_code (*ble_set_static_identity)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_get_current_irk)(void* gap_handle);
    bt_result_code (*ble_set_address)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_get_address)(void* gap_handle);
    bt_result_code (*ble_set_bonded_devices)(void* gap_handle, ble_keys_t *bonded_device_list, uint8_t count_in);
    bt_result_code (*ble_connect)(void* gap_handle, ble_connect_params_t *conn_param);
    bt_result_code (*ble_disconnect)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_smp_reply)(void* gap_handle, spp_reply_data_t *reply_data);
    bt_result_code (*ble_add_white_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_remove_white_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_add_resolving_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_remove_resolving_list)(void* gap_handle, bt_device_t* device);
    bt_result_code (*ble_set_phy)(void* gap_handle, bt_device_t* device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_result_code (*ble_add_private_channel)(void* gap_handle, uint16_t private_cid);
    bt_result_code (*ble_send_packet)(void* gap_handle, bt_device_t* device, uint16_t private_cid, uint8_t *packet, uint16_t packet_size);
        
    /**
     * Send HCI command for testing purpose
     * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
     * @return Bluetooth Error status code (0- Success)
     */
    bt_result_code (*enter_bluetooth_test_mode)(bt_test_mode test_mode);
} btm_gap_interface_t;

btm_gap_interface_t* get_gap_instance(void);

#endif
