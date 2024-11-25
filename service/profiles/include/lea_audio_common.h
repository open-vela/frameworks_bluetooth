/****************************************************************************
 * service/profiles/include/lea_audio_sink.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#ifndef __LEA_AUDIO_COMMON_H__
#define __LEA_AUDIO_COMMON_H__

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bt_device.h"
#include "bt_list.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LEA_BIT(_b) (1 << (_b))
#define MAX_PROVIDER_NAME_LENGTH 16
#define MAX_URI_SCHEMES_LENGTH 16
#define MAX_INCOMING_CALL_URI_LENGTH 16
#define MAX_CALL_URI_LENGTH 16
#define MAX_UCI_LENGTH 8
#define MAX_FRIENDLY_NAME_LENGTH 8

#ifndef CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER
#define CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER 2
#endif

#ifndef CONFIG_BLUETOOTH_LEAUDIO_CLIENT_MAX_DEVICES
#define CONFIG_BLUETOOTH_LEAUDIO_CLIENT_MAX_DEVICES 8
#endif

#ifndef CONFIG_BLUETOOTH_LEAUDIO_CLIENT_PAC_MAX_NUMBER
#define CONFIG_BLUETOOTH_LEAUDIO_CLIENT_PAC_MAX_NUMBER 3
#endif

#define LEA_CLIENT_MAX_STREAM_NUM (CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER * CONFIG_BLUETOOTH_LEAUDIO_CLIENT_MAX_DEVICES)

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum { /* UUIDs */
    GATT_UUID_MEDIA_CONTROL = 0x1848,
    GATT_UUID_GENERIC_MEDIA_CONTROL = 0x1849,
    GATT_UUID_TELEPHONE_BEARER = 0x184B,
    GATT_UUID_GENERIC_TELEPHONE_BEARER = 0x184C,
};

typedef enum {
    ADPT_LEA_GMCS_ID = 1,
    ADPT_LEA_MCS1_ID,
    ADPT_LEA_GTBS_ID,
    ADPT_LEA_TBS1_ID,
    ADPT_LEA_CSIS1_ID,
    ADPT_LEA_SERVICE_MAX_ID,
} bt_lea_service_id;

typedef enum {
    ADPT_LEA_ASE_TARGET_LOW_LATENCY = 1,
    ADPT_LEA_ASE_TARGET_BALANCED = 2,
    ADPT_LEA_ASE_TARGET_HIGH_RELIABILITY = 3,
} lea_adpt_ase_target_latency_t;

typedef enum {
    ADPT_LEA_ASE_TARGET_PHY_1M = 1,
    ADPT_LEA_ASE_TARGET_PHY_2M = 2,
    ADPT_LEA_ASE_TARGET_PHY_CODED = 3,
} lea_adpt_ase_target_phy_t;

typedef enum {
    ADPT_LEA_METADATA_PREFERRED_AUDIO_CONTEXTS = 0x01,
    ADPT_LEA_METADATA_STREAMING_AUDIO_CONTEXTS,
    ADPT_LEA_METADATA_PROGRAM_INFO,
    ADPT_LEA_METADATA_LANGUAGE,
    ADPT_LEA_METADATA_CCID_LIST,
    ADPT_LEA_METADATA_PARENTAL_RATING,
    ADPT_LEA_METADATA_PROGRAM_INFO_URI,
    ADPT_LEA_METADATA_EXTENDED_METADATA = 0xFE,
    ADPT_LEA_METADATA_VENDOR_SPECIFIC = 0xFF,
} lea_adpt_metadata_type_t;

typedef enum {
    ADPT_LEA_ASE_STATE_IDLE,
    ADPT_LEA_ASE_STATE_CODEC_CONFIG,
    ADPT_LEA_ASE_STATE_QOS_CONFIG,
    ADPT_LEA_ASE_STATE_ENABLING,
    ADPT_LEA_ASE_STATE_STREAMING,
    ADPT_LEA_ASE_STATE_DISABLING,
    ADPT_LEA_ASE_STATE_RELEASING,
} lea_adpt_ase_state_t;

typedef enum {
    LEA_ASE_OP_CONFIG_CODEC,
    LEA_ASE_OP_CONFIG_QOS,
    LEA_ASE_OP_ENABLE,
    LEA_ASE_OP_DISABLE,
    LEA_ASE_OP_UPDATE_METADATA,
    LEA_ASE_OP_RELEASE
} lea_ase_opcode;

/* Possible TBS bearer technologies. */
typedef enum {
    ADPT_LEA_TBS_BEARER_RESERVED, /**< Reserved */
    ADPT_LEA_TBS_BEARER_3G, /**< 3G network */
    ADPT_LEA_TBS_BEARER_4G, /**< 4G network */
    ADPT_LEA_TBS_BEARER_LTE, /**< LTE network */
    ADPT_LEA_TBS_BEARER_WIFI, /**< Wi-Fi network */
    ADPT_LEA_TBS_BEARER_5G, /**< 5G network. */
    ADPT_LEA_TBS_BEARER_GSM, /**< GSM network. */
    ADPT_LEA_TBS_BEARER_CDMA, /**< CDMA network. */
    ADPT_LEA_TBS_BEARER_2G, /**< 2G network. */
    ADPT_LEA_TBS_BEARER_WCDMA, /**< WCDMA network. */
} lea_adpt_bearer_technology_t;

enum {
    ADPT_LEA_CALL_FLAGS_OUTGOING_CALL = 0x01,
    ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_SERVER = 0x02,
    ADPT_LEA_CALL_FLAGS_INFORMATION_WITHHELD_BY_NETWORK = 0x04
};

typedef enum {
    ADPT_LEA_TBS_CALL_STATE_INCOMING, /**< Incoming call: a remote party is calling. */
    ADPT_LEA_TBS_CALL_STATE_DIALING, /**< Dialing (outgoing call): Call the remote party, but remote party is not being alerted. */
    ADPT_LEA_TBS_CALL_STATE_ALERTING, /**< Alerting (outgoing call): Remote party is being alerted. */
    ADPT_LEA_TBS_CALL_STATE_ACTIVE, /**< Active (ongoing call): The call is in an active conversation. */
    ADPT_LEA_TBS_CALL_STATE_LOCALLY_HELD, /**< Locally Held: The call is held locally. */
    ADPT_LEA_TBS_CALL_STATE_REMOTELY_HELD, /**< Remotely Held: The call is held by the remote party. */
    ADPT_LEA_TBS_CALL_STATE_BOTH_HELD, /**< Locally and Remotely Held: The call is held both locally and remotely. */
    ADPT_LEA_TBS_CALL_STATE_INVAILD,
} lea_adpt_call_state_t;

/** Call control point opcodes. */
typedef enum {
    ADPT_LEA_TBS_CALL_CONTROL_ACCEPT, /**< Accept the specified incoming call. */
    ADPT_LEA_TBS_CALL_CONTROL_TERMINATE, /**< End the specified active, alerting, dialing, incoming or held call. */
    ADPT_LEA_TBS_CALL_CONTROL_LOCAL_HOLD, /**< Place the specified active or incoming call on local hold. */
    ADPT_LEA_TBS_CALL_CONTROL_LOCAL_RETRIEVE, /**< If the specified call is locally held, move it to an active call. Or, if it is locally and remotely held, move it to a remotely held call.*/
    ADPT_LEA_TBS_CALL_CONTROL_ORIGINATE, /**< Initiate a call to the remote party identified by the URI. */
    ADPT_LEA_TBS_CALL_CONTROL_JOIN, /**< Put calls (not in remotely held state) in the list to active and join the calls. Any calls in one of the remotely held state move to remotely held state and are joined with the other calls. */
} lea_adpt_call_control_opcode_t;

typedef enum {
    ADPT_LEA_TBS_REASON_URI, /**< Improperly formed URI value. */
    ADPT_LEA_TBS_REASON_CALL_FAILED, /**< The call failed. */
    ADPT_LEA_TBS_REASON_ENDED_BY_REMOTE, /**< The remote party ended the call. */
    ADPT_LEA_TBS_REASON_ENDED_FROM_SERVER, /**< The call ended from the server. */
    ADPT_LEA_TBS_REASON_LINE_WAS_BUSY, /**< The line was busy. */
    ADPT_LEA_TBS_REASON_NETWORK_CONGESTION, /**< Network congestion. */
    ADPT_LEA_TBS_REASON_ENDED_BY_CLIENT, /**< The client terminated the call. */
    ADPT_LEA_TBS_REASON_NO_SERVICE, /**< No service. */
    ADPT_LEA_TBS_REASON_NO_ANSWER, /**< No answer. */
    ADPT_LEA_TBS_REASON_UNSPECIFIED, /**< Any other unspecified reason. */
} lea_adpt_termination_reason_t;

/** Result of call control operation. */
typedef enum {
    ADPT_LEA_TBS_CALL_CONTROL_SUCCESS, /**< The operation was successful. */
    ADPT_LEA_TBS_CALL_CONTROL_NOT_SUPPORTED, /**< An invalid opcode was used. */
    ADPT_LEA_TBS_CALL_CONTROL_OPEARTION_NOT_POSSIBLE, /**< The requested operation can't be completed. */
    ADPT_LEA_TBS_CALL_CONTROL_INVALID_CALL_INDEX, /**< The call index used for the requested action is invalid. */
    ADPT_LEA_TBS_CALL_CONTROL_STATE_MISMATCH, /**< The call was not in the expected state for the requested action. */
    ADPT_LEA_TBS_CALL_CONTROL_LACK_OF_RESOURCES, /**< Lack of internal resorces to complete the requested action. */
    ADPT_LEA_TBS_CALL_CONTROL_INVALID_OUTGOING_URI, /**< The Outgoing URI is incorrect or invalid. */
} lea_adpt_call_control_result_t;

typedef enum {
    ADPT_LEA_PAC_TYPE_SINK_PAC = 0x2BC9,
    ADPT_LEA_PAC_TYPE_SOURCE_PAC = 0x2BCB,
} lea_pac_type_t;

typedef enum {
    ADPT_LEA_FORMAT_TRANSPARENT = 0x03,
    ADPT_LEA_FORMAT_LC3 = 0x06,
    ADPT_LEA_FORMAT_G729A = 0x07,
    ADPT_LEA_FORMAT_VENDOR = 0xFF,
} lea_codec_fromat;

typedef enum {
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_8000 = LEA_BIT(0),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_11025 = LEA_BIT(1),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_16000 = LEA_BIT(2),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_22050 = LEA_BIT(3),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_24000 = LEA_BIT(4),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_32000 = LEA_BIT(5),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_44100 = LEA_BIT(6),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_48000 = LEA_BIT(7),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_88200 = LEA_BIT(8),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_96000 = LEA_BIT(9),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_176400 = LEA_BIT(10),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_192000 = LEA_BIT(11),
    ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_384000 = LEA_BIT(12),
} lea_sample_frequencies_t;

typedef enum {
    ADPT_LEA_SUPPORTED_FRAME_DURATION_7_5 = LEA_BIT(0),
    ADPT_LEA_SUPPORTED_FRAME_DURATION_10 = LEA_BIT(1),
    ADPT_LEA_PREFERRED_FRAME_DURATION_7_5 = LEA_BIT(4),
    ADPT_LEA_PREFERRED_FRAME_DURATION_10 = LEA_BIT(5),
} lea_frame_durations_t;

typedef enum {
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_1 = LEA_BIT(0),
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_2 = LEA_BIT(1),
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_3 = LEA_BIT(2),
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_4 = LEA_BIT(3),
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_5 = LEA_BIT(4),
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_6 = LEA_BIT(5),
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_7 = LEA_BIT(6),
    ADPT_LEA_SUPPORTED_CHANNEL_COUNT_8 = LEA_BIT(7),
} lea_channal_counts_t;

typedef enum {
    ADPT_LEA_SIRK_TYPE_ENCRYPTED,
    ADPT_LEA_SIRK_TYPE_PLAIN_TEXT,
    ADPT_LEA_SIRK_TYPE_OOB_ONLY = 0xFF,
} lea_sirk_type_t;

/* Context ID */
typedef enum {
    ADPT_LEA_CTX_ID_UNSPECIFIED,
    ADPT_LEA_CTX_ID_CONVERSATIONAL,
    ADPT_LEA_CTX_ID_MEDIA,
    ADPT_LEA_CTX_ID_GAME,
    ADPT_LEA_CTX_ID_INSTRUCTIONAL,
    ADPT_LEA_CTX_ID_VOICE_ASSISTANTS,
    ADPT_LEA_CTX_ID_LIVE,
    ADPT_LEA_CTX_ID_SOUND_EFFECTS,
    ADPT_LEA_CTX_ID_NOTIFICATIONS,
    ADPT_LEA_CTX_ID_RINGTONE,
    ADPT_LEA_CTX_ID_ALERTS,
    ADPT_LEA_CTX_ID_EMERGENCY_ALARM,
    ADPT_LEA_CTX_ID_NUMBER,
} lea_adpt_context_id_t;

typedef enum {
    ADPT_LEA_CONTEXT_TYPE_PROHIBITED,
    ADPT_LEA_CONTEXT_TYPE_UNSPECIFIED = LEA_BIT(ADPT_LEA_CTX_ID_UNSPECIFIED),
    ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL = LEA_BIT(ADPT_LEA_CTX_ID_CONVERSATIONAL),
    ADPT_LEA_CONTEXT_TYPE_MEDIA = LEA_BIT(ADPT_LEA_CTX_ID_MEDIA),
    ADPT_LEA_CONTEXT_TYPE_GAME = LEA_BIT(ADPT_LEA_CTX_ID_GAME),
    ADPT_LEA_CONTEXT_TYPE_INSTRUCTIONAL = LEA_BIT(ADPT_LEA_CTX_ID_INSTRUCTIONAL),
    ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS = LEA_BIT(ADPT_LEA_CTX_ID_VOICE_ASSISTANTS),
    ADPT_LEA_CONTEXT_TYPE_LIVE = LEA_BIT(ADPT_LEA_CTX_ID_LIVE),
    ADPT_LEA_CONTEXT_TYPE_SOUND_EFFECTS = LEA_BIT(ADPT_LEA_CTX_ID_SOUND_EFFECTS),
    ADPT_LEA_CONTEXT_TYPE_NOTIFICATIONS = LEA_BIT(ADPT_LEA_CTX_ID_NOTIFICATIONS),
    ADPT_LEA_CONTEXT_TYPE_RINGTONE = LEA_BIT(ADPT_LEA_CTX_ID_RINGTONE),
    ADPT_LEA_CONTEXT_TYPE_ALERTS = LEA_BIT(ADPT_LEA_CTX_ID_ALERTS),
    ADPT_LEA_CONTEXT_TYPE_EMERGENCY_ALARM = LEA_BIT(ADPT_LEA_CTX_ID_EMERGENCY_ALARM),
} lea_adpt_context_types_t;

typedef enum {
    ADPT_LEA_LC3_SET_UNKNOWN,
    ADPT_LEA_LC3_SET_8_1_1, /* Odd is 7.5 ms */
    ADPT_LEA_LC3_SET_8_2_2, /* Even is 10 ms */
    ADPT_LEA_LC3_SET_16_1_3,
    ADPT_LEA_LC3_SET_16_2_4,
    ADPT_LEA_LC3_SET_24_1_5,
    ADPT_LEA_LC3_SET_24_2_6,
    ADPT_LEA_LC3_SET_32_1_7,
    ADPT_LEA_LC3_SET_32_2_8,
    ADPT_LEA_LC3_SET_441_1_9,
    ADPT_LEA_LC3_SET_441_2_10,
    ADPT_LEA_LC3_SET_48_1_11,
    ADPT_LEA_LC3_SET_48_2_12,
    ADPT_LEA_LC3_SET_48_3_13,
    ADPT_LEA_LC3_SET_48_4_14,
    ADPT_LEA_LC3_SET_48_5_15,
    ADPT_LEA_LC3_SET_48_6_16,
} lea_lc3_set_id_t;

typedef enum {
    ADPT_LEA_SAMPLE_FREQUENCY_UNKNOWN,
    ADPT_LEA_SAMPLE_FREQUENCY_8000 = 1,
    ADPT_LEA_SAMPLE_FREQUENCY_11025,
    ADPT_LEA_SAMPLE_FREQUENCY_16000,
    ADPT_LEA_SAMPLE_FREQUENCY_22050,
    ADPT_LEA_SAMPLE_FREQUENCY_24000,
    ADPT_LEA_SAMPLE_FREQUENCY_32000,
    ADPT_LEA_SAMPLE_FREQUENCY_44100,
    ADPT_LEA_SAMPLE_FREQUENCY_48000,
    ADPT_LEA_SAMPLE_FREQUENCY_88200,
    ADPT_LEA_SAMPLE_FREQUENCY_96000,
    ADPT_LEA_SAMPLE_FREQUENCY_176400,
    ADPT_LEA_SAMPLE_FREQUENCY_192000,
    ADPT_LEA_SAMPLE_FREQUENCY_384000,
} lea_sample_frequency_t;

typedef enum {
    ADPT_LEA_FRAME_DURATION_7_5,
    ADPT_LEA_FRAME_DURATION_10,
} lea_frame_duration;

typedef struct {
    uint8_t frequency;
    uint8_t duration;
    uint16_t octets;
} lea_lc3_config_t;

typedef struct {
    uint16_t contexts;
    uint8_t set_number;
    uint8_t* set_10_ids;
} lea_lc3_prefer_config;

typedef struct {
    uint8_t format;
    uint16_t company_id;
    uint16_t codec_id;
} lea_codec_id_t;

typedef struct {
    lea_codec_id_t codec_id;
    uint16_t mask;
    uint8_t frequency;
    uint8_t duration;
    uint32_t allocation;
    uint16_t octets;
    uint8_t blocks;
} lea_codec_config_t;

typedef struct
{
    uint16_t stream_handle;
    uint32_t channel_allocation;
} lea_stream_info_t;

typedef struct
{
    uint16_t mask;
    uint16_t frequencies;
    uint8_t durations;
    uint8_t channels;
    uint16_t frame_octets_min;
    uint16_t frame_octets_max;
    uint8_t max_frames;
} lea_codec_cap_t;

typedef struct
{
    uint8_t type;
    union {
        uint32_t preferred_contexts;
        uint32_t streaming_contexts;
        uint8_t program_info[64];
        uint32_t language;
        uint8_t ccid_list[64];
        uint32_t parental_rating;
        uint8_t program_info_uri[64];
        uint8_t extended_metadata[64];
        uint8_t vendor_specific[64];
    };
} lea_metadata_t;

typedef struct {
    lea_pac_type_t pac_type;
    uint32_t pac_id;
    lea_codec_id_t codec_id;
    lea_codec_cap_t codec_pac;
    uint8_t md_number;
    lea_metadata_t* md_value;
} lea_pac_info_t;

typedef struct
{
    uint16_t sink;
    uint16_t source;
} lea_audio_context_t;

typedef struct {
    uint8_t pac_number;
    lea_pac_info_t* pac_list;
    uint32_t sink_location;
    uint32_t source_location;
    lea_audio_context_t supported_ctx;
    lea_audio_context_t available_ctx;
} lea_pacs_info_t;

typedef struct {
    uint8_t sink_ase_number;
    uint8_t source_ase_number;
} lea_ascs_info_t;

typedef struct {
    uint8_t bass_number;
} lea_bass_info_t;

typedef struct {
    uint32_t csis_id;
    uint8_t set_size;
    uint8_t sirk[16];
    uint8_t sirk_type;
    uint8_t rank;
} lea_csis_info_t;

typedef struct {
    uint8_t csis_number;
    uint8_t rfu;
    lea_csis_info_t* csis_info;
} lea_csis_infos_t;

typedef struct {
    bool is_source;
    bool started;
    uint8_t target_latency;
    uint8_t target_phy;
    uint8_t channal_num;
    uint16_t iso_handle;
    uint16_t sdu_size;
    uint16_t max_sdu;
    uint32_t stream_id;
    uint32_t group_id;
    bt_address_t addr;
    lea_codec_config_t codec_cfg;
} lea_audio_stream_t;

typedef struct {
    void* reserved_data;
    uint16_t iso_handle;
    uint8_t reserved_handle;
    uint16_t sdu_length;
    uint8_t* sdu;
} lea_send_iso_data_t;

typedef struct {
    struct list_node node;
    uint32_t time_stamp;
    uint16_t seq;
    uint16_t length;
    uint8_t sdu[1];
} lea_recv_iso_data_t;

/* LE Audio TBS struct */
typedef struct {
    void* bearer_ref; /**< Application specified bearer identity. */
    uint8_t provider_name[MAX_PROVIDER_NAME_LENGTH]; /**< Initial Bearer Provider Name. Zero terminated UTF-8 string. */
    uint8_t uci[MAX_UCI_LENGTH]; /**< Bearer UCI. Zero terminated UTF-8 string. */
    uint8_t uri_schemes[MAX_URI_SCHEMES_LENGTH]; /**< Initial list of Bearer URI schemes supported. Zero terminated UTF-8 string. */
    uint8_t technology; /**< Initial Bearer Technology, one of #SERVICE_LEA_TBS_BEARER_TECHNLOGY.  */
    uint8_t signal_strength; /**< Initial Bearer Signal Strength, 0 indicates no service; 1 to 100 indicates the valid signal strength. 255 indicates that signal strength is unavailable or has no meaning.  */
    uint8_t signal_strength_report_interval; /**< Initial Signal Strength reporting interval in seconds. 0 to 255. 0 indicates that reporting signal strength only when it is changed. */
    uint16_t status_flags; /**< Server feature status. Bits of #SERVICE_LEA_TBS_STATUS_FLAGS. */
    uint16_t optional_opcodes_supported; /**< Call control point optional Opcodes supported. Bits of #SERVICE_LEA_TBS_SUPPORTED_CALL_CONTROL_OPCODES. */
} lea_tbs_telephone_bearer_t;

typedef struct {
    uint8_t index; /**< Call Index, 1 to 255. */
    uint8_t state; /**< Initial Call State, one of #SERVICE_LEA_TBS_CALL_STATE. */
    uint8_t flags; /**< Initial Call flags, bits of #SERVICE_LEA_TBS_CALL_FLAGS. */
    uint8_t call_uri[MAX_CALL_URI_LENGTH]; /**< The Incoming Call URI or Outgoing Call URI. Zero terminated UTF-8 string. Set to NULL if the URI is unknown. */
    uint8_t incoming_target_uri[MAX_INCOMING_CALL_URI_LENGTH]; /**< The Incoming Call Target Bearer URI. Zero terminated UTF-8 string. Set to NULL for an outgoing call or if the URI is unknown. */
    uint8_t friendly_name[MAX_FRIENDLY_NAME_LENGTH]; /**< The Friendly Name of the incoming or outgoing call. Zero terminated UTF-8 string. Set to NULL if the URI is unknown. */
} lea_tbs_calls_t;

typedef struct {
    uint8_t index; /**< Call Index, 1 to 255. */
    uint8_t state; /**< Call State, one of #SERVICE_LEA_TBS_CALL_STATE. */
    uint8_t flags; /**< Call flags, bits of #SERVICE_LEA_TBS_CALL_FLAGS. */
} lea_tbs_call_state_t;

typedef struct {
    uint8_t index; /**< Call Index, 1 to 255. */
    uint8_t state; /**< Call State, one of #SERVICE_LEA_TBS_CALL_STATE. */
    uint8_t flags; /**< Call flags, bits of #SERVICE_LEA_TBS_CALL_FLAGS. */
    char call_uri[0]; /**< The Incoming Call URI or Outgoing Call URI. Zero terminated UTF-8 string. Set to NULL if the URI is unknown. */
} lea_tbs_call_list_item_t;

typedef void (*lea_audio_suspend_callback)(void);

typedef void (*lea_audio_resume_callback)(void);

typedef void (*lea_audio_meatadata_updated_callback)(void);

typedef void (*lea_audio_send_callback)(uint8_t* buffer, uint16_t length);

#endif
