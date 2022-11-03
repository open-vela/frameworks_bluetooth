/****************************************************************************
 *
 *   copyright (c) 2021 xiaomi inc. all rights reserved.
 *
 * redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. neither the name nuttx nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * this software is provided by the copyright holders and contributors
 * "as is" and any express or implied warranties, including, but not
 * limited to, the implied warranties of merchantability and fitness
 * for a particular purpose are disclaimed. in no event shall the
 * copyright owner or contributors be liable for any direct, indirect,
 * incidental, special, exemplary, or consequential damages (including,
 * but not limited to, procurement of substitute goods or services; loss
 * of use, data, or profits; or business interruption) however caused
 * and on any theory of liability, whether in contract, strict
 * liability, or tort (including negligence or otherwise) arising in
 * any way out of the use of this software, even if advised of the
 * possibility of such damage.
 *
 ****************************************************************************/
#ifndef __btm_gap_h__
#define __btm_gap_h__

#include "btm_manager.h"

#define MAX_PAIR_DEVICE 10
#define MAX_CONNECTED_DEVICE 2

/**
 * @brief: hci event callback (only for the raw hci command sent by upper layer)
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {hci_event_t*} hci_event - include evt_code and parameters
 * @return {*}
 */
typedef void (*hci_event_callback)(void* handle, hci_event_t* hci_event);

// hci command struct
typedef struct {
    uint8_t ogf; ///< opcode group field
    uint16_t ocf; ///< opcode command field
    hci_event_callback cb; // callback
    uint8_t length; ///< length of the params
    char params[1]; // parameters
} bt_hci_command_t;

/**
 * @brief: connection changed state callback.
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_device_t*} device - remote device info
 * @param {bt_connection_state} state - current connection state of remote device.
 * @return {*}
 */
typedef void (*bt_connection_state_changed_callback)(void* handle, bt_device_t* device, bt_connection_state state);

/**
 * @brief:  br/edr device found callback, invoked in response to bt_start_discovery()
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_device_t*} device - newly found device
 * @return {*}
 */
typedef void (*device_found_callback)(void* handle, bt_device_t* device);

/**
 * @brief: br/edr device's name updated callback, invoked in response to bt_start_discovery()/bt_get_remote_name()
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_address} bd_addr - remote address
 * @param {char*} bt_name -  remote device name
 * @param {uint8_t} length - buffer length
 * @return {*}
 */
typedef void (*received_remote_name_callback)(void* handle, bt_address bd_addr, char* bt_name, uint8_t length);

/**
 * @brief:discoverystatechangedcallback
 *              discovery state change callback, invoked in response to bt_set_scan_mode()
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_discovery_state} state - newly state
 * @return {*}
 */
typedef void (*discovery_state_changed_callback)(void* handle, bt_discovery_state state);

/**
 * @brief: ssp pairing reqeust callback - just works & numeric comparison
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {ssp_request_data_t*} request_data -  request data from callback
 * @return {*}
 */
typedef void (*ssp_request_callback)(void* handle, ssp_request_data_t* request_data);

/**
 * @brief: bonding state change callback - invoked in response to bt_create_bond(), bt_cancel_bond(),
 *              bt_remove_bond()
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_device_t*} device - remote bt address
 * @param {bt_bond_state} state - bond state
 * @return {*}
 */
typedef void (*bond_state_changed_callback)(void* handle, bt_device_t* device, bt_bond_state state);

/**
 * @brief: local name callback - invoked in response to bt_set_local_name()
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {char*} bt_name -  local name
 * @param {uint8_t} length - name length
 * @return {*}
 */
typedef void (*local_name_callback)(void* handle, char* bt_name, uint8_t length);

/**
 * @brief: local address callback - invoked in response to bt_set_local_address()
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_device_t*} device - local device
 * @return {*}
 */
typedef void (*local_address_callback)(void* handle, bt_device_t* device);

/**
 * @brief: get local device class.
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {uint32_t} device_class - device class_of_device
 * @return {*}
 */
typedef void (*local_device_class_callback)(void* handle, uint32_t device_class);

/**
 * @brief:  connection state changed callback.
 * @note: handle must be create before this funciton.
 * @param {void*} handle
 * @param {bt_device_t*} device - device info of connection state changed.
 * @param {bt_connection_state} state - connection state
 * @return {*}
 */
typedef void (*connection_state_callback)(void* handle, bt_device_t* device, bt_connection_state state);

/**
 * @brief:smp request callback.
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {ssp_request_data_t*} request_data -  request data info.
 * @return {*}
 */
typedef void (*smp_request_callback)(void* gap_handle, ssp_request_data_t* request_data);

/**
 * @brief: ble phy update callback.
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {bt_address} remote_addr - remote address
 * @param {ble_phy_type} tx_phy - transfer phy
 * @param {ble_phy_type} rx_phy - receive phy
 * @param {bt_status } status - phy update success or failed.
 * @return {*}
 */
typedef void (*ble_phy_update_callback)(void* gap_handle, bt_address remote_addr, ble_phy_type tx_phy, ble_phy_type rx_phy, bt_status status);

/**
 * @brief:  ble address update callback- invoked in response to ble_set_address.
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {bt_address} ble_addr - newly ble address .
 * @param {ble_addr_type} ble_addr_type -rx phy type updated.
 * @return {*}
 */
typedef void (*ble_address_callback)(void* gap_handle, bt_address ble_addr, ble_addr_type ble_addr_type);

/**
 * @brief: request confirmation for pairing callback
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {bt_address} remote_addr - remote address
 * @param {bool} local_initiate - local initiates the pairing or not.
 * @param {bool} is_bondable - local is bondable or not.
 * @return {*}
 */
typedef void (*pairing_request_callback)(void* gap_handle, bt_address remote_addr, bool local_initiate, bool is_bondable);

/**
 * @brief: report the current irk along with the identity address.
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {bt_common_key} irk - irk for the current identity address.
 * @param {bt_address} ble_addr - the current local identity address.
 * @param {ble_addr_type} ble_addr_type - type of the current local identity address.
 * @return {*}
 */
typedef void (*ble_irk_callback)(void* gap_handle, bt_common_key irk, bt_address ble_addr, ble_addr_type ble_addr_type);

/**
 * @brief: bredr device link key deleted callback
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {bt_address} remote_addr - remote bt address
 * @param {bt_status} reason -  reason of link key deleted. service_bt_status_success if deleted by the application.
 * @return {*}
 */
typedef void (*delete_linkey_callback)(void* gap_handle, bt_address remote_addr, bt_status reason);

/**
 * @brief:  bluetooth link connection request callback
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {bt_address} remote_addr - remote bt address
 * @return {*}
 */
typedef void (*link_connect_request_callback)(void* gap_handle, bt_address remote_addr);

/**
 * @brief: ble advertising started callback
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {uint8_t} adv_id - advertising id set by adv start
 * @return {*}
 */
typedef void (*ble_adv_started_callback)(void* gap_handle, uint8_t adv_id);

/**
 * @brief:ble advertising stopped callback, in response to  ble adv start  or  ble adv stop
 * it is also called when the advertising is terminated by the controller due to connection or advertising duration
 * timeout.
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {uint8_t} adv_id - advertising id
 * @return {*}
 */
typedef void (*ble_adv_stopped_callback)(void* gap_handle, uint8_t adv_id);

/**
 * @brief:BLE connection parameters update complete callback. Connection update can be initiated by either side.
 * @note: handle must be create before this funciton.
 * @param {void*} gap_handle
 * @param {bt_address}  remote_addr - Remote address
 * @param {bt_status}  status      - Result. If result is not SERVICE_BT_STATUS_SUCCESS, the parameter values followed shall be ignored.
 * @param {uint16_t}   connection_interval - Connection interval (N) used for this connection. (N * 1.25ms)
 * @param {uint16_t}   peripheral_latency - Peripheral latency for this connection in number of subrated connection events.
 * @param {uint16_t}   supervision_timeout - Supervision timeout (N) for this connection. (N * 10ms)
 * @return   void
 */
typedef void (*ble_connection_updated_callback)(void* gap_handle, bt_address remote_addr, bt_status status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout);

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
    ble_adv_started_callback ble_adv_started_cb;
    ble_adv_stopped_callback ble_adv_stopped_cb;
    ble_connection_updated_callback ble_connection_updated_cb;
} btm_gap_callbacks_t;

/*gap interface*/

typedef struct {
    size_t size;

    /**
     * @brief: register gap callback
     * @note: handle must be create before this funciton.
     * @param {void*} manager_handle
     * @param {void**} gap_handle
     * @param {btm_gap_callbacks_t*} callbacks
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*gap_register_callbacks)(void* manager_handle, void** gap_handle, const btm_gap_callbacks_t* callbacks);

    /**
     * @brief: gap clean up bluetooth stack.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*gap_cleanup)(void* gap_handle);

    /**
     * @brief: gap get local bt address.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_address} addr -  local bt address
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_get_local_address)(void* handle, bt_address addr);

    /**
     * @brief: gap set local io capability.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_io_capability} io_capability -  local io capability.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_local_io_capability)(void* handle, bt_io_capability io_capability);

    /**
     * @brief: gap set local name.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {char*} bt_name - local bt name, ended with 0.
     * @param {uint8_t} len - length of bt_name.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_local_name)(void* handle, char* bt_name, uint8_t len);

    /**
     * @brief: gap  get local name.
     * @note: handle must be create before this funciton.
     * @return {char*} - local name.
     */
    char* (*bt_get_local_name)(void* handle);

    /**
     * @brief: gap get remote name, will be respose in received_remote_name_callback.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device -  reomte device info.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_get_remote_name)(void* handle, bt_device_t* device);

    /**
     * @brief: gap reply to pairing request.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device - reomte device info.
     * @param {int} accept - 0 is accept.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_reply_pair_request)(void* handle, bt_device_t* device, int accept);

    /**
     * @brief: gap create bonding with remote deivce, including acl link creation
     *  and required pairing (bonding) procedure and sdp procedure.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {spp_reply_data_t*} reply_data
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_ssp_reply)(void* gap_handle, spp_reply_data_t* reply_data);

    /**
     * @brief: gap  create bonding with remote deivce, including acl link creation
    *  and required pairing (bonding) procedure and sdp procedure
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device -  reomte device info.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_create_bond)(void* handle, bt_device_t* device);

    /**
     * @brief: gap cancel ongoing bonding procedure which previous executed by
     * service_adapter_create_bond()
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device - reomte device info.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_cancel_bond)(void* handle, bt_device_t* device);

    /**
     * @brief: gap  remove remote device from bonding history.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device - reomte device info.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_remove_bond)(void* handle, bt_device_t* device);

    /**
     * @brief: gap get remote device from bonding history, reponsed by get_bonded_device_list_callback
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device_list - device list array. max number of device list array is .
     * @param {int} max_out - num of bonded device got.
     * @return {bt_result_code} error status code (0- success)
     */
    int (*bt_get_bonded_devices)(void* handle, bt_device_t* device_list, int max_out);

    /**
     * @brief: gap get remote device from connected device, reponsed by get_connected_device_list_callback
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device_list - device list array.max number of device list array is max_conntede_device
     * @param {int} max_out -  num of bonded device got.
     * @return {bt_result_code} error status code (0- success)
     */
    int (*bt_get_connected_devices)(void* handle, bt_device_t* device_list, int max_out);

    /**
     * @brief: gap set bt scan mode.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_scan_mode} scanmode - bt scan mode (connectable, discoverable)
     * @param {bool} bondable - bondable mode (0 - none bondable; 1 - bondable)
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_scan_mode)(void* handle, bt_scan_mode scanmode, bool bondable);

    /**
     * @brief: gap start br/edr device discovery.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {uint32_t} timeout - duration for the discovery procedure
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_start_discovery)(void* handle, uint32_t timeout);

    /**
     * @brief: gap cancel br/edr device discovery.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_stop_discovery)(void* handle);

    /**
     * @brief: gap set br/edr link role.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device - reomte device info.
     * @param {bt_link_role} role - link role.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_link_role)(void* handle, bt_device_t* device, bt_link_role role);

    /**
     * @brief: gap disconnect bt link.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device - reomte device info.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_disconnect_link)(void* handle, bt_device_t* device);

    /**
     * @brief: gap enable CTKD bonding.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bool} brkey_to_lekey - br link key to le ltk.
     * @param {bool} lekey_to_brkey - le ltk to br link key.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_enable_ctkd_bonding)(void* handle, bool brkey_to_lekey, bool lekey_to_brkey);

#ifdef hci_vsc_command
    /**
     * @brief: gap gap send hci command for testing purpose. these command should use the hci_command_complete_event
     * or hci_vendor_specific_event as the complete event. if the complete event is none of the
     * hci_command_complete_event and hci_vendor_specific_event, event_type shall be set to
     * service_hci_command_completed_by_none, and the application shall use the gap_hci_event_cb
     * to receive any vendor extended hci events created due to execution of this command.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_hci_command_t*} command - hci command
     * @param {hci_command_complete_event} event_type - type of the complete event for the command
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_send_hci_command)(void* handle, bt_hci_command_t* command, hci_command_complete_event event_type);
#endif

    /**
     * @brief: gap start br/edr service discovery
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device -  remote bt device
     * @param {bt_uuid_t} uuid -  type of the service to discovered. if null is set, return all.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_start_service_discovery)(void* gap_handle, bt_device_t* device, bt_uuid_t uuid);

    /**
     * @brief: gap  stop br/edr service discovery
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote bt device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_stop_service_discovery)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap get remote device's supported services (uuid), only invoke after getting bt_bond_state_sdp_done state from stack
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} remote_addr - remote bt device
     * @param {bt_uuid_t*} service_list - uuid list for the remote supported service
     * @param {uint8_t} count_in - max return number
     * @return {bt_result_code} error status code (0- success)
     */
    int (*bt_get_remote_services)(void* gap_handle, bt_device_t* remote_addr, bt_uuid_t* service_list, uint8_t count_in);

    /**
     * @brief: gap start ble adv  ae to be added
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {advertise_param_t*} param - ble scan params
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_start_advertising)(void* gap_handle, advertise_param_t* param);

    /**
     * @brief: gap stop ble adv
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {uint8_t} adv_id - advertising id specified by upper layer
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_stop_advertising)(void* gap_handle, uint8_t adv_id);

    /**
     * @brief: gap set local identity address which is a static address. the address should meet the
     * requirements of static random address definition in core spec. the irk
     * for the new identity address is reported to the application using gap_ble_irk_cb.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_set_static_identity)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap  set local identity address which is a public address. the address is different with
     * the one used by bredr link.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_set_public_identity)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap return the irk for the current identity address to the application using gap_ble_irk_cb.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_get_current_irk)(void* gap_handle);

    /**
     * @brief: gap set local random address. the address should meet the requirements of random address definition in core spec.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_set_address)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap get local ble address. the address can be either a random address or a public address.
     * the address is reported to the application using gap_ble_address_cb.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_get_address)(void* gap_handle);

    /**
     * @brief: gap set ble bonded devices stored in the application.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {ble_keys_t*} bonded_device_list - bonded remote device list
     * @param {uint8_t} count_in - number of bonded remote devices
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_set_bonded_devices)(void* gap_handle, ble_keys_t* bonded_device_list, uint8_t count_in);

    /**
     * @brief: gap create an encrypted ble connection with remote device
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {ble_connect_params_t*} conn_param -  connection parameters
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_connect)(void* gap_handle, ble_connect_params_t* conn_param);

    /**
     * @brief: gap disconnects an established ble connection,
     * or cancels a ble connection attempt currently in progress.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_disconnect)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap  reply remote's smp pairing request
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {spp_reply_data_t*} reply_data - ssp reply detail data
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_smp_reply)(void* gap_handle, spp_reply_data_t* reply_data);

    /**
     * @brief: gap add a device to the contrller white list
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_add_white_list)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap remove a device from the contrller white list
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_remove_white_list)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap add a device to the contrller resolving list
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_add_resolving_list)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap remove a device from the contrller resolving list
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_remove_resolving_list)(void* gap_handle, bt_device_t* device);

    /**
     * @brief: gap set the current transmitter phy and receiver phy of the connection.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - remote device
     * @param {ble_phy_type} tx_phy -  transmitter phy
     * @param {ble_phy_type} rx_phy - receiver phy
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_set_phy)(void* gap_handle, bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy);

    /**
     * @brief: gap add a private channel for receiving application defined packets over ble link.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {uint16_t} private_cid - application defined channel id. range: 0x20 - 0x3e (any value not assigned by sig yet)
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_add_private_channel)(void* gap_handle, uint16_t private_cid);

    /**
     * @brief: gap send application defined packets over ble link.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device - receive device
     * @param {uint16_t} private_cid - private channel id
     * @param {uint8_t*} packet -  packet to send
     * @param {uint16_t} packet_size -  size, in bytes, of the packet to send
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*ble_send_packet)(void* gap_handle, bt_device_t* device, uint16_t private_cid, uint8_t* packet, uint16_t packet_size);

    /**
     * @brief: gap get ble bonded device list, from the newest connected to the oldest connected.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device_list - bonded ble device list
     * @param {int} max_out - max return number
     * @return {int} the number of the connected devices
     */
    int (*ble_get_bonded_devices)(void* gap_handle, bt_device_t* device_list, int max_out);

    /**
     * @brief: gap get ble connected device list, from the newest connected to the oldest connected.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device_list - connected ble device list
     * @param {int} max_out - max return number
     * @return {int} the number of the connected devices
     */
    int (*ble_get_connected_devices)(void* gap_handle, bt_device_t* device_list, int max_out);

    /**
     * @brief: gap get ble devices in the white list, from the newest connected to the oldest connected.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device_list -  connected ble device list
     * @param {int} max_out - max return number
     * @return {int} the number of the connected devices
     */
    int (*ble_get_whitelist_devices)(void* gap_handle, bt_device_t* device_list, int max_out);

    /**
     * @brief: gap get ble devices in the resolving list, from the newest connected to the oldest connected.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_device_t*} device_list - ble devices in the white list
     * @param {int} max_out - max return number
     * @return {int} the number of the resolvinglist devices
     */
    int (*ble_get_resolvinglist_devices)(void* gap_handle, bt_device_t* device_list, int max_out);

    /**
     * send hci command for testing purpose
     * hci_cmd_packet[in] complete hci command packet, e.g. 01 03 0c 00
     * @return bluetooth error status code (0- success)
     */
    /**
     * @brief: gap enter bluetooth rf test mode, reboot to xxx mode, for bredr only. the ble shall control tx and rx separately.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {test_mode} mode -  bluetooth rf test mode (dut mode)
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*enter_bluetooth_test_mode)(void* gap_handle, test_mode mode);

    /**
     * @brief: gap set  local device of class.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {uint32_t} class_of_device -  local device of class.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_local_device_class)(void* handle, uint32_t class_of_device);

    /**
     * @brief: gap get local device of class.
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @return {uint32_t} device of class.
     */
    uint32_t (*bt_get_local_device_class)(void* handle);

    /**
     * @brief: gap set local address
     * @note: handle must be create before this funciton.
     * @param {void*} handle
     * @param {bt_device_t*} device - local device
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_local_address)(void* handle, bt_device_t* device);

    /**
     * @brief: gap set bt inquiry scan parameters
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_scan_type} scan_type - bt scan type (standard, interlaced).
       *                    scan_type_unknown to ignore.
     * @param {uint16_t} scan_interval -  inquiry scan interval. range 0x0012 - 0x1000 (unit 0.625ms).
       *                        any invalid value to ignore.
     * @param {uint16_t} scan_window -  inquiry scan window. range 0x0011 - scan_interval (unit 0.625ms).
                             any invalid value to ignore.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_inquiry_scan_parameters)(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window);

    /**
     * @brief: gap set bt page scan parameters
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_scan_type} scan_type - bt scan type (standard, interlaced).
       *                    scan_type_unknown to ignore.
     * @param {uint16_t} scan_interval -  page scan interval. range 0x0012 - 0x1000 (unit 0.625ms).
                               any invalid value to ignore.
     * @param {uint16_t} scan_window -  page scan window. range 0x0011 - scan_interval (unit 0.625ms).
                             any invalid value to ignore.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_page_scan_parameters)(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window);

    /**
     * @brief: gap  reply link request from remote device
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_address} remote_addr - remote device
     * @param {bool} accept - accept link request
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_reply_link_request)(void* gap_handle, bt_address remote_addr, bool accept);

    /**
     * @brief: Specify a set of channels occupied by the other 2.4G wireless, e.g. WIFI,
     * which coexists with the Bluetooth radio in the same device.
     * @note: handle must be create before this funciton.
     * @param {void*} gap_handle
     * @param {bt_afh_radio_channel_info_t} channels - Specifies the set of channels. NULL specifies that no other
 *                2.4G wireless is detected.
     * @param {uint16_t} number - Number of channels. Ignored if channels is NULL.
     * @return {bt_result_code} error status code (0- success)
     */
    bt_result_code (*bt_set_afh_channel_classification)(void* gap_handle, bt_afh_radio_channel_info_t* channels, uint16_t number);
} btm_gap_interface_t;

/**
 * @brief: gap get gap instance
 * @return {btm_gap_interface_t*} gap handle
 */
btm_gap_interface_t* get_gap_instance(void);

#endif
