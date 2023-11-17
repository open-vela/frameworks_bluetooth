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

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_pbap.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief PCE connection state changed callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param bd_addr - address of peer device.
 * @param state - pce connection state
 */
typedef void (*pce_connection_state_callback)(void* cookie, bt_address_t* bd_addr, profile_connection_state_t state);

/**
 * @brief PCE directory changed callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param bd_addr - address of peer device.
 * @param status - directory changed status. 0 on success, otherwise failed.
 */
typedef void (*pce_dir_changed_callback)(void* cookie, bt_address_t* bd_addr, uint16_t status);

/**
 * @brief PCE vCard Listing data received callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param bd_addr - address of peer device.
 * @param len - length, in Bytes, of data.
 * @param data - the UTF-8 Coded vCard-listing object.
 */
typedef void (*pce_vcard_listing_data_callback)(void* cookie, bt_address_t* bd_addr, uint16_t len, char* data);

/**
 * @brief PCE vCard Listing finished callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param bd_addr - address of peer device.
 * @param status - status for vCard Listing request. 0 on success, otherwise failed.
 */
typedef void (*pce_vcard_listing_end_callback)(void* cookie, bt_address_t* bd_addr, uint16_t status);

/**
 * @brief PCE vCard data received callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param bd_addr - address of peer device.
 * @param len - length, in Bytes, of data.
 * @param data - the UTF-8 Coded vCard object.
 */
typedef void (*pce_vcard_data_callback)(void* cookie, bt_address_t* bd_addr, uint16_t len, char* data);

/**
 * @brief PCE vCard finished callback.
 *
 * @param cookie - callbacks cookie, the return value of bt_pbap_pce_register_callbacks.
 * @param bd_addr - address of peer device.
 * @param status - status for vCard request. 0 on success, otherwise failed.
 */
typedef void (*pce_vcard_end_callback)(void* cookie, bt_address_t* bd_addr, uint16_t status);

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
} pbap_pce_callbacks_t;

/**
 * @brief The Property Mask used when request vCard object.
 */
#define PCE_PROPERTY_MASK_ALL (0) /* All properties */
#define PCE_PROPERTY_MASK_VERSION (1L << 0) /* vCard Version */
#define PCE_PROPERTY_MASK_FN (1L << 1) /* Formatted Name */
#define PCE_PROPERTY_MASK_N (1L << 2) /* Structured Presentation of Name */
#define PCE_PROPERTY_MASK_PHOTO (1L << 3) /* Associated Image or Photo */
#define PCE_PROPERTY_MASK_BDAY (1L << 4) /* Birthday */
#define PCE_PROPERTY_MASK_ADR (1L << 5) /* Delivery Address */
#define PCE_PROPERTY_MASK_LABEL (1L << 6) /* Delivery */
#define PCE_PROPERTY_MASK_TEL (1L << 7) /* Telephone Number */
#define PCE_PROPERTY_MASK_EMAIL (1L << 8) /* Electronic Mail Address */
#define PCE_PROPERTY_MASK_MAILER (1L << 9) /* Electronic Mail */
#define PCE_PROPERTY_MASK_TZ (1L << 10) /* Time Zone */
#define PCE_PROPERTY_MASK_GEO (1L << 11) /* Geographoc Position */
#define PCE_PROPERTY_MASK_TITLE (1L << 12) /* Job */
#define PCE_PROPERTY_MASK_ROLE (1L << 13) /* Role within the Organization */
#define PCE_PROPERTY_MASK_LOGO (1L << 14) /* Organization Logo */
#define PCE_PROPERTY_MASK_AGENT (1L << 15) /* vCard of Person Representing */
#define PCE_PROPERTY_MASK_ORG (1L << 16) /* Name of Organization */
#define PCE_PROPERTY_MASK_NOTE (1L << 17) /* Comments */
#define PCE_PROPERTY_MASK_REV (1L << 18) /* Revision */
#define PCE_PROPERTY_MASK_SOUND (1L << 19) /* Pronunciation of Name */
#define PCE_PROPERTY_MASK_URL (1L << 20) /* Uniform Resource Locator */
#define PCE_PROPERTY_MASK_UID (1L << 21) /* Unique ID */
#define PCE_PROPERTY_MASK_KEY (1L << 22) /* Public Encryption Key */
#define PCE_PROPERTY_MASK_NICKNAME (1L << 23) /* Nickname */
#define PCE_PROPERTY_MASK_CATEGORIES (1L << 24) /* Categories */
#define PCE_PROPERTY_MASK_PROID (1L << 25) /* Product ID */
#define PCE_PROPERTY_MASK_CLASS (1L << 26) /* Class information */
#define PCE_PROPERTY_MASK_SORT_STRING (1L << 27) /* String used for sorting operations */
#define PCE_PROPERTY_MASK_X_IRMC_CALL_DATETIME (1L << 28) /* Time stamp */
#define PCE_PROPERTY_MASK_X_BT_SPEEDDIALKEY (1L << 29) /* Speed-dial shortcut */
#define PCE_PROPERTY_MASK_X_BT_UCI (1L << 30) /* Uniform Caller Identifier */
#define PCE_PROPERTY_MASK_X_BT_UID (1L << 31) /* Bluetooth Contact Unique Identifier */

/**
 * @brief Register callback functions for PCE service.
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - phone book client callback functions.
 * @return void* - callback cookie, NULL on failure.
 */
void* BTSYMBOLS(bt_pbap_pce_register_callbacks)(bt_instance_t* ins, const pbap_pce_callbacks_t* callbacks);

/**
 * @brief Unregister PCE callback function.
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callbacks cookie.
 * @return true - on callback unregister success.
 * @return false - on callback cookie not found.
 */
bool BTSYMBOLS(bt_pbap_pce_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Connect to the phone book server.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from a phone book server.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Change directory at the phone book server.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @param dir - the child directory to be changed, or NULL to the parent directory.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_change_directory)(bt_instance_t* ins, bt_address_t* addr, const char* dir);

/**
 * @brief Retrive the vCard Listing via specific property.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @param property - the vCard property that the search operation shall be carried out on.
 *                   PCE_SEARCH_PROPERTY_NONE if no property is specified, all the vCards
 *                   would be returned in this case.
 * @param value - the value to query, UTF-8 string terminated by '\0'.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_pull_vcard_listing)(bt_instance_t* ins, bt_address_t* addr, pbap_search_property_t property, const char* value);

/**
 * @brief Retrive a specific vCard Entry via vCard name.
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @param object - vCard name, shall be Object name (*.vcf) or X-BT-UID (X-BT-UID:*).
 * @param filter - a bitwise value used to indicate the properties contained in the requested
 *                 vCard objects, e.g., PCE_PROPERTY_MASK_N | PCE_PROPERTY_MASK_TEL.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negative errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_pbap_pce_pull_vcard)(bt_instance_t* ins, bt_address_t* addr, const char* object, uint64_t filter);

#endif /* __BT_PBAP_PCE_H__ */