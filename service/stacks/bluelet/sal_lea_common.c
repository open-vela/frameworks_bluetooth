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
#include "lea_ccp_service.h"
#include "lea_client_service.h"
#include "lea_mcpc_service.h"
#include "lea_server_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_ccp_interface.h"
#include "sal_lea_client_interface.h"
#include "sal_lea_csis_interface.h"
#include "sal_lea_mcpc_interface.h"
#include "sal_lea_mcps_interface.h"
#include "sal_lea_server_interface.h"
#include "sal_lea_tbs_interface.h"
#include "sal_lea_vmicpc_interface.h"
#include "sal_lea_vmicps_interface.h"

static void adpt_stack_state_callback(bool enabled);
static void adpt_storage_callback(void *data, uint32_t size);
static void adpt_connection_state_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state, bool initiator);
static void adpt_remote_services_callback(BD_ADDR remote_addr, uint8_t number, SERVICE_LEA_PRIMARY_SERVICE_S *services);

static void adpt_stream_state_callback(BD_ADDR remote_addr, uint32_t stream_id, bool added);
static void adpt_stream_start_callback(SERVICE_LEA_AUDIO_STREAM_S *lea_stream);
static void adpt_stream_stop_callback(uint32_t stream_id);
static void adpt_stream_recv_callback(uint32_t stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data);

bool bt_sal_lea_is_source_stream(uint32_t stream_id);

static const LEA_GENERIC_CALLBACK_S adpt_generic_callbacks = {
    .lea_app_state_cb = adpt_stack_state_callback,
    .lea_app_storage_cb = adpt_storage_callback,
    .lea_connection_state_changed_cb = adpt_connection_state_callback,
    .lea_remote_services_cb = adpt_remote_services_callback,
};

static const LEA_AUDIO_STREAM_CALLBACK_S adpt_audio_stream_callbacks = {
    .lea_audio_stream_state_cb = adpt_stream_state_callback,
    .lea_streaming_start_cb = adpt_stream_start_callback,
    .lea_streaming_stop_cb = adpt_stream_stop_callback,
    .lea_received_iso_data_cb = adpt_stream_recv_callback,
};
static bool g_lea_inited = false;

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPS
extern const LEA_MCS_CALLBACK_S adpt_lea_mcp_server_callbacks;
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPC
extern const LEA_MCC_CALLBACK_S adpt_lea_mcp_client_callbacks;
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
extern const LEA_TBS_CALLBACK_S adpt_lea_ccp_server_callbacks;
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP
extern const LEA_TBC_CALLBACK_S adpt_lea_ccp_client_callbacks;
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPC
extern const LEA_VCC_CALLBACK_S adpt_lea_vcs_client_callbacks;
extern const LEA_MICC_CALLBACK_S adpt_lea_mics_client_callbacks;
static const LEA_VOCC_CALLBACK_S adpt_lea_vocs_client_callbacks; // todo
static const LEA_AICC_CALLBACK_S adpt_lea_aics_client_callbacks; // todo
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS
extern const LEA_VCS_CALLBACK_S adpt_lea_vcs_server_callbacks;
extern const LEA_MICS_CALLBACK_S adpt_lea_mics_server_callbacks;
static const LEA_VOCS_CALLBACK_S adpt_lea_vocs_server_callbacks; // todo
static const LEA_AICS_CALLBACK_S adpt_lea_aics_server_callbacks; // todo
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
extern const LEA_CSIC_CALLBACK_S adpt_lea_csip_client_callbacks;
extern const LEA_UCC_CALLBACK_S adpt_lea_ucc_client_callbacks;
static const LEA_BCSRC_CALLBACK_S adpt_lea_bcsrc_callabcks; // todo
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
extern const LEA_CSIS_CALLBACK_S adpt_lea_csip_server_callbacks;
extern const LEA_UCS_CALLBACK_S adpt_lea_uc_server_callbacks;
#endif

static const LEA_INIT_INFO_CALLBACK_S lea_callbacks = {
    .lea_generic_cbks = &adpt_generic_callbacks,
    .lea_audio_stream_cbks = &adpt_audio_stream_callbacks,
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPS
    .lea_mcp_server_cbks = &adpt_lea_mcp_server_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPC
    .lea_mcp_client_cbks = &adpt_lea_mcp_client_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
    .lea_ccp_server_cbks = &adpt_lea_ccp_server_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP
    .lea_ccp_client_cbks = &adpt_lea_ccp_client_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPC
    .lea_vcs_client_cbks = &adpt_lea_vcs_client_callbacks,
    .lea_mics_client_cbks = &adpt_lea_mics_client_callbacks,
    .lea_vocs_client_cbks = &adpt_lea_vocs_client_callbacks,
    .lea_aics_client_cbks = &adpt_lea_aics_client_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICPS
    .lea_vcs_server_cbks = &adpt_lea_vcs_server_callbacks,
    .lea_mics_server_cbks = &adpt_lea_mics_server_callbacks,
    .lea_vocs_server_cbks = &adpt_lea_vocs_server_callbacks,
    .lea_aics_server_cbks = &adpt_lea_aics_server_callbacks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
    .lea_csip_client_cbks = &adpt_lea_csip_client_callbacks,
    .lea_uc_client_cbks = &adpt_lea_ucc_client_callbacks,
    .lea_bcsrc_cbks = &adpt_lea_bcsrc_callabcks,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
    .lea_request_vcs_info_cb = adpt_req_vcs_info_callback,
    .lea_request_mics_info_cb = adpt_req_mics_info_callback,
    .lea_request_pacs_info_cb = adpt_req_pacs_info_callback,
    .lea_request_ascs_info_cb = adpt_req_ascs_info_callback,
    .lea_request_bass_info_cb = adpt_req_bass_info_callback,
    .lea_request_csis_info_cb = adpt_req_csis_info_callback,
    .lea_csip_server_cbks = &adpt_lea_csip_server_callbacks,
    .lea_uc_server_cbks = &adpt_lea_uc_server_callbacks,
#endif
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_stack_state_callback(bool enabled)
{
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
    lea_client_on_stack_state_changed((lea_client_stack_state_t)enabled);
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
    lea_server_on_stack_state_changed((lea_server_stack_state_t)enabled);
#endif
}

static void adpt_storage_callback(void *data, uint32_t size)
{
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
    lea_client_on_storage_changed(data, size);
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
    lea_server_on_storage_changed(data, size);
#endif
}

static void adpt_connection_state_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state, bool initiator)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    if (initiator) {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
        lea_client_on_connection_state_changed(&addr, state);
#endif
    } else {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
        lea_server_on_connection_state_changed(&addr, state);
#endif
    }
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
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP
            if (current->type == GATT_UUID_GENERIC_TELEPHONE_BEARER) {
                adpt_tbs_sid_changed(current->sid);
            }
#endif
            current++;
        }
    }
}

static void adpt_stream_state_callback(BD_ADDR remote_addr, uint32_t stream_id, bool added)
{
    bt_address_t addr;
    SERVICE_LEA_ISO_STREAM_ID_S *sid_s = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));

    BT_LOGD("%s, Addr:%s, Stream ID:0x%08x, %s, GID:%d, SID:%d, ASE_ID:%d", __func__, bt_addr_str(&addr), stream_id, added ? "Added" : "Removed", sid_s->gid, sid_s->sid, sid_s->ase_id);
    BT_LOGD("%s, %s, %s", sid_s->features & LEA_IGIS_FEATURE_BROADCAST ? "BIS" : "CIS",
            sid_s->features & LEA_IGIS_FEATURE_INITIATOR ? "Initor" : "Acceptor",
            sid_s->features & LEA_IGIS_FEATURE_SOURCE ? "Source" : "Sink");

    if (sid_s->features & LEA_IGIS_FEATURE_INITIATOR) {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
        adpt_client_stream_state_callback(&addr, stream_id, added);
#endif
    } else {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
        adpt_server_stream_state_callback(&addr, stream_id, added);
#endif
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

static void adpt_stream_start_callback(SERVICE_LEA_AUDIO_STREAM_S *lea_stream)
{
    lea_audio_stream_t audio_stream;
    SERVICE_LEA_ISO_STREAM_ID_S *sid_s;
    uint32_t stream_id;

    sid_s = (SERVICE_LEA_ISO_STREAM_ID_S *)&lea_stream->stream_id;
    stream_id = lea_stream->stream_id;

    memset(&audio_stream, 0, sizeof(lea_audio_stream_t));
    audio_stream.stream_id = stream_id;
    audio_stream.iso_handle = lea_stream->iso_handle;
    audio_stream.max_sdu = lea_stream->max_sdu;
    audio_stream.is_source = bt_sal_lea_is_source_stream(stream_id);
    memcpy(&audio_stream.codec_cfg, &lea_stream->codec_cfg, sizeof(lea_codec_config_t));
    audio_stream.channal_num = lea_client_get_channel(audio_stream.codec_cfg.allocation);
    audio_stream.sdu_size = audio_stream.channal_num * audio_stream.codec_cfg.blocks * audio_stream.codec_cfg.octets;

    BT_LOGD("%s, stream_id:0x%08x, is_source:%d, channal_num:%d, sdu_size:%d", __func__, stream_id,
            audio_stream.is_source, audio_stream.channal_num, audio_stream.sdu_size);

    if (sid_s->features & LEA_IGIS_FEATURE_INITIATOR) {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
        adpt_client_stream_start_callback(&audio_stream);
#endif
    } else {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
        adpt_server_stream_start_callback(&audio_stream);
#endif
    }
}

static void adpt_stream_stop_callback(uint32_t stream_id)
{
    SERVICE_LEA_ISO_STREAM_ID_S *sid_s = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;

    if (sid_s->features & LEA_IGIS_FEATURE_INITIATOR) {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
        adpt_client_stream_stop_callback(stream_id);
#endif
    } else {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
        adpt_server_stream_stop_callback(stream_id);
#endif
    }
}

static void adpt_stream_recv_callback(uint32_t stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data)
{
    SERVICE_LEA_ISO_STREAM_ID_S *sid_s = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;

    if (sid_s->features & LEA_IGIS_FEATURE_INITIATOR) {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
        adpt_client_stream_recv_callback(stream_id, iso_data);
#endif
    } else {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
        adpt_server_stream_recv_callback(stream_id, iso_data);
#endif
    }
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bt_status_t bt_sal_lea_init()
{
    SERVICE_LEA_ROLE roles[] = {
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
        LEA_ROLE_TMAP_CG,
        LEA_ROLE_TMAP_UMS,
        LEA_ROLE_TMAP_BMS,
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
        LEA_ROLE_TMAP_CT,
        LEA_ROLE_TMAP_UMR,
        LEA_ROLE_TMAP_BMR
#endif
    };
    int num = sizeof(roles) / sizeof(roles[0]);

    if (g_lea_inited) {
        return BT_STATUS_SUCCESS;
    }

    g_lea_inited = true;

    SAL_CHECK_RET(stack_adapter_lea_init(roles, num, &lea_callbacks),
                  SERVICE_BT_STATUS_SUCCESS);

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

bt_status_t bt_sal_lea_disconnect(bt_address_t *addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_disconnect(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bool bt_sal_lea_is_source_stream(uint32_t stream_id)
{
    SERVICE_LEA_ISO_STREAM_ID_S *sid = (SERVICE_LEA_ISO_STREAM_ID_S *)&stream_id;
    return sid->features & LEA_IGIS_FEATURE_SOURCE;
}

lea_send_iso_data_t *bt_sal_lea_alloc_send_buffer(uint16_t length, uint16_t handle)
{
    return (lea_send_iso_data_t *)stack_adapter_lea_get_iso_data_sent_buffer(length, handle);
}

bt_status_t bt_sal_lea_send_iso_data(lea_send_iso_data_t *packet)
{
    SAL_CHECK_RET(stack_adapter_lea_send_iso_data((SERVICE_LEA_SENT_ISO_DATA_S *)packet), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_lea_cleanup(void)
{
    g_lea_inited = false;
    stack_adapter_lea_cleanup();
}