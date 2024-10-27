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
#ifndef _BT_CONTROLLER_VENDOR_ACTIONS_H__
#define _BT_CONTROLLER_VENDOR_ACTIONS_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>

#include "bt_utils.h"
#include "bt_vendor.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CONFIG_LEAS_CALL_SINK_SUPPORTED_SAMPLE_FREQUENCY (ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_8000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_16000)

#define CONFIG_LEAS_CALL_SOURCE_SUPPORTED_SAMPLE_FREQUENCY (ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_8000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_16000)

#define CONFIG_LEAS_MEDIA_SINK_SUPPORTED_SAMPLE_FREQUENCY (ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_16000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_32000 | ADPT_LEA_SUPPORTED_SAMPLE_FREQUENCY_48000)

#define CONFIG_LEAS_CALL_SINK_METADATA_PREFER_CONTEX (ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL | ADPT_LEA_CONTEXT_TYPE_INSTRUCTIONAL | ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | ADPT_LEA_CONTEXT_TYPE_SOUND_EFFECTS | ADPT_LEA_CONTEXT_TYPE_NOTIFICATIONS | ADPT_LEA_CONTEXT_TYPE_RINGTONE | ADPT_LEA_CONTEXT_TYPE_ALERTS | ADPT_LEA_CONTEXT_TYPE_EMERGENCY_ALARM)

#define CONFIG_LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX (ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL | ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | ADPT_LEA_CONTEXT_TYPE_LIVE)

#define CONFIG_LEAS_MEDIA_SINK_METADATA_PREFER_CONTEX (ADPT_LEA_CONTEXT_TYPE_MEDIA | ADPT_LEA_CONTEXT_TYPE_GAME | ADPT_LEA_CONTEXT_TYPE_LIVE)

#define CONFIG_LEAS_PACS_FRAME_DURATION (ADPT_LEA_SUPPORTED_FRAME_DURATION_10 | ADPT_LEA_PREFERRED_FRAME_DURATION_10)

#undef CONFIG_VSC_MAX_LEN
#define CONFIG_VSC_MAX_LEN 64

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum {
    LEA_CODEC_SINK,
    LEA_CODEC_SOURCE,
};

static inline bool actions_a2dp_offload_start_builder(a2dp_offload_config_t* config,
    uint8_t* offload, size_t* size)
{
    uint8_t* param = offload;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x03); // cmd
    UINT8_TO_STREAM(param, 0x02); // offload  start
    UINT16_TO_STREAM(param, config->acl_hdl); // acl handle
    UINT8_TO_STREAM(param, 0x03); // codec type
    UINT16_TO_STREAM(param, config->l2c_rcid); // cid
    UINT16_TO_STREAM(param, config->frame_sample); // frame sample
    UINT16_TO_STREAM(param, 0x0000); // frame length, reserved
    UINT16_TO_STREAM(param, config->mtu); // MTU
    UINT8_TO_STREAM(param, 0x00); // Padding
    UINT8_TO_STREAM(param, 0x00); // Extension
    UINT8_TO_STREAM(param, 0x00); // Marker
    UINT8_TO_STREAM(param, 0x60); // Payload Type
    UINT8_TO_STREAM(param, 0x01); // SSRC

    *size = param - offload;
    return true;
}

static inline bool actions_a2dp_offload_stop_builder(a2dp_offload_config_t* config,
    uint8_t* offload, size_t* size)
{
    uint8_t* param = offload;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x03);
    UINT8_TO_STREAM(param, 0x03); // offload  stop

    *size = param - offload;
    return true;
}

static inline bool actions_hfp_offload_start_builder(hfp_offload_config_t* config,
    uint8_t* offload, size_t* size)
{
    uint8_t* param = offload;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x02); // cmd
    UINT8_TO_STREAM(param, 0x00); // offload  start
    UINT16_TO_STREAM(param, config->sco_hdl); // sco handle
    UINT8_TO_STREAM(param, config->sco_codec); // codec type

    *size = param - offload;
    return true;
}

static inline bool actions_hfp_offload_stop_builder(hfp_offload_config_t* config,
    uint8_t* offload, size_t* size)
{
    uint8_t* param = offload;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x02); // cmd
    UINT8_TO_STREAM(param, 0x01); // offload  stop
    UINT16_TO_STREAM(param, config->sco_hdl); // sco handle
    UINT8_TO_STREAM(param, config->sco_codec); // codec type

    *size = param - offload;
    return true;
}

static inline bool actions_lea_offload_start_builder(lea_offload_config_t* config,
    uint8_t* offload, size_t* size)
{
    uint8_t* param = offload;
    int stream_num;
    int index;

    if (config->codec[LEA_CODEC_SOURCE].stream_num > config->codec[LEA_CODEC_SINK].stream_num) {
        stream_num = config->codec[LEA_CODEC_SOURCE].stream_num;
    } else {
        stream_num = config->codec[LEA_CODEC_SINK].stream_num;
    }

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x04); // cmd
    UINT8_TO_STREAM(param, 0x04); // offload  start
    UINT8_TO_STREAM(param, 0x01); // mono or stereo
    UINT8_TO_STREAM(param, stream_num); // stream number

    for (index = 0; index < stream_num; index++) {
        if (config->initiator) {
            UINT8_TO_STREAM(param, 0x01); // todo: channel
            if (config->codec[LEA_CODEC_SOURCE].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SOURCE].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }

            if (config->codec[LEA_CODEC_SINK].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SINK].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }
        } else {
            UINT8_TO_STREAM(param, 0x03); // todo: channel
            if (config->codec[LEA_CODEC_SINK].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SINK].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }

            if (config->codec[LEA_CODEC_SOURCE].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SOURCE].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }
        }
    }

    *size = param - offload;
    return true;
}

static inline bool actions_lea_offload_stop_builder(lea_offload_config_t* config,
    uint8_t* offload, size_t* size)
{
    uint8_t* param = offload;
    int stream_num;
    int index;

    if (config->codec[LEA_CODEC_SOURCE].stream_num > config->codec[LEA_CODEC_SINK].stream_num) {
        stream_num = config->codec[LEA_CODEC_SOURCE].stream_num;
    } else {
        stream_num = config->codec[LEA_CODEC_SINK].stream_num;
    }

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x0000); // fill ocf

    UINT8_TO_STREAM(param, 0x04); // cmd
    UINT8_TO_STREAM(param, 0x05); // offload  stop
    UINT8_TO_STREAM(param, 0x01); // mono or stereo
    UINT8_TO_STREAM(param, stream_num); // stream number

    for (index = 0; index < stream_num; index++) {
        if (config->initiator) {
            UINT8_TO_STREAM(param, 0x01); // todo: channel
            if (config->codec[LEA_CODEC_SOURCE].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SOURCE].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }
            if (config->codec[LEA_CODEC_SINK].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SINK].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }
        } else {
            UINT8_TO_STREAM(param, 0x03); // todo: channel
            if (config->codec[LEA_CODEC_SINK].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SINK].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }

            if (config->codec[LEA_CODEC_SOURCE].active) {
                UINT16_TO_STREAM(param, config->codec[LEA_CODEC_SOURCE].streams_info[index].stream_handle);
            } else {
                UINT16_TO_STREAM(param, 0x0000);
            }
        }
    }
    *size = param - offload;
    return true;
}

static inline bool actions_dlf_enable_command_builder(le_dlf_config_t* config, uint8_t* data, size_t* size)
{
    uint8_t* param = data;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x00d7); // fill ocf

    // vendor specified fields
    UINT8_TO_STREAM(param, 0x05); // data length
    UINT8_TO_STREAM(param, 0x01); // subcode
    UINT8_TO_STREAM(param, (uint8_t)config->connection_handle);
    UINT8_TO_STREAM(param, 0x00);
    UINT16_TO_STREAM(param, config->dlf_timeout);

    *size = param - data;

    return true;
}

static inline bool actions_dlf_disable_command_builder(le_dlf_config_t* config, uint8_t* data, size_t* size)
{
    uint8_t* param = data;

    UINT8_TO_STREAM(param, 0x3f); // fill ogf
    UINT16_TO_STREAM(param, 0x00d7); // fill ocf

    // vendor specified fields
    UINT8_TO_STREAM(param, 0x05); // data length
    UINT8_TO_STREAM(param, 0x01); // subcode
    UINT8_TO_STREAM(param, (uint8_t)config->connection_handle);
    UINT8_TO_STREAM(param, 0x01);
    UINT16_TO_STREAM(param, 0x0000);

    *size = param - data;

    return true;
}

#endif /* _BT_CONTROLLER_VENDOR_H__ */
