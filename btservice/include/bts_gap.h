#ifndef __BTS_GAP__H__
#define __BTS_GAP__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include "bts_gap_service.h"
#include "stack_adapter_common.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef void (*bts_adapter_state_changed_callback)(stack_state_t state);
typedef void (*bts_gap_init_done_callback)();
typedef void (*bts_received_remote_name_callback)(BD_ADDR bd_addr, char *bt_name, uint8_t length);
typedef void (*bts_discovery_state_changed_callback)(bt_discovery_state state);
typedef void (*bts_ssp_request_callback)(bt_ssp_request_data_t *request_data);
typedef void (*bts_device_found_callback)(bt_device_t* device);
typedef void (*bts_bond_state_changed_callback)(bt_device_t* device, bt_bond_state state);
typedef void (*bts_connection_state_callback)(bt_device_t* device, bt_connection_state state);
typedef void (*bts_get_bonded_device_list_callback)(bt_address*bonded_device_list, uint8_t umber);
typedef void (*bts_connected_device_list_callback)(bt_address*connected_device_list, uint8_t umber);
typedef void (*bts_hci_event_callback)(bt_hci_event_t *hci_event);
//typedef void (*bts_service_discovered_callback)( bd_addr_t remote_addr, br_service_t* services, uint16_t size);
typedef void (*bts_update_ble_bonded_devices_callback)(ble_keys_t *bonded_device_list, uint8_t count_in);
typedef void (*bts_smp_request_callback)(ssp_request_data_t *request_data);
typedef void (*bts_ble_phy_update_callback)( bd_addr_t remote_addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, bt_status status);
typedef void (*bts_ble_address_callback)( bd_addr_t ble_addr, ble_addr_type ble_addr_type);

typedef struct {
    /* * set to sizeof(GAP_CALLBACKS_S) */
    uint8_t size;
    bts_adapter_state_changed_callback adapter_state_changed_cb;
    bts_gap_init_done_callback gap_init_done_cb;
    bts_received_remote_name_callback remote_name_cb;
    bts_discovery_state_changed_callback discovery_state_changed_cb;
    bts_ssp_request_callback spp_request_cb;
    bts_device_found_callback device_found_cb;
    bts_bond_state_changed_callback bond_state_changed_cb;
    bts_connection_state_callback connection_state_changed_cb;
    bts_get_bonded_device_list_callback bonded_list_cb;
    bts_connected_device_list_callback connected_list_cb;
    bts_hci_event_callback hci_event_cb;
    //TODO:: Add Patch Download Start Callback
    bts_smp_request_callback smp_request_cb;
    bts_ble_phy_update_callback ble_phy_update_cb;
    bts_ble_address_callback ble_address_cb;
} bts_gap_callback_t;

bt_result_code gap_init(bts_gap_callback_t* cb);
bt_result_code gap_enable(void);
bt_result_code gap_disable(bool normal_disable);
stack_state_t gap_get_stack_state(void);
/*Local property*/

 bt_result_code bts_set_local_address(bt_device_t *device);
 bt_address* bts_get_local_address(void);
 bt_result_code bts_set_local_io_capability(bt_io_capability io_capability);
 bt_result_code bts_set_local_name(char *bt_name, uint8_t len);
 char* bts_get_local_name(void);
 bt_result_code bts_set_local_device_class(uint32_t class_of_device);

 uint32_t bts_get_local_device_class(void);

/*Remote device*/
 bt_result_code bts_get_remote_name(bt_device_t *device);
 bt_result_code bts_get_connection_state(bt_device_t *device);
int  bts_get_remote_services(bt_device_t* remote_addr, bt_uuid_t *service_list, uint8_t count_in);

/*Bond*/
 bt_bond_state bts_get_bond_state(bt_device_t *device);
 bt_result_code bts_reply_pair_request(bt_device_t *device, bool accept);
 bt_result_code bts_create_bond(bt_device_t *device);
 bt_result_code bts_cancel_bond(bt_device_t *device);
 bt_result_code bts_remove_bond(bt_device_t *device);
 bt_result_code bts_get_bonded_devices(void);

/*Connection*/
 bt_result_code bts_connect_all(bt_device_t *device);
 bt_result_code bts_disonnect_all(bt_device_t *device);
 bt_result_code bts_get_connected_devices(void);

/*Discovery*/
 bt_result_code bts_set_scan_mode(bt_scan_mode scanMode, bool bondable);

 bt_result_code bts_start_discovery(uint32_t timeout);
 bt_result_code bts_stop_discovery(void);
 
 /*service discovery*/

bt_result_code bts_start_service_discovery(bt_device_t *device, bt_uuid_t uuid);
bt_result_code bts_stop_service_discovery(bt_device_t *device);

/*VSC command*/
 bt_result_code bts_send_hci_command_v1(bt_hci_command_t *command, bt_service_hci_command_complete_event event_type);

/*ble interface*/
bt_result_code bts_set_ble_scan_parameters(scan_params_t *scan_param);
bt_result_code bts_set_ble_scan_filter(ble_scan_filter_t *scan_filter);
bt_result_code bts_start_ble_scan(void);
bt_result_code bts_stop_ble_scan(void);
bt_result_code bts_start_ble_adv(advertise_param_t *scan_params);
bt_result_code bts_stop_ble_adv(uint8_t adv_id);

bt_result_code bts_ble_set_static_identity(bt_device_t *device);
bt_result_code bts_ble_get_current_irk(void);
bt_result_code bts_ble_set_address(bt_device_t *device);
bt_result_code bts_ble_get_address(void);
bt_result_code bts_ble_set_bonded_devices(ble_keys_t *bonded_device_list, uint8_t count_in);
bt_result_code bts_ble_connect(ble_connect_params_t *conn_param);
bt_result_code bts_ble_disconnect(bt_device_t *device);
bt_result_code bts_ble_smp_reply(spp_reply_data_t *reply_data);
bt_result_code bts_ble_add_white_list(bt_device_t *device);
bt_result_code bts_ble_remove_white_list(bt_device_t *device);
bt_result_code bts_ble_add_resolving_list(bt_device_t *device);
bt_result_code bts_ble_remove_resolving_list(bt_device_t *device);
bt_result_code bts_ble_set_phy(bt_device_t *device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
bt_result_code bts_ble_add_private_channel(uint16_t private_cid);
bt_result_code bts_ble_send_packet(bt_device_t *device, uint16_t private_cid,
        uint8_t *packet, uint16_t packet_size);
       
/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code bts_send_hci_command(uint8_t *hci_cmd_packet, hci_event_callback cb);
bt_result_code bts_enter_bluetooth_test_mode(bt_test_mode test_mode);

void gap_read_data_storage(void);
void gap_update_data_storage(void);

#endif


