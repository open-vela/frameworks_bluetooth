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

#include "bt_device.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <nuttx/list.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LEA_BIT(_b) (1 << (_b))

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
    ADPT_LEA_CONTEXT_TYPE_PROHIBITED,
    ADPT_LEA_CONTEXT_TYPE_UNSPECIFIED = LEA_BIT(0),
    ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL = LEA_BIT(1),
    ADPT_LEA_CONTEXT_TYPE_MEDIA = LEA_BIT(2),
    ADPT_LEA_CONTEXT_TYPE_GAME = LEA_BIT(3),
    ADPT_LEA_CONTEXT_TYPE_INSTRUCTIONAL = LEA_BIT(4),
    ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS = LEA_BIT(5),
    ADPT_LEA_CONTEXT_TYPE_LIVE = LEA_BIT(6),
    ADPT_LEA_CONTEXT_TYPE_SOUND_EFFECTS = LEA_BIT(7),
    ADPT_LEA_CONTEXT_TYPE_NOTIFICATIONS = LEA_BIT(8),
    ADPT_LEA_CONTEXT_TYPE_RINGTONE = LEA_BIT(9),
    ADPT_LEA_CONTEXT_TYPE_ALERTS = LEA_BIT(10),
    ADPT_LEA_CONTEXT_TYPE_EMERGENCY_ALARM = LEA_BIT(11),
} lea_adpt_context_types_t;
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
    uint16_t mask;
    uint16_t frequencies;
    uint8_t durations;
    uint8_t channels;
    uint16_t frame_octets_min;
    uint16_t frame_octets_max;
    uint8_t max_frames;
} lea_codec_cap_t;

typedef struct {
    uint8_t codec_type;
    uint32_t sample_rate;
    uint8_t bits_per_sample;
    uint8_t channel_mode;
    uint32_t bit_rate;
    uint16_t sdu_size;
    uint32_t frame_size;
    uint32_t packet_size;
} lea_audio_config_t;

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
    uint32_t stream_id;
    uint16_t iso_handle;
    uint16_t max_sdu;
    uint8_t channal_num;
    bt_address_t addr;
    uint16_t sdu_size;
    bool is_source;
    uint8_t target_latency;
    uint8_t target_phy;
    lea_codec_config_t codec_cfg;
} lea_audio_stream_t;

typedef struct {
    void *reserved_data;
    uint16_t iso_handle;
    uint8_t reserved_handle;
    uint16_t sdu_length;
    uint8_t *sdu;
} lea_send_iso_data_t;

typedef struct {
    struct list_node node;
    uint32_t time_stamp;
    uint16_t seq;
    uint16_t length;
    uint8_t sdu[1];
} lea_recv_iso_data_t;

typedef void (*lea_audio_suspend_callback)(uint32_t stream_id);
typedef void (*lea_audio_resume_callback)(uint32_t stream_id);
typedef void (*lea_audio_meatadata_updated_callback)(uint32_t stream_id);
typedef void (*lea_audio_send_callback)(uint32_t stream_id, uint8_t *buffer,
                                        uint16_t length);

#endif