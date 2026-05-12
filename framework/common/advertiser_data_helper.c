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
#ifdef CONFIG_BLUETOOTH_PA_SYNC
#include "bt_pa_sync.h"
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
#include "bt_auracast_sink.h"
#endif
#endif
#include "bt_utils.h"

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

bool advertiser_data_parse(const uint8_t* data, uint8_t len, ad_parse_cb_t cb, void* context)
{
    bool ret = true;
    adv_data_t* ad;
    uint16_t offset = 0;

    if (!cb)
        return false;

    while (offset < len) {
        ad = (adv_data_t*)&data[offset];
        if (ad->len == 0) { /**< AD Type does not exist */
            offset += sizeof(ad->len); /**< Skip this entry */
            continue; /**< Goto the next item */
        }

        offset += sizeof(ad->len) + ad->len;
        if (offset > len)
            return false; /**< Incomplete AD Data */

        if (cb(ad, context) == false)
            ret = false;
    };

    return ret;
}

#ifdef CONFIG_BLUETOOTH_PA_SYNC
static bool bt_pa_sync_adv_data_parse_uuid_16(bt_pa_sync_info_t* info, uint16_t uuid_16,
    const adv_data_t* data)
{
    const uint8_t* p = data->data + sizeof(uint16_t);
    (void)p; /** Maybe unused */
    switch (uuid_16) {
#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    case BT_UUID_BROADCAST_AUDIO_ANNOUNCEMENT:
        if (data->len < 1 + sizeof(uuid_16) + 3)
            return false; /* less than AD Type (1 octet) + UUID16 (2 octets) + Broadcast ID (3 octets)*/

        STREAM_TO_UINT24(info->broadcast_id, p);
        break;
#endif
    default:
        break;
    }

    return true;
}

static bool bt_pa_sync_adv_data_parsed(const adv_data_t* data, void* context)
{
    bt_pa_sync_info_t* info = (bt_pa_sync_info_t*)context;
    const uint8_t* p = data->data;
    uint16_t uuid_16;

    switch (data->type) {
    case BT_AD_NAME_SHORT:
    case BT_AD_NAME_COMPLETE:
        strlcpy(info->name, (char*)data->data, MIN(sizeof(info->name), data->len - 1));
        break;

#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
    case BT_AD_BROADCAST_NAME:
        strlcpy(info->broadcast_name, (char*)data->data,
            MIN(sizeof(info->broadcast_name), data->len - 1));
        break;
#endif

    case BT_AD_SERVICE_DATA16:
        if (data->len < 1 + sizeof(uuid_16))
            return false; /**< less than AD Type (1 octet) + UUID16 (2 octets) */

        STREAM_TO_UINT16(uuid_16, p);
        return bt_pa_sync_adv_data_parse_uuid_16(info, uuid_16, data);
    default:
        break;
    }

    return true;
}

bt_status_t bt_pa_sync_parse_adv_data(bt_pa_sync_info_t* info, const ble_scan_result_t* result)
{
    memset(info, 0x00, sizeof(bt_pa_sync_info_t));
    info->broadcast_id = BT_INVALID_BROADCAST_ID;
    if (!advertiser_data_parse(result->adv_data, result->length, bt_pa_sync_adv_data_parsed, info))
        return BT_STATUS_FAIL;

    return result->flags & SCAN_RESULT_FLAG_PERIODIC_ADVERTISING
        ? BT_STATUS_SUCCESS
        : BT_STATUS_NOT_FOUND;
}

#ifdef CONFIG_BLUETOOTH_AURACAST_SINK
static bool bt_auracast_sink_adv_data_parse_lc3_config(bt_auracast_audio_lc3_config_t* lc3,
    uint8_t length, const uint8_t* p)
{
    /** Series of Length-Type-Value structures */
    uint8_t l, t;

    while (length) {
        STREAM_TO_UINT8(l, p);
        if (l == 0) {
            length--;
            continue; /**< invalid but not fatal */
        }

        if (length < (1 + l))
            return false; /**< length not enough */

        STREAM_TO_UINT8(t, p); /**< Type */
        switch (t) {
        case BT_CODEC_CONFIG_FREQUENCY_TYPE:
            if (l != BT_CODEC_CONFIG_FREQUENCY_LEN)
                return false;

            STREAM_TO_UINT8(lc3->sampling_frequency, p);
            if (lc3->sampling_frequency > BT_CODEC_CONFIG_FREQUENCY_384000)
                return false;

            break;
        case BT_CODEC_CONFIG_DURATION_TYPE:
            if (l != BT_CODEC_CONFIG_DURATION_LEN)
                return false;

            STREAM_TO_UINT8(lc3->duration, p);
            if (lc3->duration > BT_CODEC_CONFIG_DURATION_10_MS)
                return false;

            break;
        case BT_CODEC_CONFIG_ALLOCATION_TYPE:
            if (l != BT_CODEC_CONFIG_ALLOCATION_LEN)
                return false;

            STREAM_TO_UINT32(lc3->location, p);
            break;
        case BT_CODEC_CONFIG_OCTETS_PER_FRAME_TYPE:
            if (l != BT_CODEC_CONFIG_OCTETS_PER_FRAME_LEN)
                return false;

            STREAM_TO_UINT16(lc3->octets_per_frame, p);
            break;
        case BT_CODEC_CONFIG_BLOCKS_PER_SDU_TYPE:
            if (l != BT_CODEC_CONFIG_BLOCKS_PER_SDU_LEN)
                return false;

            STREAM_TO_UINT8(lc3->blocks_per_sdu, p);
            break;
        default:
            p += l - 1;
            break;
        }

        length -= 1 + l;
    }

    return true;
}

static bool bt_auracast_sink_adv_data_parse_metadata(bt_auracast_audio_metadata_t* metadata,
    uint8_t length, const uint8_t* p)
{
    /** Series of Length-Type-Value structures */
    uint8_t l, t;

    while (length) {
        STREAM_TO_UINT8(l, p);
        if (l == 0) {
            length--;
            continue; /**< invalid but not fatal */
        }

        if (length < (1 + l))
            return false; /**< length not enough */

        STREAM_TO_UINT8(t, p); /**< Type */
        switch (t) {
        case BT_METADATA_STREAMING_AUDIO_CONTEXT_TYPE:
            if (l != BT_METADATA_STREAMING_AUDIO_CONTEXT_LEN)
                return false;

            STREAM_TO_UINT16(metadata->context, p);
            if (!metadata->context)
                return false;

            break;
        case BT_METADATA_LANGUAGE_TYPE:
            if (l != BT_METADATA_LANGUAGE_LEN)
                return false;

            STREAM_TO_ARRAY(metadata->language, p, BT_METADATA_ISO_639_3_SIZE);
            metadata->language[BT_METADATA_ISO_639_3_SIZE] = '\0'; /**< for easy decoding */
            break;
        default:
            p += l - 1;
            break;
        }

        length -= 1 + l;
    }

    return true;
}

static bool bt_auracast_sink_adv_data_parse_uuid_16(bt_auracast_audio_info_t* info,
    uint16_t uuid_16, const adv_data_t* data)
{
    uint8_t length;
    const uint8_t* p = data->data + sizeof(uint16_t);

    if (uuid_16 != BT_UUID_BASIC_AUDIO_ANNOUNCEMENT)
        return true; /**< Doesn't care other services */

    /** Presentation Delay - 3 Octets */
    STREAM_TO_UINT24(info->presentation_delay, p);

    /** Num Subgroups - 1 octet */
    STREAM_TO_UINT8(info->num_subgroups, p);
    if (info->num_subgroups > BT_AURACAST_SINK_NUM_SUBGROUPS_SUPPORTED)
        return false;

    for (uint8_t i = 0; i < info->num_subgroups; i++) {
        bt_auracast_audio_subgroup_t* subgroup = &info->subgroup[i];

        /** Num BIS - 1 octet */
        STREAM_TO_UINT8(subgroup->num_bis, p);
        if (subgroup->num_bis > BT_AURACAST_SINK_NUM_BIS_SUPPORTED)
            return false;

        /** Codec ID - 5 octets: Coding Format(1) | Company ID(2) | Vendor-specific codec ID(2) */
        STREAM_TO_UINT8(subgroup->codec_id.coding_format, p);
        STREAM_TO_UINT16(subgroup->codec_id.company_id, p);
        STREAM_TO_UINT16(subgroup->codec_id.vendor_id, p);
        if ((subgroup->codec_id.coding_format != BT_CODEC_ID_VENDOR)
            && (subgroup->codec_id.company_id || subgroup->codec_id.vendor_id))
            return false;

        /** Codec Specific Configuration Length - 1 octet */
        STREAM_TO_UINT8(length, p);
        if (length > BT_CODEC_CONFIG_LEN_MAX)
            return false;

        /** Codec Specific Configuration - Varies */
        if (subgroup->codec_id.coding_format != BT_CODEC_ID_LC3) {
            subgroup->config.non_lc3.len = length;
            STREAM_TO_ARRAY(subgroup->config.non_lc3.config, p, length);
        } else {
            if (!bt_auracast_sink_adv_data_parse_lc3_config(&subgroup->config.lc3, length, p))
                return false;

            p += length;
        }

        /** Metadata Length - 1 octet */
        STREAM_TO_UINT8(length, p);

        /** Metadata - Varies */
        if (!bt_auracast_sink_adv_data_parse_metadata(&subgroup->metadata, length, p))
            return false;

        p += length;
        for (uint8_t k = 0; k < subgroup->num_bis; k++) {
            bt_auracast_audio_bis_info_t* bis = &subgroup->bis[k];

            /** BIS index - 1 octet */
            STREAM_TO_UINT8(bis->index, p);

            /** Codec Specific Configuration Length - 1 octet */
            STREAM_TO_UINT8(length, p);
            if (length > BT_CODEC_CONFIG_LEN_MAX)
                return false;

            /** Use subgroup codec if stream codec is not provided */
            memcpy(&bis->config, &subgroup->config, sizeof(bt_auracast_codec_specific_config_t));

            /** Codec Specific Configuration - Varies */
            if (subgroup->codec_id.coding_format != BT_CODEC_ID_LC3) {
                bis->config.non_lc3.len = length;
                STREAM_TO_ARRAY(bis->config.non_lc3.config, p, length);
            } else {
                if (!bt_auracast_sink_adv_data_parse_lc3_config(&bis->config.lc3, length, p))
                    return false;

                p += length;
            }
        }
    }

    return true;
}

static bool bt_auracast_sink_adv_data_parsed(const adv_data_t* data, void* context)
{
    bt_auracast_audio_info_t* info = (bt_auracast_audio_info_t*)context;
    const uint8_t* p = data->data;
    uint16_t uuid_16;

    switch (data->type) {
    case BT_AD_SERVICE_DATA16:
        if (data->len < 1 + sizeof(uuid_16))
            return false; /**< less than AD Type (1 octet) + UUID16 (2 octets) */

        STREAM_TO_UINT16(uuid_16, p);
        return bt_auracast_sink_adv_data_parse_uuid_16(info, uuid_16, data);
    default:
        break;
    }

    return true;
}

bt_status_t bt_auracast_sink_parse_adv_data(bt_auracast_audio_info_t* info,
    const bt_pa_sync_report_t* report)
{
    memset(info, 0x00, sizeof(bt_auracast_audio_info_t));
    info->presentation_delay = BT_AURACAST_SINK_PRESENTATION_DELAY_INVALID;

    if (!advertiser_data_parse(report->data, report->adv_data_len, bt_auracast_sink_adv_data_parsed,
            info))
        return BT_STATUS_FAIL;

    return info->num_subgroups ? BT_STATUS_SUCCESS : BT_STATUS_NOT_FOUND;
}
#endif /* CONFIG_BLUETOOTH_AURACAST_SINK */
#endif /* CONFIG_BLUETOOTH_PA_SYNC */
