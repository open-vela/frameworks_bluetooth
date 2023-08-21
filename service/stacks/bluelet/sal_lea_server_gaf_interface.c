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
#include "sal_lea_ccpc_interface.h"
#include "sal_lea_mcpc_interface.h"
#include "sal_lea_server_interface.h"
#include "sal_lea_vmicps_interface.h"

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

static void adpt_stack_state_callback(bool enabled);
static void adpt_storage_callback(void *data, uint32_t size);
static void adpt_connection_state_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state);
static void adpt_remote_services_callback(BD_ADDR remote_addr, uint8_t number, SERVICE_LEA_PRIMARY_SERVICE_S *services);

static bool adpt_req_vcs_info_callback(SERVICE_LEA_VCS_INFO_S *info);
static bool adpt_req_mics_info_callback(SERVICE_LEA_MICS_INFO_S *info);
static bool adpt_req_pacs_info_callback(SERVICE_LEA_PACS_INFO_S *info);
static bool adpt_req_ascs_info_callback(SERVICE_LEA_ASCS_INFO_S *info);
static bool adpt_req_bass_info_callback(SERVICE_LEA_BASS_INFO_S *info);
static void adpt_lea_csis_recycle_func(uint32_t number, void *csis_s);
static bool adpt_req_csis_info_callback(SERVICE_LEA_CSIS_S *info);
static void adpt_lea_csis_member_lock_cbk(uint32_t csis_id, BD_ADDR csis_addr,
                                          uint8_t lock);
static void adpt_stream_state_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id, bool added);
static uint8_t lea_server_get_channel(uint32_t allocation);
static void adpt_streaming_start_callback(SERVICE_LEA_AUDIO_STREAM_S *lea_stream);
static void adpt_streaming_stop_callback(LEA_IS_ID stream_id);
static void adpt_stream_recv_callback(LEA_IS_ID stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data);

static SERVICE_LEA_METADATA_S call_sink_metadata = { LEA_METADATA_PREFERRED_AUDIO_CONTEXTS, { LEAS_CALL_SINK_METADATA_PREFER_CONTEX } };

static SERVICE_LEA_METADATA_S media_sink_metadata = { LEA_METADATA_PREFERRED_AUDIO_CONTEXTS, { LEAS_MEDIA_SINK_METADATA_PREFER_CONTEX } };

static SERVICE_LEA_METADATA_S call_source_metadata = { LEA_METADATA_PREFERRED_AUDIO_CONTEXTS, { LEAS_CALL_SOURCE_METADATA_PREFER_CONTEX } };

static const char *default_sirk = "13579a24680b";

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

static LEA_GENERIC_CALLBACK_S adpt_generic_callbacks = {
    .lea_app_state_cb = adpt_stack_state_callback,
    .lea_app_storage_cb = adpt_storage_callback,
    .lea_connection_state_changed_cb = adpt_connection_state_callback,
    .lea_remote_services_cb = adpt_remote_services_callback,
};

static LEA_AUDIO_STREAM_CALLBACK_S adpt_audio_stream_callbacks = {
    .lea_audio_stream_state_cb = adpt_stream_state_callback,
    .lea_streaming_start_cb = adpt_streaming_start_callback,
    .lea_streaming_stop_cb = adpt_streaming_stop_callback,
    .lea_received_iso_data_cb = adpt_stream_recv_callback,
};

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPC
static const LEA_MCC_CALLBACK_S adpt_lea_mcp_client_callbacks = {
    .lea_mcc_media_player_name_cb = adpt_lea_mcc_media_player_name_callback,
    .lea_mcc_media_player_icon_object_id_cb = adpt_lea_mcc_media_player_icon_object_id_callback,
    .lea_mcc_media_player_icon_url_cb = adpt_lea_mcc_media_player_icon_url_callback,

    .lea_mcc_playback_speed_cb = adpt_lea_mcc_playback_speed_callback,
    .lea_mcc_seeking_speed_cb = adpt_lea_mcc_seeking_speed_callback,
    .lea_mcc_playing_order_cb = adpt_lea_mcc_playing_order_callback,
    .lea_mcc_playing_orders_supported_cb = adpt_lea_mcc_playing_orders_supported_callback,
    .lea_mcc_media_control_opcodes_supported_cb = adpt_lea_mcc_media_control_opcodes_supported_callback,
    .lea_mcc_content_control_id_cb = adpt_lea_mcc_content_control_id_callback,

    .lea_mcc_track_changed_cb = adpt_lea_mcc_track_changed_callback,
    .lea_mcc_track_title_cb = adpt_lea_mcc_track_title_callback,
    .lea_mcc_track_duration_cb = adpt_lea_mcc_track_duration_callback,
    .lea_mcc_track_position_cb = adpt_lea_mcc_track_position_callback,

    .lea_mcc_media_state_cb = adpt_lea_mcc_media_state_callback,
    .lea_mcc_media_control_result_cb = adpt_lea_mcc_media_control_result_callback,
    .lea_mcc_search_control_result_cb = adpt_lea_mcc_search_control_result_callback,

    .lea_mcc_current_track_segments_object_id_cb = adpt_lea_mcc_current_track_segments_object_id_callback,
    .lea_mcc_current_track_object_id_cb = adpt_lea_mcc_current_track_object_id_callback,
    .lea_mcc_next_track_object_id_cb = adpt_lea_mcc_next_track_object_id_callback,
    .lea_mcc_parent_group_object_id_cb = adpt_lea_mcc_parent_group_object_id_callback,
    .lea_mcc_current_group_object_id_cb = adpt_lea_mcc_current_group_object_id_callback,
    .lea_mcc_search_results_object_id_cb = adpt_lea_mcc_search_results_object_id_callback,
};
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCPC
static const LEA_TBC_CALLBACK_S adpt_lea_ccp_client_callbacks = {
    .lea_tbc_bearer_provider_name_cb = adpt_lea_tbc_bearer_provider_name_callback,
    .lea_tbc_bearer_uci_cb = adpt_lea_tbc_bearer_uci_callback,
    .lea_tbc_bearer_technology_cb = adpt_lea_tbc_bearer_technology_callback,
    .lea_tbc_bearer_uri_schemes_supported_list_cb = adpt_lea_tbc_bearer_uri_schemes_supported_list_callback,
    .lea_tbc_bearer_signal_strength_cb = adpt_lea_tbc_bearer_signal_strength_callback,
    .lea_tbc_bearer_signal_strength_report_interval_cb = adpt_lea_tbc_bearer_signal_strength_report_interval_callback,
    .lea_tbc_content_control_id_cb = adpt_lea_tbc_content_control_id_callback,
    .lea_tbc_status_flags_cb = adpt_lea_tbc_status_flags_callback,
    .lea_tbc_call_control_optional_opcodes_cb = adpt_lea_tbc_call_control_optional_opcodes_callback,

    .lea_tbc_incoming_call_cb = adpt_lea_tbc_incoming_call_callback,
    .lea_tbc_incoming_call_target_bearer_uri_cb = adpt_lea_tbc_incoming_call_target_bearer_uri_callback,
    .lea_tbc_call_state_cb = adpt_lea_tbc_call_state_callback,
    .lea_tbc_bearer_list_current_calls_cb = adpt_lea_tbc_bearer_list_current_calls_callback,
    .lea_tbc_call_friendly_name_cb = adpt_lea_tbc_call_friendly_name_callback,
    .lea_tbc_termination_reason_cb = adpt_lea_tbc_termination_reason_callback,

    .lea_tbc_call_control_result_cb = adpt_lea_tbc_call_control_result_callback,
};
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS
static const LEA_VCS_CALLBACK_S adpt_lea_vcs_server_callbacks = {
    .lea_vcs_set_volume_state_cb = adpt_lea_vcs_set_volume_state_callback,
    .lea_vcs_set_volume_flags_cb = adpt_lea_vcs_set_volume_flags_callback,
};

static const LEA_MICS_CALLBACK_S adpt_lea_mics_server_callbacks = {
    .lea_mics_set_mute_cb = adpt_lea_mics_set_mute_callback,
};
static const LEA_VOCS_CALLBACK_S adpt_lea_vocs_server_callbacks;
static const LEA_AICS_CALLBACK_S adpt_lea_aics_server_callbacks;
#endif

static const LEA_CSIS_CALLBACK_S adpt_lea_csip_server_callbacks = {
    .lea_csis_member_lock_cb = adpt_lea_csis_member_lock_cbk,
};

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
    // char* state[] = { "Idle", "Codec_Config", "QoS_Config", "Enabling", "Streaming", "Disabling", "Releasing" };

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

static const LEA_UCS_CALLBACK_S adpt_lea_uc_server_callbacks = {
    .lea_pacs_set_sink_locations_cb = adpt_lea_pacs_set_sink_locations_cbk,
    .lea_pacs_set_source_locations_cb = adpt_lea_pacs_set_source_locations_cbk,
    .lea_ascs_ase_cb = adpt_lea_ascs_ase_cbk,
};

static void adpt_stack_state_callback(bool enabled)
{
    lea_server_on_stack_state_changed((lea_server_stack_state_t)enabled);
}

static void adpt_storage_callback(void *data, uint32_t size)
{
    lea_server_on_storage_changed(data, size);
}

static void adpt_connection_state_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);
    lea_server_on_connection_state_changed(&addr, state);
}

static void adpt_remote_services_callback(BD_ADDR remote_addr, uint8_t number, SERVICE_LEA_PRIMARY_SERVICE_S *services)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, 6);

    BT_LOGD("%s, services num:%d", __func__, number);

    if (number) {
        SERVICE_LEA_PRIMARY_SERVICE_S *current = services;
        SERVICE_LEA_PRIMARY_SERVICE_S *end = current + number;
        while (current < end) {
            BT_LOGD("%s, sid:[%04d], type:[%04x]", __func__, current->sid, current->type);
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPC
            if (current->type == GATT_UUID_GENERIC_MEDIA_CONTROL) {
                adapt_mcs_sid_changed(current->sid);
            }
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCPC
            if (current->type == GATT_UUID_GENERIC_TELEPHONE_BEARER) {
                adpt_tbs_sid_changed(current->sid);
            }
#endif
            current++;
        }
    }
}

static bool adpt_req_vcs_info_callback(SERVICE_LEA_VCS_INFO_S *info)
{
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS
    info->step_size = CONFIG_BLUETOOTH_LEAUDIO_VCS_VOLUME_STEP;
    info->volume = CONFIG_BLUETOOTH_LEAUDIO_VCS_VOLUME_INITIAL;
    info->mute = LEA_VCS_MUTE_STATE_UNMUTED;
    info->vsps_flag = LEA_VCS_VSPS_RESET_VOLUME_SETTING;
    info->vocs_number = CONFIG_BLUETOOTH_LEAUDIO_VCS_VOCS_NUMBER;
    info->aics_number = CONFIG_BLUETOOTH_LEAUDIO_VCS_AICS_NUMBER;
#else
    info->step_size = 2;
    info->volume = 125;
    info->mute = 0;
    info->vsps_flag = 0;
    info->vocs_number = 0;
    info->aics_number = 0;
#endif
    info->vocs_list = NULL;
    info->aics_list = NULL;
    info->vocs_recycle_func = NULL;
    info->aics_recycle_func = NULL;
    return true;
}

static bool adpt_req_mics_info_callback(SERVICE_LEA_MICS_INFO_S *info)
{
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS
    info->mute = LEA_AI_MUTE_STATE_UNMUTED;
    info->aics_number = CONFIG_BLUETOOTH_LEAUDIO_MICS_AICS_NUMBER;
#else
    info->mute = 0;
    info->aics_number = 0;
#endif
    info->aics_list = NULL;
    info->recycle_func = NULL;
    return true;
}

static bool adpt_req_pacs_info_callback(SERVICE_LEA_PACS_INFO_S *info)
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

static bool adpt_req_ascs_info_callback(SERVICE_LEA_ASCS_INFO_S *info)
{
    info->sink_ase_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SINK_ASE_NUMBER;
    info->source_ase_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_SOURCE_ASE_NUMBER;
    return true;
}

static bool adpt_req_bass_info_callback(SERVICE_LEA_BASS_INFO_S *info)
{
    info->state_number = CONFIG_BLUETOOTH_LEAUDIO_SERVER_BASS_STATE_NUMBER;
    return true;
}

static void adpt_lea_csis_recycle_func(uint32_t number, void *csis_s)
{
    free(csis_s);
}

static bool adpt_req_csis_info_callback(SERVICE_LEA_CSIS_S *info)
{
    char value[BT_COMMON_KEY_SIZE] = { 0 };

    BT_LOGD("%s", __func__);
    info->csis_number = 1;
    info->csis_info = malloc(sizeof(SERVICE_LEA_CSIS_INFO_S));
    info->csis_info[0].csis_id = 0x47;
    info->csis_info[0].set_size = property_get_int32("persist.bluetooth.csis.set_size", 1);
    info->csis_info[0].sirk_type = LEA_SIRK_TYPE_ENCRYPTED;
    info->csis_info[0].rank = property_get_int32("persist.bluetooth.csis.rank", 1);

    property_get("persist.bluetooth.csis.set_sirk", value, default_sirk);
    for (int i = 0; i < BT_COMMON_KEY_SIZE; i++) {
        info->csis_info[0].sirk[i] = strtol(&value[i], NULL, 16);
    }

    info->recycle_func = adpt_lea_csis_recycle_func;
    return TRUE;
}

static void adpt_lea_csis_member_lock_cbk(uint32_t csis_id, BD_ADDR csis_addr,
                                          uint8_t lock)
{
    char *lock_s[] = { "NA", "Unlocked", "Locked" };
    BT_LOGD("%s, [CSIS 0x%08x][%s]", __func__, csis_id, lock_s[lock]);
}

static void adpt_stream_state_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id, bool added)
{
    bt_address_t addr;

    SERVICE_LEA_ISO_STREAM_ID_S *sid_s = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;

    memcpy(addr.addr, remote_addr, 6);
    BT_LOGD("%s, Stream ID:0x%08x, %s, GID:%d, SID:%d, ASE_ID:%d", __func__, stream_id, added ? "Added" : "Removed",
            sid_s->gid, sid_s->sid, sid_s->ase_id);
    BT_LOGD("%s, %s, %s", sid_s->features & LEA_IGIS_FEATURE_BROADCAST ? "BIS" : "CIS",
            sid_s->features & LEA_IGIS_FEATURE_INITIATOR ? "Initor" : "Acceptor",
            sid_s->features & LEA_IGIS_FEATURE_SOURCE ? "Source" : "Sink");

    if (added) {
        lea_server_on_stream_added(&addr, stream_id);
    } else {
        lea_server_on_stream_removed(&addr, stream_id);
    }
}

static uint8_t lea_server_get_channel(uint32_t allocation)
{
    uint8_t ch = 0;
    while (allocation) {
        if (allocation & 1) {
            ch++;
        }
        allocation >>= 1;
    }
    return ch;
}

static void adpt_streaming_start_callback(SERVICE_LEA_AUDIO_STREAM_S *lea_stream)
{
    lea_audio_stream_t audio_stream;

    memset(&audio_stream, 0, sizeof(lea_audio_stream_t));
    audio_stream.stream_id = lea_stream->stream_id;
    audio_stream.iso_handle = lea_stream->iso_handle;
    audio_stream.max_sdu = lea_stream->max_sdu;
    audio_stream.is_source = bt_sal_leas_is_source_stream(lea_stream->stream_id);
    memcpy(&audio_stream.codec_cfg, &lea_stream->codec_cfg, sizeof(lea_codec_config_t));
    audio_stream.channal_num = lea_server_get_channel(audio_stream.codec_cfg.allocation);
    audio_stream.sdu_size = audio_stream.channal_num * audio_stream.codec_cfg.blocks * audio_stream.codec_cfg.octets;

    lea_server_on_stream_started(&audio_stream);
}

static void adpt_streaming_stop_callback(LEA_IS_ID stream_id)
{
    lea_server_on_stream_stopped(stream_id);
}

static void adpt_stream_recv_callback(LEA_IS_ID stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data)
{
    lea_server_on_stream_recv(stream_id, iso_data->time_stamp, iso_data->sequenc_number,
                              iso_data->sdu, iso_data->sdu_length);
    stack_adapter_lea_mem_free(iso_data);
}

static const LEA_INIT_INFO_CALLBACK_S server_callbacks = {
    .lea_request_vcs_info_cb = adpt_req_vcs_info_callback,
    .lea_request_mics_info_cb = adpt_req_mics_info_callback,
    .lea_request_pacs_info_cb = adpt_req_pacs_info_callback,
    .lea_request_ascs_info_cb = adpt_req_ascs_info_callback,
    .lea_request_bass_info_cb = adpt_req_bass_info_callback,
    .lea_request_csis_info_cb = adpt_req_csis_info_callback,
    .lea_generic_cbks = &adpt_generic_callbacks,
    .lea_audio_stream_cbks = &adpt_audio_stream_callbacks,
    .lea_mcp_server_cbks = NULL,
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPC
    .lea_mcp_client_cbks = &adpt_lea_mcp_client_callbacks,
#endif
    .lea_ccp_server_cbks = NULL,
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCPC
    .lea_ccp_client_cbks = &adpt_lea_ccp_client_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS
    .lea_vcs_server_cbks = &adpt_lea_vcs_server_callbacks,
    .lea_vcs_client_cbks = NULL,
    .lea_mics_server_cbks = &adpt_lea_mics_server_callbacks,
    .lea_mics_client_cbks = NULL,
    .lea_vocs_server_cbks = &adpt_lea_vocs_server_callbacks,
    .lea_vocs_client_cbks = NULL,
    .lea_aics_server_cbks = &adpt_lea_aics_server_callbacks,
    .lea_aics_client_cbks = NULL,
#endif
    .lea_csip_server_cbks = &adpt_lea_csip_server_callbacks,
    .lea_csip_client_cbks = NULL,
    .lea_uc_server_cbks = &adpt_lea_uc_server_callbacks,
    .lea_uc_client_cbks = NULL,
    .lea_bcsrc_cbks = NULL,
};

bt_status_t bt_sal_lea_server_init()
{
    SERVICE_LEA_ROLE roles[3] = { LEA_ROLE_TMAP_CT, LEA_ROLE_TMAP_UMR, LEA_ROLE_TMAP_BMR };

    SAL_CHECK_RET(stack_adapter_lea_init(roles, 3, &server_callbacks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
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

bt_status_t bt_sal_lea_server_disconnect(bt_address_t *addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_disconnect(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_server_request_disable(bt_address_t *addr, uint8_t ase_id)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_ucs_auto_disalbe(bd_addr, bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_lea_server_cleanup()
{
    stack_adapter_lea_cleanup();
}

bool bt_sal_leas_is_source_stream(uint32_t stream_id)
{
    SERVICE_LEA_ISO_STREAM_ID_S *sid = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;
    return sid->features & LEA_IGIS_FEATURE_SOURCE;
}

lea_send_iso_data_t *bt_sal_leas_alloc_send_buffer(uint16_t length, uint16_t handle)
{
    return (lea_send_iso_data_t *)stack_adapter_lea_get_iso_data_sent_buffer(length, handle);
}

bt_status_t bt_sal_leas_send_iso_data(lea_send_iso_data_t *packet)
{
    SAL_CHECK_RET(stack_adapter_lea_send_iso_data((SERVICE_LEA_SENT_ISO_DATA_S *)packet), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif