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
#ifndef _BT_ADAPTER_INTERNAL_H__
#define _BT_ADAPTER_INTERNAL_H__

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_adapter.h"
#include "bt_device.h"
#include "bt_status.h"
#include <stdbool.h>

enum {
    DISCOVER_STATE_CHANGE_EVT,
    DEVICE_FOUND_EVT,
    REMOTE_NAME_RECIEVED_EVT,
    CONNECT_REQUEST_EVT,
    CONNECTION_STATE_CHANGE_EVT,
    PAIR_REQUEST_EVT,
    PIN_REQUEST_EVT,
    SSP_REQUEST_EVT,
    BOND_STATE_CHANGE_EVT,
    ENC_STATE_CHANGE_EVT,
    LINK_KEY_UPDATE_EVT,
    LINK_KEY_REMOVED_EVT,
    LINK_ROLE_CHANGED_EVT,
    LINK_MODE_CHANGED_EVT,
    LINK_POLICY_CHANGED_EVT,
    SDP_SEARCH_DONE_EVT,
    LE_ADDR_UPDATE_EVT,
    LE_PHY_UPDATE_EVT,
    LE_IRK_UPDATE_EVT,
    LE_WHITELIST_UPDATE_EVT,
    LE_BONDED_DEVICE_UPDATE_EVT,
    LE_SC_LOCAL_OOB_DATA_GOT_EVT,
};

typedef struct {
    void* device;
    bond_state_t previous_state;
    bool is_ctkd;
} bond_state_change_message_t;

typedef struct {
    bt_address_t addr; // Remote BT address
    ble_addr_type_t addr_type; // if link type is ble connection type
    uint8_t transport;
    bt_status_t status;
    connection_state_t connection_state;
    uint32_t hci_reason_code;
} acl_state_param_t;

typedef struct {
    uint8_t evt_id;
    union {
        bt_discovery_state_t state;
        bt_discovery_result_t result;
        struct {
            bt_address_t addr;
            uint8_t name[BT_REM_NAME_MAX_LEN + 1];
        } remote_name;
    };
} adapter_discovery_evt_t;

typedef struct {
    uint8_t evt_id;
    union {
        struct {
            bt_address_t local_addr;
            ble_addr_type_t type;
        } addr_update;
        struct
        {
            /* data */
            bt_address_t addr;
            uint8_t tx_phy;
            uint8_t rx_phy;
            uint8_t status;
        } phy_update;
        struct {
            bt_address_t local_addr;
            ble_addr_type_t type;
            bt_128key_t irk;
        } irk_update;
        struct
        {
            /* data */
            bt_address_t addr;
            bool is_add;
            bt_status_t status;
        } whitelist;
        struct
        {
            /* data */
            remote_device_le_properties_t* props;
            uint16_t bonded_devices_cnt;
        } bonded_devices;
        struct {
            bt_address_t addr;
            bt_128key_t c_val;
            bt_128key_t r_val;
        } oob_data;
    };
} adapter_ble_evt_t;

typedef struct {
    bt_address_t addr;
    uint8_t evt_id;
    union {
        uint32_t cod;
        acl_state_param_t acl_params;
        struct {
            bool local_initiate;
            bool is_bondable;
        } pair_req;
        struct {
            uint32_t cod;
            bool min_16_digit;
            char name[BT_REM_NAME_MAX_LEN + 1];
        } pin_req;
        struct {
            uint32_t cod;
            bt_pair_type_t ssp_type;
            uint32_t pass_key;
            uint8_t transport;
            char name[BT_REM_NAME_MAX_LEN + 1];
        } ssp_req;
        struct {
            bond_state_t state;
            uint8_t transport;
            bt_status_t status;
            bool is_ctkd;
        } bond_state;
        struct {
            bool encrypted;
            uint8_t transport;
        } enc_state;
        struct {
            bt_128key_t key;
            bt_link_key_type_t type;
            bt_status_t status;
        } link_key;
        struct {
            bt_link_role_t role;
        } link_role;
        struct {
            bt_link_mode_t mode;
            uint16_t sniff_interval;
        } link_mode;
        struct {
            bt_link_policy_t policy;
        } link_policy;
        struct {
            uint16_t uuid_size;
            bt_uuid_t* uuids;
        } sdp;
    };
} adapter_remote_event_t;

typedef struct adapter_state_machine adapter_state_machine_t;

enum {
    APP_SET_LE_ONLY = 0,
    SYS_SET_BT_ALL
};

enum {
    BT_BREDR_STACK_STATE_OFF,
    BT_BREDR_STACK_STATE_ON,
    BLE_STACK_STATE_OFF,
    BLE_STACK_STATE_ON
};

enum adapter_event {
    SYS_TURN_ON = 0,
    SYS_TURN_OFF,
    TURN_ON_BLE,
    TURN_OFF_BLE,
    /*
        Don't support BREDR-only mode. If the user chooses TURN_ON,
        we turn on ble first by default, and then turn on bt
    */
    BREDR_ENABLED,
    BREDR_DISABLED,
    BREDR_PROFILE_ENABLED,
    BREDR_PROFILE_DISABLED,
    BREDR_ENABLE_TIMEOUT,
    BREDR_DISABLE_TIMEOUT,
    BREDR_ENABLE_PROFILE_TIMEOUT,
    BREDR_DISABLE_PROFILE_TIMEOUT,
    BLE_ENABLED,
    BLE_DISABLED,
    BLE_PROFILE_ENABLED,
    BLE_PROFILE_DISABLED,
    BLE_ENABLE_TIMEOUT,
    BLE_DISABLE_TIMEOUT,
    BLE_ENABLE_PROFILE_TIMEOUT,
    BLE_DISABLE_PROFILE_TIMEOUT,
};

/* adapter state machine API functions*/
adapter_state_machine_t* adapter_state_machine_new(void* context);
void adapter_state_machine_destory(adapter_state_machine_t* stm);
bt_status_t adapter_send_event(uint16_t event_id, void* data);
bt_status_t adapter_on_profile_services_startup(uint8_t transport, bool ret);
bt_status_t adapter_on_profile_services_shutdown(uint8_t transport, bool ret);
/* adapter notification */
void adapter_notify_state_change(bt_adapter_state_t prev, bt_adapter_state_t current);
void adapter_on_le_enabled(bool enablebt);
void adapter_on_le_disabled(void);
void adapter_on_br_enabled(void);
void adapter_on_br_disabled(void);

/* adapter sal callback invoke functions */
void adapter_on_adapter_state_changed(uint8_t stack_state);
void adapter_on_device_found(bt_discovery_result_t* result);
void adapter_on_scan_mode_changed(bt_scan_mode_t mode);
void adapter_on_discovery_state_changed(bt_discovery_state_t state);
void adapter_on_remote_name_recieved(bt_address_t* addr, const char* name);
void adapter_on_connect_request(bt_address_t* addr, uint32_t cod);
void adapter_on_connection_state_changed(acl_state_param_t* param);
void adapter_on_pairing_request(bt_address_t* addr, bool local_initiate, bool is_bondable);
void adapter_on_ssp_request(bt_address_t* addr, uint8_t transport,
    uint32_t cod, bt_pair_type_t ssp_type,
    uint32_t pass_key, const char* name);
void adapter_on_pin_request(bt_address_t* addr, uint32_t cod,
    bool min_16_digit, const char* name);
void adapter_on_bond_state_changed(bt_address_t* addr, bond_state_t state, uint8_t transport, bt_status_t status, bool is_ctkd);
void adapter_on_service_search_done(bt_address_t* addr, bt_uuid_t* uuids, uint16_t size);
void adapter_on_encryption_state_changed(bt_address_t* addr, bool encrypted, uint8_t transport);
void adapter_on_link_key_update(bt_address_t* addr, bt_128key_t link_key, bt_link_key_type_t type);
void adapter_on_link_key_removed(bt_address_t* addr, bt_status_t status);
void adapter_on_link_role_changed(bt_address_t* addr, bt_link_role_t role);
void adapter_on_link_mode_changed(bt_address_t* addr, bt_link_mode_t mode, uint16_t sniff_interval);
void adapter_on_link_policy_changed(bt_address_t* addr, bt_link_policy_t policy);
void adapter_on_le_addr_update(bt_address_t* addr, ble_addr_type_t type);
void adapter_on_le_phy_update(bt_address_t* addr, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy, bt_status_t status);
void adapter_on_whitelist_update(bt_address_t* addr, bool is_add, bt_status_t status);
void adapter_on_le_bonded_device_update(remote_device_le_properties_t* props, uint16_t bonded_devices_cnt);
void adapter_on_le_local_oob_data_got(bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val);

/* adapter framework invoke functions */
void adapter_init(void);
void adapter_cleanup(void);
bt_status_t adapter_enable(uint8_t opt);
bt_status_t adapter_disable(uint8_t opt);
bt_adapter_state_t adapter_get_state(void);
bool adapter_is_le_enabled(void);
bt_device_type_t adapter_get_type(void);

bt_status_t adapter_set_discovery_filter(void);
bt_status_t adapter_start_discovery(uint32_t timeout);
bt_status_t adapter_cancel_discovery(void);
bool adapter_is_discovering(void);
void adapter_get_address(bt_address_t* addr);
bt_status_t adapter_set_name(const char* name);
void adapter_get_name(char* name, int size);
bt_status_t adapter_get_uuids(bt_uuid_t* uuids, uint16_t* size);
bt_status_t adapter_set_scan_mode(bt_scan_mode_t mode, bool bondable);
bt_scan_mode_t adapter_get_scan_mode(void);
bt_status_t adapter_set_device_class(uint32_t cod);
uint32_t adapter_get_device_class(void);
bt_status_t adapter_set_io_capability(bt_io_capability_t cap);

bt_io_capability_t adapter_get_io_capability(void);
bt_status_t adapter_set_inquiry_scan_parameters(bt_scan_type_t type,
    uint16_t interval,
    uint16_t window);
bt_status_t adapter_set_page_scan_parameters(bt_scan_type_t type,
    uint16_t interval,
    uint16_t window);
bt_status_t adapter_set_le_io_capability(uint32_t le_io_cap);
uint32_t adapter_get_le_io_capability(void);
bt_status_t adapter_get_le_address(bt_address_t* addr, ble_addr_type_t* type);
bt_status_t adapter_set_le_address(bt_address_t* addr);
bt_status_t adapter_set_le_identity_address(bt_address_t* addr, bool public);
bt_status_t adapter_set_le_appearance(uint16_t appearance);
uint16_t adapter_get_le_appearance(void);
bt_status_t adapter_get_bonded_devices(bt_transport_t transport, bt_address_t** addr, int* size, bt_allocator_t allocator);
bt_status_t adapter_get_connected_devices(bt_transport_t transport, bt_address_t** addr, int* size, bt_allocator_t allocator);
void adapter_set_auto_accept_connection(bool enable);
bool adapter_is_support_bredr(void);
bool adapter_is_support_le(void);
bool adapter_is_support_leaudio(void);
bt_status_t adapter_get_remote_identity_address(bt_address_t* bd_addr, bt_address_t* id_addr);
bt_device_type_t adapter_get_remote_device_type(bt_address_t* addr);
bool adapter_get_remote_name(bt_address_t* addr, char* name);
uint32_t adapter_get_remote_device_class(bt_address_t* addr);
bt_status_t adapter_get_remote_uuids(bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator);
uint16_t adapter_get_remote_appearance(bt_address_t* addr);
int8_t adapter_get_remote_rssi(bt_address_t* addr);
bool adapter_get_remote_alias(bt_address_t* addr, char* alias);
bt_status_t adapter_set_remote_alias(bt_address_t* addr, const char* alias);
bool adapter_is_remote_connected(bt_address_t* addr, bt_transport_t transport);
bool adapter_is_remote_encrypted(bt_address_t* addr, bt_transport_t transport);
bool adapter_is_bond_initiate_local(bt_address_t* addr, bt_transport_t transport);
bond_state_t adapter_get_remote_bond_state(bt_address_t* addr, bt_transport_t transport);
bool adapter_is_remote_bonded(bt_address_t* addr, bt_transport_t transport);
bt_status_t adapter_connect(bt_address_t* addr);
bt_status_t adapter_disconnect(bt_address_t* addr);
bt_status_t adapter_le_connect(bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param);
bt_status_t adapter_le_disconnect(bt_address_t* addr);
bt_status_t adapter_connect_request_reply(bt_address_t* addr, bool accept);
bt_status_t adapter_le_set_phy(bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy);
bt_status_t adapter_le_enable_key_derivation(bool brkey_to_lekey,
    bool lekey_to_brkey);
bt_status_t adapter_le_add_whitelist(bt_address_t* addr);
bt_status_t adapter_le_remove_whitelist(bt_address_t* addr);
bt_status_t adapter_create_bond(bt_address_t* addr, bt_transport_t transport);
bt_status_t adapter_remove_bond(bt_address_t* addr, uint8_t transport);
bt_status_t adapter_cancel_bond(bt_address_t* addr);
bt_status_t adapter_pair_request_reply(bt_address_t* addr, bool accept);
bt_status_t adapter_set_pairing_confirmation(bt_address_t* addr, uint8_t transport, bool accept);
bt_status_t adapter_set_pin_code(bt_address_t* addr, bool accept,
    char* pincode, int len);
bt_status_t adapter_set_pass_key(bt_address_t* addr, uint8_t transport, bool accept, uint32_t passkey);
bt_status_t adapter_le_set_legacy_tk(bt_address_t* addr, bt_128key_t tk_val);
bt_status_t adapter_le_set_remote_oob_data(bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val);
bt_status_t adapter_le_get_local_oob_data(bt_address_t* addr);
bt_status_t adapter_switch_role(bt_address_t* addr, bt_link_role_t role);
bt_status_t adapter_set_afh_channel_classification(uint16_t central_frequency,
    uint16_t band_width,
    uint16_t number);
void* adapter_register_callback(void* remote, const adapter_callbacks_t* adapter_cbs);
bool adapter_unregister_callback(void** remote, void* cookie);

void adapter_dump(void);
void adapter_dump_device(bt_address_t* addr);
// void adapter_dump_profile(enum profile_id id);
void adapter_dump_all_device(void);

#endif /* _BT_ADAPTER_INTERNAL_H__ */
