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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "stack_adapter_gap.h"
#include "stack_adapter_lea_gaf.h"

#include "bluetooth.h"
#include "bt_status.h"
#include "lea_audio_common.h"
#include "lea_ccpc_service.h"
#include "lea_mcpc_service.h"
#include "lea_server_service.h"
#include "lea_vmicps_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_server_interface.h"

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER

#define LEAS_CALL_SINK_METADATA_PREFER_CONTEX LEA_CONTEXT_TYPE_CONVERSATIONAL | LEA_CONTEXT_TYPE_INSTRUCTIONAL | LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | LEA_CONTEXT_TYPE_SOUND_EFFECTS | LEA_CONTEXT_TYPE_NOTIFICATIONS | LEA_CONTEXT_TYPE_RINGTONE | LEA_CONTEXT_TYPE_ALERTS | LEA_CONTEXT_TYPE_EMERGENCY_ALARM

#define LEAS_MEDIA_SINK_METADATA_PREFER_CONTEX LEA_CONTEXT_TYPE_MEDIA | LEA_CONTEXT_TYPE_GAME | LEA_CONTEXT_TYPE_LIVE

#define LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX LEA_CONTEXT_TYPE_CONVERSATIONAL | LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | LEA_CONTEXT_TYPE_LIVE

#define LEAS_PACS_CALL_SINK_SUPPORTED_SF LEA_SUPPORTED_SAMPLE_FREQUENCY_8000 | LEA_SUPPORTED_SAMPLE_FREQUENCY_16000 | LEA_SUPPORTED_SAMPLE_FREQUENCY_24000 | LEA_SUPPORTED_SAMPLE_FREQUENCY_32000

#define LEAS_PACS_CALL_SOURCE_SUPPORTED_SF (LEA_SUPPORTED_SAMPLE_FREQUENCY_8000 | LEA_SUPPORTED_SAMPLE_FREQUENCY_16000 | LEA_SUPPORTED_SAMPLE_FREQUENCY_24000 | LEA_SUPPORTED_SAMPLE_FREQUENCY_32000)

#define LEAS_PACS_MEDIA_SINK_SUPPORTED_SF (LEA_SUPPORTED_SAMPLE_FREQUENCY_32000 | LEA_SUPPORTED_SAMPLE_FREQUENCY_44100 | LEA_SUPPORTED_SAMPLE_FREQUENCY_48000)

#define LEAS_PACS_FRAME_DURATION (LEA_SUPPORTED_FRAME_DURATION_7_5 | LEA_SUPPORTED_FRAME_DURATION_10 | LEA_PREFERRED_FRAME_DURATION_10)

#define LEAS_PAC_LC3_CODEC          \
    {                               \
        LEA_CODING_FORMAT_LC3, 0, 0 \
    }

static void adpt_lea_pacs_set_sink_locations_cbk(uint32_t locations);
static void adpt_lea_pacs_set_source_locations_cbk(uint32_t locations);
static void adpt_lea_ascs_ase_cbk(BD_ADDR remote_addr, SERVICE_LEA_ASE_VALUE_S *ase);

const LEA_UCS_CALLBACK_S adpt_lea_uc_server_callbacks = {
    .lea_pacs_set_sink_locations_cb = adpt_lea_pacs_set_sink_locations_cbk,
    .lea_pacs_set_source_locations_cb = adpt_lea_pacs_set_source_locations_cbk,
    .lea_ascs_ase_cb = adpt_lea_ascs_ase_cbk,
};

static SERVICE_LEA_METADATA_S call_sink_metadata = { LEA_METADATA_PREFERRED_AUDIO_CONTEXTS, { LEAS_CALL_SINK_METADATA_PREFER_CONTEX } };

static SERVICE_LEA_METADATA_S media_sink_metadata = { LEA_METADATA_PREFERRED_AUDIO_CONTEXTS, { LEAS_MEDIA_SINK_METADATA_PREFER_CONTEX } };

static SERVICE_LEA_METADATA_S call_source_metadata = { LEA_METADATA_PREFERRED_AUDIO_CONTEXTS, { LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX } };

static SERVICE_LEA_PAC_INFO_S leas_pac_info[3] = {
    {LEA_PAC_TYPE_SINK_PAC,
     1,
     LEAS_PAC_LC3_CODEC,
     { LEA_CSC_MASK_ALL, LEAS_PACS_CALL_SINK_SUPPORTED_SF, LEAS_PACS_FRAME_DURATION,
        LEA_SUPPORTED_CHANNEL_COUNT_1, 26, 80, 1 },
     1,
     &call_sink_metadata  },
    { LEA_PAC_TYPE_SINK_PAC,
     2,
     LEAS_PAC_LC3_CODEC,
     { LEA_CSC_MASK_ALL, LEAS_PACS_MEDIA_SINK_SUPPORTED_SF, LEAS_PACS_FRAME_DURATION,
        LEA_SUPPORTED_CHANNEL_COUNT_1 | LEA_SUPPORTED_CHANNEL_COUNT_2, 60, 155, 2 },
     1,
     &media_sink_metadata },
    { LEA_PAC_TYPE_SOURCE_PAC,
     3,
     LEAS_PAC_LC3_CODEC,
     { LEA_CSC_MASK_ALL, LEAS_PACS_CALL_SOURCE_SUPPORTED_SF, LEAS_PACS_FRAME_DURATION,
        LEA_SUPPORTED_CHANNEL_COUNT_1, 26, 80, 1 },
     1,
     &call_source_metadata},
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_pacs_set_sink_locations_cbk(uint32_t locations)
{
    BT_LOGD("[Local][PACS][SinkAudioLocation 0x%08x]", locations);
}

static void adpt_lea_pacs_set_source_locations_cbk(uint32_t locations)
{
    BT_LOGD("[Local][PACS][SourceAudioLocation 0x%08x]", locations);
}

static void adpt_lea_ascs_ase_cbk(BD_ADDR remote_addr, SERVICE_LEA_ASE_VALUE_S *ase)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    char *state[] = { "Idle", "Codec_Config", "QoS_Config", "Enabling", "Streaming", "Disabling", "Releasing" };

    BT_LOGD("%s, remote_addr:%s, ASE_ID:%d, State:%s, Type:%x", __func__, bt_addr_str(&addr), ase->ase_id,
            state[ase->ase_state], ase->ase_type);

    switch (ase->ase_state) {
    case ADPT_LEA_ASE_STATE_CODEC_CONFIG: {
        SERVICE_LEA_ASE_CODEC_CFG_PARAM_S *cc = ase->parameters.cc;
        BT_LOGD("Codec, codec_id:%u, frequency:%u, duration:%u, allocation:%o, octets:%u, blocks:%u",
                cc->codec_cfg.codec_id.codec_id, cc->codec_cfg.frequency, cc->codec_cfg.duration,
                cc->codec_cfg.allocation, cc->codec_cfg.octets, cc->codec_cfg.blocks);
        break;
    }
    case ADPT_LEA_ASE_STATE_QOS_CONFIG: {
        SERVICE_LEA_ASE_QOS_CFG_PARAM_S *qc = ase->parameters.qc;
        BT_LOGD("Qos, sdu_interval:%u, max_sdu:%u, rtn:%u, max_latency:%u, delay:%u",
                qc->sdu_interval, qc->max_sdu, qc->rtn, qc->max_latency, qc->delay);
        break;
    }
    case ADPT_LEA_ASE_STATE_ENABLING: {
        SERVICE_LEA_ASE_ENABLING_PARAM_S *ec = ase->parameters.ec;
        BT_LOGD("Enabling, stream_id:0x%08x, metadata_number:%u", ec->stream_id, ec->metadata_number);
        break;
    }
    default:
        break;
    }

    lea_server_on_ascs_event(&addr, ase->ase_id, ase->ase_state, ase->ase_type);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bool adpt_req_pacs_info_callback(SERVICE_LEA_PACS_INFO_S *info)
{
    info->pac_number = 3;
    info->pac_list = leas_pac_info;
    info->sink_location = LEA_AUDIO_LOCATION_FRONT_LEFT | LEA_AUDIO_LOCATION_FRONT_RIGHT; /* Stereo output */
    info->source_location = LEA_AUDIO_LOCATION_FRONT_LEFT; /* One-MIC input */
    info->supported_ctx.sink = LEA_CONTEXT_TYPE_ALL;
    info->supported_ctx.source = LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX;
    info->available_ctx.sink = LEA_CONTEXT_TYPE_ALL;
    info->available_ctx.source = LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX;
    info->recycle_func = NULL; /* pac_list is constant, not to recycle */
    return true;
}

bool adpt_req_ascs_info_callback(SERVICE_LEA_ASCS_INFO_S *info)
{
    info->sink_ase_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SINK_ASE_NUMBER;
    info->source_ase_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SOURCE_ASE_NUMBER;
    return true;
}

bool adpt_req_bass_info_callback(SERVICE_LEA_BASS_INFO_S *info)
{
    info->state_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_BASS_STATE_NUMBER;
    return true;
}

void adpt_server_stream_state_callback(bt_address_t *addr, uint32_t stream_id, bool added)
{
    if (added) {
        lea_server_on_stream_added(addr, stream_id);
    } else {
        lea_server_on_stream_removed(addr, stream_id);
    }
}

void adpt_server_stream_start_callback(lea_audio_stream_t *lea_stream)
{
    lea_server_on_stream_started(lea_stream);
}

void adpt_server_stream_stop_callback(uint32_t stream_id)
{
    lea_server_on_stream_stopped(stream_id);
}

void adpt_server_stream_recv_callback(uint32_t stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data)
{
    lea_server_on_stream_recv(stream_id, iso_data->time_stamp, iso_data->sequenc_number,
                              iso_data->sdu, iso_data->sdu_length);
    stack_adapter_lea_mem_free(iso_data);
}

bt_status_t bt_sal_lea_server_start_announce(uint8_t adv_id, uint8_t type,
                                             uint8_t *adv_data, uint8_t adv_size, uint8_t *md_data, uint8_t md_size)
{
    SERVICE_SCAN_ADV_PARAMS_S adv_param;
    SERVICE_LEA_EXT_AD_S ext_ad;
    SERVICE_LEA_METADATA_S md;

    memset(&adv_param, 0, sizeof(SERVICE_SCAN_ADV_PARAMS_S));
    adv_param.adv_id = adv_id;
    adv_param.params.adv_type = BLE_ADV_IND;
    adv_param.params.channel_map = ADV_CHANNEL_DEFAULT;
    adv_param.params.interval = 60;
    adv_param.params.tx_power = -10;
    service_adapter_gap_start_ble_adv(&adv_param);

    memset(&ext_ad, 0, sizeof(SERVICE_LEA_EXT_AD_S));
    ext_ad.announcement_type = type;
    ext_ad.adv_id = adv_id;
    ext_ad.adv_data_length = adv_size;
    ext_ad.adv_data = adv_data;
    ext_ad.available_ctx.sink = LEA_CONTEXT_TYPE_ALL;
    ext_ad.available_ctx.source = LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX;
    md.type = LEA_METADATA_EXTENDED_METADATA;
    md.u.vendor_specific.data_length = md_size - 1;
    md.u.vendor_specific.data = md_data;
    ext_ad.metadata = &md;
    ext_ad.metadata_number = 1;
    stack_adapter_lea_set_adv_data(&ext_ad);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_server_stop_announce(uint8_t adv_id)
{
    SAL_CHECK_RET(service_adapter_gap_stop_ble_adv(adv_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_server_request_disable(bt_address_t *addr, uint8_t ase_id)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_ucs_auto_disalbe(bd_addr, ase_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif