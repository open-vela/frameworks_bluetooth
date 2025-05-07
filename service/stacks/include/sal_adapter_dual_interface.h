/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __SAL_ADAPTER_CLASSIC_INTERFACE_H_
#define __SAL_ADAPTER_CLASSIC_INTERFACE_H_

#include <stdint.h>

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_device.h"
#include "bt_status.h"
#include "bt_vhal.h"

#include "bluetooth_define.h"
#include "power_manager.h"

/* service adapter layer for BREDR */
// #ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
bt_status_t bt_sal_init(const bt_vhal_interface* vhal);
void bt_sal_cleanup(void);

/* Adapter power */
bt_status_t bt_sal_enable(bt_controller_id_t id);
bt_status_t bt_sal_disable(bt_controller_id_t id);
bool bt_sal_is_enabled(bt_controller_id_t id);

/* Adapter properties */
bt_status_t bt_sal_set_name(bt_controller_id_t id, char* name);
const char* bt_sal_get_name(bt_controller_id_t id);
bt_status_t bt_sal_get_address(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_set_io_capability(bt_controller_id_t id, bt_io_capability_t cap);
bt_io_capability_t bt_sal_get_io_capability(bt_controller_id_t id);
bt_status_t bt_sal_set_device_class(bt_controller_id_t id, uint32_t cod);
uint32_t bt_sal_get_device_class(bt_controller_id_t id);
bt_status_t bt_sal_set_scan_mode(bt_controller_id_t id, bt_scan_mode_t scan_mode, bool bondable);
bt_scan_mode_t bt_sal_get_scan_mode(bt_controller_id_t id);
bool bt_sal_get_bondable(bt_controller_id_t id);

/* Inquiry/page and inquiry/page scan */
bt_status_t bt_sal_start_discovery(bt_controller_id_t id, uint32_t timeout);
bt_status_t bt_sal_stop_discovery(bt_controller_id_t id);
bt_status_t bt_sal_set_page_scan_parameters(bt_controller_id_t id, bt_scan_type_t type,
    uint16_t interval, uint16_t window);
bt_status_t bt_sal_set_inquiry_scan_parameters(bt_controller_id_t id, bt_scan_type_t type,
    uint16_t interval, uint16_t window);

/* Remote device RNR/connection/bond/properties */
bt_status_t bt_sal_get_remote_name(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_auto_accept_connection(bt_controller_id_t id, bool enable);
bt_status_t bt_sal_sco_connection_reply(bt_controller_id_t id, bt_address_t* addr, bool accept);
bt_status_t bt_sal_acl_connection_reply(bt_controller_id_t id, bt_address_t* addr, bool accept);
bt_status_t bt_sal_pair_reply(bt_controller_id_t id, bt_address_t* addr, uint8_t reason);
bt_status_t bt_sal_ssp_reply(bt_controller_id_t id, bt_address_t* addr,
    bool accept, bt_pair_type_t type, uint32_t passkey);
bt_status_t bt_sal_pin_reply(bt_controller_id_t id, bt_address_t* addr,
    bool accept, char* pincode, int len);
connection_state_t bt_sal_get_connection_state(bt_controller_id_t id, bt_address_t* addr);
uint16_t bt_sal_get_acl_connection_handle(bt_controller_id_t id, bt_address_t* addr, bt_transport_t trasnport);
uint16_t bt_sal_get_sco_connection_handle(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_connect(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_disconnect(bt_controller_id_t id, bt_address_t* addr, uint8_t reason);
bt_status_t bt_sal_create_bond(bt_controller_id_t id, bt_address_t* addr, bt_transport_t transport, bt_addr_type_t type);
bt_status_t bt_sal_cancel_bond(bt_controller_id_t id, bt_address_t* addr, bt_transport_t transport);
bt_status_t bt_sal_remove_bond(bt_controller_id_t id, bt_address_t* addr, bt_transport_t transport);
bt_status_t bt_sal_set_remote_oob_data(bt_controller_id_t id, bt_address_t* addr,
    bt_oob_data_t* p192_val, bt_oob_data_t* p256_val);
bt_status_t bt_sal_remove_remote_oob_data(bt_controller_id_t id, bt_address_t* addr);
bt_status_t bt_sal_get_local_oob_data(bt_controller_id_t id);
bt_status_t bt_sal_get_remote_device_info(bt_controller_id_t id, bt_address_t* addr, remote_device_properties_t* properties);
bt_status_t bt_sal_set_bonded_devices(bt_controller_id_t id, remote_device_properties_t* props, int cnt);
bt_status_t bt_sal_get_bonded_devices(bt_controller_id_t id, remote_device_properties_t* props, int* cnt);
bt_status_t bt_sal_get_connected_devices(bt_controller_id_t id, remote_device_properties_t* props, int* cnt);

/* Service discovery */
bt_status_t bt_sal_start_service_discovery(bt_controller_id_t id, bt_address_t* addr, bt_uuid_t* uuid);
bt_status_t bt_sal_stop_service_discovery(bt_controller_id_t id, bt_address_t* addr);

/* Link policy */
bt_status_t bt_sal_set_power_mode(bt_controller_id_t id, bt_address_t* addr, bt_pm_mode_t* mode);
bt_status_t bt_sal_set_link_role(bt_controller_id_t id, bt_address_t* addr, bt_link_role_t role);
bt_status_t bt_sal_set_link_policy(bt_controller_id_t id, bt_address_t* addr, bt_link_policy_t policy);
bt_status_t bt_sal_set_afh_channel_classification(bt_controller_id_t id, uint16_t central_frequency,
    uint16_t band_width, uint16_t number);
bt_status_t bt_sal_set_afh_channel_classification_1(bt_controller_id_t id, uint8_t* map);

/* VSC */
bt_status_t bt_sal_send_hci_command(bt_controller_id_t id, uint8_t ogf, uint16_t ocf, uint8_t length, uint8_t* buf,
    bt_hci_event_callback_t cb, void* context);
// #endif
#endif /* __SAL_ADAPTER_CLASSIC_INTERFACE_V2_H_ */
