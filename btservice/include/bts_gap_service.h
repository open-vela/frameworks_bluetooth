#ifndef __BTS_GAP_SERVICE__H__
#define __BTS_GAP_SERVICE__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include "btm_gap.h"
#include "bts_common.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/


typedef void (*bts_service_adapter_state_changed_callback)(void* handle, stack_state_t state);
typedef void (*bts_service_gap_init_done_callback)(void* gap_handle);
typedef void (*bts_service_received_remote_name_callback)(void* gap_handle, bt_address bd_addr, char *bt_name, uint8_t length);
typedef void (*bts_service_discovery_state_changed_callback)(void* gap_handle, bt_discovery_state state);
typedef void (*bts_service_ssp_request_callback)(void* gap_handle, bt_ssp_request_data_t *request_data);
typedef void (*bts_service_device_found_callback)(void* gap_handle, bt_device_t device);
typedef void (*bts_service_bond_state_changed_callback)(void* gap_handle, bt_device_t device, bt_bond_state state);
typedef void (*bts_service_connected_state_callback)(void* gap_handle, bt_device_t device, bt_connection_state state);
typedef void (*bts_service_get_bonded_device_list_callback)(void* gap_handle, bt_address*bonded_device_list, uint8_t umber);
typedef void (*bts_service_connected_device_list_callback)(void* gap_handle, bt_address*connected_device_list, uint8_t umber);
typedef void (*bts_service_hci_event_callback)(void* gap_handle, bt_hci_event_t *hci_event);

typedef struct {
    /* * set to sizeof(GAP_CALLBACKS_S) */
    uint8_t size;
    bts_service_adapter_state_changed_callback adapter_state_changed;
    bts_service_gap_init_done_callback gap_init_done_cb;
    bts_service_received_remote_name_callback remote_name_cb;
    bts_service_discovery_state_changed_callback discovery_state_changed_cb;
    bts_service_ssp_request_callback ssp_request_cb;
    bts_service_device_found_callback device_found_cb;
    bts_service_bond_state_changed_callback bond_state_changed;
    bts_service_connected_state_callback connection_state_changed;
    bts_service_get_bonded_device_list_callback get_bonded_deivce_list_cb;
    bts_service_connected_device_list_callback get_connected_devices;
    bts_service_hci_event_callback hci_event_cb;
    //TODO:: Add Patch Download Start Callback
} bts_service_gap_callbacks_t;

typedef bt_result_code (*bts_service_gap_register_callbacks)(void* gap_handle, const bts_service_gap_callbacks_t* callbacks);

typedef struct
{
    size_t size;
    
    bts_service_gap_register_callbacks register_callbacks;
    /*Local property*/
    bt_result_code (*set_local_address)(void* gap_handle, bt_device_t device);
    bt_address* (*get_local_address)(void* gap_handle);
    bt_result_code (*set_local_io_capability)(void* gap_handle, bt_io_capability io_capability);
    bt_result_code (*set_local_name)(void* gap_handle, char *bt_name, uint8_t len);
    char* (*get_local_name)(void* gap_handle);
    bt_result_code (*set_local_device_class)(void* gap_handle, uint32_t class_of_device);
    bt_result_code (*get_local_device_class)(void* gap_handle);

    /*Remote device*/
    bt_result_code (*get_remote_name)(void* gap_handle, bt_device_t device);
    bt_result_code (*get_connection_state)(void* gap_handle, bt_device_t device);
    int  (*get_remote_services)(void* gap_handle, bt_device_t remote_addr, bt_uuid_t *service_list, uint8_t count_in);

    /*Bond*/
    bt_bond_state (*get_bond_state)(void* gap_handle, bt_device_t device);
    bt_result_code (*reply_pair_request)(void* gap_handle, bt_device_t device, bool accept);
    bt_result_code (*create_bond)(void* gap_handle, bt_device_t device);
    bt_result_code (*cancel_bond)(void* gap_handle, bt_device_t device);
    bt_result_code (*remove_bond)(void* gap_handle, bt_device_t device);
    bt_result_code (*get_bonded_devices)(void* gap_handle);

    /*Connection*/
    bt_result_code (*connect_all)(void* gap_handle, bt_device_t device);
    bt_result_code (*disonnect_all)(void* gap_handle, bt_device_t device);
    bt_result_code (*get_connected_devices)(void* gap_handle);

    /*Discovery*/
    bt_result_code (*set_scan_mode)(void* gap_handle, bt_scan_mode scanMode, bool bondable);
    //bt_result_code btGapSetLinkMode(bt_address remote_addr, BTLinkMode link_mode);
    bt_result_code (*start_discovery)(void* gap_handle, uint32_t timeout);
    bt_result_code (*stop_discovery)(void* gap_handle);
    
    /*service discovery*/
    bt_result_code (*start_service_discovery)(void* gap_handle, bt_device_t device, bt_uuid_t uuid);
    bt_result_code (*stop_service_discovery)(void* gap_handle, bt_device_t device);

    /*VSC command*/
    bt_result_code (*send_hci_command_v1)(void* gap_handle, bt_hci_command_t *command, bt_service_hci_command_complete_event event_type);

    /*ble interface*/
    bt_result_code (*set_ble_scan_parameters)(void* gap_handle, scan_params_t *scan_param);
    bt_result_code (*set_ble_scan_filter)(void* gap_handle, ble_scan_filter_t *scan_filter);
    bt_result_code (*start_ble_scan)(void* gap_handle);
    bt_result_code (*stop_ble_scan)(void* gap_handle);
    bt_result_code (*start_ble_adv)(void* gap_handle, advertise_param_t *scan_params);
    bt_result_code (*stop_ble_adv)(void* gap_handle, uint8_t adv_id);
    bt_result_code (*ble_set_static_identity)(void* gap_handle, bt_device_t device);
    bt_result_code (*ble_get_current_irk)(void* gap_handle);
    bt_result_code (*ble_set_address)(void* gap_handle, bt_device_t device);
    bt_result_code (*ble_get_address)(void* gap_handle);
    bt_result_code (*ble_set_bonded_devices)(void* gap_handle, ble_keys_t *bonded_device_list, uint8_t count_in);
    bt_result_code (*ble_connect)(void* gap_handle, ble_connect_params_t *conn_param);
    bt_result_code (*ble_disconnect)(void* gap_handle, bt_device_t device);
    bt_result_code (*ble_smp_reply)(void* gap_handle, spp_reply_data_t *reply_data);
    bt_result_code (*ble_add_white_list)(void* gap_handle, bt_device_t device);
    bt_result_code (*ble_remove_white_list)(void* gap_handle, bt_device_t device);
    bt_result_code (*ble_add_resolving_list)(void* gap_handle, bt_device_t device);
    bt_result_code (*ble_remove_resolving_list)(void* gap_handle, bt_device_t device);
    bt_result_code (*ble_set_phy)(void* gap_handle, bt_device_t device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
    bt_result_code (*ble_add_private_channel)(void* gap_handle, uint16_t private_cid);
    bt_result_code (*ble_send_packet)(void* gap_handle, bt_device_t device, uint16_t private_cid, uint8_t *packet, uint16_t packet_size);
        
    /**
     * Send HCI command for testing purpose
     * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
     * @return Bluetooth Error status code (0- Success)
     */
    bt_result_code (*send_hci_command)(uint8_t *hci_cmd_packet, hci_event_callback cb);
    bt_result_code (*enter_bluetooth_test_mode)(bt_test_mode test_mode);

}gap_service_interface_t;

bt_result_code gap_service_init(void);
gap_service_interface_t* get_gap_service_instance(void);

#endif