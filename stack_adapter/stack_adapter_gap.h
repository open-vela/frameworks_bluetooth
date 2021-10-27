/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_gap.h
Abstract:
    Bluetooth Stack GAP interface
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef STACK_ADAPTER_GAP_H
#define STACK_ADAPTER_GAP_H
#ifdef __cplusplus
extern "C" {
#endif
#include "stack_adapter_common.h"

/**
 * BR/EDR device found callback, invoked in response to service_adapter_start_device_discovery()
 * @param[in] device - newly found device
 * @return   void
 */
typedef void (*gap_device_found_callback)(SERVICE_REMOTE_DEVICE_S *device);

/**
 * BR/EDR device's name updated callback, invoked in response to service_adapter_start_device_discovery()
 * @param[in] bt_name - remote device name
 * @param[in] length  - buffer length
 * @return   void
 */
typedef void (*gap_received_remote_name_callback)(BD_ADDR bd_addr, char *bt_name, uint8_t length);

/**
 * Discovery state change callback, invoked in response to service_adapter_start_device_discovery()
 * @param[in] device - newly found device
 * @return   void
 */
typedef void (*gap_discovery_state_changed_callback)(SERVICE_BT_DISCOVERY_STATE state);

/**
 * Legacy pairing pin request callback
 * @param[in] request_data - request data from callback
 * @return   void
 */
typedef void (*gap_pin_request_callback)(SERVICE_PIN_REQUEST_DATA_S *request_data);

/**
 * SSP pairing reqeust callback - Just Works & Numeric Comparison
 * @param[in] request_data - request data from callback
 * @return   void
 */
typedef void (*gap_ssp_request_callback)(SERVICE_SSP_REQUEST_DATA_S *request_data);

/**
 * Bonding state change callback - invoked in response to service_adapter_create_bond(), service_adapter_cancel_bond(),
 * service_adapter_remove_bond()
 * @param[in] status - Bluetooth Error status code (0- Success)
 * @param[in] remote_addr - remote BT address
 * @param[in] state - bond state
 * @return   void
 */
typedef void (*gap_bond_state_changed_callback)(BD_ADDR remote_addr, SERVICE_BT_BOND_STATE state);

/**
 * ACL state change callback
 * @param[in] acl_state_param - details of acl state changed
 * @return   void
 */
typedef void (*gap_acl_state_changed_callback)(SERVICE_ACL_STATE_PARAM_S *acl_state_param);

/**
 * BLE scan result callback
 * @param[in] scan_result_data - scan result data from callback
 * @return   void
 */
typedef void (*gap_ble_scan_result_callback)(SERVICE_SCAN_RESULT_DATA_S *scan_result_data);

/**
 * BLE advertising started callback, in response to service_adapter_ble_adv_start()
 * @param[in] status - Bluetooth Error status code (0- Success)
 * @param[in] adv_id - advertising id set by service_adapter_ble_adv_start()
 * @return   void
 */
typedef void (*gap_ble_adv_started_callback)(uint8_t adv_id);

/**
 * BLE advertising stopped callback, in response to service_adapter_ble_adv_start() or service_adapter_ble_adv_stop()
 * @param[in] status - Bluetooth Error status code (0- Success)
 * @param[in] adv_id - advertising id set by service_adapter_ble_adv_start()
 * @return   void
 */
typedef void (*gap_ble_adv_stopped_callback)(uint8_t adv_id);

/**
 * Bluetooth link role changed callback, in response to service_adapter_set_bluetooth_link_role() or host stack self
 * changed
 * @param[in] remote_addr - remote BT address
 * @param[in] link_role - BT link role
 * @return   void
 */
typedef void (*gap_bt_link_role_changed_callback)(BD_ADDR remote_addr,
        SERVICE_BT_LINK_ROLE link_role);

/**
 * Bluetooth scan mode changed callback, in response to service_adapter_gap_set_scan_mode() or host stack self changed
 * @param[in] scan_mode
 * @return   void
 */
typedef void (*gap_scan_mode_changed_callback)(SERVICE_BT_SCAN_MODE scan_mode);

/**
 * Bluetooth mode (sniff, active) changed callback, in response to service_adapter_gap_set_link_mode() or host stack
 * self changed
 * @param[in] remote_addr- remote BT address
 * @param[in] scan_mode
 * @return   void
 */
typedef void (*gap_link_mode_changed_callback)(BD_ADDR remote_addr, SERVICE_BT_LINK_MODE link_mode,
        uint16_t sniff_interval);

/**
 * Bluetooth link connection request callback
 * @param[in] remote_addr- remote BT address
 * @return   void
 */
typedef void (*gap_link_connect_request_callback)(BD_ADDR remote_addr);

/**
 * Bluetooth link policy changed callback, in response to service_adapter_gap_set_link_policy() or host stack self
 * changed
 * @param[in] remote_addr- remote BT address
 * @param[in] scan_mode
 * @return   void
 */
typedef void (*gap_link_policy_changed_callback)(BD_ADDR remote_addr,
        SERVICE_BT_LINK_POLICY link_policy);

/**
 * Bluetooth stack state changed callback
 * @param[in] stack_state
 * @return   void
 */
typedef void (*gap_stack_state_changed_callback)(SERVICE_BT_STACK_STATE stack_state);

/**
 * HCI event callback (only for the raw HCI command sent by upper layer)
 * @param[in] hci_event, include evt_code and parameters
 * @return   void
 */
typedef void (*gap_hci_event_callback)(SERVICE_BT_HCI_EVENT_S *hci_event);

/**
 * Write a HCI packet to the Bluetooth Controller through the transport.
 * @param[in] hci_packet - one raw HCI packet, it can be HCI command, ACL or SCO packet identified by the first byte. 1
 * - Commmand; 2 - ACL; 3 - SCO/eSCO.
 * @param[in] length - total length, in bytes, of the hci_packet buffer.
 * @return   void
 */
typedef void (*gap_transport_write_packet_callback)(uint8_t *hci_packet, uint32_t length);

/**
 * TODO
 */
typedef void (*gap_init_done_callback)(void);

/**
 * BREDR device link key created/updated callback
 * @param[in] bonded_device - bonded device information
 * @return   void
 */
typedef void (*gap_update_br_link_key_callback)(SERVICE_REMOTE_DEVICE_S *bonded_device);

/**
 * BREDR device link key deleted callback
 * @param[in] remote_addr - remote BT address
 * @return   void
 */
typedef void (*gap_delete_br_link_key_callback)(BD_ADDR remote_addr);

/**
 * Request confirmation for pairing callback
 * @param[in] remote_addr - remote BT address
 * @param[in] local_initiate - Local initiates the pairing or not.
 * @param[in] is_bondable - Local is bondable or not.
 * @return   void
 */
typedef void (*gap_pairing_request_callback)(BD_ADDR remote_addr, bool local_initiate,
        bool is_bondable);

/*******************************************************************************
 *
 * the list of remote BR services discovered
 * @param       remote_addr     - Remote address
 * @param       services        - full list of services
 * @param       size            - list size
 * @return      void
 *
 ******************************************************************************/
typedef void (*gap_service_discovered_callback)(BD_ADDR remote_addr, SERVICE_BR_SERVICE_S *services,
        uint16_t size);

/*******************************************************************************
 *
 * PHY changed callback
 * @param       remote_addr     - Remote address
 * @param       br_link         - TRUE if BREDR connection; FALSE if BLE connection.
 * @param       encryption_on   - TRUE if encryption is on; FALSE if encryption is off.
 * @return      void
 *
 ******************************************************************************/
typedef void (*gap_link_encryption_state_callback)(BD_ADDR remote_addr, bool br_link,
        bool encryption_on);

/**
 * SMP pairing reqeust callback - Just Works & Numeric Comparison
 * @param[in] request_data - request data from callback
 * @return   void
 */
typedef void (*gap_smp_request_callback)(SERVICE_SSP_REQUEST_DATA_S *request_data);

/**
 * BLE device link keys created/updated/removed callback
 * @param[in] bonded_device_list - List of current BLE bonded devices
 * @param[in] count_in - Number of current BLE bonded devices
 * @return   void
 */
typedef void (*gap_update_ble_bonded_devices_callback)(SERVICE_BLE_KEYS_S *bonded_device_list,
        uint8_t count_in);

/**
 * Result of adding device to the white list
 * @param[in] remote_addr - remote BT address
 * @param[in] status - Bluetooth Error status code (0- Success)
 * @return   void
 */
typedef void (*gap_ble_add_white_list_callback)(BD_ADDR remote_addr, SERVICE_BT_STATUS status);

/**
 * Result of removing device from the white list
 * @param[in] remote_addr - remote BT address
 * @param[in] status - Bluetooth Error status code (0- Success)
 * @return   void
 */
typedef void (*gap_ble_remove_white_list_callback)(BD_ADDR remote_addr, SERVICE_BT_STATUS status);

/**
 * Result of adding device to the resolving list
 * @param[in] remote_addr - remote BT address
 * @param[in] status - Bluetooth Error status code (0- Success)
 * @return   void
 */
typedef void (*gap_ble_add_resolving_list_callback)(BD_ADDR remote_addr, SERVICE_BT_STATUS status);

/**
 * Result of removing device from the resolving list
 * @param[in] remote_addr - remote BT address
 * @param[in] status - Bluetooth Error status code (0- Success)
 * @return   void
 */
typedef void (*gap_ble_remove_resolving_list_callback)(BD_ADDR remote_addr,
        SERVICE_BT_STATUS status);

/**
 * Report the current LE address when it is set or read.
 * @param[in] ble_addr - Local address for the current BLE operations.
 * @param[in] ble_addr_type - Type of the Local address for the current BLE operations.
 * @return   void
 */
typedef void (*gap_ble_address_callback)(BD_ADDR ble_addr, SERVICE_BLE_ADDR_TYPE ble_addr_type);

/*******************************************************************************
 *
 * PHY changed callback
 * @param       remote_addr     - Remote address
 * @param       tx_phy          - transmitter PHY
 * @param       rx_phy          - transmitter PHY
 * @param       status          - operation status
 * @return      void
 *
 ******************************************************************************/
typedef void (*gap_ble_phy_update_callback)(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_BT_STATUS status);

/**
 * Report the current IRK along with the identity address.
 * @param[in] irk - IRK for the current identity address.
 * @param[in] ble_addr - The current local identity address.
 * @param[in] ble_addr_type - Type of the current local identity address.
 * @return   void
 */
typedef void (*gap_ble_irk_callback)(BT_COMMON_KEY irk, BD_ADDR ble_addr,
                                     SERVICE_BLE_ADDR_TYPE ble_addr_type);

/*******************************************************************************
 *
 * PHY changed callback
 * @param       remote_addr     - Remote address
 * @param       private_cid     - Private channel ID
 * @param       packet          - Packet received
 * @param       packet_size     - Size, in bytes, of the packet received
 * @return      void
 *
 ******************************************************************************/
typedef void (*gap_ble_packet_received_callback)(BD_ADDR remote_addr, uint16_t private_cid,
        uint8_t *packet, uint16_t packet_size);

/*******************************************************************************
 *
 * Timeout handler
 * @param       app_arg     - Application parameter attched to this timer when it is added.
 * @param       timer       - Timer object returned when the timer is added.
 * @return      void
 *
 ******************************************************************************/
typedef void (*gap_timeout_callback)(void *app_arg, BT_TIMER_OBJ timer);

/* * Stack GAP callback structure */
typedef struct {
    /* * set to sizeof(GAP_CALLBACKS_S) */
    uint8_t size;
    gap_stack_state_changed_callback gap_stack_state_changed_cb;
    gap_received_remote_name_callback gap_received_remote_name_cb;
    gap_device_found_callback gap_device_found_cb;
    gap_discovery_state_changed_callback gap_discovery_state_changed_cb;
    gap_pin_request_callback gap_pin_request_cb;
    gap_ssp_request_callback gap_ssp_request_cb;
    gap_bond_state_changed_callback gap_bond_state_changed_cb;
    gap_acl_state_changed_callback gap_acl_state_changed_cb;
    gap_ble_scan_result_callback gap_ble_scan_result_cb;
    gap_ble_adv_started_callback gap_ble_adv_started_cb;
    gap_ble_adv_stopped_callback gap_ble_adv_stopped_cb;
    gap_link_connect_request_callback gap_link_connect_request_cb;
    gap_bt_link_role_changed_callback gap_bt_link_role_changed_cb;
    gap_scan_mode_changed_callback gap_scan_mode_changed_cb;
    gap_link_mode_changed_callback gap_link_mode_changed_cb;
    gap_link_policy_changed_callback gap_link_policy_changed_cb;
    gap_hci_event_callback gap_hci_event_cb;
    gap_transport_write_packet_callback gap_transport_write_packet_cb;
    gap_init_done_callback gap_init_done_cb;
    gap_update_br_link_key_callback gap_update_br_link_key_cb;
    gap_delete_br_link_key_callback gap_delete_br_link_key_cb;
    gap_pairing_request_callback gap_pairing_request_cb;
    gap_service_discovered_callback gap_service_discovered_cb;
    gap_link_encryption_state_callback gap_link_encryption_state_cb;
    gap_smp_request_callback gap_smp_request_cb;
    gap_update_ble_bonded_devices_callback gap_update_ble_bonded_devices_cb;
    gap_ble_add_white_list_callback gap_ble_add_white_list_cb;
    gap_ble_remove_white_list_callback gap_ble_remove_white_list_cb;
    gap_ble_add_resolving_list_callback gap_ble_add_resolving_list_cb;
    gap_ble_remove_resolving_list_callback gap_ble_remove_resolving_list_cb;
    gap_ble_address_callback gap_ble_address_cb;
    gap_ble_phy_update_callback gap_ble_phy_update_cb;
    gap_ble_irk_callback gap_ble_irk_cb;
    gap_ble_packet_received_callback gap_ble_packet_received_cb;
    //TODO:: Add Patch Download Start Callback
} GAP_CALLBACKS_S;

// HCI command struct
typedef struct {
    uint8_t ogf;                // OpCode Group Field
    uint16_t ocf;               // OpCode Command Field
    gap_hci_event_callback cb;  // callback
    uint8_t length;             // length of the params
    char params[1];             // parameters
} SERVICE_HCI_COMMAND_S;

/* * GAP API */

/**
 * Initialize Bluetooth stack
 * @param    void
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_init(void);

/**
 * Clean up Bluetooth stack
 * @param    void
 * @return   void
 */
void service_adapter_gap_cleanup(void);

/**
 * Register callback functions for Stack's GAP module
 * @param    cbs - callback fucntion struct
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_register_gap_callback(GAP_CALLBACKS_S *cbs);

/**
 * Enable Bluetooth
 * @param    void
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_enable(void);

/**
 * Disable Bluetooth
 * @param[in] normal_disable- TRUE means normal disable, FALSE means force disable
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_disable(bool normal_disable);

/**
 * Set event filter
 * @param[in] filter - Event Filter
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_event_filter(SERVICE_BT_EVENT_FILTER_S *filter);

/**
 * Start BR/EDR device discovery
 * @param    duration - duration for the discovery procedure
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_start_device_discovery(uint32_t duration);

/**
 * Cancel BR/EDR device discovery
 * @param    void
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_stop_device_discovery(void);

/**
 *  Create bonding with remote deivce, including ACL link creation
 *  and required pairing (bonding) procedure and SDP procedure
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_create_bond(BD_ADDR remote_addr);

/**
 * Cancel ongoing bonding procedure which previous executed by
 * service_adapter_create_bond()
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_cancel_bond(BD_ADDR remote_addr);

/**
 * Remove remote device from bonding history
 * @param    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_remove_bond(BD_ADDR remote_addr);

/**
 * Create ACL link for the remote device.
 * @param[in]    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_reconnect_link(BD_ADDR remote_addr);

/**
 * Disconnect the ACL link for the remote device，
 * @param[in]    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_disconnect_link(BD_ADDR remote_addr);

/**
 * Reply link request from remote device
 * @param[in]    remote_addr - Remote BT address
 * @param[in]    accept - accept link request
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_reply_link_request(BD_ADDR remote_addr, bool accept);

/**
 * Reply remote's SSP pairing request
 * @param[in]    reply_data - ssp reply detail data
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ssp_reply(SERVICE_SSP_REPLY_DATA_S *reply_data);

/**
 * Reply remote's legacy pairing request (BT 2.0 and before)
 * @param[in]    remote_addr - Remote BT address
 * @param[in]    accept - Accept pairing request
 * @param[in]    pincode - pin code input by user
 * @param[in]    len - length of the pincode
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_pin_reply(BD_ADDR remote_addr, bool accept, char *pincode,
        int len);

/**
 * Get Remote device's supported services (UUID), only invoke after getting BT_BOND_STATE_SDP_DONE state from stack
 * @param[in]    remote_addr - Remote BT address
 * @param[out]    service_list - UUID list for the remote supported service
 * @param[in]    count_in - max return number
 * @return   The number of the UUID
 */
int service_adapter_gap_get_remote_services(BD_ADDR remote_addr, BT_UUID_T *service_list,
        uint8_t count_in);

/**
 * Get Remote bonded device list, from the newest connected to the oldest connected.
 * @param[out] bonded remote device list
 * @param[in]    count_in - max return number
 * @return   The number of the bonded devices
 */
int service_adapter_gap_get_bonded_devices(SERVICE_REMOTE_DEVICE_S *bonded_device_list,
        uint8_t count_in);

/**
 * Get Remote connected device list
 * @param[out] connected remote device list
 * @param[in]    count_in - max return number
 * @return   The number of the connected devices
 */
int service_adapter_gap_get_connected_devices(SERVICE_REMOTE_DEVICE_S *connected_device_list,
        uint8_t count_in);

/**
 * Set a remote bonded device stored in the application.
 * @param[in] bonded remote device list
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_bonded_device(SERVICE_REMOTE_DEVICE_S *bonded_device);

/**
 * Set local BT address
 * @param[in]    addr - local BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_local_address(BD_ADDR addr);

/**
 * Get local BT address
 * @param[out]   bd_addr - local BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_get_local_address(BD_ADDR bd_addr);

/**
 * Set local IO capability
 * @param[in] io_capability - local IO capability
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_local_io_capability(SERVICE_BT_IO_CAPABILITY
        io_capability);

/**
 * Set local BT name
 * @param[in]    bt_name - local BT name, ended with 0.
 * @param[in]    len - length of bt_name
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_local_name(char *bt_name, uint8_t len);

/**
 * Get local BT name
 * @param[out]    btName - local BT name, ended with 0.
 * @param[in]    length- max length of the btName
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_get_local_name(char **bt_name, uint8_t length);

/**
 * Set local BT name
 * @param[in] class_of_device - local device of class.
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_local_device_class(uint32_t class_of_device);

/**
 * Get local BT name
 * @param[out] pclass_of_device - local device of class.
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_get_local_device_class(uint32_t *pclass_of_device);

/**
 * Get remote BT name
 * @param[in] remote_addr
 * @return    Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_get_remote_name(BD_ADDR remote_addr);

/**
 * Set BT scan mode
 * @param[in]    scan_mode - BT scan mode (connectable, discoverable)
 * @param[in]    bondable - Bondable mode (0 - none bondable; 1 - bondable)
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_scan_mode(SERVICE_BT_SCAN_MODE scan_mode, bool bondable);

/**
 * Set BT link mode
 * @param[in]    link_mode - BT link mode (sniff, active)
 * @param[in] remote_addr
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_link_mode(BD_ADDR remote_addr,
        SERVICE_BT_LINK_MODE link_mode,
        SERVICE_BT_SNIFF_PARAM_S *param);

/**
 * Get Stack BT state
 * @param[in]    void
 * @return   Stack BT state
 */
SERVICE_BT_STACK_STATE service_adapter_gap_get_stack_state(void);

/**
 * Get Remote device info
 * @param[in]    btAddr - remote BT address
 * @return   remote device struct for the mapping remote BT address
 */
SERVICE_BT_STATUS service_adapter_gap_get_remote_device_info(BD_ADDR remote_addr,
        SERVICE_REMOTE_DEVICE_S *device);

/**
 * Set BLE scan parameters,
 * @param[in]    scan_param, include scan_interval,scan_window,scan_phy
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_ble_scan_parameters(SERVICE_SCAN_PARAMS_S *scan_param);

/**
 * Set BLE scan filter,
 * @param[in]    scan_filter, bd_addr- remote device addr, length- length of the adv_data_mask
 *                      adv_data_mask - only reported to service layer if adv_data contains adv_data_mask
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_ble_scan_filter(SERVICE_BLE_SCAN_FILTER_S *scan_filter);

/**
 * Start BLE scan,
 * @param    void
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_start_ble_scan(void);

/**
 * Stop BLE scan,
 * @param    void
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_stop_ble_scan(void);

/**
 * Start BLE ADV  AE to be added
 * @param[in]    scan_params -  ble scan params
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_start_ble_adv(SERVICE_SCAN_ADV_PARAMS_S *scan_params);

/**
 * Stop BLE ADV
 * @param[in]    adv_id - advertising id specified by upper layer
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_stop_ble_adv(uint8_t adv_id);

/**
 * Set local identity address which is a Static address. The address should meet the
 * requirements of Static random address definition in Core Spec. The IRK
 * for the new identity address is reported to the application using gap_ble_irk_cb.
 * @param[in]    static_addr - Static address specified by the application.
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_set_static_identity(BD_ADDR static_addr);

/**
 * Set local random address. The address should meet the requirements of random address definition in Core Spec.
 * @param[in]    private_addr - random address specified by the application.
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_set_address(BD_ADDR private_addr);

/**
 * Get local BLE address. The address can be either a random address or a public address.
 * The address is reported to the application using gap_ble_address_cb.
 * @param   void
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_get_address(void);

/**
 * Set BLE bonded devices stored in the application.
 * @param[in] bonded_device_list - bonded remote device list
 * @param[in] count_in - number of bonded remote devices
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_set_bonded_devices(SERVICE_BLE_KEYS_S *bonded_device_list,
        uint8_t count_in);

/**
 * Create an encrypted BLE connection with remote device
 * @param[in]    conn_param    - Connection parameters
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_connect(SERVICE_LE_CONNECT_PARAMS_S *conn_param);

/**
 * Disconnects an established BLE connection,
 * or cancels a BLE connection attempt currently in progress.
 * @param[in]    remote_addr    - Remote address. NULL to cancel the incomplete white list connection.
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_disconnect(BD_ADDR remote_addr);

/**
 * Reply remote's SMP pairing request
 * @param[in]    reply_data - ssp reply detail data
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_smp_reply(SERVICE_SSP_REPLY_DATA_S *reply_data);

/**
 * Add a device to the Contrller white list
 * @param[in]    remote_addr    - Remote address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_add_white_list(BD_ADDR remote_addr);

/**
 * Remove a device from the Contrller white list
 * @param[in]    remote_addr    - Remote address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_remove_white_list(BD_ADDR remote_addr);

/**
 * Add a device to the Contrller resolving list
 * @param[in]    remote_addr    - Remote address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_add_resolving_list(BD_ADDR remote_addr);

/**
 * Remove a device from the Contrller resolving list
 * @param[in]    remote_addr    - Remote address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_remove_resolving_list(BD_ADDR remote_addr);

/**
 * Set the current transmitter PHY and receiver PHY of the connection.
 * @param[in]    remote_addr    - Remote address
 * @param[in]    tx_phy         - transmitter PHY
 * @param[in]    rx_phy         - receiver PHY
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_set_phy(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
        SERVICE_BLE_PHY_TYPE rx_phy);

/**
 * Add a private channel for receiving application defined packets over BLE link.
 * @param[in]    private_cid - Application defined channel ID. Range: 0x20 - 0x3E (Any value not assigned by SIG yet)
 * @return   Bluetooth Error status code (0- Success)
*/
SERVICE_BT_STATUS service_adapter_gap_ble_add_private_channel(uint16_t private_cid);

/**
 * Send application defined packets over BLE link.
 * @param       remote_addr     - Remote address
 * @param       private_cid     - Private channel ID
 * @param       packet          - Packet to send
 * @param       packet_size     - Size, in bytes, of the packet to send
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_ble_send_packet(BD_ADDR remote_addr, uint16_t private_cid,
        uint8_t *packet, uint16_t packet_size);

/**
 * Send HCI command for testing purpose. These command should use the HCI_Command_Complete_Event
 * or HCI_Vendor_Specific_Event as the complete event. If the complete event is none of the
 * HCI_Command_Complete_Event and HCI_Vendor_Specific_Event, event_type shall be set to
 * SERVICE_HCI_COMMAND_COMPLETED_BY_NONE, and the application shall use the gap_hci_event_cb
 * to receive any vendor extended HCI events created due to execution of this command.
 * @param[in]    command, hci command
 * @param[in]    event_type, type of the complete event for the command
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_send_hci_command_v1(SERVICE_HCI_COMMAND_S *command,
        SERVICE_HCI_COMMAND_COMPLETE_EVENT_TYPE event_type);

/**
 * Send HCI command for testing purpose
 * @param[in]    command, hci command
 * @return   Bluetooth Error status code (0- Success)
 */
#define service_adapter_gap_send_hci_command(_cmd)  service_adapter_gap_send_hci_command_v1(_cmd, SERVICE_HCI_COMMAND_COMPLETED_BY_COMMAND_COMPLETE_EVENT)

/**
 * Set Bluetooth master/slave role for the active link ，
 * @param[in]    remote_addr - remote BT address
 * @param[in]    link_role - BT link role (not TWS role)
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_link_role(BD_ADDR remote_addr,
        SERVICE_BT_LINK_ROLE link_role);

/**
 * Set Bluetooth link policy for active link ，
 * @param[in]    remote_addr - remote BT address
 * @param[in]    link_policy
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_link_policy(BD_ADDR remote_addr,
        SERVICE_BT_LINK_POLICY link_policy);

/**
 * Default link role (master/slave) when accepting incoming connection ，
 * @param[in]    link_role - BT link role (not TWS role)
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_set_bluetooth_accept_link_role(SERVICE_BT_LINK_ROLE
        link_role);

/**
 * @param[in]    remote_addr - remote BT address
 * @param[in]    link_role - BT link role (not TWS role)
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_accept_link(BD_ADDR remote_addr,
        SERVICE_BT_LINK_ROLE link_role);

/**
 * @param[in]    remote_addr - remote BT address
 * @param[in]    link_role - BT link role (not TWS role)
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_reject_link(BD_ADDR remote_addr);

/**
 * @param[in]    remote_addr - remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_accept_sco_link(BD_ADDR remote_addr);

/**
 * @param[in]    remote_addr - remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_reject_sco_link(BD_ADDR remote_addr);

/**
 * Enter Bluetooth RF test mode, reboot to XXX mode, for BREDR only. The BLE shall control TX and RX separately.
 * @param[in]    test_mode - Bluetooth RF test mode (DUT mode)
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_enter_bluetooth_test_mode(SERVICE_BT_TEST_MODE test_mode);

/**
 * Reports the HCI packet received from the Bluetooth Controller to the Host Stack.
 * @param[in] hci_packet - one raw HCI packet, it can be HCI event, ACL or SCO packet identified by the first byte. 1 -
 * Commmand; 2 - ACL; 3 - SCO/eSCO.
 * @param[in] length - total length, in bytes, of the hci_packet buffer.
 * @return   void
 */
void service_adapter_gap_receive_hci_packet(uint8_t *hci_packet, uint32_t length);

/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
void service_gap_send_hci_command(uint8_t *hci_cmd_packet, gap_hci_event_callback cb);

/**
 * Reply to pairing request.
 * @param[in]    remote_addr - remote BT address
 * @param[in]    response - The response code. 0 means accept.
 *               Other value speicifies the reason why pairing is rejected. Refer to HCI error code definition.
 * @return   void
 */
void service_adapter_gap_reply_pairing_request(BD_ADDR remote_addr, uint8_t response);

/**
 * Start BR/EDR service discovery
 * @param    remote_addr - Remote BT address
 * @param    uuid - Type of the service to discovered. If NULL is set, return all.
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_start_service_discovery(BD_ADDR remote_addr, BT_UUID_T uuid);

/**
 * Stop BR/EDR service discovery
 * @param[in]    remote_addr - Remote BT address
 * @return   Bluetooth Error status code (0- Success)
 */
SERVICE_BT_STATUS service_adapter_gap_stop_service_discovery(BD_ADDR remote_addr);

/**
 * Add an application defined timer
 * @app_arg[in]    Application parameter attached to this timer.
 * @milli_sec[in]  Timeout value.
 * @cbk[in]        Callback function to call when timer expired.
 * @return   Object handle used to identify this timer, which can be used to remove the timer before it is expired.
 */
BT_TIMER_OBJ service_adapter_gap_add_timer(void *app_arg, uint32_t milli_sec,
        gap_timeout_callback cbk);

/**
 * Stop and remove an application defined timer
 * @timer[in]    Object handle used to identify the timer.
 * @return   Application parameter attached to this timer if the timer is found.
 */
void *service_adapter_gap_delete_timer(BT_TIMER_OBJ timer);

#ifdef __cplusplus
}
#endif

#endif
