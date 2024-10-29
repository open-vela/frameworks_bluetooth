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
#ifndef _BT_DEVICE_H__
#define _BT_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_status.h"
#include <stdint.h>

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @cond
 */

/**
 * @brief Profile connection state
 *
 */
typedef enum {
    PROFILE_STATE_DISCONNECTED,
    PROFILE_STATE_CONNECTING,
    PROFILE_STATE_CONNECTED,
    PROFILE_STATE_DISCONNECTING,
} profile_connection_state_t;

/**
 * @brief Profile connection reason
 *
 */
typedef enum {
    PROFILE_REASON_SUCCESS = 0x0000,
    PROFILE_REASON_COLLISION = 0x0001,

    PROFILE_REASON_UNSPECIFIED = 0xFFFF,
} profile_connection_reason_t;

/**
 * @brief ACL connection state
 *
 */
typedef enum {
    CONNECTION_STATE_DISCONNECTED,
    CONNECTION_STATE_CONNECTING,
    CONNECTION_STATE_DISCONNECTING,
    CONNECTION_STATE_CONNECTED,
    CONNECTION_STATE_ENCRYPTED_BREDR,
    CONNECTION_STATE_ENCRYPTED_LE
} connection_state_t;

/**
 * @brief Bond state
 *
 */
typedef enum {
    BOND_STATE_NONE,
    BOND_STATE_BONDING,
    BOND_STATE_BONDED,
    BOND_STATE_CANCELING
} bond_state_t;

/**
 * @brief bluetooth connection policy
 *
 */
typedef enum {
    CONNECTION_POLICY_ALLOWED,
    CONNECTION_POLICY_FORBIDDEN,
    CONNECTION_POLICY_UNKNOWN,
} connection_policy_t;

/**
 * @endcond
 */

/**
 * @brief Get the BLE Identity Address of a remote device.
 *
 * Retrieves the BLE Identity Address (`id_addr`) of a remote device. The Identity Address is a fixed
 * BLE address used to identify the device, distinct from the current BLE address (`bd_addr`) when 
 * privacy features such as Resolvable Private Address (RPA) are enabled.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param bd_addr - Current BLE address of the remote device:
 *                  - Public Device Address
 *                  - Random Device Address (Static or Resolvable Private Address)
 * @param[out] id_addr - Pointer to store the Identity Address, which will be one of:
 *                       - Public Device Address
 *                       - Static Random Address
 * 
 * @return bt_status_t
 *         - `BT_STATUS_SUCCESS`: Successfully retrieved the Identity Address.
 *         - Negative error code: Operation failed (e.g., invalid address or device not found).
 *
 * @note **Difference Between `bd_addr` and `id_addr`:**
 *       - **`bd_addr`**: The device's current BLE connection address, which may change if privacy 
 *         features such as RPA are used. It is used for ongoing communication.
 *       - **`id_addr`**: The stable Identity Address, which will always be one of:
 *         - **Public Device Address**: Globally unique and assigned by the manufacturer.
 *         - **Static Random Address**: Fixed and randomly generated, persistent across power cycles.
 *
 * @note **Bluetooth Address Details:**
 *       - **Public Device Address**: Globally unique address assigned by the manufacturer, also used 
 *         as BD_ADDR for BR/EDR devices.
 *       - **Random Device Address**: Includes:
 *         - **Static Address**: Fixed random address when privacy is not enabled.
 *         - **Resolvable Private Address (RPA)**: Temporary address used for privacy, resolved to 
 *           the Identity Address using the IRK (Identity Resolving Key).
 *       - **Identity Address**: A fixed address used to identify the device.
 *
 *       The Identity Address is typically obtained during pairing and stored for future use.
 *
 * **Example:**
 * @code
bt_address_t bd_addr; // Current BLE connection address
bt_address_t id_addr; // Identity Address to retrieve
if (bt_device_get_identity_address(ins, &bd_addr, &id_addr) == BT_STATUS_SUCCESS) {
    // Successfully retrieved the Identity Address
} else {
    // Handle failure
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_get_identity_address)(bt_instance_t* ins, bt_address_t* bd_addr, bt_address_t* id_addr);

/**
 * @brief Get the BLE address type of a remote device.
 *
 * Retrieves the BLE address type, indicating whether it is Public, Static Random, RPA, 
 * or other specific types.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return ble_addr_type_t
 *         - The BLE address type of the device.
 *         - Returns `BLE_ADDR_TYPE_UNKNOWN` if the device is not found.
 *
 * @note **Address Types:**
 *       - **BT_LE_ADDR_TYPE_PUBLIC**: Public address, globally unique and unchanging.
 *       - **BT_LE_ADDR_TYPE_RANDOM**: Random address, used during connections (e.g., RPA or static random).  
 *         When used locally (e.g., in advertising), this indicates a static random address 
 *         set via `bt_adapter_set_le_address`.
 *       - **BT_LE_ADDR_TYPE_PUBLIC_ID**: Public identity address, using public address for identification 
 *         even if a static random address is set.
 *       - **BT_LE_ADDR_TYPE_RANDOM_ID**: Random identity address (e.g., RPA), used when 
 *         the privacy feature is enabled.
 *       - **BT_LE_ADDR_TYPE_ANONYMOUS**: Anonymous address, often used with Accept/White Lists.
 *       - **BT_LE_ADDR_TYPE_UNKNOWN**: The address type cannot be determined.
 *
 * **Example:**
 * @code
ble_addr_type_t addr_type = bt_device_get_address_type(ins, &addr);
if (addr_type != BLE_ADDR_TYPE_UNKNOWN) {
    // Handle the specific address type
    if (addr_type == BLE_ADDR_TYPE_PUBLIC) {
        // Process public address
    }
} else {
    // Device not found
}
 * @endcode
 */
ble_addr_type_t BTSYMBOLS(bt_device_get_address_type)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get device type of a remote device.
 *
 * Retrieves the device type (e.g., BR/EDR, LE, Dual Mode) of a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return bt_device_type_t - Device type; zero if the device is not found.
 *
 * **Example:**
 * @code
bt_device_type_t device_type = bt_device_get_device_type(ins, &addr);
if (device_type != 0) {
    // Use device_type as needed
} else {
    // Handle device not found
}
 * @endcode
 */
bt_device_type_t BTSYMBOLS(bt_device_get_device_type)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Retrieve the name of a remote device.
 *
 * Retrieves the user-friendly name of a remote Bluetooth device in UTF-8 encoding.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param[out] name - Buffer to store the device name. The name is UTF-8 encoded.
 *                    - According to the Bluetooth specification, the buffer size for the device name must not exceed 248 bytes.
 *                    - On the Vela platform, the maximum allowed name length is defined as `BT_DEV_NAME_MAX_LEN`.
 *                      To handle the null terminator properly, the buffer should be sized at `BT_DEV_NAME_MAX_LEN + 1`,
 *                      where the last byte ensures the string is null-terminated.
 * @param length - Size of the name buffer.
 * @return true - The device name was successfully retrieved.
 * @return false - The device was not found or the name could not be retrieved.
 *
 * @note **Buffer Requirements:**
 *       - Ensure the `name` buffer is large enough to store the device name, including space for the null terminator.
 *       - On the Vela platform:
 *         - The maximum effective name length is `BT_DEV_NAME_MAX_LEN`.
 *         - Allocate `BT_DEV_NAME_MAX_LEN + 1` bytes to account for the null terminator and avoid buffer overflow issues.
 *       - According to the Bluetooth specification, the buffer size must not exceed 248 bytes.
 *
 * **Example:**
 * @code
char name[BT_DEV_NAME_MAX_LEN + 1]; // Allocate buffer for the name and null terminator.
if (bt_device_get_name(ins, &addr, name, sizeof(name))) {
    // Use name as needed
} else {
    // Handle device not found
}
 * @endcode
 */
bool BTSYMBOLS(bt_device_get_name)(bt_instance_t* ins, bt_address_t* addr, char* name, uint32_t length);

/**
 * @brief Get the Class of Device (CoD) of a remote device.
 *
 * Retrieves the Class of Device (CoD) value of a remote device. 
 * The Class of Device is a parameter received during the device discovery procedure 
 * on the BR/EDR physical transport, indicating the type of device. 
 * The Class of Device parameter is only used on BR/EDR and BR/EDR/LE devices 
 * using the BR/EDR physical transport.
 *
 * - The CoD parameter consists of:
 *   - **Major Device Class**: Represents the primary category of the device (e.g., computer, phone, audio).
 *   - **Minor Device Class**: Provides a more specific classification (e.g., headset, smartphone).
 *   - **Service Class**: Indicates supported services (e.g., telephony, audio streaming).
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return uint32_t - CoD value; zero if the device is not found.
 *
 * **Example:**
 * @code
uint32_t cod = bt_device_get_device_class(ins, &addr);
if (cod != 0) {
    // Use cod as needed
} else {
    // Handle device not found
}
 * @endcode
 */
uint32_t BTSYMBOLS(bt_device_get_device_class)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get the list of supported UUIDs of a remote device.
 *
 * Retrieves the list of Universally Unique Identifiers (UUIDs) supported by a remote device. 
 * A UUID is a universally unique identifier that is expected to be unique across all 
 * space and time (more precisely, the probability of independently-generated UUIDs 
 * being the same is negligible). Normally, a client searches for services based on 
 * specific desired characteristics, each represented by a UUID.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param[out] uuids - Pointer to an array of UUIDs; memory is allocated using the provided allocator.
 * @param[out] size - Number of UUIDs retrieved.
 * @param allocator - Allocator function used to allocate memory for the UUID array.
 * @return bt_status_t
 *         - `BT_STATUS_SUCCESS`: UUIDs successfully retrieved.
 *         - Negative error code: Operation failed.
 *
 * **Example:**
 * @code
bt_uuid_t* uuids = NULL;
uint16_t size = 0;
if (bt_device_get_uuids(ins, &addr, &uuids, &size, my_allocator) == BT_STATUS_SUCCESS) {
    // Use the UUID array as needed
    // Free memory using the allocator's deallocation function
} else {
    // Handle the error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_get_uuids)(bt_instance_t* ins, bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator);

/**
 * @brief Get the BLE appearance of a remote device.
 *
 * Retrieves the Appearance characteristic of a remote device. The Appearance 
 * characteristic contains a 16-bit number that can be mapped to an icon or string 
 * that describes the physical representation of the device during the device discovery 
 * procedure. It is a characteristic of the GAP service located on the device’s GATT Server.
 *
 * @note Currently not supported.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return uint16_t - Appearance value; zero if the device is not found or the operation is unsupported.
 */
uint16_t BTSYMBOLS(bt_device_get_appearance)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get the RSSI (Received Signal Strength Indication) of a remote device.
 *
 * @note This API is applicable only availble for BR/EDR connected devices.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return int8_t - RSSI value.
 */
int8_t BTSYMBOLS(bt_device_get_rssi)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get the alias of a remote device.
 *
 * Retrieves the alias (user-defined name) of a remote device. If an alias is not set, the device name is used.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param[out] alias - Buffer to store the alias.
 * @param length -length of the alias buffer, the alias buffer size shall be no smaller than 64.
 * @return true - Success.
 * @return false - Device not found.
 *
 * **Example:**
 * @code
char alias[64]; // The alias buffer size shall be no smaller than 64
if (bt_device_get_alias(ins, &addr, alias, sizeof(alias))) {
    // Use alias as needed
} else {
    // Handle device not found
}
 * @endcode
 */
bool BTSYMBOLS(bt_device_get_alias)(bt_instance_t* ins, bt_address_t* addr, char* alias, uint32_t length);

/**
 * @brief Set the alias of a remote device.
 *
 * Assigns an alias (user-defined name) to a remote device.
 * The length of the alias name shall be less than BT_LOC_NAME_MAX_LEN.
 * 
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param alias - New alias for the device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_set_alias(ins, &addr, "My Device") == BT_STATUS_SUCCESS) {
    // Alias set successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_set_alias)(bt_instance_t* ins, bt_address_t* addr, const char* alias);

/**
 * @brief Check if a remote device is connected.
 *
 * Determines whether a remote device is currently connected.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type, see @ref bt_transport_t.
 * @return true - Device is connected.
 * @return false - Device is not connected.
 *
 * **Example:**
 * @code
if (bt_device_is_connected(ins, &addr, BT_TRANSPORT_BR_EDR)) {
    // Device is connected
} else {
    // Device is not connected
}
 * @endcode
 */
bool BTSYMBOLS(bt_device_is_connected)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport);

/**
 * @brief Check if a remote device connection is encrypted.
 *
 * Determines whether the connection to a remote device is encrypted.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type, see @ref bt_transport_t.
 * @return true - Connection is encrypted.
 * @return false - Connection is not encrypted.
 *
 * **Example:**
 * @code
if (bt_device_is_encrypted(ins, &addr, BT_TRANSPORT_LE)) {
    // Connection is encrypted
} else {
    // Connection is not encrypted
}
 * @endcode
 */
bool BTSYMBOLS(bt_device_is_encrypted)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport);

/**
 * @brief Check if bonding was initiated from the local device.
 *
 * Determines whether the bonding process with a remote device was initiated locally.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type, see @ref bt_transport_t.
 * @return true - Bonding initiated from local device.
 * @return false - Bonding initiated from remote device.
 *
 * **Example:**
 * @code
if (bt_device_is_bond_initiate_local(ins, &addr, BT_TRANSPORT_LE)) {
    // Bonding initiated locally
} else {
    // Bonding initiated remotely
}
 * @endcode
 */
bool BTSYMBOLS(bt_device_is_bond_initiate_local)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport);

/**
 * @brief Get the bond state with a remote device.
 *
 * Retrieves the current bonding state with a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type, see @ref bt_transport_t.
 * @return bond_state_t - Current bond state, see @ref bond_state_t.
 *
 * **Example:**
 * @code
bond_state_t bond_state = bt_device_get_bond_state(ins, &addr, BT_TRANSPORT_BR_EDR);
switch (bond_state) {
    case BOND_STATE_NONE:
        // Not bonded
        break;
    case BOND_STATE_BONDING:
        // Bonding in progress
        break;
    case BOND_STATE_BONDED:
        // Bonded
        break;
    case BOND_STATE_CANCELING:
        // Bonding is being canceled
        break;
}
 * @endcode
 */
bond_state_t BTSYMBOLS(bt_device_get_bond_state)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport);

/**
 * @brief Check if a remote device is bonded.
 *
 * Determines whether a remote device is bonded with the local device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type, see @ref bt_transport_t.
 * @return true - Device is bonded.
 * @return false - Device is not bonded.
 *
 * **Example:**
 * @code
if (bt_device_is_bonded(ins, &addr, BT_TRANSPORT_LE)) {
    // Device is bonded
} else {
    // Device is not bonded
}
 * @endcode
 */
bool BTSYMBOLS(bt_device_is_bonded)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport);

/**
 * @brief Initiate bonding with a remote device.
 *
 * Starts the bonding process with a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type (0: LE, 1: BR/EDR).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_create_bond(ins, &addr, BT_TRANSPORT_BR_EDR) == BT_STATUS_SUCCESS) {
    // Bonding initiated successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_create_bond)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport);

/**
 * @brief Remove bonding with a remote device.
 *
 * Removes the bonding information of a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type (0: LE, 1: BR/EDR).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_remove_bond(ins, &addr, BT_TRANSPORT_LE) == BT_STATUS_SUCCESS) {
    // Bonding removed successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_remove_bond)(bt_instance_t* ins, bt_address_t* addr, uint8_t transport);

/**
 * @brief Cancel an ongoing bonding process.
 *
 * Cancels the bonding process with a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_cancel_bond(ins, &addr) == BT_STATUS_SUCCESS) {
    // Bonding canceled successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_cancel_bond)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Reply to a pairing request.
 *
 * Responds to a pairing request from a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param accept - true to accept the pairing request; false to reject.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_pair_request_reply(ins, &addr, true) == BT_STATUS_SUCCESS) {
    // Pairing request accepted
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_pair_request_reply)(bt_instance_t* ins, bt_address_t* addr, bool accept);

/**
 * @brief Set pairing confirmation for secure pairing.
 *
 * Confirms or rejects a pairing confirmation request, typically used in Just Works or Passkey Confirmation scenarios.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type (0: LE, 1: BR/EDR).
 * @param accept - true to accept the pairing; false to reject.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_set_pairing_confirmation(ins, &addr, BT_TRANSPORT_LE, true) == BT_STATUS_SUCCESS) {
    // Pairing confirmed
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_set_pairing_confirmation)(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept);

/**
 * @brief Set the PIN code for pairing.
 *
 * Provides a PIN code in response to a pairing request that requires one.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param accept - true to accept the pairing; false to reject.
 * @param pincode - Pointer to the PIN code string.
 * @param len - Length of the PIN code string.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
char pin[] = "1234";
if (bt_device_set_pin_code(ins, &addr, true, pin, strlen(pin)) == BT_STATUS_SUCCESS) {
    // PIN code set successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_set_pin_code)(bt_instance_t* ins, bt_address_t* addr, bool accept, char* pincode, int len);

/**
 * @brief Set the passkey for pairing.
 *
 * Provides a passkey in response to a pairing request that requires one, for both BR/EDR and LE transports.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param transport - Transport type (0: LE, 1: BR/EDR).
 * @param accept - true to accept the pairing; false to reject.
 * @param passkey - The passkey value.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example for BR/EDR:**
 * @code
if (bt_device_set_pass_key(ins, &addr, BT_TRANSPORT_BR_EDR, true, 123456) == BT_STATUS_SUCCESS) {
    // Passkey set successfully
} else {
    // Handle error
}
 * @endcode
 *
 * **Example for LE:**
 * @code
if (bt_device_set_pass_key(ins, &addr, BT_TRANSPORT_LE, true, 123456) == BT_STATUS_SUCCESS) {
    // Passkey set successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_set_pass_key)(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept, uint32_t passkey);

/**
 * @brief Set the OOB Temporary Key (TK) for LE legacy pairing.
 *
 * Provides the OOB TK value used during LE legacy pairing.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param tk_val - OOB TK value (128-bit key).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_set_le_legacy_tk)(bt_instance_t* ins, bt_address_t* addr, bt_128key_t tk_val);

/**
 * @brief Set remote OOB data for LE Secure Connections pairing.
 *
 * Provides the remote OOB data (Confirmation and Random values) used during LE Secure Connections pairing.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param c_val - LE Secure Connections Confirmation value (128-bit key).
 * @param r_val - LE Secure Connections Random value (128-bit key).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_set_le_sc_remote_oob_data)(bt_instance_t* ins, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val);

/**
 * @brief Get local OOB data for LE Secure Connections pairing.
 *
 * Initiates the generation of local OOB data for LE Secure Connections pairing.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device (can be NULL).
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_get_le_sc_local_oob_data(ins, &addr) == BT_STATUS_SUCCESS) {
    // OOB data generation initiated
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_get_le_sc_local_oob_data)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Connect to a remote device.
 *
 * Initiates an ACL connection to a remote BR/EDR device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_connect(ins, &addr) == BT_STATUS_SUCCESS) {
    // Connection initiated successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from a remote device.
 *
 * Terminates the ACL connection with a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 *
 * **Example:**
 * @code
if (bt_device_disconnect(ins, &addr) == BT_STATUS_SUCCESS) {
    // Disconnection initiated successfully
} else {
    // Handle error
}
 * @endcode
 */
bt_status_t BTSYMBOLS(bt_device_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Connect to a remote LE device.
 *
 * Initiates a connection to a remote LE device with specified parameters.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote LE device.
 * @param type - LE address type, see @ref ble_addr_type_t.
 * @param param - Pointer to connection parameters, see @ref ble_connect_params_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_connect_le)(bt_instance_t* ins, bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param);

/**
 * @brief Disconnect from a remote LE device.
 *
 * Terminates the LE connection with a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote LE device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_disconnect_le)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Reply to a connection request.
 *
 * Responds to a connection request from a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param accept - true to accept the connection; false to reject.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_connect_request_reply)(bt_instance_t* ins, bt_address_t* addr, bool accept);

/**
 * @brief Set the LE PHY parameters.
 *
 * Configures the PHY (Physical Layer) parameters for an LE connection.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote LE device.
 * @param tx_phy - Preferred TX PHY, see @ref ble_phy_type_t.
 * @param rx_phy - Preferred RX PHY, see @ref ble_phy_type_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_set_le_phy)(bt_instance_t* ins, bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy);

/**
 * @brief Connect to all profiles.
 *
 * @note Currently not supported.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 */
void BTSYMBOLS(bt_device_connect_all_profile)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from all profiles.
 *
 * @note Currently not supported.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 */
void BTSYMBOLS(bt_device_disconnect_all_profile)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Enable enhanced mode for a connection.
 *
 * Enables an enhanced mode (e.g., eSCO, sniff mode) for a connection with a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param mode - Enhanced mode to enable, see @ref bt_enhanced_mode_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_enable_enhanced_mode)(bt_instance_t* ins, bt_address_t* addr, bt_enhanced_mode_t mode);

/**
 * @brief Disable enhanced mode for a connection.
 *
 * Disables an enhanced mode (e.g., eSCO, sniff mode) for a connection with a remote device.
 *
 * @param ins - Bluetooth client instance, see @ref bt_instance_t.
 * @param addr - Address of the remote device.
 * @param mode - Enhanced mode to disable, see @ref bt_enhanced_mode_t.
 * @return bt_status_t - BT_STATUS_SUCCESS on success; a negative error code on failure.
 */
bt_status_t BTSYMBOLS(bt_device_disable_enhanced_mode)(bt_instance_t* ins, bt_address_t* addr, bt_enhanced_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* _BT_DEVICE_H__ */