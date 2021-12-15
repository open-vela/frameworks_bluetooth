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
typedef void (*bts_adapter_state_changed_callback)(SERVICE_BT_STACK_STATE state);
typedef void (*bts_received_remote_name_callback)(BD_ADDR bd_addr, char* bt_name, uint8_t length);
typedef void (*bts_discovery_state_changed_callback)(BT_DISCOVERY_STATE state);
typedef void (*bts_ssp_request_callback)(ssp_request_data_t* request_data);
typedef void (*bts_device_found_callback)(bt_device_t* device);
typedef void (*bts_bond_state_changed_callback)(bt_device_t* device, BT_BOND_STATE state);
typedef void (*bts_connection_state_callback)(bt_device_t* device, BT_CONNECTION_STATE state);
typedef void (*bts_hci_event_callback)(hci_event_t* hci_event);
//typedef void (*bts_service_discovered_callback)( bt_address remote_addr, br_service_t* services, uint16_t size);
typedef void (*bts_update_ble_bonded_devices_callback)(ble_keys_t* bonded_device_list, uint8_t count_in);
typedef void (*bts_smp_request_callback)(ssp_request_data_t* request_data);
typedef void (*bts_ble_phy_update_callback)(bt_address remote_addr, BLE_PHY_TYPE tx_phy, BLE_PHY_TYPE rx_phy, BT_STATUS  status);
typedef void (*bts_ble_address_callback)(bt_address ble_addr, BLE_ADDR_TYPE BLE_ADDR_TYPE);
typedef void (*bts_pairing_request_callback)(BD_ADDR remote_addr, bool local_initiate, bool is_bondable);
typedef void (*bts_ble_irk_callback)(bt_common_key irk, bt_address ble_addr, BLE_ADDR_TYPE BLE_ADDR_TYPE);

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
    bts_ble_irk_callback ble_irk_cb;
} bts_gap_callback_t;

BT_RESULT_CODE gap_init(bts_gap_callback_t* cb);
void gap_cleanup(void);

BT_RESULT_CODE gap_enable(void);
BT_RESULT_CODE gap_disable(bool normal_disable);
SERVICE_BT_STACK_STATE gap_get_stack_state(void);
/*Local property*/

BT_RESULT_CODE bts_set_local_address(bt_device_t* device);
BT_RESULT_CODE bts_get_local_address(bt_address addr);
BT_RESULT_CODE bts_set_local_io_capability(BT_IO_CAPABILITY io_capability);
BT_RESULT_CODE bts_set_local_name(char* bt_name, uint8_t len);
char* bts_get_local_name(void);
BT_RESULT_CODE bts_set_local_device_class(uint32_t class_of_device);

uint32_t bts_get_local_device_class(void);

/*Remote device*/
BT_RESULT_CODE bts_get_remote_name(bt_device_t* device);
int bts_get_remote_services(bt_device_t* remote_addr, bt_uuid_t* service_list, uint8_t count_in);

/*Bond*/
BT_RESULT_CODE bts_reply_pair_request(bt_device_t* device, int accept);
BT_RESULT_CODE bts_create_bond(bt_device_t* device);
BT_RESULT_CODE bts_cancel_bond(bt_device_t* device);
BT_RESULT_CODE bts_remove_bond(bt_device_t* device);
int bts_get_bonded_devices(bt_device_t* device_list);

/*Connection*/
int bts_get_connected_devices(bt_device_t* device_list);

/*Discovery*/
BT_RESULT_CODE bts_set_scan_mode(BT_SCAN_MODE scanMode, bool bondable);

BT_RESULT_CODE bts_start_discovery(uint32_t timeout);
BT_RESULT_CODE bts_stop_discovery(void);

/*service discovery*/

BT_RESULT_CODE bts_start_service_discovery(bt_device_t* device, bt_uuid_t uuid);
BT_RESULT_CODE bts_stop_service_discovery(bt_device_t* device);

/*VSC command*/
BT_RESULT_CODE bts_send_hci_command(bt_hci_command_t* command, BT_SERVICE_HCI_COMMAND_COMPLETE_EVENT event_type);

BT_RESULT_CODE bts_ble_set_static_identity(bt_device_t* device);
BT_RESULT_CODE bts_ble_get_current_irk(void);
BT_RESULT_CODE bts_ble_set_address(bt_device_t* device);
BT_RESULT_CODE bts_ble_get_address(void);
BT_RESULT_CODE bts_ble_set_bonded_devices(ble_keys_t* bonded_device_list, uint8_t count_in);
BT_RESULT_CODE bts_ble_connect(ble_connect_params_t* conn_param);
BT_RESULT_CODE bts_ble_disconnect(bt_device_t* device);
BT_RESULT_CODE bts_ble_smp_reply(spp_reply_data_t* reply_data);
BT_RESULT_CODE bts_ble_add_white_list(bt_device_t* device);
BT_RESULT_CODE bts_ble_remove_white_list(bt_device_t* device);
BT_RESULT_CODE bts_ble_add_resolving_list(bt_device_t* device);
BT_RESULT_CODE bts_ble_remove_resolving_list(bt_device_t* device);
BT_RESULT_CODE bts_ble_set_phy(bt_device_t* device, BLE_PHY_TYPE tx_phy, BLE_PHY_TYPE rx_phy);
BT_RESULT_CODE bts_ble_add_private_channel(uint16_t private_cid);
BT_RESULT_CODE bts_ble_send_packet(bt_device_t* device, uint16_t private_cid,
    uint8_t* packet, uint16_t packet_size);

/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
BT_RESULT_CODE bts_enter_bluetooth_test_mode(TEST_MODE test_mode);

void gap_read_data_storage(void);
void gap_update_data_storage(void);

BT_RESULT_CODE gap_create_factory_info(bool force);
BT_RESULT_CODE gap_update_device_name(uint8_t* bt_name, uint8_t len_name);
BT_RESULT_CODE gap_read_device_info(void);

#endif
