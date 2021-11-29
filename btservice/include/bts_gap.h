#ifndef __BTS_GAP__H__
#define __BTS_GAP__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bts_gap_service.h"
#include "stack_adapter_common.h"
#include <stdio.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef void (*bts_adapter_state_changed_callback)(stack_state_t state);
typedef void (*bts_received_remote_name_callback)(BD_ADDR bd_addr, char* bt_name, uint8_t length);
typedef void (*bts_discovery_state_changed_callback)(bt_discovery_state state);
typedef void (*bts_ssp_request_callback)(ssp_request_data_t* request_data);
typedef void (*bts_device_found_callback)(bt_device_t* device);
typedef void (*bts_bond_state_changed_callback)(bt_device_t* device, bt_bond_state state);
typedef void (*bts_connection_state_callback)(bt_device_t* device, bt_connection_state state);
typedef void (*bts_hci_event_callback)(hci_event_t* hci_event);
//typedef void (*bts_service_discovered_callback)( bt_address remote_addr, br_service_t* services, uint16_t size);
typedef void (*bts_update_ble_bonded_devices_callback)(ble_keys_t* bonded_device_list, uint8_t count_in);
typedef void (*bts_smp_request_callback)(ssp_request_data_t* request_data);
typedef void (*bts_ble_phy_update_callback)(bt_address remote_addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, bt_status status);
typedef void (*bts_ble_address_callback)(bt_address ble_addr, ble_addr_type ble_addr_type);
typedef void (*bts_pairing_request_callback)(BD_ADDR remote_addr, bool local_initiate, bool is_bondable);
typedef struct {
    /* * set to sizeof(GAP_CALLBACKS_S) */
    uint8_t size;
    bts_adapter_state_changed_callback adapter_state_changed_cb;
    bts_received_remote_name_callback remote_name_cb;
    bts_discovery_state_changed_callback discovery_state_changed_cb;
    bts_ssp_request_callback spp_request_cb;
    bts_pairing_request_callback pairing_request_cb;
    bts_device_found_callback device_found_cb;
    bts_bond_state_changed_callback bond_state_changed_cb;
    bts_connection_state_callback connection_state_changed_cb;
    bts_hci_event_callback hci_event_cb;
    bts_smp_request_callback smp_request_cb;
    bts_update_ble_bonded_devices_callback update_ble_bonede_device_cb;
    bts_ble_phy_update_callback ble_phy_update_cb;
    bts_ble_address_callback ble_address_cb;
} bts_gap_callback_t;

bt_result_code gap_init(bts_gap_callback_t* cb);
void gap_cleanup(void);

bt_result_code gap_enable(void);
bt_result_code gap_disable(bool normal_disable);
stack_state_t gap_get_stack_state(void);
/*Local property*/

bt_result_code bts_set_local_address(bt_device_t* device);
bt_result_code bts_get_local_address(bt_address addr);
bt_result_code bts_set_local_io_capability(bt_io_capability io_capability);
bt_result_code bts_set_local_name(char* bt_name, uint8_t len);
char* bts_get_local_name(void);
bt_result_code bts_set_local_device_class(uint32_t class_of_device);

uint32_t bts_get_local_device_class(void);

/*Remote device*/
bt_result_code bts_get_remote_name(bt_device_t* device);
int bts_get_remote_services(bt_device_t* remote_addr, bt_uuid_t* service_list, uint8_t count_in);

/*Bond*/
bt_result_code bts_reply_pair_request(bt_device_t* device, int accept);
bt_result_code bts_create_bond(bt_device_t* device);
bt_result_code bts_cancel_bond(bt_device_t* device);
bt_result_code bts_remove_bond(bt_device_t* device);
int bts_get_bonded_devices(bt_device_t* device_list);

/*Connection*/
int bts_get_connected_devices(bt_device_t* device_list);

/*Discovery*/
bt_result_code bts_set_scan_mode(bt_scan_mode scanMode, bool bondable);

bt_result_code bts_start_discovery(uint32_t timeout);
bt_result_code bts_stop_discovery(void);

/*service discovery*/

bt_result_code bts_start_service_discovery(bt_device_t* device, bt_uuid_t uuid);
bt_result_code bts_stop_service_discovery(bt_device_t* device);

/*VSC command*/
bt_result_code bts_send_hci_command(bt_hci_command_t* command, bt_service_hci_command_complete_event event_type);

bt_result_code bts_ble_set_static_identity(bt_device_t* device);
bt_result_code bts_ble_get_current_irk(void);
bt_result_code bts_ble_set_address(bt_device_t* device);
bt_result_code bts_ble_get_address(void);
bt_result_code bts_ble_set_bonded_devices(ble_keys_t* bonded_device_list, uint8_t count_in);
bt_result_code bts_ble_connect(ble_connect_params_t* conn_param);
bt_result_code bts_ble_disconnect(bt_device_t* device);
bt_result_code bts_ble_smp_reply(spp_reply_data_t* reply_data);
bt_result_code bts_ble_add_white_list(bt_device_t* device);
bt_result_code bts_ble_remove_white_list(bt_device_t* device);
bt_result_code bts_ble_add_resolving_list(bt_device_t* device);
bt_result_code bts_ble_remove_resolving_list(bt_device_t* device);
bt_result_code bts_ble_set_phy(bt_device_t* device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
bt_result_code bts_ble_add_private_channel(uint16_t private_cid);
bt_result_code bts_ble_send_packet(bt_device_t* device, uint16_t private_cid,
    uint8_t* packet, uint16_t packet_size);

/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code bts_enter_bluetooth_test_mode(bt_test_mode test_mode);

void gap_read_data_storage(void);
void gap_update_data_storage(void);

bt_result_code gap_create_factory_info(bool force);
bt_result_code gap_update_device_name(uint8_t* bt_name, uint8_t len_name);
bt_result_code gap_read_device_info(void);

#endif
