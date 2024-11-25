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

typedef struct adv_data {
    uint8_t len;
    uint8_t type;
    uint8_t data[0];
} adv_data_t;

typedef struct advertiser_data_ advertiser_data_t;
typedef void (*ad_dump_cb_t)(const char* str);

bool advertiser_data_dump(uint8_t* data, uint16_t len, ad_dump_cb_t dump);

advertiser_data_t* advertiser_data_new(void);
void advertiser_data_free(advertiser_data_t* ad);
uint8_t* advertiser_data_build(advertiser_data_t* ad, uint16_t* len);
void advertiser_data_set_name(advertiser_data_t* ad, const char* name);
void advertiser_data_set_flags(advertiser_data_t* ad, uint8_t flags);
void advertiser_data_set_appearance(advertiser_data_t* ad, uint16_t appearance);
void advertiser_data_add_data(advertiser_data_t* ad, uint8_t type, uint8_t* data, uint8_t len);
void advertiser_data_remove_data(advertiser_data_t* ad, uint8_t type, uint8_t* data, uint8_t len);
void advertiser_data_add_manufacture_data(advertiser_data_t* ad,
    uint16_t manufacture_id,
    uint8_t* data, uint8_t length);
bool advertiser_data_add_service_uuid(advertiser_data_t* ad, const bt_uuid_t* uuid);
bool advertiser_data_add_service_data(advertiser_data_t* ad,
    const bt_uuid_t* uuid,
    uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif
#endif /* __ADVERTISER_DATA_H__ */