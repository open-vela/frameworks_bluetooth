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
#ifndef _BT_CONTROLLER_VENDOR_H__
#define _BT_CONTROLLER_VENDOR_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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

bool a2dp_offload_start_builder(a2dp_offload_config_t *config,
                                uint8_t *offload, size_t *size);

bool a2dp_offload_stop_builder(a2dp_offload_config_t *config,
                               uint8_t *offload, size_t *size);

#endif /* _BT_CONTROLLER_VENDOR_H__ */
