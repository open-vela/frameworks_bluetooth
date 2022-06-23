#ifndef __BTS_GAP__H__
#define __BTS_GAP__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bts_gap_service.h"
#include "bts_service.h"
#include "stack_adapter_common.h"
#include <stdio.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef void (*bts_adapter_state_changed_callback)(SERVICE_BT_STACK_STATE state);
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
typedef void (*bts_ble_phy_update_callback)(bt_address remote_addr, ble_phy_type tx_phy, ble_phy_type rx_phy, bt_status status);
typedef void (*bts_ble_address_callback)(bt_address ble_addr, ble_addr_type ble_addr_type);
typedef void (*bts_pairing_request_callback)(BD_ADDR remote_addr, bool local_initiate, bool is_bondable);
typedef void (*bts_ble_irk_callback)(bt_common_key irk, bt_address ble_addr, ble_addr_type ble_addr_type);
typedef void (*bts_delete_linkey_callback)(bt_address remote_addr, bt_status reason);
typedef void (*bts_link_connect_request_callback)(bt_address remote_addr);
typedef void (*bts_ble_adv_started_callback)(uint8_t adv_id);
typedef void (*bts_ble_adv_stopped_callback)(uint8_t adv_id);
typedef void (*bts_ble_connection_updated_callback)(bt_address remote_addr, bt_status status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout);

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
    bts_delete_linkey_callback delete_linkey_cb;
    bts_link_connect_request_callback link_connect_request_cb;
    bts_ble_adv_started_callback ble_adv_started_cb;
    bts_ble_adv_stopped_callback ble_adv_stopped_cb;
    bts_ble_connection_updated_callback ble_connection_updated_cb;
} bts_gap_callback_t;

bt_result_code gap_init(bts_gap_callback_t* cb);
void gap_cleanup(void);

bt_result_code gap_enable(void);
bt_result_code gap_disable(bool normal_disable);
SERVICE_BT_STACK_STATE gap_get_stack_state(void);
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
bt_result_code bts_ssp_reply(spp_reply_data_t* reply_data);
bt_result_code bts_create_bond(bt_device_t* device);
bt_result_code bts_cancel_bond(bt_device_t* device);
bt_result_code bts_remove_bond(bt_device_t* device);
int bts_get_bonded_devices(bt_device_t* device_list, int max_out);

/*Connection*/
int bts_get_connected_devices(bt_device_t* device_list, int max_out);

/*Discovery*/
bt_result_code bts_set_scan_mode(bt_scan_mode scanMode, bool bondable);

bt_result_code bts_start_discovery(uint32_t timeout);
bt_result_code bts_stop_discovery(void);

/*service discovery*/

bt_result_code bts_start_service_discovery(bt_device_t* device, bt_uuid_t uuid);
bt_result_code bts_stop_service_discovery(bt_device_t* device);
bt_result_code bts_set_link_role(bt_device_t* device, bt_link_role role);
/*VSC command*/
bt_result_code bts_send_hci_command(bt_hci_command_t* command, hci_command_complete_event event_type);

bt_result_code bts_ble_set_static_identity(bt_device_t* device);
bt_result_code bts_ble_set_public_identity(bt_device_t* device);
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
bt_result_code bts_ble_set_phy(bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy);
bt_result_code bts_ble_add_private_channel(uint16_t private_cid);
bt_result_code bts_ble_send_packet(bt_device_t* device, uint16_t private_cid,
    uint8_t* packet, uint16_t packet_size);
int bts_get_ble_bonded_devices(bt_device_t* device_list, int max_out);
int bts_get_ble_connected_devices(bt_device_t* device_list, int max_out);
int bts_get_ble_whitelist_devices(bt_device_t* device_list, int max_out);
int bts_get_ble_resolvinglist_devices(bt_device_t* device_list, int max_out);
/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code bts_enter_bluetooth_test_mode(test_mode mode);

bt_result_code bts_set_page_scan_parameters(bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window);

bt_result_code bts_set_inquiry_scan_parameters(bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window);

bt_result_code bts_reply_link_request(BD_ADDR remote_addr, bool accept);

bt_result_code bts_set_afh_channel_classification(bt_afh_radio_channel_info_t* channels, uint16_t number);

void gap_bt_config_init(bts_service_adapter_state_changed_callback cb);
bt_result_code gap_bt_update_name(char* name, uint8_t size);
bt_result_code gap_bt_update_io_capability(bt_io_capability io_capability);
bt_result_code gap_bt_update_device_class(uint32_t class_of_device);
bt_result_code gap_bt_update_scan_mode(bt_scan_mode scan_mode, bool bondable);
void gap_bt_bond_store(void);
void gap_ble_bond_store(ble_keys_t* key, uint8_t count);
bt_result_code gap_ble_whitelist_store_update(bool added, bt_address addr);
void gap_bt_config_cleanup(void);

#endif
