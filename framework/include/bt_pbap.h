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

#ifndef __BT_PBAP_H__
#define __BT_PBAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include <stddef.h>

/* According to Generic Object Exchange Profile (GOEP) v2.1.1:
   The GOEP implementations shall support a minimum Maximum OBEX Packet Length (MOPL)
   size of 255 Bytes. */
#define PBAP_PKT_LEN_MAX 512

/**
 * @brief PCE SearchProperty header to indicate to the Server with vCard property
 *        the search operation shall be carrid out on.
 */
typedef enum {
    PBAP_SEARCH_PROPERTY_NAME = 0,
    PBAP_SEARCH_PROPERTY_NUMBER = 1,
    PBAP_SEARCH_PROPERTY_SOUND = 2,

    PCE_SEARCH_PROPERTY_NONE = 0xFF
} bt_pbap_search_property_t;

/**
 * @brief The Property Mask used when request vCard object.
 */
#define PBAP_PROPERTY_MASK_ALL (0) /* All properties */
#define PBAP_PROPERTY_MASK_VERSION (1L << 0) /* vCard Version */
#define PBAP_PROPERTY_MASK_FN (1L << 1) /* Formatted Name */
#define PBAP_PROPERTY_MASK_N (1L << 2) /* Structured Presentation of Name */
#define PBAP_PROPERTY_MASK_PHOTO (1L << 3) /* Associated Image or Photo */
#define PBAP_PROPERTY_MASK_BDAY (1L << 4) /* Birthday */
#define PBAP_PROPERTY_MASK_ADR (1L << 5) /* Delivery Address */
#define PBAP_PROPERTY_MASK_LABEL (1L << 6) /* Delivery */
#define PBAP_PROPERTY_MASK_TEL (1L << 7) /* Telephone Number */
#define PBAP_PROPERTY_MASK_EMAIL (1L << 8) /* Electronic Mail Address */
#define PBAP_PROPERTY_MASK_MAILER (1L << 9) /* Electronic Mail */
#define PBAP_PROPERTY_MASK_TZ (1L << 10) /* Time Zone */
#define PBAP_PROPERTY_MASK_GEO (1L << 11) /* Geographoc Position */
#define PBAP_PROPERTY_MASK_TITLE (1L << 12) /* Job */
#define PBAP_PROPERTY_MASK_ROLE (1L << 13) /* Role within the Organization */
#define PBAP_PROPERTY_MASK_LOGO (1L << 14) /* Organization Logo */
#define PBAP_PROPERTY_MASK_AGENT (1L << 15) /* vCard of Person Representing */
#define PBAP_PROPERTY_MASK_ORG (1L << 16) /* Name of Organization */
#define PBAP_PROPERTY_MASK_NOTE (1L << 17) /* Comments */
#define PBAP_PROPERTY_MASK_REV (1L << 18) /* Revision */
#define PBAP_PROPERTY_MASK_SOUND (1L << 19) /* Pronunciation of Name */
#define PBAP_PROPERTY_MASK_URL (1L << 20) /* Uniform Resource Locator */
#define PBAP_PROPERTY_MASK_UID (1L << 21) /* Unique ID */
#define PBAP_PROPERTY_MASK_KEY (1L << 22) /* Public Encryption Key */
#define PBAP_PROPERTY_MASK_NICKNAME (1L << 23) /* Nickname */
#define PBAP_PROPERTY_MASK_CATEGORIES (1L << 24) /* Categories */
#define PBAP_PROPERTY_MASK_PROID (1L << 25) /* Product ID */
#define PBAP_PROPERTY_MASK_CLASS (1L << 26) /* Class information */
#define PBAP_PROPERTY_MASK_SORT_STRING (1L << 27) /* String used for sorting operations */
#define PBAP_PROPERTY_MASK_X_IRMC_CALL_DATETIME (1L << 28) /* Time stamp */
#define PBAP_PROPERTY_MASK_X_BT_SPEEDDIALKEY (1L << 29) /* Speed-dial shortcut */
#define PBAP_PROPERTY_MASK_X_BT_UCI (1L << 30) /* Uniform Caller Identifier */
#define PBAP_PROPERTY_MASK_X_BT_UID (1L << 31) /* Bluetooth Contact Unique Identifier */

#ifdef __cplusplus
}
#endif

#endif /* __BT_PBAP_H__ */