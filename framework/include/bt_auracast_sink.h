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

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

#define AURACAST_BITFIELD(x) (1UL << x)
#define AURACAST_BITFIELD_ALL (0xFFFFFFFE) /**< bit[0] is reserved */

#define BT_AURACAST_BROADCAST_CODE_LEN (16)
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
 * @brief Auracast sync established.
 *
 * @param cookie Callback cookie
 * @param addr The Bluetooth address and address type of the remote device
 * @param sid The advertising set id (0x00-0x0F) to identify the periodic advertising
 */
typedef void (*on_auracast_sync_established_callback)(void* cookie, const bt_le_address_t* addr,
    uint8_t sid);

/**
 * @brief Auracast sync terminated.
 *
 * @param cookie Callback cookie
 * @param addr The Bluetooth address and address type of the remote device
 * @param sid The advertising set id (0x00-0x0F) to identify the periodic advertising
 */
typedef void (*on_auracast_sync_terminated_callback)(void* cookie, const bt_le_address_t* addr,
    uint8_t sid);

typedef struct {
    on_auracast_sync_established_callback on_sync_established;
    on_auracast_sync_terminated_callback on_sync_terminated;
} bt_auracast_sink_callbacks_t;

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

/**
 * @brief Register callback functions to Auracast sink service.
 *
 * An application may register interested callbacks on initialization, this includes connection
 * state changed callbacks.
 *
 * @param[in] ins - the Bluetooth client instance
 * @param[in] cbs - Auracast sink callback functions, see @ref bt_auracast_sink_callbacks_t
 *
 * @return void* - callbacks cookie, if the callback is registered successfuly.
 * @return NULL - the callback is already registered or registration fails.
 */
void* BTSYMBOLS(bt_auracast_sink_register_callbacks)(bt_instance_t* ins,
    const bt_auracast_sink_callbacks_t* cbs);

/**
 * @brief Unregister callback functions from Auracast sink service.
 *
 * An application shall unregister the callbacks when logging out to release resources.
 *
 * @param[in] ins - the Bluetooth client instance
 * @param[in] cookie - callbacks cookie
 *
 * @return true - callback unregistration successful.
 * @return false - callback cookie not found or callback unregistration failed.
 */
bool BTSYMBOLS(bt_auracast_sink_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Synchronize to an Auracast source described in the periodic advertising train.
 *
 * @param[in] ins The Bluetooth instance, see @ref bt_instance_t
 * @param[in] addr The Bluetooth address and address type of the remote device
 * @param[in] sid The advertising set id subfield to identify the periodic advertising, range from
 *                0x00 to 0x0F
 * @param[in] bitfield Bitwise value of which BIS is to synchronize, e.g., BIS[x] is synchronized if
 *                     bit x is set. The value of x ranges from 0x1 to 0x1F
 *                     Value x can be acquired from `index` in @ref bt_auracast_audio_bis_info_t
 *                     See @ref bt_auracast_sink_parse_adv_data
 *                     See @ref AURACAST_BITFIELD(x)
 * @param[in] broadcast_code 16-octet code used for deriving the session key for decrypting payloads
 *                           of BISes in the BIG. NULL if not encrypted
 *
 * @return `BT_STATUS_SUCCESS` on success, or other error codes on failure.
 */
bt_status_t BTSYMBOLS(bt_auracast_sink_create_sync)(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, uint32_t bitfield, const uint8_t* broadcast_code);

/**
 * @brief Terminate from an Auracast source.
 *
 * @param[in] ins The Bluetooth instance, see @ref bt_instance_t
 * @param[in] addr The Bluetooth address and address type of the remote device
 * @param[in] sid The advertising set id subfield to identify the periodic advertising, range from
 *                0x00 to 0x0F
 *
 * @return `BT_STATUS_SUCCESS` on success, or other error codes on failure.
 */
bt_status_t BTSYMBOLS(bt_auracast_sink_terminate_sync)(bt_instance_t* ins,
    const bt_le_address_t* addr, uint8_t sid);

#ifdef CONFIG_BLUETOOTH_FRAMEWORK_ASYNC
#include "bt_async.h"

/**
 * @brief Register callback functions to Auracast sink service.
 *
 * An application may register interested callbacks on initialization, this includes connection
 * state changed callbacks.
 *
 * @param[in] ins - the Bluetooth client instance
 * @param[in] cbs - Auracast sink callback functions, see @ref bt_auracast_sink_callbacks_t
 * @param[in] cb - Callback function for asynchronous call
 * @param[in] userdata - User context carried in `cb`
 *
 * @return `BT_STATUS_SUCCESS` on success.
 * @return Other error codes on failure.
 */
bt_status_t bt_auracast_sink_register_callbacks_async(bt_instance_t* ins,
    const bt_auracast_sink_callbacks_t* cbs, bt_register_callback_cb_t cb, void* userdata);

/**
 * @brief Unregister callback functions from Auracast sink service.
 *
 * An application shall unregister the callbacks when logging out to release resources.
 *
 * @param[in] ins - the Bluetooth client instance
 * @param[in] cookie - callbacks cookie
 * @param[in] cb - Callback function for asynchronous call
 * @param[in] userdata - User context carried in `cb`
 *
 * @return `BT_STATUS_SUCCESS` on success.
 * @return Other error codes on failure.
 */
bt_status_t bt_auracast_sink_unregister_callbacks_async(bt_instance_t* ins, void* cookie,
    bt_status_cb_t cb, void* userdata);

/**
 * @brief Synchronize to an Auracast source described in the periodic advertising train.
 *
 * @param[in] ins The Bluetooth instance, see @ref bt_instance_t
 * @param[in] addr The Bluetooth address and address type of the remote device
 * @param[in] sid The advertising set id subfield to identify the periodic advertising, range from
 *                0x00 to 0x0F
 * @param[in] bitfield Bitwise value of which BIS is to synchronize, e.g., BIS[x] is synchronized if
 *                     bit x is set. The value of x ranges from 0x1 to 0x1F
 *                     Value x can be acquired from `index` in @ref bt_auracast_audio_bis_info_t
 *                     See @ref bt_auracast_sink_parse_adv_data
 *                     See @ref AURACAST_BITFIELD(x)
 * @param[in] broadcast_code 16-octet code used for deriving the session key for decrypting payloads
 *                           of BISes in the BIG. NULL if not encrypted
 * @param[in] cb - Callback function for asynchronous call
 * @param[in] userdata - User context carried in `cb`
 *
 * @return IPC status.
 */
bt_status_t bt_auracast_sink_create_sync_async(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, uint32_t bitfield, const uint8_t* broadcast_code, bt_status_cb_t cb,
    void* userdata);

/**
 * @brief Terminate from an Auracast source.
 *
 * @param[in] ins The Bluetooth instance, see @ref bt_instance_t
 * @param[in] addr The Bluetooth address and address type of the remote device
 * @param[in] sid The advertising set id subfield to identify the periodic advertising, range from
 *                0x00 to 0x0F
 * @param[in] cb - Callback function for asynchronous call
 * @param[in] userdata - User context carried in `cb`
 *
 * @return IPC status.
 */
bt_status_t bt_auracast_sink_terminate_sync_async(bt_instance_t* ins, const bt_le_address_t* addr,
    uint8_t sid, bt_status_cb_t cb, void* userdata);

#endif /* CONFIG_BLUETOOTH_FRAMEWORK_ASYNC */

#ifdef __cplusplus
}
#endif

#endif /* __BT_AURACAST_SINK_H__ */
