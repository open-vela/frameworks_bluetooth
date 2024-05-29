/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#ifndef __SAL_ADAPTER_INTERFACE_H_
#define __SAL_ADAPTER_INTERFACE_H_

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_status.h"
#include <stdint.h>
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
#include "bt_le_advertiser.h"
#include "bt_le_scan.h"
#include "scan_manager.h"
#endif
#include "bluetooth_define.h"

/* service adapter layer for BREDR */
bt_status_t bt_sal_init(void);
void bt_sal_cleanup(void);
bt_status_t bt_sal_enable(void);
bt_status_t bt_sal_disable(void);
bt_status_t bt_sal_set_local_name(char* name);
bt_status_t bt_sal_get_local_address(bt_address_t* addr);
bt_status_t bt_sal_set_local_io_capability(bt_io_capability_t cap);
const char* bt_sal_get_local_name(void);
bt_status_t bt_sal_set_local_device_class(uint32_t cod);
uint32_t bt_sal_get_local_device_class(void);
bt_status_t bt_sal_set_scan_mode(bt_scan_mode_t scan_mode, bool bondable);
bt_status_t bt_sal_set_discovery_filter(bt_discovery_filter_t* filter);
bt_status_t bt_sal_start_discovery(uint32_t timeout);
bt_status_t bt_sal_stop_discovery(void);
bt_status_t bt_sal_set_page_scan_parameters(bt_scan_type_t type,
    uint16_t interval,
    uint16_t window);
bt_status_t bt_sal_set_inquiry_scan_parameters(bt_scan_type_t type,
    uint16_t interval,
    uint16_t window);
/* Remote device */
bt_status_t bt_sal_get_remote_name(bt_address_t* addr);
int bt_sal_get_remote_services(bt_address_t* addr, bt_uuid_t* service_list, uint8_t count_in);
bt_status_t bt_sal_reply_sco_link_request(bt_address_t* addr, bool accept);
bt_status_t bt_sal_reply_link_request(bt_address_t* addr, bool accept);
bt_status_t bt_sal_reply_pair_request(bt_address_t* addr, uint8_t reason);
bt_status_t bt_sal_ssp_reply(bt_address_t* addr,
    bool accept,
    bt_pair_type_t type,
    uint32_t passkey);
bt_status_t bt_sal_pin_reply(bt_address_t* addr,
    bool accept,
    char* pincode,
    int len);
uint16_t bt_sal_get_acl_link_handle(bt_address_t* addr);
bt_status_t bt_sal_connect(bt_address_t* addr);
bt_status_t bt_sal_disconnect(bt_address_t* addr);
bt_status_t bt_sal_create_bond(bt_address_t* addr);
bt_status_t bt_sal_cancel_bond(bt_address_t* addr);
bt_status_t bt_sal_remove_bond(bt_address_t* addr);
bt_status_t bt_sal_ssp_set_remote_oob_data(bt_address_t* addr,
    bt_128key_t c_192_val, bt_128key_t r_192_val,
    bt_128key_t c_256_val, bt_128key_t r_256_val);
bt_status_t bt_sal_ssp_get_local_oob_data(void);
bt_status_t bt_sal_get_remote_device_info(bt_address_t* addr, remote_device_properties_t* properties);
bt_status_t bt_sal_set_bonded_devices(remote_device_properties_t* prop);
bt_status_t bt_sal_get_bonded_devices(remote_device_properties_t* properties, int* cnt);
bt_status_t bt_sal_get_connected_devices(remote_device_properties_t* properties, int* cnt);
bt_status_t bt_sal_start_service_discovery(bt_address_t* addr, bt_uuid_t* uuid);
bt_status_t bt_sal_stop_service_discovery(bt_address_t* addr);
bt_status_t bt_sal_set_link_role(bt_address_t* addr, bt_link_role_t role);
bt_status_t bt_sal_set_link_policy(bt_address_t* addr, bt_link_policy_t policy);
bt_status_t bt_sal_set_afh_channel_classification(uint16_t central_frequency,
    uint16_t band_width,
    uint16_t number);
/* service adapter layer for LE */
// #ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
bt_status_t bt_sal_le_init(void);
void bt_sal_le_cleanup(void);
bt_status_t bt_sal_le_enable(void);
bt_status_t bt_sal_le_disable(void);
bt_status_t bt_sal_le_set_io_capability(bt_io_capability_t cap);
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
bt_status_t bt_sal_le_set_scan_parameters(ble_scan_params_t* params);
/* maybe implement it in scan service */
bt_status_t bt_sal_le_set_scan_filters(ble_scan_filter_t* filter);
bt_status_t bt_sal_le_start_scan(void);
bt_status_t bt_sal_le_stop_scan(void);
#endif
#ifdef CONFIG_BLUETOOTH_BLE_ADV
bt_status_t bt_sal_le_start_adv(uint8_t adv_id,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len);
bt_status_t bt_sal_le_stop_adv(uint8_t adv_id);
#endif
int bt_sal_get_le_bonded_devices(void);
int bt_sal_get_le_connected_devices(void);
int bt_sal_get_le_whitelist_devices(void);
int bt_sal_get_le_resolvinglist_devices(void);
bt_status_t bt_sal_le_set_static_identity(bt_address_t* addr);
bt_status_t bt_sal_le_set_public_identity(bt_address_t* addr);
bt_status_t bt_sal_le_set_remote_irk(bt_address_t* addr, ble_addr_type_t type, bt_128key_t irk);
bt_status_t bt_sal_le_get_current_irk(void);
bt_status_t bt_sal_le_set_address(bt_address_t* addr);
bt_status_t bt_sal_le_get_address(void);
bt_status_t bt_sal_le_set_bonded_devices(remote_device_le_properties_t* props, uint16_t prop_cnt);
bt_status_t bt_sal_le_connect(bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* params);
bt_status_t bt_sal_le_disconnect(bt_address_t* addr);
bt_status_t bt_sal_le_create_bond(bt_address_t* addr, ble_addr_type_t type);
bt_status_t bt_sal_le_remove_bond(bt_address_t* addr);
bt_status_t bt_sal_le_smp_reply(bt_address_t* addr,
    bool accept,
    bt_pair_type_t type,
    uint32_t passkey);
bt_status_t bt_sal_le_set_legacy_tk(bt_address_t* addr, bt_128key_t tk_val);
bt_status_t bt_sal_le_set_remote_oob_data(bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val);
bt_status_t bt_sal_le_get_local_oob_data(bt_address_t* addr);
bt_status_t bt_sal_le_add_white_list(bt_address_t* addr);
bt_status_t bt_sal_le_remove_white_list(bt_address_t* addr);
bt_status_t bt_sal_le_add_resolving_list(bt_address_t* addr);
bt_status_t bt_sal_le_remove_resolving_list(bt_address_t* addr);
bt_status_t bt_sal_le_set_phy(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy);
bt_status_t bt_sal_le_set_appearance(uint16_t appearance);
uint16_t bt_sal_le_get_appearance(void);
bt_status_t bt_sal_le_enable_key_derivation(bool brkey_to_lekey,
    bool lekey_to_brkey);

bt_status_t bt_sal_send_hci_command(uint8_t ogf, uint16_t ocf, uint8_t length, uint8_t* buf,
    bt_hci_event_callback_t cb, void* context);
#endif /* __SAL_ADAPTER_INTERFACE_H_ */
