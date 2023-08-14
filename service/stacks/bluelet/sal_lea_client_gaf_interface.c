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

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT

#include "stack_adapter_gap.h"
#include "stack_adapter_lea_gaf.h"

#include "bluetooth.h"
#include "bt_status.h"
#include "lea_audio_common.h"
#include "lea_client_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_client_interface.h"
#include "sal_lea_mcps_interface.h"
#include "sal_lea_tbs_interface.h"
#include "sal_lea_vmicpc_interface.h"

static void adpt_stack_state_callback(bool enabled);
static void adpt_storage_callback(void *data, uint32_t size);
static void adpt_connection_state_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state);
static void adpt_remote_services_callback(BD_ADDR remote_addr, uint8_t number, SERVICE_LEA_PRIMARY_SERVICE_S *services);

static void adpt_stream_state_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id, bool added);
static uint8_t lea_client_get_channel(uint32_t allocation);
static void adpt_streaming_start_callback(SERVICE_LEA_AUDIO_STREAM_S *lea_stream);
static void adpt_streaming_stop_callback(LEA_IS_ID stream_id);
static void adpt_stream_recv_callback(LEA_IS_ID stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data);

static const LEA_GENERIC_CALLBACK_S adpt_generic_callbacks = {
    .lea_app_state_cb = adpt_stack_state_callback,
    .lea_app_storage_cb = adpt_storage_callback,
    .lea_connection_state_changed_cb = adpt_connection_state_callback,
    .lea_remote_services_cb = adpt_remote_services_callback,
};

static const LEA_AUDIO_STREAM_CALLBACK_S adpt_audio_stream_callbacks = {
    .lea_audio_stream_state_cb = adpt_stream_state_callback,
    .lea_streaming_start_cb = adpt_streaming_start_callback,
    .lea_streaming_stop_cb = adpt_streaming_stop_callback,
    .lea_received_iso_data_cb = adpt_stream_recv_callback,
};

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPS
static const LEA_MCS_CALLBACK_S adpt_lea_mcp_server_callbacks = {
    .lea_mcs_state_cb = adpt_lea_mcs_state_callback,
    .lea_mcs_player_set_cb = adpt_lea_mcs_player_set_callback,
    .lea_mcs_object_added_cb = adpt_lea_mcs_object_added_callback,

    .lea_mcs_set_position_cb = adpt_lea_mcs_set_position_callback,
    .lea_mcs_set_playback_speed_cb = adpt_lea_mcs_set_playback_speed_callback,
    .lea_mcs_set_current_track_cb = adpt_lea_mcs_set_current_track_callback,
    .lea_mcs_set_next_track_cb = adpt_lea_mcs_set_next_track_callback,
    .lea_mcs_set_current_group_cb = adpt_lea_mcs_set_current_group_callback,
    .lea_mcs_set_playing_order_cb = adpt_lea_mcs_set_playing_order_callback,

    .lea_mcs_play_cb = adpt_lea_mcs_play_callback,
    .lea_mcs_pause_cb = adpt_lea_mcs_pause_callback,
    .lea_mcs_fast_rewind_cb = adpt_lea_mcs_fast_rewind_callback,
    .lea_mcs_fast_forward_cb = adpt_lea_mcs_fast_forward_callback,
    .lea_mcs_stop_cb = adpt_lea_mcs_stop_callback,
    .lea_mcs_move_cb = adpt_lea_mcs_move_callback,
    .lea_mcs_previous_segment_cb = adpt_lea_mcs_previous_segment_callback,
    .lea_mcs_next_segment_cb = adpt_lea_mcs_next_segment_callback,
    .lea_mcs_first_segment_cb = adpt_lea_mcs_first_segment_callback,
    .lea_mcs_last_segment_cb = adpt_lea_mcs_last_segment_callback,
    .lea_mcs_goto_segment_cb = adpt_lea_mcs_goto_segment_callback,
    .lea_mcs_previous_track_cb = adpt_lea_mcs_previous_track_callback,
    .lea_mcs_next_track_cb = adpt_lea_mcs_next_track_callback,
    .lea_mcs_first_track_cb = adpt_lea_mcs_first_track_callback,
    .lea_mcs_last_track_cb = adpt_lea_mcs_last_track_callback,
    .lea_mcs_goto_track_cb = adpt_lea_mcs_goto_track_callback,
    .lea_mcs_previous_group_cb = adpt_lea_mcs_previous_group_callback,
    .lea_mcs_next_group_cb = adpt_lea_mcs_next_group_callback,
    .lea_mcs_first_group_cb = adpt_lea_mcs_first_group_callback,
    .lea_mcs_last_group_cb = adpt_lea_mcs_last_group_callback,
    .lea_mcs_goto_group_cb = adpt_lea_mcs_goto_group_callback,

    .lea_mcs_search_track_name_cb = adpt_lea_mcs_search_track_name_callback,
    .lea_mcs_search_artist_name_cb = adpt_lea_mcs_search_artist_name_callback,
    .lea_mcs_search_album_name_cb = adpt_lea_mcs_search_album_name_callback,
    .lea_mcs_search_group_name_cb = adpt_lea_mcs_search_group_name_callback,
    .lea_mcs_search_earliest_year_cb = adpt_lea_mcs_search_earliest_year_callback,
    .lea_mcs_search_latest_year_cb = adpt_lea_mcs_search_latest_year_callback,
    .lea_mcs_search_genre_cb = adpt_lea_mcs_search_genre_callback,
    .lea_mcs_search_tracks_cb = adpt_lea_mcs_search_tracks_callback,
    .lea_mcs_search_groups_cb = adpt_lea_mcs_search_groups_callback,
};
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
static const LEA_TBS_CALLBACK_S adpt_lea_ccp_server_callbacks = {
    .lea_tbs_state_cb = adpt_lea_tbs_state_callback,
    .lea_tbs_bearer_set_cb = adpt_lea_tbs_bearer_set_callback,
    .lea_tbs_call_added_cb = adpt_lea_tbs_call_added_callback,
    .lea_tbs_call_removed_cb = adpt_lea_tbs_call_removed_callback,

    .lea_tbs_accept_cb = adpt_lea_tbs_accept_callback,
    .lea_tbs_terminate_cb = adpt_lea_tbs_terminate_callback,
    .lea_tbs_local_hold_cb = adpt_lea_tbs_local_hold_callback,
    .lea_tbs_local_retrieve_cb = adpt_lea_tbs_local_retrieve_callback,
    .lea_tbs_originate_cb = adpt_lea_tbs_originate_callback,
    .lea_tbs_join_cb = adpt_lea_tbs_join_callback,
};
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPS
static const LEA_MCS_CALLBACK_S adpt_lea_mcp_server_callbacks;
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
static const LEA_TBS_CALLBACK_S adpt_lea_ccp_server_callbacks;
#endif
static const LEA_VCC_CALLBACK_S adpt_lea_vcs_client_callbacks;
static const LEA_MICC_CALLBACK_S adpt_lea_mics_client_callbacks;

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPC
static const LEA_VCC_CALLBACK_S adpt_lea_vcs_client_callbacks = {
    .lea_vcc_volume_state_cb = adpt_lea_vcc_volume_state_cbk,
    .lea_vcc_volume_flags_cb = adpt_lea_vcc_volume_flags_cbk,
};

static const LEA_MICC_CALLBACK_S adpt_lea_mics_client_callbacks = {
    .lea_micc_mute_cb = adpt_lea_micc_mute_cbk,
};

static const LEA_VOCC_CALLBACK_S adpt_lea_vocs_client_callbacks;
static const LEA_AICC_CALLBACK_S adpt_lea_aics_client_callbacks;
#endif

extern const LEA_CSIC_CALLBACK_S adpt_lea_csip_client_callbacks;
static const LEA_BCSRC_CALLBACK_S adpt_lea_bcsrc_callabcks;

void adpt_lea_ucc_pac_callback(BD_ADDR remote_addr, SERVICE_LEA_PAC_INFO_S *pac_info)
{
    SERVICE_LEA_CODEC_CAP_S *cc = &pac_info->codec_cap;
    lea_client_capability_t cap;
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    memset(&cap, 0, sizeof(lea_client_capability_t));

    cap.is_source = (pac_info->pac_type == LEA_PAC_TYPE_SINK_PAC ? 0 : 1);
    cap.pac_id = pac_info->pac_id;
    memcpy(&cap.codec_id, &pac_info->codec_id, sizeof(pac_info->codec_id));
    memcpy(&cap.codec_cap, &pac_info->codec_cap, sizeof(pac_info->codec_cap));
    cap.metadata_number = pac_info->metadata_number;
    memcpy(cap.metadata_value, pac_info->metadata, sizeof(pac_info->metadata) * cap.metadata_number);

    BT_LOGD("%s, pac_id:0x%08x, type:%s, codec_format:%d", __func__, pac_info->pac_id, pac_info->pac_type == LEA_PAC_TYPE_SINK_PAC ? "Sink" : "Source", pac_info->codec_id.format);
    BT_LOGD("frequencies:%d, durations:%d, channels:%d, frame_octets_min:%d, frame_octets_max:%d, max_frames:%d",
            cc->frequencies, cc->durations, cc->channels, cc->frame_octets_min, cc->frame_octets_max, cc->max_frames);

    lea_client_on_pac_event(&addr, &cap);
    stack_adapter_lea_recycle_pac_s(pac_info);
}

void adpt_lea_ucc_ase_callback(BD_ADDR remote_addr, SERVICE_LEA_ASE_VALUE_S *ase)
{
    char *state[] = { "Idle", "Codec_Config", "QoS_Config",
                      "Enabling", "Streaming", "Disabling", "Releasing" };
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, ASE_ID:%d, State:%s, Type:%s", __func__, ase->ase_id,
            state[ase->ase_state], ase->ase_type == LEA_ASE_TYPE_SOURCE_ASE ? "Source" : "Sink");

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

    lea_client_on_ascs_event(&addr, ase->ase_state, ase->ase_type == LEA_ASE_TYPE_SOURCE_ASE, ase->ase_id);
    stack_adapter_lea_recycle_ase_s(ase);
}

void adpt_lea_ucc_sink_audio_locations_callback(BD_ADDR remote_addr, uint32_t location)
{
    bt_address_t addr;

    BT_LOGD("%s, location:0x%08x", __func__, location);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_audio_localtion_event(&addr, false, location);
}

void adpt_lea_ucc_source_audio_locations_callback(BD_ADDR remote_addr, uint32_t location)
{
    bt_address_t addr;

    BT_LOGD("%s, location:0x%08x", __func__, location);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_audio_localtion_event(&addr, true, location);
}

void apdt_lea_ucc_available_audio_contexts_callback(BD_ADDR remote_addr, uint16_t sink_ctxs,
                                                    uint16_t src_ctxs)
{
    bt_address_t addr;

    BT_LOGD("%s, sink_ctxs:0x%04x, source_ctxs:0x%04x", __func__, sink_ctxs, src_ctxs);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_available_audio_contexts_event(&addr, sink_ctxs, src_ctxs);
}

void adpt_lea_ucc_supported_audio_contexts_callback(BD_ADDR remote_addr, uint16_t sink_ctxs,
                                                    uint16_t src_ctxs)
{
    bt_address_t addr;

    BT_LOGD("%s, sink_ctxs:0x%04x, source_ctxs:0x%04x", __func__, sink_ctxs, src_ctxs);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_supported_audio_contexts_event(&addr, sink_ctxs, src_ctxs);
}

void adpt_lea_ucc_discover_complete_callback(BD_ADDR remote_addr, SERVICE_GATT_STATUS result)
{
    BT_LOGD("%s, result:%d", __func__, result);
}

void adpt_lea_ucc_config_codec_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                 SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    BT_LOGD("[Stream: 0x%08x][UCC configuring codec complete, result %d]\r\n>", stream_id, result);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_CONFIG_CODEC, result);
}

void adpt_lea_ucc_config_qos_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                               SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    BT_LOGD("[Stream: 0x%08x][UCC configuring QoS complete, result %d]\r\n>", stream_id, result);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_CONFIG_QOS, result);
}

void adpt_lea_ucc_enable_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                           SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    BT_LOGD("[Stream: 0x%08x][UCC enabling complete, result %d]\r\n>", stream_id, result);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_ENABLE, result);
}

void adpt_lea_ucc_disable_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                            SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    BT_LOGD("[Stream: 0x%08x][UCC disabling complete, result %d]\r\n>", stream_id, result);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_DISABLE, result);
}

void adpt_lea_ucc_release_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                            SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    BT_LOGD("[Stream: 0x%08x][UCC releasing complete, result %d]\r\n>", stream_id, result);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_RELEASE, result);
}

void adpt_lea_ucc_update_metadata_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                    SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    BT_LOGD("[Stream: 0x%08x][UCC updating metadata complete, result %d]\r\n>", stream_id, result);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_UPDATE_METADATA, result);
}

void adpt_lea_ucc_cap_annoucement_callback(BD_ADDR remote_addr, uint8_t type)
{
    BT_LOGD("%s,  type:%d", __func__, type);
}

void adpt_lea_ucc_bap_annoucement_callback(BD_ADDR remote_addr,
                                           SERVICE_LEA_BAP_ANNOUNCEMENT_S *announcement)
{
    BT_LOGD("%s, type:%d, available sink_ctx:0x%04x, available source_ctx:0x%04x",
            __func__, announcement->type, announcement->available_ctx.sink, announcement->available_ctx.source);
}

static const LEA_UCC_CALLBACK_S adpt_lea_ucc_client_callbacks = {
    .lea_ucc_pac_cb = adpt_lea_ucc_pac_callback,
    .lea_ucc_ase_cb = adpt_lea_ucc_ase_callback,
    .lea_ucc_sink_audio_locations_cb = adpt_lea_ucc_sink_audio_locations_callback,
    .lea_ucc_source_audio_locations_cb = adpt_lea_ucc_source_audio_locations_callback,
    .lea_ucc_available_audio_contexts_cb = apdt_lea_ucc_available_audio_contexts_callback,
    .lea_ucc_supported_audio_contexts_cb = adpt_lea_ucc_supported_audio_contexts_callback,
    .lea_ucc_discover_complete_cb = adpt_lea_ucc_discover_complete_callback,
    .lea_ucc_config_codec_complete_cb = adpt_lea_ucc_config_codec_complete_callback,
    .lea_ucc_config_qos_complete_cb = adpt_lea_ucc_config_qos_complete_callback,
    .lea_ucc_enable_complete_cb = adpt_lea_ucc_enable_complete_callback,
    .lea_ucc_disable_complete_cb = adpt_lea_ucc_disable_complete_callback,
    .lea_ucc_release_complete_cb = adpt_lea_ucc_release_complete_callback,
    .lea_ucc_update_metadata_complete_cb = adpt_lea_ucc_update_metadata_complete_callback,
    .lea_ucc_cap_annoucement_cb = adpt_lea_ucc_cap_annoucement_callback,
    .lea_ucc_bap_annoucement_cb = adpt_lea_ucc_bap_annoucement_callback,
};

static void adpt_stack_state_callback(bool enabled)
{
    lea_client_on_stack_state_changed((lea_client_stack_state_t)enabled);
}

static void adpt_storage_callback(void *data, uint32_t size)
{
    lea_client_on_storage_changed(data, size);
}

static void adpt_connection_state_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_connection_state_changed(&addr, state);
}

static void adpt_remote_services_callback(BD_ADDR remote_addr, uint8_t number, SERVICE_LEA_PRIMARY_SERVICE_S *services)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, services num:%d", __func__, number);

    if (number) {
        SERVICE_LEA_PRIMARY_SERVICE_S *current = services;
        SERVICE_LEA_PRIMARY_SERVICE_S *end = current + number;
        while (current < end) {
            BT_LOGD("%s, sid:[%04d], type:[%04x]", __func__, current->sid, current->type);
            current++;
        }
    }
}

static void adpt_stream_state_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id, bool added)
{
    bt_address_t addr;
    SERVICE_LEA_ISO_STREAM_ID_S *sid_s = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;

    BT_LOGD("%s, Stream ID:0x%08x, %s, GID:%d, SID:%d, ASE_ID:%d", __func__, stream_id, added ? "Added" : "Removed",
            sid_s->gid, sid_s->sid, sid_s->ase_id);
    BT_LOGD("%s, %s, %s", sid_s->features & LEA_IGIS_FEATURE_BROADCAST ? "BIS" : "CIS",
            sid_s->features & LEA_IGIS_FEATURE_INITIATOR ? "Initor" : "Acceptor",
            sid_s->features & LEA_IGIS_FEATURE_SOURCE ? "Source" : "Sink");
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    if (added) {
        lea_client_on_stream_added(&addr, stream_id);
    } else {
        lea_client_on_stream_removed(&addr, stream_id);
    }
}

static uint8_t lea_client_get_channel(uint32_t allocation)
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
    audio_stream.is_source = bt_sal_leac_is_source_stream(lea_stream->stream_id);
    memcpy(&audio_stream.codec_cfg, &lea_stream->codec_cfg, sizeof(lea_codec_config_t));
    audio_stream.channal_num = lea_client_get_channel(audio_stream.codec_cfg.allocation);
    audio_stream.sdu_size = audio_stream.channal_num * audio_stream.codec_cfg.blocks * audio_stream.codec_cfg.octets;

    BT_LOGD("%s, stream_id:0x%08x, is_source:%d, channal_num:%d, sdu_size:%d", __func__, lea_stream->stream_id,
            audio_stream.is_source, audio_stream.channal_num, audio_stream.sdu_size);
    lea_client_on_stream_started(&audio_stream);
}

static void adpt_streaming_stop_callback(LEA_IS_ID stream_id)
{
    lea_client_on_stream_stopped(stream_id);
}

static void adpt_stream_recv_callback(LEA_IS_ID stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data)
{
    lea_client_on_stream_recv(stream_id, iso_data->time_stamp, iso_data->sequenc_number,
                              iso_data->sdu, iso_data->sdu_length);
    stack_adapter_lea_mem_free(iso_data);
}

static const LEA_INIT_INFO_CALLBACK_S client_callbacks = {
    .lea_generic_cbks = &adpt_generic_callbacks,
    .lea_audio_stream_cbks = &adpt_audio_stream_callbacks,
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPS
    .lea_mcp_server_cbks = &adpt_lea_mcp_server_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
    .lea_ccp_server_cbks = &adpt_lea_ccp_server_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPC
    .lea_vcs_client_cbks = &adpt_lea_vcs_client_callbacks,
    .lea_mics_client_cbks = &adpt_lea_mics_client_callbacks,
    .lea_vocs_client_cbks = &adpt_lea_vocs_client_callbacks,
    .lea_aics_client_cbks = &adpt_lea_aics_client_callbacks,
#endif
    .lea_csip_client_cbks = &adpt_lea_csip_client_callbacks,
    .lea_uc_client_cbks = &adpt_lea_ucc_client_callbacks,
    .lea_bcsrc_cbks = &adpt_lea_bcsrc_callabcks,
};

bt_status_t bt_sal_lea_client_init()
{
    SERVICE_LEA_ROLE roles[3] = { LEA_ROLE_TMAP_CG, LEA_ROLE_TMAP_UMS, LEA_ROLE_TMAP_BMS };

    SAL_CHECK_RET(stack_adapter_lea_init(roles, 3, &client_callbacks),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_client_connect(bt_address_t *addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_client_connect(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_client_disconnect(bt_address_t *addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_disconnect(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_create(uint32_t *group_id, uint8_t salt, lea_ase_config_codec_t *codec,
                                        lea_ase_config_qos_t *qos)
{
    uint32_t gid;
    SERVICE_LEA_ASE_CONFIG_CODEC_OP_S *codec_op = (SERVICE_LEA_ASE_CONFIG_CODEC_OP_S *)codec;
    SERVICE_LEA_ASE_CONFIG_QOS_OP_S *qos_op = (SERVICE_LEA_ASE_CONFIG_QOS_OP_S *)qos;

    gid = stack_adapter_lea_get_iso_group_id(salt, false);
    SAL_CHECK_RET(stack_adapter_lea_ucc_group_create(gid, codec_op, qos_op), SERVICE_BT_STATUS_SUCCESS);

    *group_id = gid;
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_alloc_stream_id(uint32_t group_id, uint8_t cis_id, uint8_t ase_id, bool is_source,
                                       uint32_t *stream_id)
{
    *stream_id = stack_adapter_lea_get_iso_stream_id(group_id, cis_id, ase_id, is_source);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_free_stream_id(uint32_t stream_id)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_add_stream(uint32_t group_id, lea_audio_stream_t *stream)
{
    SERVICE_LEA_UCC_CIG_MEMBER_S member;
    SERVICE_LEA_ASE_CONFIG_CODEC_OP_S codec;

    memset(&member, 0, sizeof(SERVICE_LEA_UCC_CIG_MEMBER_S));
    memset(&codec, 0, sizeof(SERVICE_LEA_ASE_CONFIG_CODEC_OP_S));

    memcpy(member.peer_addr, &stream->addr, sizeof(BD_ADDR));
    member.codec_config = &codec;
    member.stream_id = stream->stream_id;
    member.allocation = stream->codec_cfg.allocation;

    codec.target_latency = stream->target_latency;
    codec.target_phy = stream->target_phy;
    codec.codec_cfg.codec_id.format = stream->codec_cfg.codec_id.format;
    codec.codec_cfg.mask = LEA_CSC_MASK_ALL;
    codec.codec_cfg.frequency = stream->codec_cfg.frequency;
    codec.codec_cfg.duration = stream->codec_cfg.duration;
    codec.codec_cfg.octets = stream->codec_cfg.octets;
    codec.codec_cfg.blocks = stream->codec_cfg.blocks;
    codec.codec_cfg.allocation = stream->codec_cfg.allocation;

    SAL_CHECK_RET(stack_adapter_lea_ucc_group_add_stream(&member), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_remove_stream(uint32_t group_id, uint8_t number, uint32_t *stream_id)
{
    SAL_CHECK_RET(stack_adapter_lea_ucc_group_remove_stream(0, number, stream_id), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_request_codec(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids)
{
    SAL_CHECK_RET(stack_adapter_lea_ucc_config_codec(0, stream_num, stream_ids), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_request_qos(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids)
{
    SAL_CHECK_RET(stack_adapter_lea_ucc_config_qos(0, stream_num, stream_ids), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_request_enable(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids, lea_metadata_t *metadata)
{
    SERVICE_LEA_ASE_ENABLING_PARAM_S enable_par[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];
    int index;
    SERVICE_LEA_METADATA_S md;

    md.type = metadata->type;
    md.u.streaming_contexts = metadata->streaming_contexts;
    for (index = 0; index < stream_num; index++) {
        enable_par[index].stream_id = stream_ids[index];
        enable_par[index].metadata_number = 1;
        enable_par[index].metadata = &md;
    }

    SAL_CHECK_RET(stack_adapter_lea_ucc_enable(0, stream_num, enable_par), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_request_disable(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids)
{
    SAL_CHECK_RET(stack_adapter_lea_ucc_disable(0, stream_num, stream_ids), SERVICE_BT_STATUS_SUCCESS);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_delete(uint32_t group_id)
{
    return BT_STATUS_SUCCESS;
}

void bt_sal_lea_client_cleanup()
{
    stack_adapter_lea_cleanup();
}

bool bt_sal_leac_is_source_stream(uint32_t stream_id)
{
    SERVICE_LEA_ISO_STREAM_ID_S *sid = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;
    return sid->features & LEA_IGIS_FEATURE_SOURCE;
}

lea_send_iso_data_t *bt_sal_leac_alloc_send_buffer(uint16_t length, uint16_t handle)
{
    return (lea_send_iso_data_t *)stack_adapter_lea_get_iso_data_sent_buffer(length, handle);
}

bt_status_t bt_sal_leac_send_iso_data(lea_send_iso_data_t *packet)
{
    SAL_CHECK_RET(stack_adapter_lea_send_iso_data((SERVICE_LEA_SENT_ISO_DATA_S *)packet), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif // CONFIG_BLUETOOTH_LEAUDIO_CLIENT