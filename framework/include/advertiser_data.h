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
#ifndef __ADVERTISER_DATA_H__
#define __ADVERTISER_DATA_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdint.h>

#include "bt_uuid.h"

#define BT_AD_FLAGS 0x01 /* Flags */
#define BT_AD_UUID16_SOME 0x02 /* Incomplete List of 16­bit Service Class UUIDs */
#define BT_AD_UUID16_ALL 0x03 /* Complete List of 16­bit Service Class UUIDs */
#define BT_AD_UUID32_SOME 0x04 /* Incomplete List of 32­bit Service Class UUIDs */
#define BT_AD_UUID32_ALL 0x05 /* Complete List of 32­bit Service Class UUIDs */
#define BT_AD_UUID128_SOME 0x06 /* Incomplete List of 128­bit Service Class UUIDs */
#define BT_AD_UUID128_ALL 0x07 /* Complete List of 128­bit Service Class UUIDs */
#define BT_AD_NAME_SHORT 0x08 /* Shortened Local Name */
#define BT_AD_NAME_COMPLETE 0x09 /* Complete Local Name */
#define BT_AD_TX_POWER 0x0a /* Tx Power Level */
#define BT_AD_CLASS_OF_DEV 0x0d /* Class of Device */
#define BT_AD_SSP_HASH 0x0e /* Simple Pairing Hash C­192 */
#define BT_AD_SSP_RANDOMIZER 0x0f /* Simple Pairing Randomizer R­192 */
#define BT_AD_DEVICE_ID 0x10 /* Device ID */
#define BT_AD_SMP_TK 0x10 /* Security Manager TK Value */
#define BT_AD_SMP_OOB_FLAGS 0x11 /* Security Manager Out of Band Flags */
#define BT_AD_PERIPHERAL_CONN_INTERVAL 0x12 /* Peripheral Connection Interval Range */
#define BT_AD_SOLICIT16 0x14 /* List of 16­bit Service Solicitation UUIDs */
#define BT_AD_SOLICIT128 0x15 /* List of 128­bit Service Solicitation UUIDs */
#define BT_AD_SERVICE_DATA16 0x16 /* Service Data ­ 16­bit UUID */
#define BT_AD_PUBLIC_ADDRESS 0x17 /* Public Target Address */
#define BT_AD_RANDOM_ADDRESS 0x18 /* Random Target Address */
#define BT_AD_GAP_APPEARANCE 0x19 /* Appearance */
#define BT_AD_ADVERTISING_INTERVAL 0x1a /* Advertising Interval */
#define BT_AD_LE_DEVICE_ADDRESS 0x1b /* LE Bluetooth Device Address */
#define BT_AD_LE_ROLE 0x1c /* LE Role */
#define BT_AD_SSP_HASH_P256 0x1d /* Simple Pairing Hash C­256 */
#define BT_AD_SSP_RANDOMIZER_P256 0x1e /* Simple Pairing Randomizer R­256 */
#define BT_AD_SOLICIT32 0x1f /* List of 32­bit Service Solicitation UUIDs */
#define BT_AD_SERVICE_DATA32 0x20 /* Service Data ­ 32­bit UUID */
#define BT_AD_SERVICE_DATA128 0x21 /* Service Data ­ 128­bit UUID */
#define BT_AD_LE_SC_CONFIRM_VALUE 0x22 /* LE Secure Connections Confirmation Value */
#define BT_AD_LE_SC_RANDOM_VALUE 0x23 /* LE Secure Connections Random Value */
#define BT_AD_URI 0x24 /* URI */
#define BT_AD_INDOOR_POSITIONING 0x25 /* Indoor Positioning */
#define BT_AD_TRANSPORT_DISCOVERY 0x26 /* Transport Discovery Data */
#define BT_AD_LE_SUPPORTED_FEATURES 0x27 /* LE Supported Features */
#define BT_AD_CHANNEL_MAP_UPDATE_IND 0x28 /* Channel Map Update Indication */
#define BT_AD_MESH_PROV 0x29 /* PB­ADV */
#define BT_AD_MESH_DATA 0x2a /* Mesh Message */
#define BT_AD_MESH_BEACON 0x2b /* Mesh Beacon */
#define BT_AD_BIG_INFO 0x2C /* BIGInfo */
#define BT_AD_BROADCAST_CODE 0x2D /* Broadcast_Code */
#define BT_AD_RESOLVABLE_SET_IDENTIFIER 0x2E /* Resolvable Set Identifier */
#define BT_AD_ADV_INTERVAL_LONG 0x2F /* Advertising Interval ­ long */
#define BT_AD_BROADCAST_NAME 0x30 /* Broadcast_Name */
#define BT_AD_ENCRYPTED_ADV_DATA 0x31 /* Encrypted Advertising Data */
#define BT_AD_PERIODIC_ADV_RSP_TIMING 0x32 /* Periodic Advertising Response Timing Information */
#define BT_AD_3D_INFO_DATA 0x3d /* 3D Information Data */
#define BT_AD_MANUFACTURER_DATA 0xff /* Manufacturer Specific Data */

#define BT_LE_AD_NAME_LEN 29

#define BT_AD_FLAG_LIMITED_DISCOVERABLE 1
#define BT_AD_FLAG_GENERAL_DISCOVERABLE 2
#define BT_AD_FLAG_BREDR_NOT_SUPPORT 4
#define BT_AD_FLAG_DUAL_MODE 8

/**
 * @cond
 */
typedef struct adv_data {
    uint8_t len;
    uint8_t type;
    uint8_t data[0];
} adv_data_t;
/**
 * @endcond
 */

typedef struct advertiser_data_ advertiser_data_t;

/**
 * @brief Advertising data dump callback function.
 *
 * Function prototype for dumping advertising data in string format.
 *
 * @param str - Null-terminated string to dump.
 */
typedef void (*ad_dump_cb_t)(const char* str);

/**
 * @brief Dump advertising data.
 *
 * Parses the advertising data and outputs it using the provided dump callback.
 *
 * @param data - Pointer to the advertising data buffer.
 * @param len - Length of the advertising data buffer.
 * @param dump - Callback function to output the parsed data.
 * @return true - Parsing and dumping was successful.
 * @return false - Parsing failed.
 *
 * **Example:**
 * @code
static void on_scan_result_cb(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    PRINT_ADDR("ScanResult ------[%s]------", &result->addr);
    PRINT("AddrType:%d", result->addr_type);
    PRINT("Rssi:%d", result->rssi);
    PRINT("Type:%d", result->adv_type);
    advertiser_data_dump((uint8_t*)result->adv_data, result->length, NULL);
    PRINT("\n");
}
 * @endcode
 */
bool advertiser_data_dump(uint8_t* data, uint16_t len, ad_dump_cb_t dump);

/**
 * @brief Create a new advertiser data object.
 *
 * Allocates and initializes a new advertiser data object.
 *
 * @return advertiser_data_t* - Pointer to the new advertiser data object; NULL on failure.
 *
 * **Example:**
 * @code
advertiser_data_t* ad = advertiser_data_new();
if (ad == NULL) {
    // Handle allocation failure
}
 * @endcode
 */
advertiser_data_t* advertiser_data_new(void);

/**
 * @brief Free an advertiser data object.
 *
 * Frees the memory associated with an advertiser data object.
 *
 * @param ad - Pointer to the advertiser data object to free.
 *
 * **Example:**
 * @code
advertiser_data_free(ad);
 * @endcode
 */
void advertiser_data_free(advertiser_data_t* ad);

/**
 * @brief Build the advertising data buffer.
 *
 * Constructs the advertising data buffer from the advertiser data object.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param[out] len - Pointer to store the length of the advertising data buffer.
 * @return uint8_t* - Pointer to the advertising data buffer; NULL on failure.
 *
 * **Note:** The returned buffer should be freed by the caller when no longer needed.
 *
 * **Example:**
 * @code
uint16_t len;
uint8_t* data = advertiser_data_build(ad, &len);
if (data != NULL) {
    // Use the advertising data buffer
    free(data);
}
 * @endcode
 */
uint8_t* advertiser_data_build(advertiser_data_t* ad, uint16_t* len);

/**
 * @brief Set the device name in advertising data.
 *
 * Sets the local device name to be included in the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param name - Null-terminated string containing the device name.
 *
 * **Example:**
 * @code
advertiser_data_set_name(ad, "My Device");
 * @endcode
 */
void advertiser_data_set_name(advertiser_data_t* ad, const char* name);

/**
 * @brief Set the advertising flags.
 *
 * Sets the advertising flags to be included in the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param flags - Advertising flags, see BT_AD_FLAG_* definitions.
 *
 * **Example:**
 * @code
advertiser_data_set_flags(ad, BT_AD_FLAG_GENERAL_DISCOVERABLE | BT_AD_FLAG_BREDR_NOT_SUPPORT);
 * @endcode
 */
void advertiser_data_set_flags(advertiser_data_t* ad, uint8_t flags);

/**
 * @brief Set the GAP appearance.
 *
 * Sets the GAP appearance value to be included in the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param appearance - Appearance value, see GAP Appearance Values.
 *
 * **Example:**
 * @code
advertiser_data_set_appearance(ad, 0x03C0); // HID Generic
 * @endcode
 */
void advertiser_data_set_appearance(advertiser_data_t* ad, uint16_t appearance);

/**
 * @brief Add custom data to advertising data.
 *
 * Adds a custom AD structure to the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param type - AD type, see BT_AD_* definitions.
 * @param data - Pointer to the data payload.
 * @param len - Length of the data payload.
 *
 * **Example:**
 * @code
uint8_t custom_data[] = {0x01, 0x02, 0x03};
advertiser_data_add_data(ad, BT_AD_MANUFACTURER_DATA, custom_data, sizeof(custom_data));
 * @endcode
 */
void advertiser_data_add_data(advertiser_data_t* ad, uint8_t type, uint8_t* data, uint8_t len);

/**
 * @brief Remove custom data from advertising data.
 *
 * Removes a custom AD structure from the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param type - AD type, see BT_AD_* definitions.
 * @param data - Pointer to the data payload.
 * @param len - Length of the data payload.
 *
 * **Example:**
 * @code
advertiser_data_remove_data(ad, BT_AD_MANUFACTURER_DATA, custom_data, sizeof(custom_data));
 * @endcode
 */
void advertiser_data_remove_data(advertiser_data_t* ad, uint8_t type, uint8_t* data, uint8_t len);

/**
 * @brief Add manufacturer-specific data to advertising data.
 *
 * Adds manufacturer-specific data to the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param manufacture_id - Manufacturer ID assigned by the Bluetooth SIG.
 * @param data - Pointer to the manufacturer-specific data.
 * @param length - Length of the manufacturer-specific data.
 *
 * **Example:**
 * @code
uint8_t manuf_data[] = {0x12, 0x34, 0x56};
advertiser_data_add_manufacture_data(ad, 0x038F, manuf_data, sizeof(manuf_data)); // 0x038F is Xiaomi Inc.
 * @endcode
 */
void advertiser_data_add_manufacture_data(advertiser_data_t* ad,
    uint16_t manufacture_id,
    uint8_t* data, uint8_t length);

/**
 * @brief Add a service UUID to advertising data.
 *
 * Adds a service UUID to the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param uuid - Pointer to the UUID to add, see @ref bt_uuid_t.
 * @return true - UUID added successfully.
 * @return false - Failed to add UUID.
 *
 * **Example:**
 * @code
bt_uuid_t service_uuid;
bt_uuid_from_string("0000180D-0000-1000-8000-00805F9B34FB", &service_uuid); // Heart Rate Service
advertiser_data_add_service_uuid(ad, &service_uuid);
 * @endcode
 */
bool advertiser_data_add_service_uuid(advertiser_data_t* ad, const bt_uuid_t* uuid);

/**
 * @brief Add service data to advertising data.
 *
 * Adds service-specific data associated with a service UUID to the advertising data.
 *
 * @param ad - Pointer to the advertiser data object.
 * @param uuid - Pointer to the UUID of the service, see @ref bt_uuid_t.
 * @param data - Pointer to the service data payload.
 * @param len - Length of the service data payload.
 * @return true - Service data added successfully.
 * @return false - Failed to add service data.
 *
 * **Example:**
 * @code
bt_uuid_t service_uuid;
bt_uuid_from_string("0000180F-0000-1000-8000-00805F9B34FB", &service_uuid); // Battery Service
uint8_t battery_level = 85; // 85%
advertiser_data_add_service_data(ad, &service_uuid, &battery_level, 1);
 * @endcode
 */
bool advertiser_data_add_service_data(advertiser_data_t* ad,
    const bt_uuid_t* uuid,
    uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif
#endif /* __ADVERTISER_DATA_H__ */