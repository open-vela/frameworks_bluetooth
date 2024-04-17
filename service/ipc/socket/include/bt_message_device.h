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

#ifdef __BT_MESSAGE_CODE__
BT_DEVICE_MESSAGE_START,
    BT_DEVICE_GET_IDENTITY_ADDRESS,
    BT_DEVICE_GET_ADDRESS_TYPE,
    BT_DEVICE_GET_DEVICE_TYPE,
    BT_DEVICE_GET_NAME,
    BT_DEVICE_GET_DEVICE_CLASS,
    BT_DEVICE_GET_UUIDS,
    BT_DEVICE_GET_APPEARANCE,
    BT_DEVICE_GET_RSSI,
    BT_DEVICE_GET_ALIAS,
    BT_DEVICE_SET_ALIAS,
    BT_DEVICE_GET_ACL_HANDLE,
    BT_DEVICE_IS_CONNECTED,
    BT_DEVICE_IS_ENCRYPTED,
    BT_DEVICE_IS_BOND_INITIATE_LOCAL,
    BT_DEVICE_GET_BOND_STATE,
    BT_DEVICE_IS_BONDED,
    BT_DEVICE_CREATE_BOND,
    BT_DEVICE_REMOVE_BOND,
    BT_DEVICE_CANCEL_BOND,
    BT_DEVICE_PAIR_REQUEST_REPLY,
    BT_DEVICE_SET_PAIRING_CONFIRMATION,
    BT_DEVICE_SET_PIN_CODE,
    BT_DEVICE_SET_PASS_KEY,
    BT_DEVICE_SET_LE_LEGACY_TK,
    BT_DEVICE_SET_LE_SC_REMOTE_OOB_DATA,
    BT_DEVICE_GET_LE_SC_LOCAL_OOB_DATA,
    BT_DEVICE_CONNECT,
    BT_DEVICE_DISCONNECT,
    BT_DEVICE_CONNECT_LE,
    BT_DEVICE_DISCONNECT_LE,
    BT_DEVICE_CONNECT_REQUEST_REPLY,
    BT_DEVICE_SET_LE_PHY,
    BT_DEVICE_CONNECT_ALL_PROFILE,
    BT_DEVICE_DISCONNECT_ALL_PROFILE,
    BT_DEVICE_MESSAGE_END,
#endif

#ifndef _BT_MESSAGE_DEVICE_H__
#define _BT_MESSAGE_DEVICE_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_device.h"

    typedef union {
        bt_status_t status;
        bt_adapter_state_t state;
        bt_device_type_t dtype;
        bool bbool;
        bt_scan_mode_t mode;
        uint32_t v32;
        uint16_t v16;
        uint8_t v8;
        bt_io_capability_t ioc;
        ble_addr_type_t atype;
        bond_state_t bstate;
        bt_address_t addr;
    } bt_device_result_t;

    typedef union {
        struct {
            bt_address_t addr;
        } _bt_device_get_identity_address,
            _bt_device_get_address_type,
            _bt_device_get_device_type,
            _bt_device_get_device_class,
            _bt_device_get_appearance,
            _bt_device_get_rssi,
            _bt_device_cancel_bond,
            _bt_device_connect,
            _bt_device_disconnect,
            _bt_device_disconnect_le,
            _bt_device_addr,
            _bt_device_get_le_sc_local_oob_data,
            _bt_device_get_acl_handle;

        struct {
            bt_address_t addr;
            char name[64];
            uint32_t length;
        } _bt_device_get_name;

        struct {
            bt_address_t addr;
            bt_uuid_t uuids[16];
            uint16_t size;
        } _bt_device_get_uuids;

        struct {
            bt_address_t addr;
            char alias[64];
            uint32_t length;
        } _bt_device_get_alias;

        struct {
            bt_address_t addr;
            char alias[64];
        } _bt_device_set_alias;

        struct {
            bt_address_t addr;
            bt_transport_t transport;
        } _bt_device_create_bond,
            _bt_device_remove_bond,
            _bt_device_is_connected,
            _bt_device_is_encrypted,
            _bt_device_is_bond_initiate_local,
            _bt_device_get_bond_state,
            _bt_device_is_bonded;

        struct {
            bt_address_t addr;
            bool accept;
        } _bt_device_pair_request_reply;

        struct {
            bt_address_t addr;
            uint8_t transport;
            bool accept;
        } _bt_device_set_pairing_confirmation;

        struct {
            bt_address_t addr;
            bool accept;
            char pincode[64];
            int len;
        } _bt_device_set_pin_code;

        struct {
            bt_address_t addr;
            uint8_t transport;
            bool accept;
            uint32_t passkey;
        } _bt_device_set_pass_key;

        struct {
            bt_address_t addr;
            bt_128key_t tk_val;
        } _bt_device_set_le_legacy_tk;

        struct {
            bt_address_t addr;
            bt_128key_t c_val;
            bt_128key_t r_val;
        } _bt_device_set_le_sc_remote_oob_data;

        struct {
            bt_address_t addr;
            ble_addr_type_t type;
            ble_connect_params_t param;
        } _bt_device_connect_le;

        struct {
            bt_address_t addr;
            bool accept;
        } _bt_device_connect_request_reply;

        struct {
            bt_address_t addr;
            ble_phy_type_t tx_phy;
            ble_phy_type_t rx_phy;
        } _bt_device_set_le_phy;

    } bt_message_device_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_DEVICE_H__ */
