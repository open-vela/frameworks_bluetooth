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
#include <stdbool.h>
#include <stdint.h>

#include "advertiser_data.h"
#include "bt_debug.h"

typedef struct {
    uint8_t ad_type;
    const char* desc;
} ad_type_desc_t;

static const ad_type_desc_t ad_type_map[] = {
    { BT_AD_FLAGS, "Flags" },
    { BT_AD_UUID16_SOME, "Incomplete List of 16­bit Service Class UUIDs" },
    { BT_AD_UUID16_ALL, "Complete List of 16­bit Service Class UUIDs" },
    { BT_AD_UUID32_SOME, "Incomplete List of 32­bit Service Class UUIDs" },
    { BT_AD_UUID32_ALL, "Complete List of 32­bit Service Class UUIDs" },
    { BT_AD_UUID128_SOME, "Incomplete List of 128­bit Service Class UUIDs" },
    { BT_AD_UUID128_ALL, "Complete List of 128­bit Service Class UUIDs" },
    { BT_AD_NAME_SHORT, "Shortened Local Name" },
    { BT_AD_NAME_COMPLETE, "Complete Local Name" },
    { BT_AD_TX_POWER, "Tx Power Level" },
    { BT_AD_CLASS_OF_DEV, "Class of Device" },
    { BT_AD_SSP_HASH, "Simple Pairing Hash C­192" },
    { BT_AD_SSP_RANDOMIZER, "Simple Pairing Randomizer R­192" },
    { BT_AD_SMP_TK, "Security Manager TK Value" },
    { BT_AD_SMP_OOB_FLAGS, "Security Manager Out of Band Flags" },
    { BT_AD_PERIPHERAL_CONN_INTERVAL, "Peripheral Connection Interval Range" },
    { BT_AD_SOLICIT16, "List of 16­bit Service Solicitation UUIDs" },
    { BT_AD_SOLICIT128, "List of 128­bit Service Solicitation UUIDs" },
    { BT_AD_SERVICE_DATA16, "Service Data ­ 16­bit UUID" },
    { BT_AD_PUBLIC_ADDRESS, "Public Target Address" },
    { BT_AD_RANDOM_ADDRESS, "Random Target Address" },
    { BT_AD_GAP_APPEARANCE, "Appearance" },
    { BT_AD_ADVERTISING_INTERVAL, "Advertising Interval" },
    { BT_AD_LE_DEVICE_ADDRESS, "LE Bluetooth Device Address" },
    { BT_AD_LE_ROLE, "LE Role" },
    { BT_AD_SSP_HASH_P256, "Simple Pairing Hash C­256" },
    { BT_AD_SSP_RANDOMIZER_P256, "Simple Pairing Randomizer R­256" },
    { BT_AD_SOLICIT32, "List of 32­bit Service Solicitation UUIDs" },
    { BT_AD_SERVICE_DATA32, "Service Data ­ 32­bit UUID" },
    { BT_AD_SERVICE_DATA128, "Service Data ­ 128­bit UUID" },
    { BT_AD_LE_SC_CONFIRM_VALUE, "LE Secure Connections Confirmation Value" },
    { BT_AD_LE_SC_RANDOM_VALUE, "LE Secure Connections Random Value" },
    { BT_AD_URI, "URI" },
    { BT_AD_INDOOR_POSITIONING, "Indoor Positioning" },
    { BT_AD_TRANSPORT_DISCOVERY, "Transport Discovery Data" },
    { BT_AD_LE_SUPPORTED_FEATURES, "LE Supported Features" },
    { BT_AD_CHANNEL_MAP_UPDATE_IND, "Channel Map Update Indication" },
    { BT_AD_MESH_PROV, "PB­ADV" },
    { BT_AD_MESH_DATA, "Mesh Message" },
    { BT_AD_MESH_BEACON, "Mesh Beacon" },
    { BT_AD_BIG_INFO, "BIGInfo" },
    { BT_AD_BROADCAST_CODE, "Broadcast_Code" },
    { BT_AD_RESOLVABLE_SET_IDENTIFIER, "Resolvable Set Identifier" },
    { BT_AD_ADV_INTERVAL_LONG, "Advertising Interval ­ long" },
    { BT_AD_BROADCAST_NAME, "Broadcast_Name" },
    { BT_AD_ENCRYPTED_ADV_DATA, "Encrypted Advertising Data" },
    { BT_AD_PERIODIC_ADV_RSP_TIMING, "Periodic Advertising Response Timing Information" },
    { BT_AD_3D_INFO_DATA, "3D Information Data" },
    { BT_AD_MANUFACTURER_DATA, "Manufacturer Specific Data" },
};

static const char* show_ad_type_desc(uint8_t type)
{
    for (int i = 0; i < sizeof(ad_type_map) / sizeof(ad_type_map[0]); i++) {
        if (ad_type_map[i].ad_type == type)
            return ad_type_map[i].desc;
    }

    return "Unknown";
}

static void advertiser_data_info(adv_data_t* ad)
{
    if (ad->len < 1) {
        return;
    }

    syslog(4, "AdvType:(%s)\n", show_ad_type_desc(ad->type));
    lib_dumpbuffer("AdvData:", ad->data, ad->len - 1);

    switch (ad->type) {
    case BT_AD_FLAGS:
        break;
    case BT_AD_UUID16_SOME:
        break;
    case BT_AD_UUID16_ALL:
        break;
    case BT_AD_UUID32_SOME:
        break;
    case BT_AD_UUID32_ALL:
        break;
    case BT_AD_UUID128_SOME:
        break;
    case BT_AD_UUID128_ALL:
        break;
    case BT_AD_NAME_SHORT:
        break;
    case BT_AD_NAME_COMPLETE:
        break;
    case BT_AD_TX_POWER:
        break;
    case BT_AD_CLASS_OF_DEV:
        break;
    case BT_AD_SSP_HASH:
        break;
    case BT_AD_SSP_RANDOMIZER:
        break;
    case BT_AD_SMP_TK:
        /* if ad->len == 8, ad type is SMP_TK */
        break;
    case BT_AD_SMP_OOB_FLAGS:
        break;
    case BT_AD_PERIPHERAL_CONN_INTERVAL:
        break;
    case BT_AD_SOLICIT16:
        break;
    case BT_AD_SOLICIT128:
        break;
    case BT_AD_SERVICE_DATA16:
        break;
    case BT_AD_PUBLIC_ADDRESS:
        break;
    case BT_AD_RANDOM_ADDRESS:
        break;
    case BT_AD_GAP_APPEARANCE:
        break;
    case BT_AD_ADVERTISING_INTERVAL:
        break;
    case BT_AD_LE_DEVICE_ADDRESS:
        break;
    case BT_AD_LE_ROLE:
        break;
    case BT_AD_SSP_HASH_P256:
        break;
    case BT_AD_SSP_RANDOMIZER_P256:
        break;
    case BT_AD_SOLICIT32:
        break;
    case BT_AD_SERVICE_DATA32:
        break;
    case BT_AD_SERVICE_DATA128:
        break;
    case BT_AD_LE_SC_CONFIRM_VALUE:
        break;
    case BT_AD_LE_SC_RANDOM_VALUE:
        break;
    case BT_AD_URI:
        break;
    case BT_AD_INDOOR_POSITIONING:
        break;
    case BT_AD_TRANSPORT_DISCOVERY:
        break;
    case BT_AD_LE_SUPPORTED_FEATURES:
        break;
    case BT_AD_CHANNEL_MAP_UPDATE_IND:
        break;
    case BT_AD_MESH_PROV:
        break;
    case BT_AD_MESH_DATA:
        break;
    case BT_AD_MESH_BEACON:
        break;
    case BT_AD_BIG_INFO:
        break;
    case BT_AD_BROADCAST_CODE:
        break;
    case BT_AD_RESOLVABLE_SET_IDENTIFIER:
        break;
    case BT_AD_ADV_INTERVAL_LONG:
        break;
    case BT_AD_BROADCAST_NAME:
        break;
    case BT_AD_ENCRYPTED_ADV_DATA:
        break;
    case BT_AD_PERIODIC_ADV_RSP_TIMING:
        break;
    case BT_AD_3D_INFO_DATA:
        break;
    case BT_AD_MANUFACTURER_DATA:
        break;
    default:
        break;
    }
}

bool advertiser_data_dump(uint8_t* data, uint16_t len, ad_dump_cb_t dump)
{
    uint16_t offset = 0;

    while (offset < len) {
        adv_data_t* ad = (adv_data_t*)&data[offset];

        advertiser_data_info(ad);
        offset += ad->len + 1;
    };

    return true;
}
