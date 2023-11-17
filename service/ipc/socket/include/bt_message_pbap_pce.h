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
BT_PBAP_PCE_MESSAGE_START,
    BT_PBAP_PCE_REGISTER_CALLBACK,
    BT_PBAP_PCE_UNREGISTER_CALLBACK,
    BT_PBAP_PCE_CONNECT,
    BT_PBAP_PCE_DISCONNECT,
    BT_PBAP_PCE_CHANGE_DIRECTORY,
    BT_PBAP_PCE_PULL_VCARD_LISTING,
    BT_PBAP_PCE_PULL_VCARD,
    BT_PBAP_PCE_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_PBAP_PCE_CALLBACK_START,
    BT_PBAP_PCE_ON_CONNECTION_STATE_CHANGED,
    BT_PBAP_PCE_ON_DIR_CHANGED,
    BT_PBAP_PCE_ON_VCARD_LISTING_DATA_RECEIVED,
    BT_PBAP_PCE_ON_VCARD_LISTING_END,
    BT_PBAP_PCE_ON_VCARD_DATA_RECEIVED,
    BT_PBAP_PCE_ON_VCARD_END,
    BT_PBAP_PCE_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_PBAP_PCE_H__
#define _BT_MESSAGE_PBAP_PCE_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_pbap_pce.h"

    typedef union {
        bt_status_t status;
        profile_connection_state_t profile_conn_state;
        bool value_bool;
    } bt_pbap_pce_result_t;

    typedef union {
        struct {
            bt_address_t addr;
        } _bt_pbap_pce_connect,
            _bt_pbap_pce_disconnect;

        struct {
            bt_address_t addr;
            char dir[PBAP_PKT_LEN_MAX + 1];
        } _bt_pbap_pce_change_dir;

        struct {
            bt_address_t addr;
            pbap_search_property_t property;
            char value[PBAP_PKT_LEN_MAX + 1];
        } _bt_pbap_pce_pull_vcard_listing;

        struct {
            bt_address_t addr;
            char object[PBAP_PKT_LEN_MAX + 1];
            uint64_t filter;
        } _bt_pbap_pce_pull_vcard;
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
        } _on_vcard_listing_data_received;

        struct {
            bt_address_t addr;
            uint16_t status;
        } _on_vcard_listing_end;

        struct {
            bt_address_t addr;
            char object[PBAP_PKT_LEN_MAX + 1];
            uint16_t len;
        } _on_vcard_data_received;

        struct {
            bt_address_t addr;
            uint16_t status;
        } _on_vcard_end;
    } bt_message_pbap_pce_callbacks_t;
#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_PBAP_PCE_H__ */
