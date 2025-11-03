/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#ifndef __BT_PBAP_PCE_H__
#define __BT_PBAP_PCE_H__

#include <stddef.h>

#include "bt_pbap.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

#define BT_PBAP_PCE_PROPERTY_MAX_LEN 32
#define BT_PBAP_PCE_VCARD_HANDLE_MAX_LEN 64
#define BT_PBAP_PCE_NUMBER_MAX_COUNT 2

typedef struct {
    char name[BT_PBAP_PCE_PROPERTY_MAX_LEN];
    char numbers[BT_PBAP_PCE_PROPERTY_MAX_LEN][BT_PBAP_PCE_NUMBER_MAX_COUNT];
} bt_pce_contact_t;

/**
 * @brief PCE connection state changed callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param addr - address of peer device.
 * @param state - pce connection state.
 */
typedef void (*pce_connection_state_callback)(void* cookie, bt_address_t* addr,
    profile_connection_state_t state);

/**
 * @brief PCE directory changed callback. This callback may be generated after calling
 *        @ref bt_pbap_pce_change_directory.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param addr - address of peer device.
 * @param status - directory changed status. 0 on success, otherwise failed.
 */
typedef void (*pce_dir_changed_callback)(void* cookie, bt_address_t* addr, uint16_t status);

/**
 * @brief PCE vCard Listing data received callback. This callback may be generated after calling
 *        @ref bt_pbap_pce_pull_vcard_listing.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param addr - address of peer device.
 * @param len - length, in Bytes, of data.
 * @param data - the UTF-8 Coded vCard-listing object.
 */
typedef void (*pce_vcard_listing_data_callback)(void* cookie, bt_address_t* addr, uint16_t len,
    const char* data);

/**
 * @brief PCE vCard Listing finished callback. This callback is generated as a result of
 *        @ref bt_pbap_pce_pull_vcard_listing.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param addr - address of peer device.
 * @param status - status for vCard Listing request. 0 on success, otherwise failed.
 */
typedef void (*pce_vcard_listing_end_callback)(void* cookie, bt_address_t* addr, uint16_t status);

/**
 * @brief PCE vCard data received callback. This callback may be generated after calling
 *        @ref bt_pbap_pce_pull_vcard.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param addr - address of peer device.
 * @param len - length, in Bytes, of data.
 * @param data - the UTF-8 Coded vCard object.
 */
typedef void (*pce_vcard_data_callback)(void* cookie, bt_address_t* addr, uint16_t len,
    const char* data);

/**
 * @brief PCE vCard finished callback. This callback is generated as a result of
 *        @ref bt_pbap_pce_pull_vcard.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param addr - address of peer device.
 * @param status - status for vCard request. 0 on success, otherwise failed.
 */
typedef void (*pce_vcard_end_callback)(void* cookie, bt_address_t* addr, uint16_t status);

/**
 * @brief PCE vCard callback. This callback is generated as a result of
 *        @ref bt_pbap_pce_get_contact_by_name or @ref bt_pbap_pce_get_contact_by_number.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param status - status of the operation.
 * @param property - the property that is requested.
 * @param value - the property value for geting a contact.
 * @param contact - the contact information.
 */
typedef void (*pce_contact_report_callback)(void* cookie, bt_status_t status,
    bt_pbap_search_property_t property, const char* value, const bt_pce_contact_t* contact);

/**
 * @brief PCE event callbacks structure
 */
typedef struct {
    size_t size;
    pce_connection_state_callback connection_state_cb;
    pce_dir_changed_callback dir_changed_cb;
    pce_vcard_listing_data_callback vcard_listing_data_cb;
    pce_vcard_listing_end_callback vcard_listing_end_cb;
    pce_vcard_data_callback vcard_data_cb;
    pce_vcard_end_callback vcard_end_cb;
    pce_contact_report_callback contact_report_cb;
} pbap_pce_callbacks_t;

/**
 * @brief Register callback functions for PCE service.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] callbacks - phone book client callback functions.
 * @return void* - callback cookie, NULL on failure.
 */
void* BTSYMBOLS(bt_pbap_pce_register_callbacks)(bt_instance_t* ins,
    const pbap_pce_callbacks_t* callbacks);

/**
 * @brief Unregister PCE callback function.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] cookie - callbacks cookie.
 * @return true - on callback unregister success.
 * @return false - on callback cookie not found.
 */
bool BTSYMBOLS(bt_pbap_pce_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Connect to the phone book server.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from a phone book server.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Change directory at the phone book server. A @ref pce_dir_changed_callback may be
 *        generated when the directory is changed.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - address of peer device.
 * @param[in] dir - the child directory to be changed, or NULL to the parent directory.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_change_directory)(bt_instance_t* ins, bt_address_t* addr,
    const char* dir);

/**
 * @brief Retrive the vCard Listing via specific property. Several
 *        @ref pce_vcard_listing_data_callback may be invoked before a
 *        @ref pce_vcard_listing_end_callback is received.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - address of peer device.
 * @param[in] property - the vCard property that the search operation shall be carried out on.
 *                       PCE_SEARCH_PROPERTY_NONE if no property is specified, all the vCards
 *                       would be returned in this case.
 * @param[in] value - the value to query, UTF-8 string terminated by '\0'.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_pull_vcard_listing)(bt_instance_t* ins, bt_address_t* addr,
    bt_pbap_search_property_t property, const char* value);

/**
 * @brief Retrive a specific vCard Entry via vCard name.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - address of peer device.
 * @param[in] object - vCard name, shall be Object name (*.vcf) or X-BT-UID (X-BT-UID:*).
 * @param[in] filter - a bitwise value used to indicate the properties contained in the requested
 *                     vCard objects, e.g., PBAP_PROPERTY_MASK_N | PBAP_PROPERTY_MASK_TEL.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_pull_vcard)(bt_instance_t* ins, bt_address_t* addr,
    const char* object, uint64_t filter);

/**
 * @brief Retrive a specific contact by its name.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - address of peer device.
 * @param[in] name - name of the contact.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_get_contact_by_name)(bt_instance_t* ins, bt_address_t* addr,
    const char* name);

/**
 * @brief Retrive a specific contact by its phone number.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - address of peer device.
 * @param[in] number - phone number of the contact.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_get_contact_by_number)(bt_instance_t* ins, bt_address_t* addr,
    const char* number);

/**
 * @brief set an Bluetooth address to the blacklist.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - The Bluetooth address is added to the blacklist.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_add_to_blacklist)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief remove an Bluetooth address to the blacklist.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - The Bluetooth address that is removed from the blacklist.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */

bt_status_t BTSYMBOLS(bt_pbap_pce_remove_from_blacklist)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief check if an Bluetooth address is in the blacklist.
 *
 * @param[in] ins - Bluetooth client instance.
 * @param[in] addr - The Bluetooth address to check.
 * @return true - address in blacklist.
 * @return false - address not in blacklist.
 */
bool BTSYMBOLS(bt_pbap_pce_is_in_blacklist)(bt_instance_t* ins, bt_address_t* addr);

#endif /* __BT_PBAP_PCE_H__ */