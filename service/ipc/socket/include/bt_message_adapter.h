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
BT_ADAPTER_MESSAGE_START,
    BT_ADAPTER_ENABLE,
    BT_ADAPTER_DISABLE,
    BT_ADAPTER_ENABLE_LE,
    BT_ADAPTER_DISABLE_LE,
    BT_ADAPTER_GET_STATE,
    BT_ADAPTER_GET_TYPE,
    BT_ADAPTER_SET_DISCOVERY_FILTER,
    BT_ADAPTER_START_DISCOVERY,
    BT_ADAPTER_CANCEL_DISCOVERY,
    BT_ADAPTER_IS_DISCOVERING,
    BT_ADAPTER_GET_ADDRESS,
    BT_ADAPTER_SET_NAME,
    BT_ADAPTER_GET_NAME,
    BT_ADAPTER_GET_UUIDS,
    BT_ADAPTER_SET_SCAN_MODE,
    BT_ADAPTER_GET_SCAN_MODE,
    BT_ADAPTER_SET_DEVICE_CLASS,
    BT_ADAPTER_GET_DEVICE_CLASS,
    BT_ADAPTER_SET_IO_CAPABILITY,
    BT_ADAPTER_GET_IO_CAPABILITY,
    BT_ADAPTER_SET_INQUIRY_SCAN_PARAMETERS,
    BT_ADAPTER_SET_PAGE_SCAN_PARAMETERS,
    BT_ADAPTER_GET_BONDED_DEVICES,
    BT_ADAPTER_GET_CONNECTED_DEVICES,
    BT_ADAPTER_DISCONNECT_ALL_DEVICES,
    BT_ADAPTER_IS_SUPPORT_BREDR,
    BT_ADAPTER_REGISTER_CALLBACK,
    BT_ADAPTER_UNREGISTER_CALLBACK,
    BT_ADAPTER_IS_LE_ENABLED,
    BT_ADAPTER_IS_SUPPORT_LE,
    BT_ADAPTER_IS_SUPPORT_LEAUDIO,
    BT_ADAPTER_GET_LE_ADDRESS,
    BT_ADAPTER_SET_LE_ADDRESS,
    BT_ADAPTER_SET_LE_IDENTITY_ADDRESS,
    BT_ADAPTER_SET_LE_IO_CAPABILITY,
    BT_ADAPTER_GET_LE_IO_CAPABILITY,
    BT_ADAPTER_SET_LE_APPEARANCE,
    BT_ADAPTER_GET_LE_APPEARANCE,
    BT_ADAPTER_LE_ENABLE_KEY_DERIVATION,
    BT_ADAPTER_LE_ADD_WHITELIST,
    BT_ADAPTER_LE_REMOVE_WHITELIST,
    BT_ADAPTER_SET_AFH_CHANNEL_CLASSFICATION,
    BT_ADAPTER_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_ADAPTER_CALLBACK_START,
    BT_ADAPTER_ON_ADAPTER_STATE_CHANGED,
    BT_ADAPTER_ON_DISCOVERY_STATE_CHANGED,
    BT_ADAPTER_ON_DISCOVERY_RESULT,
    BT_ADAPTER_ON_SCAN_MODE_CHANGED,
    BT_ADAPTER_ON_DEVICE_NAME_CHANGED,
    BT_ADAPTER_ON_PAIR_REQUEST,
    BT_ADAPTER_ON_PAIR_DISPLAY,
    BT_ADAPTER_ON_CONNECT_REQUEST,
    BT_ADAPTER_ON_CONNECTION_STATE_CHANGED,
    BT_ADAPTER_ON_BOND_STATE_CHANGED,
    BT_ADAPTER_ON_LE_SC_LOCAL_OOB_DATA_GOT,
    BT_ADAPTER_ON_REMOTE_NAME_CHANGED,
    BT_ADAPTER_ON_REMOTE_ALIAS_CHANGED,
    BT_ADAPTER_ON_REMOTE_COD_CHANGED,
    BT_ADAPTER_ON_REMOTE_UUIDS_CHANGED,
    BT_ADAPTER_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_ADAPTER_H__
#define _BT_MESSAGE_ADAPTER_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_adapter.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t state; /* bt_adapter_state_t */
        uint8_t dtype; /* bt_device_type_t */
        uint8_t bbool; /* boolean */

        uint8_t mode; /* bt_scan_mode_t */
        uint8_t ioc; /* bt_io_capability_t */
        uint16_t v16;

        uint32_t v32;
    } bt_adapter_result_t;

    typedef union {
        struct {
            bt_address_t addr;
        } _bt_adapter_get_address,
            _bt_adapter_set_le_address,
            _bt_adapter_le_add_whitelist,
            _bt_adapter_le_remove_whitelist;

        struct {
            char name[64];
        } _bt_adapter_set_name,
            _bt_adapter_get_name;

        struct {
            uint32_t v32;
        } _bt_adapter_start_discovery,
            _bt_adapter_set_device_class,
            _bt_adapter_set_le_io_capability;

        struct {
            uint16_t size;
            uint8_t pad[2];
            bt_uuid_t uuids[BT_UUID_MAX_NUM];
        } _bt_adapter_get_uuids;

        struct {
            uint8_t mode; /* bt_scan_mode_t */
            uint8_t bondable; /* boolean */
        } _bt_adapter_set_scan_mode;

        struct {
            uint8_t cap; /* bt_io_capability_t */
        } _bt_adapter_set_io_capability;

        struct {
            bt_address_t addr;
            uint8_t type; /* ble_addr_type_t */
        } _bt_adapter_get_le_address;

        struct {
            bt_address_t addr;
            uint8_t pub; /* boolean */
        } _bt_adapter_set_le_identity_address;

        struct {
            uint16_t v16;
        } _bt_adapter_set_le_appearance;

        struct {
            uint32_t num; /* int */
            bt_address_t addr[32];
            uint8_t transport; /* bt_transport_t */
        } _bt_adapter_get_bonded_devices,
            _bt_adapter_get_connected_devices;

        struct {
            uint8_t brkey_to_lekey; /* boolean */
            uint8_t lekey_to_brkey; /* boolean */
        } _bt_adapter_le_enable_key_derivation;

        struct {
            uint8_t type; /* bt_scan_type_t */
            uint8_t pad[3];
            uint16_t interval;
            uint16_t window;
        } _bt_adapter_set_inquiry_scan_parameters,
            _bt_adapter_set_page_scan_parameters;

        struct {
            uint16_t central_frequency;
            uint16_t band_width;
            uint16_t number;
        } _bt_adapter_set_afh_channel_classification;
    } bt_message_adapter_t;

    typedef union {
        struct {
            uint8_t state; /* bt_adapter_state_t */
        } _on_adapter_state_changed;

        struct {
            uint8_t state; /* bt_discovery_state_t */
        } _on_discovery_state_changed;

        struct {
            bt_discovery_result_t result;
        } _on_discovery_result;

        struct {
            uint8_t mode; /* bt_scan_mode_t */
        } _on_scan_mode_changed;

        struct {
            char device_name[64];
        } _on_device_name_changed;

        struct {
            bt_address_t addr;
        } _on_pair_request;

        struct {
            bt_address_t addr;
            uint8_t transport; /* bt_transport_t */
            uint8_t type; /* bt_pair_type_t */
            uint32_t passkey;
        } _on_pair_display;

        struct {
            bt_address_t addr;
        } _on_connect_request;

        struct {
            bt_address_t addr;
            uint8_t transport; /* bt_transport_t */
            uint8_t state; /* connection_state_t */
        } _on_connection_state_changed;

        struct {
            bt_address_t addr;
            uint8_t transport; /* bt_transport_t */
            uint8_t previous_state; /* bond_state_t */
            uint8_t current_state; /* bond_state_t */
            uint8_t is_ctkd; /* boolean */
        } _on_bond_state_changed;

        struct {
            bt_address_t addr;
            bt_128key_t c_val;
            bt_128key_t r_val;
        } _on_le_sc_local_oob_data_got;

        struct {
            char name[64];
            bt_address_t addr;
        } _on_remote_name_changed;

        struct {
            char alias[64];
            bt_address_t addr;
        } _on_remote_alias_changed;

        struct {
            uint32_t cod;
            bt_address_t addr;
        } _on_remote_cod_changed;

        struct {
            bt_address_t addr;
            uint16_t size;
            bt_uuid_t uuids[BT_UUID_MAX_NUM];
        } _on_remote_uuids_changed;
    } bt_message_adapter_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_ADAPTER_H__ */
