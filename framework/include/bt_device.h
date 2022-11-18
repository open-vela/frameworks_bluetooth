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

<<<<<<< HEAD
#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

=======
>>>>>>> bluetooth framework re-implement base
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
<<<<<<< HEAD
 * @brief Get identity address of remote device
 *
 * @param ins - bluetooth client instance.
 * @param bd_addr - remote device address.
 * @param[out] id_addr - identity address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_device_get_identity_address)(bt_instance_t *ins, bt_address_t *bd_addr, bt_address_t *id_addr);

/**
 * @brief Get identity address of remote device
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return ble_addr_type_t - address type, UNKNOWN on device not found.
 */
ble_addr_type_t BTSYMBOLS(bt_device_get_address_type)(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Get remote device type
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return bt_device_type_t - device type, zero on device not found.
 */
bt_device_type_t BTSYMBOLS(bt_device_get_device_type)(bt_instance_t *ins, bt_address_t *addr);
=======
 * @brief
 * @note Not support
 * @param ins
 * @param addr
 * @return bt_address_t*
 */
bt_address_t *bt_device_get_identity_address(bt_instance_t *ins, bt_address_t *addr);
ble_addr_type_t bt_device_get_address_type(bt_instance_t *ins, bt_address_t *addr);
bt_device_type_t bt_device_get_device_type(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Get remote device name
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param[out] name - remote device name.
 * @param length - maximum length of name buffer.
 * @return true - on success.
 * @return false - on device not found.
 */
<<<<<<< HEAD
bool BTSYMBOLS(bt_device_get_name)(bt_instance_t *ins, bt_address_t *addr, char *name, uint32_t length);
=======
bool bt_device_get_name(bt_instance_t *ins, bt_address_t *addr, char *name, uint32_t length);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Get remote device class
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return uint32_t - cod, zero on device not found.
 */
<<<<<<< HEAD
uint32_t BTSYMBOLS(bt_device_get_device_class)(bt_instance_t *ins, bt_address_t *addr);
=======
uint32_t bt_device_get_device_class(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Get remote device support uuids
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param[out] uuids - out remote device uuids array.
 * @param[out] size - out remote device uuid array size.
 * @param allocator - array allocator.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_get_uuids)(bt_instance_t *ins, bt_address_t *addr, bt_uuid_t **uuids, uint16_t *size, bt_allocator_t allocator);
=======
bt_status_t bt_device_get_uuids(bt_instance_t *ins, bt_address_t *addr, bt_uuid_t **uuids, uint16_t *size, bt_allocator_t allocator);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Get remote device LE apperance
 * @note Not support
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return uint16_t - apperance, zero on device not found.
 */
<<<<<<< HEAD
uint16_t BTSYMBOLS(bt_device_get_appearance)(bt_instance_t *ins, bt_address_t *addr);
=======
uint16_t bt_device_get_appearance(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Get remote device RSSI
 * @note Not support
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return int8_t - rssi.
 */
<<<<<<< HEAD
int8_t BTSYMBOLS(bt_device_get_rssi)(bt_instance_t *ins, bt_address_t *addr);
=======
int8_t bt_device_get_rssi(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Get remote device alias, default use remote name if not set
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param[out] alias - out alias buffer.
 * @param length - maximum length of alias buffer.
 * @return true on success.
 * @return false on device not found.
 */
<<<<<<< HEAD
bool BTSYMBOLS(bt_device_get_alias)(bt_instance_t *ins, bt_address_t *addr, char *alias, uint32_t length);
=======
bool bt_device_get_alias(bt_instance_t *ins, bt_address_t *addr, char *alias, uint32_t length);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Set remote device alias
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param alias - alias.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_set_alias)(bt_instance_t *ins, bt_address_t *addr, const char *alias);
=======
bt_status_t bt_device_set_alias(bt_instance_t *ins, bt_address_t *addr, const char *alias);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Check remote deivce is connected
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return true - connected.
 * @return false - not connected.
 */
<<<<<<< HEAD
bool BTSYMBOLS(bt_device_is_connected)(bt_instance_t *ins, bt_address_t *addr, bt_transport_t transport);
=======
bool bt_device_is_connected(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Check remote deivce is encrypted
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return true - encrypted
 * @return false - not encrypted
 */
<<<<<<< HEAD
bool BTSYMBOLS(bt_device_is_encrypted)(bt_instance_t *ins, bt_address_t *addr, bt_transport_t transport);
=======
bool bt_device_is_encrypted(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Check is bond initiate from local
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return true - initiate from local.
 * @return false - initiate from remote.
 */
<<<<<<< HEAD
bool BTSYMBOLS(bt_device_is_bond_initiate_local)(bt_instance_t *ins, bt_address_t *addr, bt_transport_t transport);
=======
bool bt_device_is_bond_initiate_local(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Get remote device bond state
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return bond_state_t - bond state.
 */
<<<<<<< HEAD
bond_state_t BTSYMBOLS(bt_device_get_bond_state)(bt_instance_t *ins, bt_address_t *addr, bt_transport_t transport);
=======
bond_state_t bt_device_get_bond_state(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Check remote device is bonded
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return true - bonded
 * @return false - not bonded
 */
<<<<<<< HEAD
bool BTSYMBOLS(bt_device_is_bonded)(bt_instance_t *ins, bt_address_t *addr, bt_transport_t transport);
=======
bool bt_device_is_bonded(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Initiate bond to remote device
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param transport - transport type (0:BLE, 1:BREDR).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_create_bond)(bt_instance_t *ins, bt_address_t *addr, bt_transport_t transport);
=======
bt_status_t bt_device_create_bond(bt_instance_t *ins, bt_address_t *addr, bt_transport_t transport);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Remove bonded device
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param transport - transport type (0:BLE, 1:BREDR).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_remove_bond)(bt_instance_t *ins, bt_address_t *addr, uint8_t transport);
=======
bt_status_t bt_device_remove_bond(bt_instance_t *ins, bt_address_t *addr, uint8_t transport);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Cancel bonding
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_cancel_bond)(bt_instance_t *ins, bt_address_t *addr);
=======
bt_status_t bt_device_cancel_bond(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Reply pairing request
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param accept - true:accept, false:reject.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_pair_request_reply)(bt_instance_t *ins, bt_address_t *addr, bool accept);
=======
bt_status_t bt_device_pair_request_reply(bt_instance_t *ins, bt_address_t *addr, bool accept);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Set pairing confirmation
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param transport - transport type (0:BLE, 1:BREDR).
 * @param accept - true:accept, false:reject.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_set_pairing_confirmation)(bt_instance_t *ins, bt_address_t *addr, uint8_t transport, bool accept);
=======
bt_status_t bt_device_set_pairing_confirmation(bt_instance_t *ins, bt_address_t *addr, uint8_t transport, bool accept);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Set pairing PIN code
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param accept - true:accept, false:reject.
 * @param pincode - pin code string.
 * @param len - length of pin code string
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_set_pin_code)(bt_instance_t *ins, bt_address_t *addr, bool accept,
                                              char *pincode, int len);
=======
bt_status_t bt_device_set_pin_code(bt_instance_t *ins, bt_address_t *addr, bool accept,
                                   char *pincode, int len);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Set simple securty pair passkey or LE smp key
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param transport - transport type (0:BLE, 1:BREDR).
 * @param accept - true:accept, false:reject.
 * @param passkey - on transport is BREDR, mean ssp passkey; on transport is LE, mean smp key.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_set_pass_key)(bt_instance_t *ins, bt_address_t *addr, uint8_t transport, bool accept, uint32_t passkey);

/**
 * @brief Set remote OOB data for LE pairing
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param tk_val - Legacy pairing OOB TK value. NULL if not available.
 * @param c_val - LE Secure Connection Confirmation value. NULL if not available.
 * @param r_val - LE Secure Connection Random value. NULL if not available.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_device_set_le_oob_data)(bt_instance_t *ins, bt_address_t *addr, bt_128key_t tk_val, bt_128key_t c_val, bt_128key_t r_val);
=======
bt_status_t bt_device_set_pass_key(bt_instance_t *ins, bt_address_t *addr, uint8_t transport, bool accept, uint32_t passkey);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Connect to peer device
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_connect)(bt_instance_t *ins, bt_address_t *addr);
=======
bt_status_t bt_device_connect(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Disconnect from ACL connection.
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_disconnect)(bt_instance_t *ins, bt_address_t *addr);
=======
bt_status_t bt_device_disconnect(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Connect to LE device
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote LE device address.
 * @param type - LE address type.
 * @param param - connect params.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_connect_le)(bt_instance_t *ins, bt_address_t *addr,
                                            ble_addr_type_t type,
                                            ble_connect_params_t *param);
=======
bt_status_t bt_device_connect_le(bt_instance_t *ins, bt_address_t *addr,
                                 ble_addr_type_t type,
                                 ble_connect_params_t *param);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Disconnect from LE connection
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote LE device address.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_disconnect_le)(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Reply connect request
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote device address.
 * @param accept - true:accept, false:reject.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_device_connect_request_reply)(bt_instance_t *ins, bt_address_t *addr, bool accept);
=======
bt_status_t bt_device_disconnect_le(bt_instance_t *ins, bt_address_t *addr);
>>>>>>> bluetooth framework re-implement base

/**
 * @brief Set LE phy
 *
 * @param ins - bluetooth client instance.
 * @param addr - remote LE device address.
 * @param tx_phy - tx phy (0:1M, 1:2M, 2:CODED).
 * @param rx_phy - rx phy (0:1M, 1:2M, 2:CODED).
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_device_set_le_phy)(bt_instance_t *ins, bt_address_t *addr,
                                            ble_phy_type_t tx_phy,
                                            ble_phy_type_t rx_phy);

/**
 * @brief Connect to all profile.
 * @note not support.
 * @param ins - bluetooth client instance.
 */
void BTSYMBOLS(bt_device_connect_all_profile)(bt_instance_t *ins, bt_address_t *addr);

/**
 * @brief Disconnect from all profile.
 * @note not support.
 * @param ins - bluetooth client instance.
 */
void BTSYMBOLS(bt_device_disconnect_all_profile)(bt_instance_t *ins, bt_address_t *addr);

uint16_t BTSYMBOLS(bt_device_get_acl_handle)(bt_instance_t *ins, bt_address_t *addr);
=======
bt_status_t bt_device_set_le_phy(bt_instance_t *ins, bt_address_t *addr,
                                 ble_phy_type_t tx_phy,
                                 ble_phy_type_t rx_phy);
>>>>>>> bluetooth framework re-implement base
#ifdef __cplusplus
}
#endif

<<<<<<< HEAD
#endif /* _BT_DEVICE_H__ */
=======
#endif /* _BT_DEVICE_H__ */
>>>>>>> bluetooth framework re-implement base
