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

#include <nuttx/list.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/
typedef struct {
    uint32_t frame_size;
    uint32_t octets;
} lc3_param_t;

typedef struct {
    uint16_t freq;
    uint16_t mode;
} sbc_param_t;

typedef struct {
    uint8_t codec_type;
    uint32_t sample_rate;
    uint8_t bits_per_sample;
    uint8_t channel_mode;
    uint32_t bit_rate;
    uint16_t sdu_size;
    uint8_t specific_info[20];
    union {
        lc3_param_t lc3;
        sbc_param_t sbc;
    };
} lea_audio_config_t;

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