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

static void adpt_lea_ucc_pac_callback(BD_ADDR remote_addr, SERVICE_LEA_PAC_INFO_S *pac_info);
static void adpt_lea_ucc_ase_callback(BD_ADDR remote_addr, SERVICE_LEA_ASE_VALUE_S *ase);
static void adpt_lea_ucc_sink_audio_locations_callback(BD_ADDR remote_addr, uint32_t location);
static void adpt_lea_ucc_source_audio_locations_callback(BD_ADDR remote_addr, uint32_t location);
static void apdt_lea_ucc_available_audio_contexts_callback(BD_ADDR remote_addr, uint16_t sink_ctxs, uint16_t src_ctxs);
static void adpt_lea_ucc_supported_audio_contexts_callback(BD_ADDR remote_addr, uint16_t sink_ctxs,
                                                           uint16_t src_ctxs);
static void adpt_lea_ucc_discover_complete_callback(BD_ADDR remote_addr, SERVICE_GATT_STATUS result);
static void adpt_lea_ucc_config_codec_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                        SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result);
static void adpt_lea_ucc_config_qos_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                      SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result);
static void adpt_lea_ucc_enable_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                  SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result);
static void adpt_lea_ucc_disable_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                   SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result);
static void adpt_lea_ucc_release_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                   SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result);
static void adpt_lea_ucc_update_metadata_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                           SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result);
static void adpt_lea_ucc_cap_annoucement_callback(BD_ADDR remote_addr, uint8_t type);
static void adpt_lea_ucc_bap_annoucement_callback(BD_ADDR remote_addr,
                                                  SERVICE_LEA_BAP_ANNOUNCEMENT_S *announcement);

const LEA_UCC_CALLBACK_S adpt_lea_ucc_client_callbacks = {
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

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_ucc_pac_callback(BD_ADDR remote_addr, SERVICE_LEA_PAC_INFO_S *pac_info)
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
    memcpy(cap.metadata_value, pac_info->metadata, sizeof(SERVICE_LEA_METADATA_S) * cap.metadata_number);

    BT_LOGD("%s, addr:%s, pac_id:0x%08x, type:%s, codec_format:%d", __func__, bt_addr_str(&addr), pac_info->pac_id, pac_info->pac_type == LEA_PAC_TYPE_SINK_PAC ? "Sink" : "Source", pac_info->codec_id.format);
    BT_LOGD("frequencies:%d, durations:%d, channels:%d, frame_octets_min:%d, frame_octets_max:%d, max_frames:%d",
            cc->frequencies, cc->durations, cc->channels, cc->frame_octets_min, cc->frame_octets_max, cc->max_frames);

    lea_client_on_pac_event(&addr, &cap);
    stack_adapter_lea_recycle_pac_s(pac_info);
}

static void adpt_lea_ucc_ase_callback(BD_ADDR remote_addr, SERVICE_LEA_ASE_VALUE_S *ase)
{
    char *state[] = { "Idle", "Codec_Config", "QoS_Config",
                      "Enabling", "Streaming", "Disabling", "Releasing" };
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, addr:%s, ASE_ID:%d, State:%s, Type:%s", __func__, bt_addr_str(&addr), ase->ase_id,
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

static void adpt_lea_ucc_sink_audio_locations_callback(BD_ADDR remote_addr, uint32_t location)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, addr:%s, location:0x%08x", __func__, bt_addr_str(&addr), location);
    lea_client_on_audio_localtion_event(&addr, false, location);
}

static void adpt_lea_ucc_source_audio_locations_callback(BD_ADDR remote_addr, uint32_t location)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, addr:%s, location:0x%08x", __func__, bt_addr_str(&addr), location);
    lea_client_on_audio_localtion_event(&addr, true, location);
}

static void apdt_lea_ucc_available_audio_contexts_callback(BD_ADDR remote_addr, uint16_t sink_ctxs,
                                                           uint16_t src_ctxs)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, addr:%s, sink_ctxs:0x%04x, source_ctxs:0x%04x", __func__, bt_addr_str(&addr), sink_ctxs, src_ctxs);
    lea_client_on_available_audio_contexts_event(&addr, sink_ctxs, src_ctxs);
}

static void adpt_lea_ucc_supported_audio_contexts_callback(BD_ADDR remote_addr, uint16_t sink_ctxs,
                                                           uint16_t src_ctxs)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, addr:%s, sink_ctxs:0x%04x, source_ctxs:0x%04x", __func__, bt_addr_str(&addr), sink_ctxs, src_ctxs);
    lea_client_on_supported_audio_contexts_event(&addr, sink_ctxs, src_ctxs);
}

static void adpt_lea_ucc_discover_complete_callback(BD_ADDR remote_addr, SERVICE_GATT_STATUS result)
{
    BT_LOGD("%s, result:%d", __func__, result);
}

static void adpt_lea_ucc_config_codec_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                        SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("addr:%s, [Stream: 0x%08x][UCC configuring codec complete, result %d]", bt_addr_str(&addr), stream_id, result);
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_CONFIG_CODEC, result);
}

static void adpt_lea_ucc_config_qos_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                      SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("addr:%s, [Stream: 0x%08x][UCC configuring QoS complete, result %d]", bt_addr_str(&addr), stream_id, result);
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_CONFIG_QOS, result);
}

static void adpt_lea_ucc_enable_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                  SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("addr:%s, [Stream: 0x%08x][UCC enabling complete, result %d]", bt_addr_str(&addr), stream_id, result);
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_ENABLE, result);
}

static void adpt_lea_ucc_disable_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                   SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("addr:%s, [Stream: 0x%08x][UCC disabling complete, result %d]", bt_addr_str(&addr), stream_id, result);
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_DISABLE, result);
}

static void adpt_lea_ucc_release_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                   SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("addr:%s, [Stream: 0x%08x][UCC releasing complete, result %d]", bt_addr_str(&addr), stream_id, result);
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_RELEASE, result);
}

static void adpt_lea_ucc_update_metadata_complete_callback(BD_ADDR remote_addr, LEA_IS_ID stream_id,
                                                           SERVICE_LEA_ASE_CONTROL_RESPONSE_CODE result)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("addr:%s, [Stream: 0x%08x][UCC updating metadata complete, result %d]", bt_addr_str(&addr), stream_id, result);
    lea_client_on_ascs_completed(&addr, stream_id, LEA_ASE_OP_UPDATE_METADATA, result);
}

static void adpt_lea_ucc_cap_annoucement_callback(BD_ADDR remote_addr, uint8_t type)
{
    BT_LOGD("%s,  type:%d", __func__, type);
}

static void adpt_lea_ucc_bap_annoucement_callback(BD_ADDR remote_addr,
                                                  SERVICE_LEA_BAP_ANNOUNCEMENT_S *announcement)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    BT_LOGD("%s, addr:%s, type:%d, available sink_ctx:0x%04x, available source_ctx:0x%04x", bt_addr_str(&addr),
            __func__, announcement->type, announcement->available_ctx.sink, announcement->available_ctx.source);
}

/****************************************************************************
 * Public function
 ****************************************************************************/
void adpt_client_stream_state_callback(bt_address_t *addr, uint32_t stream_id, bool added)
{
    if (added) {
        lea_client_on_stream_added(addr, stream_id);
    } else {
        lea_client_on_stream_removed(addr, stream_id);
    }
}

void adpt_client_stream_start_callback(lea_audio_stream_t *audio_stream)
{
    lea_client_on_stream_started(audio_stream);
}

void adpt_client_stream_stop_callback(uint32_t stream_id)
{
    lea_client_on_stream_stopped(stream_id);
}

void adpt_client_stream_recv_callback(uint32_t stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data)
{
    lea_client_on_stream_recv(stream_id, iso_data->time_stamp, iso_data->sequenc_number,
                              iso_data->sdu, iso_data->sdu_length);
    stack_adapter_lea_mem_free(iso_data);
}

bt_status_t bt_sal_lea_client_connect(bt_address_t *addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_client_connect(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_discovery_service(bt_address_t *addr)
{
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
    SERVICE_LEA_ASE_ENABLING_PARAM_S enable_par[LEA_CLIENT_MAX_STREAM_NUM];
    int index;
    SERVICE_LEA_METADATA_S md;

    for (index = 0; index < stream_num; index++) {
        md.type = metadata[index].type;
        md.u.streaming_contexts = metadata[index].streaming_contexts;
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

bt_status_t bt_sal_lea_ucc_group_request_release(uint32_t group_id)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_request_update_metadata(uint32_t group_id, uint8_t number, lea_metadata_t *data)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_ucc_group_request_delete(uint32_t group_id)
{
    return BT_STATUS_SUCCESS;
}

#endif // CONFIG_BLUETOOTH_LEAUDIO_CLIENT