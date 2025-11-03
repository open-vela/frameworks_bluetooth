/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef _BT_MESSAGE_PBAP_PCE_H__
#define _BT_MESSAGE_PBAP_PCE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_ipc_code.h"
#include "bt_pbap_pce.h"

#define BT_IPC_CODE_COMMAND_PBAP_PCE_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, 0)

#define PBAP_PCE_SUBCODE_REGISTER_CALLBACK 1
#define BT_PBAP_PCE_REGISTER_CALLBACK BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_REGISTER_CALLBACK)
#define PBAP_PCE_SUBCODE_UNREGISTER_CALLBACK 2
#define BT_PBAP_PCE_UNREGISTER_CALLBACK BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_UNREGISTER_CALLBACK)
#define PBAP_PCE_SUBCODE_CONNECT 3
#define BT_PBAP_PCE_CONNECT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_CONNECT)
#define PBAP_PCE_SUBCODE_DISCONNECT 4
#define BT_PBAP_PCE_DISCONNECT BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_DISCONNECT)
#define PBAP_PCE_SUBCODE_CHANGE_DIRECTORY 5
#define BT_PBAP_PCE_CHANGE_DIRECTORY BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_CHANGE_DIRECTORY)
#define PBAP_PCE_SUBCODE_PULL_VCARD_LISTING 6
#define BT_PBAP_PCE_PULL_VCARD_LISTING BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_PULL_VCARD_LISTING)
#define PBAP_PCE_SUBCODE_PULL_VCARD 7
#define BT_PBAP_PCE_PULL_VCARD BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_PULL_VCARD)
#define PBAP_PCE_SUBCODE_GET_CONTACT_BY_NAME 8
#define BT_PBAP_PBAP_SEARCH_PROPERTY_NAME BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_GET_CONTACT_BY_NAME)
#define PBAP_PCE_SUBCODE_GET_CONTACT_BY_NUMBER 9
#define BT_PBAP_PBAP_SEARCH_PROPERTY_NUMBER BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_GET_CONTACT_BY_NUMBER)
#define PBAP_PCE_SUBCODE_ADD_TO_BLACKLIST 10
#define BT_PBAP_PCE_ADD_TO_BLACKLIST BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ADD_TO_BLACKLIST)
#define PBAP_PCE_SUBCODE_REMOVE_FROM_BLACKLIST 11
#define BT_PBAP_PCE_REMOVE_FROM_BLACKLIST BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_REMOVE_FROM_BLACKLIST)
#define PBAP_PCE_SUBCODE_IS_IN_BLACKLIST 12
#define BT_PBAP_PCE_IS_IN_BLACKLIST BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_IS_IN_BLACKLIST)

#define BT_IPC_CODE_COMMAND_PBAP_PCE_END BT_IPC_CODE(BT_IPC_CODE_TYPE_COMMAND, BT_IPC_CODE_GROUP_PBAP_PCE, BT_IPC_CODE_SUBCODE_MAX_NUM)

#define BT_IPC_CODE_CALLBACK_PBAP_PCE_BEGIN BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, 0)

#define PBAP_PCE_SUBCODE_ON_CONNECTION_STATE_CHANGED 1
#define BT_PBAP_PCE_ON_CONNECTION_STATE_CHANGED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ON_CONNECTION_STATE_CHANGED)
#define PBAP_PCE_SUBCODE_ON_DIR_CHANGED 2
#define BT_PBAP_PCE_ON_DIR_CHANGED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ON_DIR_CHANGED)
#define PBAP_PCE_SUBCODE_ON_VCARD_LISTING_DATA_RECEIVED 3
#define BT_PBAP_PCE_ON_VCARD_LISTING_DATA_RECEIVED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ON_VCARD_LISTING_DATA_RECEIVED)
#define PBAP_PCE_SUBCODE_ON_VCARD_LISTING_END 4
#define BT_PBAP_PCE_ON_VCARD_LISTING_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ON_VCARD_LISTING_END)
#define PBAP_PCE_SUBCODE_ON_VCARD_DATA_RECEIVED 5
#define BT_PBAP_PCE_ON_VCARD_DATA_RECEIVED BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ON_VCARD_DATA_RECEIVED)
#define PBAP_PCE_SUBCODE_ON_VCARD_END 6
#define BT_PBAP_PCE_ON_VCARD_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ON_VCARD_END)
#define PBAP_PCE_SUBCODE_ON_GET_CONTACT_END 7
#define BT_PBAP_PCE_ON_GET_CONTACT_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, PBAP_PCE_SUBCODE_ON_GET_CONTACT_END)

#define BT_IPC_CODE_CALLBACK_PBAP_PCE_END BT_IPC_CODE(BT_IPC_CODE_TYPE_CALLBACK, BT_IPC_CODE_GROUP_PBAP_PCE, BT_IPC_CODE_SUBCODE_MAX_NUM)

typedef union {
    uint8_t status;
    uint8_t profile_conn_state;
    uint8_t value_bool;
} bt_pbap_pce_result_t;

typedef union {
    struct {
        bt_address_t addr;
    } _bt_pbap_pce_connect,
        _bt_pbap_pce_disconnect,
        _bt_pbap_pce_add_to_blacklist,
        _bt_pbap_pce_remove_from_blacklist,
        _bt_pbap_pce_is_in_blacklist;

    struct {
        bt_address_t addr;
        char dir[PBAP_PKT_LEN_MAX + 1];
    } _bt_pbap_pce_change_dir;

    struct {
        bt_address_t addr;
        bt_pbap_search_property_t property;
        char value[PBAP_PKT_LEN_MAX + 1];
    } _bt_pbap_pce_pull_vcard_listing;

    struct {
        bt_address_t addr;
        char object[PBAP_PKT_LEN_MAX + 1];
        uint64_t filter;
    } _bt_pbap_pce_pull_vcard;

    struct {
        bt_address_t addr;
        char name[BT_PBAP_PCE_PROPERTY_MAX_LEN];
    } _bt_pbap_pce_get_contact_by_name;

    struct {
        bt_address_t addr;
        char number[BT_PBAP_PCE_PROPERTY_MAX_LEN];
    } _bt_pbap_pce_get_contact_by_number;

} bt_message_pbap_pce_t;

typedef union {
    struct {
        bt_address_t addr;
        profile_connection_state_t state;
    } _on_connection_state_changed;

    struct {
        bt_address_t addr;
        uint16_t status;
    } _on_dir_changed;

    struct {
        bt_address_t addr;
        char object[PBAP_PKT_LEN_MAX + 1];
        uint16_t len;
    } _on_vcard_listing_data_received,
        _on_vcard_data_received;

    struct {
        bt_address_t addr;
        uint16_t status;
    } _on_vcard_listing_end,
        _on_vcard_end;

    struct {
        bt_status_t status;
        bt_pbap_search_property_t property;
        bt_pce_contact_t contact;
        char value[BT_PBAP_PCE_PROPERTY_MAX_LEN];
    } _on_get_contact_end;

} bt_message_pbap_pce_callbacks_t;
#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_PBAP_PCE_H__ */
