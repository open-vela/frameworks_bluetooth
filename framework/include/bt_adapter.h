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
#ifndef __BT_ADAPTER_H__
#define __BT_ADAPTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"
#include "bt_device.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief adapter state define
 *
 */
typedef enum {
    BT_ADAPTER_STATE_OFF = 0,
    BT_ADAPTER_STATE_BLE_TURNING_ON,
    BT_ADAPTER_STATE_BLE_ON,
    BT_ADAPTER_STATE_TURNING_ON,
    BT_ADAPTER_STATE_ON,
    BT_ADAPTER_STATE_TURNING_OFF,
    BT_ADAPTER_STATE_BLE_TURNING_OFF,
} bt_adapter_state_t;

/**
 * @brief Adapter state changed callback
 *
 * @param cookie - callback cookie.
 * @param state - new adapter state.
 */
typedef void (*on_adapter_state_changed_callback)(void* cookie, bt_adapter_state_t state);

/**
 * @brief Adapter discovery state changed callback
 *
 * @param cookie - callback cookie.
 * @param state - discovery state (0:stopped, 1:started).
 */
typedef void (*on_discovery_state_changed_callback)(void* cookie, bt_discovery_state_t state);

/**
 * @brief Discovery result callback
 *
 * @param cookie - callback cookie.
 * @param remote - romote device info.
 */
typedef void (*on_discovery_result_callback)(void* cookie, bt_discovery_result_t* remote);

/**
 * @brief Scan mode changed callback
 *
 * @param cookie - callback cookie.
 * @param mode - new scan mode.
 */
typedef void (*on_scan_mode_changed_callback)(void* cookie, bt_scan_mode_t mode);

/**
 * @brief Local device name changed callback
 *
 * @param cookie - callback cookie.
 * @param device_name - new local name.
 */
typedef void (*on_device_name_changed_callback)(void* cookie, const char* device_name);

/**
 * @brief Pair request callback (io cap request)
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 */
typedef void (*on_pair_request_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Pair information display callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param transport - transport type (0:BLE, 1:BREDR).
 * @param type - pair type.
 * @param passkey - passkey value, invalid on pair type equal PAIR_TYPE_PASSKEY_CONFIRMATION
 *                  or PAIR_TYPE_PASSKEY_NOTIFICATION.
 */
typedef void (*on_pair_display_callback)(void* cookie, bt_address_t* addr, bt_transport_t transport, bt_pair_type_t type, uint32_t passkey);

/**
 * @brief Connect request callback
 *
 * @param cookie - callback cookie
 * @param addr - remote addr.
 */
typedef void (*on_connect_request_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Connection state changed callback
 *
 * @param cookie - callback cookie
 * @param addr - remote addr.
 * @param transport - transport type (0:BLE, 1:BREDR).
 * @param state - ACL connection state.
 */
typedef void (*on_connection_state_changed_callback)(void* cookie, bt_address_t* addr, bt_transport_t transport, connection_state_t state);

/**
 * @brief Bond state changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param transport - transport type (0:BLE, 1:BREDR).
 * @param state - bond state.
 */
typedef void (*on_bond_state_changed_callback)(void* cookie, bt_address_t* addr, bt_transport_t transport, bond_state_t state, bool is_ctkd);

/**
 * @brief Got local OOB data for LE secure connection pairing callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param c_val - LE secure connection confirmation value.
 * @param r_val - LE secure connection random value.
 */
typedef void (*on_le_sc_local_oob_data_got_callback)(void* cookie, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val);

/**
 * @brief Remote device name changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param name - remote device name.
 */
typedef void (*on_remote_name_changed_callback)(void* cookie, bt_address_t* addr, const char* name);

/**
 * @brief Remote device alias changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param alias - alias, set by user.
 */
typedef void (*on_remote_alias_changed_callback)(void* cookie, bt_address_t* addr, const char* alias);

/**
 * @brief Remote device class changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param cod - remote device class.
 */
typedef void (*on_remote_cod_changed_callback)(void* cookie, bt_address_t* addr, uint32_t cod);

/**
 * @brief Remote device uuid changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param uuids - remote device support uuids.
 * @param size - uuid size.
 */
typedef void (*on_remote_uuids_changed_callback)(void* cookie, bt_address_t* addr, bt_uuid_t* uuids, uint16_t size);

/**
 * @brief Remote device link mode changed callback
 *
 * @param cookie - callback cookie.
 * @param addr - remote addr.
 * @param mode - link mode.
 * @param sniff_interval - sniff interval.
 */
typedef void (*on_remote_link_mode_changed_callback)(void* cookie, bt_address_t* addr, bt_link_mode_t mode, uint16_t sniff_interval);
/**
 * @brief Adapter callback structure
 *
 */
typedef struct {
    on_adapter_state_changed_callback on_adapter_state_changed;
    on_discovery_state_changed_callback on_discovery_state_changed;
    on_discovery_result_callback on_discovery_result;
    on_scan_mode_changed_callback on_scan_mode_changed;
    on_device_name_changed_callback on_device_name_changed;
    on_pair_request_callback on_pair_request;
    on_pair_display_callback on_pair_display;
    on_connect_request_callback on_connect_request;
    on_connection_state_changed_callback on_connection_state_changed;
    on_bond_state_changed_callback on_bond_state_changed;
    on_le_sc_local_oob_data_got_callback on_le_sc_local_oob_data_got;
    on_remote_name_changed_callback on_remote_name_changed;
    on_remote_alias_changed_callback on_remote_alias_changed;
    on_remote_cod_changed_callback on_remote_cod_changed;
    on_remote_uuids_changed_callback on_remote_uuids_changed;
    on_remote_link_mode_changed_callback on_remote_link_mode_changed;
} adapter_callbacks_t;

/**
 * @note Not support
 *
 */
typedef struct {
    on_bond_state_changed_callback on_bond_state_changed;
    on_remote_name_changed_callback on_remote_name_changed;
    on_remote_alias_changed_callback on_remote_alias_changed;
    on_remote_cod_changed_callback on_remote_cod_changed;
    on_remote_uuids_changed_callback on_remote_uuids_changed;
} remote_device_callbacks_t;

/**
 * @brief Enable bluetooth adapter
 *
 * @param ins - bluetooth client instance.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_enable)(bt_instance_t* ins);

/**
 * @brief Disable bluetooth adapter
 *
 * @param ins - bluetooth client instance.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_disable)(bt_instance_t* ins);

/**
 * @brief Enable ble
 *
 * @param ins - bluetooth client instance.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_enable_le)(bt_instance_t* ins);

/**
 * @brief Disable ble
 *
 * @param ins - bluetooth client instance.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_disable_le)(bt_instance_t* ins);

/**
 * @brief Get adapter state
 *
 * @param ins - bluetooth client instance.
 * @return bt_adapter_state_t - adapter state.
 */
bt_adapter_state_t BTSYMBOLS(bt_adapter_get_state)(bt_instance_t* ins);

/**
 * @brief Get adapter device type
 *
 * @param ins - bluetooth client instance.
 * @return bt_device_type_t - device type(0:EDR, 1:LE, 2:DUAL, 0xFF:unknow).
 */
bt_device_type_t BTSYMBOLS(bt_adapter_get_type)(bt_instance_t* ins);

/**
 * @brief Set discovery filter
 * @note Not support now.
 * @param ins - bluetooth client instance.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_discovery_filter)(bt_instance_t* ins);

/**
 * @brief Start discovery
 *
 * @param ins - bluetooth client instance.
 * @param timeout - maximum amount of time specified(Time = N * 1.28s, Range: 1.28 to 61.44 s).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_start_discovery)(bt_instance_t* ins, uint32_t timeout);

/**
 * @brief Cancel discovery
 *
 * @param ins - bluetooth client instance.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_cancel_discovery)(bt_instance_t* ins);

/**
 * @brief Check adapter is discvering
 *
 * @param ins - bluetooth client instance.
 * @return true - adapter is discovering.
 * @return false - adapter is not discovering.
 */
bool BTSYMBOLS(bt_adapter_is_discovering)(bt_instance_t* ins);

/**
 * @brief Read the bluetooth controller address(BD_ADDR)
 *
 * @param ins - bluetooth client instance.
 * @param[out] addr - BDADDR, empty value on adapter not enabled.
 */
void BTSYMBOLS(bt_adapter_get_address)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set adapter local name
 *
 * @param ins - bluetooth client instance.
 * @param name - adapter local name.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_name)(bt_instance_t* ins, const char* name);

/**
 * @brief Get adapter local name
 *
 * @param ins - bluetooth client instance.
 * @param[out] name - adapter local name from adapter service.
 * @param[in] length - maximum length of name buffer.
 */
void BTSYMBOLS(bt_adapter_get_name)(bt_instance_t* ins, char* name, int length);

/**
 * @brief Get adapter supported uuids
 * @note - not support now.
 * @param ins - bluetooth client instance.
 * @param[out] uuids - uuids .
 * @param[out] size - uuid size.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_uuids)(bt_instance_t* ins, bt_uuid_t* uuids, uint16_t* size);

/**
 * @brief Set adapter scan mode
 *
 * @param ins - bluetooth client instance.
 * @param mode - scan mode (0:none, 1:connectable, 2:connectable_discoverable).
 * @param bondable - bondable.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_scan_mode)(bt_instance_t* ins, bt_scan_mode_t mode, bool bondable);

/**
 * @brief Get adapter scan mode
 *
 * @param ins - bluetooth client instance.
 * @return bt_scan_mode_t - scan mode (0:none, 1:connectable, 2:connectable_discoverable).
 */
bt_scan_mode_t BTSYMBOLS(bt_adapter_get_scan_mode)(bt_instance_t* ins);

/**
 * @brief Set adapter device class
 *
 * @param ins - bluetooth client instance.
 * @param cod - class of device, zero is invalid.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_device_class)(bt_instance_t* ins, uint32_t cod);

/**
 * @brief Get adapter device class
 *
 * @param ins - bluetooth client instance.
 * @return uint32_t - class of device, zero on adapter not enabled.
 */
uint32_t BTSYMBOLS(bt_adapter_get_device_class)(bt_instance_t* ins);

/**
 * @brief Set BREDR adapter io capability
 *
 * @param ins - bluetooth client instance.
 * @param cap - BREDR io capability.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_io_capability)(bt_instance_t* ins, bt_io_capability_t cap);

/**
 * @brief Get BREDR adapter io capability
 *
 * @param ins - bluetooth client instance.
 * @return bt_io_capability_t - BREDR io capability.
 */
bt_io_capability_t BTSYMBOLS(bt_adapter_get_io_capability)(bt_instance_t* ins);

bt_status_t BTSYMBOLS(bt_adapter_set_inquiry_scan_parameters)(bt_instance_t* ins, bt_scan_type_t type,
    uint16_t interval, uint16_t window);

bt_status_t BTSYMBOLS(bt_adapter_set_page_scan_parameters)(bt_instance_t* ins, bt_scan_type_t type,
    uint16_t interval, uint16_t window);
/**
 * @brief Get adapter bonded devices list
 *
 * @param ins - bluetooth client instance.
 * @param[out] addr - out bonded devices address array.
 * @param[out] num - out bonded devices num.
 * @param allocator - address array allocator.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_bonded_devices)(bt_instance_t* ins, bt_transport_t transport, bt_address_t** addr, int* num, bt_allocator_t allocator);

/**
 * @brief Get adapter connected devices list
 *
 * @param ins - bluetooth client instance.
 * @param[out] addr - out connected devices address array.
 * @param[out] num - out connected devices num.
 * @param allocator - address array allocator.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_connected_devices)(bt_instance_t* ins, bt_transport_t transport, bt_address_t** addr, int* num, bt_allocator_t allocator);

/**
 * @brief Disconnect all connected device.
 * @note not support.
 * @param ins - bluetooth client instance.
 */
void BTSYMBOLS(bt_adapter_disconnect_all_devices)(bt_instance_t* ins);

/**
 * @brief Check BREDR adapter is supported
 *
 * @param ins - bluetooth client instance.
 * @return true - support.
 * @return false - not support.
 */
bool BTSYMBOLS(bt_adapter_is_support_bredr)(bt_instance_t* ins);

/**
 * @brief Register callback functions to adapter service
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - adapter callback functions.
 * @return void* - callback cookie, NULL on failure.
 */
void* BTSYMBOLS(bt_adapter_register_callback)(bt_instance_t* ins, const adapter_callbacks_t* adapter_cbs);

/**
 * @brief Unregister adapter callback function
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callbacks cookie.
 * @return true - on callback unregister success.
 * @return false - on callback cookie not found.
 */
bool BTSYMBOLS(bt_adapter_unregister_callback)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check LE adapter is enabled
 *
 * @param ins - bluetooth client instance.
 * @return true - enabled.
 * @return false - disabled.
 */
bool BTSYMBOLS(bt_adapter_is_le_enabled)(bt_instance_t* ins);

/**
 * @brief Check LE adapter is supported
 *
 * @param ins - bluetooth client instance.
 * @return true - support.
 * @return false - not support.
 */
bool BTSYMBOLS(bt_adapter_is_support_le)(bt_instance_t* ins);

/**
 * @brief Check LE audio adapter is supported
 *
 * @param ins - bluetooth client instance.
 * @return true - support.
 * @return false - not support.
 */
bool BTSYMBOLS(bt_adapter_is_support_leaudio)(bt_instance_t* ins);

/**
 * @brief Get LE adapter address
 *
 * @param ins - bluetooth client instance.
 * @param[out] addr - LE address.
 * @param[out] type - LE address type.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_le_address)(bt_instance_t* ins, bt_address_t* addr, ble_addr_type_t* type);

/**
 * @brief Set LE adapter private address
 *
 * @param ins - bluetooth client instance.
 * @param addr - LE address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_address)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set Le identity address
 *
 * @param ins - bluetooth client instance.
 * @param addr Le identity address
 * @param is_public - true:public, false:static
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_identity_address)(bt_instance_t* ins, bt_address_t* addr, bool is_public);

/**
 * @brief Set LE adapter io capability
 *
 * @param ins - bluetooth client instance.
 * @param le_io_cap - LE adapter io capability
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_io_capability)(bt_instance_t* ins, uint32_t le_io_cap);

/**
 * @brief Get LE adapter io capability
 *
 * @param ins - bluetooth client instance.
 * @return uint32_t - LE adapter io capability
 */
uint32_t BTSYMBOLS(bt_adapter_get_le_io_capability)(bt_instance_t* ins);

/**
 * @brief Set Le adapter appearance
 *
 * @param ins - bluetooth client instance.
 * @param appearance - le appearance, zero is invalid.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_appearance)(bt_instance_t* ins, uint16_t appearance);

/**
 * @brief Get Le adapter appearance
 *
 * @param ins - bluetooth client instance.
 * @return uint16_t - le appearance, zero on adapter not enabled.
 */
uint16_t BTSYMBOLS(bt_adapter_get_le_appearance)(bt_instance_t* ins);

/**
 * @brief Enable/Disable cross transport key derivation.
 *
 * @param ins - bluetooth client instance.
 * @param brkey_to_lekey - Enable or disable generating LE LTK from BR link key.
 * @param lekey_to_brkey - Enable or disable generating BR link key from LE LTK.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_le_enable_key_derivation)(bt_instance_t* ins,
    bool brkey_to_lekey,
    bool lekey_to_brkey);

bt_status_t BTSYMBOLS(bt_adapter_le_add_whitelist)(bt_instance_t* ins, bt_address_t* addr);

bt_status_t BTSYMBOLS(bt_adapter_le_remove_whitelist)(bt_instance_t* ins, bt_address_t* addr);

bt_status_t BTSYMBOLS(bt_adapter_set_afh_channel_classification)(bt_instance_t* ins, uint16_t central_frequency,
    uint16_t band_width, uint16_t number);

bt_status_t BTSYMBOLS(bt_adapter_set_auto_sniff)(bt_instance_t* ins, bt_auto_sniff_params_t* params);
#ifdef __cplusplus
}
#endif

#endif
