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
 * @cond
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
 * @endcond
 */

/**
 * @brief Adapter State Changed Callback.
 *
 * State Transition Diagram:
 * 
 *     +---------------------------+
 *     |   BT_ADAPTER_STATE_OFF     |
 *     +---------------------------+
 *             |
 *        turn_on |
 *             v
 *     +-------------------------------+
 *     | BT_ADAPTER_STATE_BLE_TURNING_ON |
 *     +-------------------------------+
 *             |
 *        turn_on |
 *             v
 *     +---------------------------+
 *     |   BT_ADAPTER_STATE_BLE_ON  |
 *     +---------------------------+
 *             |
 *        turn_on |
 *             v
 *     +---------------------------+
 *     | BT_ADAPTER_STATE_TURNING_ON |
 *     +---------------------------+
 *             |
 *        turn_on |
 *             v
 *     +---------------------------+
 *     |     BT_ADAPTER_STATE_ON    |
 *     +---------------------------+
 *             |
 *      turn_off |
 *             v
 *     +---------------------------+
 *     |  BT_ADAPTER_STATE_TURNING_OFF |
 *     +---------------------------+
 *             |
 *      turn_off |
 *             v
 *     +-------------------------------+
 *     | BT_ADAPTER_STATE_BLE_TURNING_OFF |
 *     +-------------------------------+
 *             |
 *        turn_off |
 *             v
 *     +---------------------------+
 *     |   BT_ADAPTER_STATE_OFF     |
 *     +---------------------------+
 * 
 * State Descriptions:
 * - `BT_ADAPTER_STATE_OFF`: The initial state. The adapter is off.
 * - `BT_ADAPTER_STATE_BLE_TURNING_ON`: BLE is in the process of being turned on.
 * - `BT_ADAPTER_STATE_BLE_ON`: BLE is fully on.
 * - `BT_ADAPTER_STATE_TURNING_ON`: The Bluetooth adapter is turning on.
 * - `BT_ADAPTER_STATE_ON`: The Bluetooth adapter is fully on.
 * - `BT_ADAPTER_STATE_TURNING_OFF`: The Bluetooth adapter is turning off.
 * - `BT_ADAPTER_STATE_BLE_TURNING_OFF`: BLE is turning off.
 * 
 * Callback invoked when the Bluetooth adapter state changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * 
 * @param state - The new state of the Bluetooth adapter, as defined in @ref bt_adapter_state_t.
 *
 * **Example:**
 * @code
void on_adapter_state_changed(void* cookie, bt_adapter_state_t state)
{
#ifdef CONFIG_BLUETOOTH_FEATURE
    bt_instance_t* ins = (bt_instance_t*)cookie;
    printf("bt_instance: %p\n", ins);
#else
    printf("Context:%p, Adapter state changed: %d\n", cookie, state);
#endif
}

const adapter_callbacks_t g_adapter_cbs = {
    .on_adapter_state_changed = on_adapter_state_changed,
};

int main(int argc, char** argv)
{
    bt_instance_t* ins = NULL;
    void* adapter_callback = NULL;

    ins = bluetooth_create_instance();
    if (ins == NULL) {
        printf("create instance error\n");
        return -1;
    }
    printf("create instance success\n");
    adapter_callback = bt_adapter_register_callback(ins, &g_adapter_cbs);
}
 * @endcode
 */
typedef void (*on_adapter_state_changed_callback)(void* cookie, bt_adapter_state_t state);

/**
 * @brief Adapter discovery state changed callback.
 *
 * Callback function invoked when the discovery state changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param state - Discovery state, see @ref bt_discovery_state_t (0: stopped, 1: started).
 *
 * **Example:**
 * @code
void on_discovery_state_changed(void* cookie, bt_discovery_state_t state)
{
    printf("Discovery state: %s\n", state == BT_DISCOVERY_STATE_STARTED ? "Started" : "Stopped");
}

const adapter_callbacks_t g_adapter_cbs = {
    .on_discovery_state_changed = on_discovery_state_changed,
};

int main(int argc, char** argv)
{
    bt_instance_t* ins = NULL;
    void* adapter_callback = NULL;

    ins = bluetooth_create_instance();
    if (ins == NULL) {
        printf("create instance error\n");
        return -1;
    }
    printf("create instance success\n");
    adapter_callback = bt_adapter_register_callback(ins, &g_adapter_cbs);
}
 * @endcode
 */
typedef void (*on_discovery_state_changed_callback)(void* cookie, bt_discovery_state_t state);

/**
 * @brief Discovery result callback.
 *
 * Callback function invoked when a remote device is discovered.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param remote - Pointer to remote device information, see @ref bt_discovery_result_t.
 *
 * **Example:**
 * @code
void on_discovery_result(void* cookie, bt_discovery_result_t* result)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(&result->addr, addr_str);
    printf("Inquiring: device [%s], name: %s, cod: %08" PRIx32 ", rssi: %d\n", addr_str, result->name, result->cod, result->rssi);
}
 * @endcode
 */
typedef void (*on_discovery_result_callback)(void* cookie, bt_discovery_result_t* remote);

/**
 * @brief Scan mode changed callback.
 *
 * Callback function invoked when the scan mode changes.
 *
  * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param mode - New scan mode, see @ref bt_scan_mode_t.
 *
 * **Example:**
 * @code
void on_scan_mode_changed(void* cookie, bt_scan_mode_t mode)
{
    printf("Adapter new scan mode: %d\n", mode);
}
 * @endcode
 */
typedef void (*on_scan_mode_changed_callback)(void* cookie, bt_scan_mode_t mode);

/**
 * @brief Local device name changed callback.
 *
 * Callback function invoked when the local device name changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param device_name - New local device name.
 *
 * **Example:**
 * @code
void on_device_name_changed(void* cookie, const char* device_name)
{
    printf("Adapter update device name: %s\n", device_name);
}
 * @endcode
 */
typedef void (*on_device_name_changed_callback)(void* cookie, const char* device_name);

/**
 * @brief Pair request callback (IO capability request).
 *
 * Callback function invoked when a pair request is received.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 *
 * **Example:**
 * @code
void on_pair_request(void* cookie, bt_address_t* addr)
{
    // Handle pair request
}
 * @endcode
 */
typedef void (*on_pair_request_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Pair information display callback.
 *
 * Callback function invoked to display pairing information.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param transport - Transport type, see @ref bt_transport_t (0: LE, 1: BR/EDR).
 * @param type - Pairing type, see @ref bt_pair_type_t.
 * @param passkey - Passkey value; invalid if pairing type is PAIR_TYPE_PASSKEY_CONFIRMATION
 *                  or PAIR_TYPE_PASSKEY_NOTIFICATION.
 *
 * **Example:**
 * @code
void on_pair_display(void* cookie, bt_address_t* addr, bt_transport_t transport, bt_pair_type_t type, uint32_t passkey)
{
    // Display pairing information
}
 * @endcode
 */
typedef void (*on_pair_display_callback)(void* cookie, bt_address_t* addr, bt_transport_t transport, bt_pair_type_t type, uint32_t passkey);

/**
 * @brief Connect request callback.
 *
 * Callback function invoked when a connect request is received.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 *
 * **Example:**
 * @code
void on_connect_request(void* cookie, bt_address_t* addr)
{
    // Handle connect request
}
 * @endcode
 */
typedef void (*on_connect_request_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief Connection state changed callback.
 *
 * Callback function invoked when the ACL connection state changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param transport - Transport type, see @ref bt_transport_t (0: LE, 1: BR/EDR).
 * @param state - ACL connection state, see @ref connection_state_t.
 *
 * **Example:**
 * @code
void on_connection_state_changed(void* cookie, bt_address_t* addr, bt_transport_t transport, connection_state_t state)
{
    // Handle connection state change
}
 * @endcode
 */
typedef void (*on_connection_state_changed_callback)(void* cookie, bt_address_t* addr, bt_transport_t transport, connection_state_t state);

/**
 * @brief Bond state changed callback.
 *
 * Callback function invoked when the bond state changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param transport - Transport type, see @ref bt_transport_t (0: LE, 1: BR/EDR).
 * @param state - Bond state, see @ref bond_state_t.
 * @param is_ctkd - Whether cross-transport key derivation is used.
 *
 * **Example:**
 * @code
void on_bond_state_changed(void* cookie, bt_address_t* addr, bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    // Handle bond state change
}
 * @endcode
 */
typedef void (*on_bond_state_changed_callback)(void* cookie, bt_address_t* addr, bt_transport_t transport, bond_state_t state, bool is_ctkd);

/**
 * @brief Got local OOB data for LE secure connection pairing callback.
 *
 * Callback function invoked when local OOB data for LE Secure Connections is generated.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param c_val - LE Secure Connections confirmation value.
 * @param r_val - LE Secure Connections random value.
 *
 * **Example:**
 * @code
void on_le_sc_local_oob_data_got(void* cookie, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    // Handle OOB data
}
 * @endcode
 */
typedef void (*on_le_sc_local_oob_data_got_callback)(void* cookie, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val);

/**
 * @brief Remote device name changed callback.
 *
 * Callback function invoked when the remote device's name changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param name - New name of the remote device.
 *
 * **Example:**
 * @code
void on_remote_name_changed(void* cookie, bt_address_t* addr, const char* name)
{
    // Handle remote name change
}
 * @endcode
 */
typedef void (*on_remote_name_changed_callback)(void* cookie, bt_address_t* addr, const char* name);

/**
 * @brief Remote device alias changed callback.
 *
 * Callback function invoked when the remote device's alias changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param alias - New alias set by the user.
 *
 * **Example:**
 * @code
void on_remote_alias_changed(void* cookie, bt_address_t* addr, const char* alias)
{
    // Handle remote alias change
}
 * @endcode
 */
typedef void (*on_remote_alias_changed_callback)(void* cookie, bt_address_t* addr, const char* alias);

/**
 * @brief Remote device class changed callback.
 *
 * Callback function invoked when the remote device's class changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param cod - Class of Device (CoD) of the remote device.
 *
 * **Example:**
 * @code
void on_remote_cod_changed(void* cookie, bt_address_t* addr, uint32_t cod)
{
    // Handle remote class of device change
}
 * @endcode
 */
typedef void (*on_remote_cod_changed_callback)(void* cookie, bt_address_t* addr, uint32_t cod);

/**
 * @brief Remote device UUIDs changed callback.
 *
 * Callback function invoked when the remote device's supported UUIDs change.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param uuids - Array of supported UUIDs, see @ref bt_uuid_t.
 * @param size - Number of UUIDs in the array.
 *
 * **Example:**
 * @code
void on_remote_uuids_changed(void* cookie, bt_address_t* addr, bt_uuid_t* uuids, uint16_t size)
{
    // Handle remote UUIDs change
}
 * @endcode
 */
typedef void (*on_remote_uuids_changed_callback)(void* cookie, bt_address_t* addr, bt_uuid_t* uuids, uint16_t size);

/**
 * @brief Remote device link mode changed callback.
 *
 * Callback function invoked when the link mode with the remote device changes.
 *
 * @param cookie - User-defined context:
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is enabled, it's a `bt_instance_t*`.
 *                 - If `CONFIG_BLUETOOTH_FEATURE` is disabled, it's a dynamically allocated `remote_callback_t*`.
 *                 See `bt_adapter_register_callback` and `bt_remote_callbacks_register` for details.
 * @param addr - Address of the remote device, see @ref bt_address_t.
 * @param mode - Link mode, see @ref bt_link_mode_t.
 * @param sniff_interval - Sniff interval if in sniff mode.
 *
 * **Example:**
 * @code
void on_remote_link_mode_changed(void* cookie, bt_address_t* addr, bt_link_mode_t mode, uint16_t sniff_interval)
{
    // Handle link mode change
}
 * @endcode
 */
typedef void (*on_remote_link_mode_changed_callback)(void* cookie, bt_address_t* addr, bt_link_mode_t mode, uint16_t sniff_interval);

/**
 * @cond
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
 * @endcond
 */

/**
 * @brief Enable the Bluetooth adapter.
 *
 * Turns on the Bluetooth adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_adapter_enable(ins) == BT_STATUS_SUCCESS) {
    // Adapter enabled successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_adapter_enable)(bt_instance_t* ins);

/**
 * @brief Disable the Bluetooth adapter.
 *
 * Turns off the Bluetooth adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_adapter_disable(ins) == BT_STATUS_SUCCESS) {
    // Adapter disabled successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_adapter_disable)(bt_instance_t* ins);

/**
 * @brief Enable BLE (Bluetooth Low Energy).
 *
 * Turns on the BLE functionality of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_enable_le)(bt_instance_t* ins);

/**
 * @brief Disable BLE (Bluetooth Low Energy).
 *
 * Turns off the BLE functionality of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_disable_le)(bt_instance_t* ins);

/**
 * @brief Get the current adapter state.
 *
 * Retrieves the current state of the Bluetooth adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_adapter_state_t - Current adapter state, see @ref bt_adapter_state_t.
 */
bt_adapter_state_t BTSYMBOLS(bt_adapter_get_state)(bt_instance_t* ins);

/**
 * @brief Get the adapter device type.
 *
 * Retrieves the type of the Bluetooth adapter (e.g., BR/EDR, LE, Dual Mode).
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_device_type_t - Device type (0: BR/EDR, 1: LE, 2: Dual Mode, 0xFF: Unknown).
 */
bt_device_type_t BTSYMBOLS(bt_adapter_get_type)(bt_instance_t* ins);

/**
 * @brief Set the discovery filter.
 *
 * @note Not supported currently.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_status_t - BT_STATUS_UNSUPPORTED.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_discovery_filter)(bt_instance_t* ins);

/**
 * @brief Start device discovery.
 *
 * Initiates the device discovery process to find nearby Bluetooth devices.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param timeout - Maximum amount of time to perform discovery (Time = N * 1.28s, Range: 1.28s to 61.44s).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_adapter_start_discovery(ins, 10) == BT_STATUS_SUCCESS) {
    // Discovery started successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_adapter_start_discovery)(bt_instance_t* ins, uint32_t timeout);

/**
 * @brief Cancel device discovery.
 *
 * Stops an ongoing device discovery process.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_adapter_cancel_discovery(ins) == BT_STATUS_SUCCESS) {
    // Discovery cancelled successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_adapter_cancel_discovery)(bt_instance_t* ins);

/**
 * @brief Check if the adapter is discovering.
 *
 * Determines whether the adapter is currently performing device discovery.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return true - Adapter is discovering.
 * @return false - Adapter is not discovering.
 *
 * **Example:**
 * @code
if (bt_adapter_is_discovering(ins)) {
    // Adapter is discovering
} else {
    // Adapter is not discovering
}
 * @endcode
 */
bool BTSYMBOLS(bt_adapter_is_discovering)(bt_instance_t* ins);

/**
 * @brief Read the Bluetooth controller address (BD_ADDR).
 *
 * Retrieves the Bluetooth address of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param[out] addr - Pointer to store the BD_ADDR; empty if adapter is not enabled.
 *
 * **Example:**
 * @code
bt_address_t addr;
bt_adapter_get_address(ins, &addr);
// Use addr as needed
 * @endcode
 */
void BTSYMBOLS(bt_adapter_get_address)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set the adapter's local name.
 *
 * Sets the user-friendly name of the Bluetooth adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param name - New local name for the adapter.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_adapter_set_name(ins, "My Bluetooth Device") == BT_STATUS_SUCCESS) {
    // Name set successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_adapter_set_name)(bt_instance_t* ins, const char* name);

/**
 * @brief Get the adapter's local name.
 *
 * Retrieves the user-friendly name of the Bluetooth adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param[out] name - Buffer to store the local name.
 * @param[in] length - Maximum length of the name buffer.
 *
 * **Example:**
 * @code
char name[248];
bt_adapter_get_name(ins, name, sizeof(name));
// Use name as needed
 * @endcode
 */
void BTSYMBOLS(bt_adapter_get_name)(bt_instance_t* ins, char* name, int length);

/**
 * @brief Get adapter supported UUIDs.
 *
 * @note Not supported currently.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param[out] uuids - Array to store UUIDs.
 * @param[out] size - Number of UUIDs retrieved.
 * @return bt_status_t - BT_STATUS_UNSUPPORTED.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_uuids)(bt_instance_t* ins, bt_uuid_t* uuids, uint16_t* size);

/**
 * @brief Set the adapter's scan mode.
 *
 * Configures the scan mode of the adapter (e.g., discoverable, connectable).
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param mode - Scan mode, see @ref bt_scan_mode_t (0: none, 1: connectable, 2: connectable_discoverable).
 * @param bondable - Whether the adapter is bondable.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_adapter_set_scan_mode(ins, BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE, true) == BT_STATUS_SUCCESS) {
    // Scan mode set successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_adapter_set_scan_mode)(bt_instance_t* ins, bt_scan_mode_t mode, bool bondable);

/**
 * @brief Get the adapter's scan mode.
 *
 * Retrieves the current scan mode of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_scan_mode_t - Current scan mode, see @ref bt_scan_mode_t.
 *
 * **Example:**
 * @code
bt_scan_mode_t mode = bt_adapter_get_scan_mode(ins);
// Use mode as needed
 * @endcode
 */
bt_scan_mode_t BTSYMBOLS(bt_adapter_get_scan_mode)(bt_instance_t* ins);

/**
 * @brief Set the adapter's device class.
 *
 * Sets the Class of Device (CoD) for the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param cod - Class of Device; zero is invalid.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_adapter_set_device_class(ins, 0x200404) == BT_STATUS_SUCCESS) {
    // Device class set successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_adapter_set_device_class)(bt_instance_t* ins, uint32_t cod);

/**
 * @brief Get the adapter's device class.
 *
 * Retrieves the Class of Device (CoD) of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return uint32_t - Class of Device; zero if adapter is not enabled.
 *
 * **Example:**
 * @code
uint32_t cod = bt_adapter_get_device_class(ins);
// Use cod as needed
 * @endcode
 */
uint32_t BTSYMBOLS(bt_adapter_get_device_class)(bt_instance_t* ins);

/**
 * @brief Set the BR/EDR adapter IO capability.
 *
 * Sets the Input/Output capability of the BR/EDR adapter for pairing.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param cap - IO capability, see @ref bt_io_capability_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_io_capability)(bt_instance_t* ins, bt_io_capability_t cap);

/**
 * @brief Get the BR/EDR adapter IO capability.
 *
 * Retrieves the Input/Output capability of the BR/EDR adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return bt_io_capability_t - Current IO capability, see @ref bt_io_capability_t.
 */
bt_io_capability_t BTSYMBOLS(bt_adapter_get_io_capability)(bt_instance_t* ins);

/**
 * @brief Set inquiry scan parameters.
 *
 * Configures the inquiry scan parameters for the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param type - Scan type, see @ref bt_scan_type_t.
 * @param interval - Scan interval (in slots, 0x0012 to 0x1000).
 * @param window - Scan window (in slots, 0x0011 to 0x1000).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_inquiry_scan_parameters)(bt_instance_t* ins, bt_scan_type_t type,
    uint16_t interval, uint16_t window);

/**
 * @brief Set page scan parameters.
 *
 * Configures the page scan parameters for the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param type - Scan type, see @ref bt_scan_type_t.
 * @param interval - Scan interval (in slots, 0x0012 to 0x1000).
 * @param window - Scan window (in slots, 0x0011 to 0x1000).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_page_scan_parameters)(bt_instance_t* ins, bt_scan_type_t type,
    uint16_t interval, uint16_t window);

/**
 * @brief Get the list of bonded devices.
 *
 * Retrieves the list of devices that are bonded (paired) with the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param transport - Transport type, see @ref bt_transport_t.
 * @param[out] addr - Pointer to an array of device addresses; allocated using the provided allocator.
 * @param[out] num - Number of bonded devices.
 * @param allocator - Allocator function to allocate memory for the address array.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_bonded_devices)(bt_instance_t* ins, bt_transport_t transport, bt_address_t** addr, int* num, bt_allocator_t allocator);

/**
 * @brief Get the list of connected devices.
 *
 * Retrieves the list of devices that are currently connected to the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param transport - Transport type, see @ref bt_transport_t.
 * @param[out] addr - Pointer to an array of device addresses; allocated using the provided allocator.
 * @param[out] num - Number of connected devices.
 * @param allocator - Allocator function to allocate memory for the address array.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_connected_devices)(bt_instance_t* ins, bt_transport_t transport, bt_address_t** addr, int* num, bt_allocator_t allocator);

/**
 * @brief Disconnect all connected devices.
 *
 * @note Not supported currently.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 */
void BTSYMBOLS(bt_adapter_disconnect_all_devices)(bt_instance_t* ins);

/**
 * @brief Check if BR/EDR is supported.
 *
 * Determines whether BR/EDR (Basic Rate/Enhanced Data Rate) is supported by the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return true - BR/EDR is supported.
 * @return false - BR/EDR is not supported.
 */
bool BTSYMBOLS(bt_adapter_is_support_bredr)(bt_instance_t* ins);

/**
 * @brief Register callback functions with the adapter service.
 *
 * Registers application callbacks to receive adapter events.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param callbacks - Pointer to the adapter callbacks structure, see @ref adapter_callbacks_t.
 * @return void* - Callback cookie to be used in future calls; NULL on failure.
 *
 * **Example:**
 * @code
void* cookie = bt_adapter_register_callback(ins, &my_adapter_callbacks);
if (cookie == NULL) {
    // Handle error
}
 * @endcode
 */
void* BTSYMBOLS(bt_adapter_register_callback)(bt_instance_t* ins, const adapter_callbacks_t* adapter_cbs);

/**
 * @brief Unregister adapter callback functions.
 *
 * Unregisters the application callbacks from the adapter service.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param cookie - Callback cookie obtained from registration.
 * @return true - Unregistration successful.
 * @return false - Callback cookie not found or unregistration failed.
 *
 * **Example:**
 * @code
if (bt_adapter_unregister_callback(ins, cookie)) {
    // Unregistered successfully
} else {
    // Handle error
}
 * @endcode
 */
bool BTSYMBOLS(bt_adapter_unregister_callback)(bt_instance_t* ins, void* cookie);

/**
 * @brief Check if BLE is enabled.
 *
 * Determines whether BLE functionality is currently enabled.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return true - BLE is enabled.
 * @return false - BLE is disabled.
 */
bool BTSYMBOLS(bt_adapter_is_le_enabled)(bt_instance_t* ins);

/**
 * @brief Check if BLE is supported.
 *
 * Determines whether BLE functionality is supported by the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return true - BLE is supported.
 * @return false - BLE is not supported.
 */
bool BTSYMBOLS(bt_adapter_is_support_le)(bt_instance_t* ins);

/**
 * @brief Check if LE Audio is supported.
 *
 * Determines whether LE Audio functionality is supported by the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return true - LE Audio is supported.
 * @return false - LE Audio is not supported.
 */
bool BTSYMBOLS(bt_adapter_is_support_leaudio)(bt_instance_t* ins);

/**
 * @brief Get the BLE adapter address.
 *
 * Retrieves the BLE address and address type of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param[out] addr - Pointer to store the BLE address.
 * @param[out] type - Pointer to store the BLE address type, see @ref ble_addr_type_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_get_le_address)(bt_instance_t* ins, bt_address_t* addr, ble_addr_type_t* type);

/**
 * @brief Set the BLE private address.
 *
 * Sets the BLE private address of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Pointer to the new BLE address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_address)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set the BLE identity address.
 *
 * Sets the BLE identity address of the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Pointer to the BLE identity address.
 * @param is_public - true if the address is public; false if static.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_identity_address)(bt_instance_t* ins, bt_address_t* addr, bool is_public);

/**
 * @brief Set the BLE adapter IO capability.
 *
 * Sets the Input/Output capability of the BLE adapter for pairing.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param le_io_cap - IO capability value.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_io_capability)(bt_instance_t* ins, uint32_t le_io_cap);

/**
 * @brief Get the BLE adapter IO capability.
 *
 * Retrieves the Input/Output capability of the BLE adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return uint32_t - Current IO capability value.
 */
uint32_t BTSYMBOLS(bt_adapter_get_le_io_capability)(bt_instance_t* ins);

/**
 * @brief Set the BLE adapter appearance.
 *
 * Sets the GAP appearance value of the BLE adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param appearance - GAP appearance value; zero is invalid.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_le_appearance)(bt_instance_t* ins, uint16_t appearance);

/**
 * @brief Get the BLE adapter appearance.
 *
 * Retrieves the GAP appearance value of the BLE adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @return uint16_t - GAP appearance value; zero if adapter is not enabled.
 */
uint16_t BTSYMBOLS(bt_adapter_get_le_appearance)(bt_instance_t* ins);

/**
 * @brief Enable or disable cross-transport key derivation.
 *
 * Configures the adapter to enable or disable cross-transport key derivation (CTKD).
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param brkey_to_lekey - Enable generating LE LTK from BR/EDR link key.
 * @param lekey_to_brkey - Enable generating BR/EDR link key from LE LTK.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_le_enable_key_derivation)(bt_instance_t* ins,
    bool brkey_to_lekey,
    bool lekey_to_brkey);

/**
 * @brief Add a device to the BLE whitelist.
 *
 * Adds a device address to the BLE whitelist.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the device to add, see @ref bt_address_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_le_add_whitelist)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Remove a device from the BLE whitelist.
 *
 * Removes a device address from the BLE whitelist.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the device to remove, see @ref bt_address_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_le_remove_whitelist)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set AFH (Adaptive Frequency Hopping) channel classification.
 *
 * Configures the AFH channel classification for the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param central_frequency - Central frequency in MHz.
 * @param band_width - Bandwidth in MHz.
 * @param number - Number of channels.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_afh_channel_classification)(bt_instance_t* ins, uint16_t central_frequency,
    uint16_t band_width, uint16_t number);

/**
 * @brief Set automatic sniff mode parameters.
 *
 * Configures automatic sniff mode parameters for the adapter.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param params - Pointer to auto sniff parameters, see @ref bt_auto_sniff_params_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_adapter_set_auto_sniff)(bt_instance_t* ins, bt_auto_sniff_params_t* params);

#ifdef __cplusplus
}
#endif

#endif /* __BT_ADAPTER_H__ */
