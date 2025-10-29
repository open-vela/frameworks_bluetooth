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
#ifndef __SAL_PBAP_PCE_INTERFACE_H__
#define __SAL_PBAP_PCE_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "pbap_pce_service.h"

typedef enum {
    PBAP_SEARCH_PROPERTY_NAME = 0,
    PBAP_SEARCH_PROPERTY_NUMBER = 1,
    PBAP_SEARCH_PROPERTY_SOUND = 2,

    PCE_SEARCH_PROPERTY_NONE = 0xFF
} pbap_search_property_t;

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

bt_status_t bt_sal_pbap_pce_init(void);
void bt_sal_pbap_pce_cleanup(void);
bt_status_t bt_sal_pbap_pce_connect(bt_address_t* addr);
bt_status_t bt_sal_pbap_pce_disconnect(bt_address_t* addr);
bt_status_t bt_sal_pbap_pce_change_directory(bt_address_t* addr, const char* dir);
bt_status_t bt_sal_pbap_pce_pull_vcard_listing(bt_address_t* addr, pbap_search_property_t property,
    const char* value);
bt_status_t bt_sal_pbap_pce_pull_vcard(bt_address_t* addr, const char* object, uint64_t filter);

#endif /* __SAL_PBAP_PCE_INTERFACE_H__ */
