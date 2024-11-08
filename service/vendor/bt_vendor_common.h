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
#ifndef _BT_CONTROLLER_VENDOR_COMMON_H__
#define _BT_CONTROLLER_VENDOR_COMMON_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "lea_audio_common.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_LEA_STREAM_MAX_NUM 4
#define CONFIG_LEA_CODEC_MAX_NUM 2
#define CONFIG_VSC_MAX_LEN 255 /* TODO: define by vendor */
#define CONFIG_DLF_COMMAND_MAX_LEN 10

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    uint8_t bits_per_sample; /* bits per sample ex: 16/24/32 */
    uint8_t ch_mode; /* None:0 Left:1 Right:2 */
    uint16_t frame_sample; /* frame sample*/
    uint16_t acl_hdl; /* connection handle */
    uint16_t l2c_rcid; /* l2cap channel id */
    uint16_t mtu; /* MTU size */
    uint16_t max_latency; /* maximum latency */
    uint32_t codec_type; /* codec types ex: SBC/AAC/LDAC/APTx */
    uint32_t sample_rate; /* Sample rates ex: 44.1/48/88.2/96 Khz */
    uint32_t encoded_audio_bitrate; /* encoder audio bitrates */
    uint8_t codec_info[32]; /* Codec specific information */
} a2dp_offload_config_t;

typedef struct __attribute__((packed)) {
    uint16_t acl_hdl; /* connection handle */
    uint32_t bandwidth; /* bits per second */
} acl_bandwitdh_config_t;

typedef struct
{
    uint16_t sco_codec;
    uint16_t sco_hdl; /* sco handle */
    bool is_controller_codec; /* bt controller encode/decode */
    bool is_nrec;
} hfp_offload_config_t;

typedef struct {
    bool active;
    uint8_t bits_per_sample;
    uint8_t stream_num;
    uint8_t blocks_per_sdu;
    uint16_t octets_per_frame;
    uint16_t peer_delay_ms;
    uint32_t sampling_rate;
    uint32_t frame_duration;
    lea_stream_info_t streams_info[CONFIG_LEA_STREAM_MAX_NUM];
} lea_offload_codec_t;

typedef struct {
    bool initiator;
    lea_offload_codec_t codec[CONFIG_LEA_CODEC_MAX_NUM];
} lea_offload_config_t;

typedef struct
{
    uint16_t connection_handle;
    uint16_t dlf_timeout;
} le_dlf_config_t;

#endif /* _BT_CONTROLLER_VENDOR_COMMON_H__ */
