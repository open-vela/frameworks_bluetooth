/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#ifndef __BT_AURACAST_SINK_H__
#define __BT_AURACAST_SINK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_audio_numbers.h"
#include "bt_pa_sync.h"

#define BT_AURACAST_SINK_NUM_SUBGROUPS_SUPPORTED (2)
#define BT_AURACAST_SINK_NUM_BIS_SUPPORTED (2)

#define BT_AURACAST_SINK_PRESENTATION_DELAY_INVALID (0xFFFFFFFF)

/**
 * @brief Codec-specific configuration for LC3.
 */
typedef struct bt_auracast_audio_lc3_config {
    /** Sampling frequency, e.g., `BT_CODEC_CONFIG_FREQUENCY_48000` */
    uint8_t sampling_frequency;

    /** Frame duration, e.g., `BT_CODEC_CONFIG_DURATION_10_MS` */
    uint8_t duration;

    /** Bitfield of audio location values, e.g., `BT_CODEC_CONFIG_ALLOCATION_MONO`,
     *  or (`BT_CODEC_CONFIG_ALLOCATION_FRONT_LEFT` | `BT_CODEC_CONFIG_ALLOCATION_FRONT_RIGHT`) */
    uint32_t location;

    /** Number of octets used per codec frame */
    uint16_t octets_per_frame;

    /** Number of blocks of codec frames per SDU */
    uint8_t blocks_per_sdu;
} bt_auracast_audio_lc3_config_t;

/**
 * @brief Information about the codec.
 */
typedef struct bt_auracast_audio_codec_id {
    /** E.g., `BT_CODEC_ID_LC3` */
    uint8_t coding_format;

    /** Company identifier values that are defined in Bluetooth Assigned Numbers */
    uint16_t company_id;

    /** Vendor-specific codec_ID if `coding_format` is `BT_CODEC_ID_VENDOR` , otherwise zero */
    uint16_t vendor_id;
} bt_auracast_audio_codec_id_t;

/**
 * @brief Codec Specific Configuration.
 */
typedef union bt_auracast_codec_specific_config {
    /** Valid when `codec_id.coding_format` is `BT_CODEC_ID_LC3` */
    bt_auracast_audio_lc3_config_t lc3;

    /** Valid when `codec_id.coding_format` is NOT `BT_CODEC_ID_LC3` */
    struct {
        uint8_t len; /**< length of `config` */
        uint8_t config[BT_CODEC_CONFIG_LEN_MAX];
    } non_lc3;
} bt_auracast_codec_specific_config_t;

/**
 * @brief Metadata for Auracast.
 */
typedef struct bt_auracast_audio_metadata {
    /** Streaming Audio Context, e.g., `BT_METADATA_AUDIO_CONTEXT_UNSPECIFIED` */
    uint16_t context;

    /** Language code as defined in ISO 639-3, e.g., "zho" for Chinese; "eng" for English */
    char language[BT_METADATA_ISO_639_3_SIZE + 1];
} bt_auracast_audio_metadata_t;

/**
 * @brief Information about a single BIS.
 */
typedef struct bt_auracast_audio_bis_info {
    /** BIS index value for this BIS in the subgroup */
    uint8_t index;

    /** Codec Specific Configuration */
    bt_auracast_codec_specific_config_t config;
} bt_auracast_audio_bis_info_t;

/**
 * @brief Information about the subgroups presented in a BIG.
 */
typedef struct bt_auracast_audio_subgroup {
    /** Number of BISes in the subgroup, range from 1 to `BT_AURACAST_SINK_NUM_BIS_SUPPORTED` */
    uint8_t num_bis;

    /** Codec information for the subgroup */
    bt_auracast_audio_codec_id_t codec_id;

    /** Codec Specific Configuration */
    bt_auracast_codec_specific_config_t config;

    /** Metadata */
    bt_auracast_audio_metadata_t metadata;

    /** BIS info */
    bt_auracast_audio_bis_info_t bis[BT_AURACAST_SINK_NUM_BIS_SUPPORTED];
} bt_auracast_audio_subgroup_t;

/**
 * @brief Information about the auracast source.
 */
typedef struct bt_auracast_audio_info {
    /** The Presentation_Delay in milliseconds, range from 0x000000 to 0xFFFFFF */
    uint32_t presentation_delay;

    /** Number of subgroups used to group BISes present in the BIG. range from 1 to
     *  `BT_AURACAST_SINK_NUM_SUBGROUPS_SUPPORTED` */
    uint8_t num_subgroups;

    /** Information for each subgroup */
    bt_auracast_audio_subgroup_t subgroup[BT_AURACAST_SINK_NUM_SUBGROUPS_SUPPORTED];
} bt_auracast_audio_info_t;

/**
 * @brief Parse an periodic advertising report and check if basic audio announcement is present.
 *
 * @param[out] info Buffer to store the parsed auracast audio info
 * @param[in] report Advertising report from the periodic sync report
 *
 * @return `BT_STATUS_SUCCESS` if basic audio announcement is found.
 * @return `BT_STATUS_NOT_FOUND` if no basic audio announcement is found.
 * @return Other error codes on failure.
 */
bt_status_t bt_auracast_sink_parse_adv_data(bt_auracast_audio_info_t* info,
    const bt_pa_sync_report_t* report);

#ifdef __cplusplus
}
#endif

#endif /* __BT_AURACAST_SINK_H__ */
