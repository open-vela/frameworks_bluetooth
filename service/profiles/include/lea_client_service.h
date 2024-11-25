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
#ifndef __LEA_CLIENT_SERVICE_H__
#define __LEA_CLIENT_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_device.h"
#include "bt_lea_client.h"
#include "lea_audio_common.h"

typedef struct {
    bool is_source;
    uint32_t pac_id;
    lea_codec_id_t codec_id;
    lea_codec_cap_t codec_cap;
    uint8_t metadata_number;
    lea_metadata_t metadata_value[LEA_CLIENT_MAX_STREAM_NUM];
} lea_client_capability_t;

typedef struct {
    uint8_t target_latency;
    uint8_t target_phy;
    lea_codec_config_t codec_cfg;
} lea_ase_config_codec_t;

typedef struct {
    uint32_t sdu_interval;
    uint8_t framing;
    uint8_t phy;
    uint16_t max_sdu;
    uint8_t rtn;
    uint16_t max_latency;
    uint32_t delay;
} lea_ase_config_qos_t;

typedef struct lea_client_interface {
    size_t size;
    void* (*register_callbacks)(void* remote,
        const lea_client_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** remote, void* cookie);

    bt_status_t (*connect)(bt_address_t* addr);
    bt_status_t (*connect_audio)(bt_address_t* addr, uint8_t context);
    bt_status_t (*disconnect)(bt_address_t* addr);
    bt_status_t (*disconnect_audio)(bt_address_t* addr);
    bt_status_t (*get_group_id)(bt_address_t* addr, uint32_t* group_id);
    bt_status_t (*discovery_member_start)(uint32_t group_id);
    bt_status_t (*discovery_member_stop)(uint32_t group_id);
    bt_status_t (*group_add_member)(uint32_t group_id, bt_address_t* addr);
    bt_status_t (*group_remove_member)(uint32_t group_id, bt_address_t* addr);
    bt_status_t (*group_connect_audio)(uint32_t group_id, uint8_t context);
    bt_status_t (*group_disconnect_audio)(uint32_t group_id);
    bt_status_t (*group_lock)(uint32_t group_id);
    bt_status_t (*group_unlock)(uint32_t group_id);
    profile_connection_state_t (*get_connection_state)(bt_address_t* addr);
} lea_client_interface_t;

lea_audio_stream_t* lea_client_add_stream(uint32_t stream_id, bt_address_t* remote_addr);
lea_audio_stream_t* lea_client_find_stream(uint32_t stream_id);
lea_audio_stream_t* lea_client_update_stream(lea_audio_stream_t* stream);
void lea_client_remove_stream(uint32_t stream_id);
void lea_client_remove_streams(void);

void lea_client_notify_stack_state_changed(lea_client_stack_state_t enabled);
void lea_client_notify_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);

void lea_client_on_stack_state_changed(lea_client_stack_state_t state);
void lea_client_on_connection_state_changed(bt_address_t* addr,
    profile_connection_state_t state);
void lea_client_on_storage_changed(void* value, uint32_t size);

void lea_client_on_pac_event(bt_address_t* addr, lea_client_capability_t* cap);
void lea_client_on_ascs_event(bt_address_t* addr, uint8_t ase_state, bool is_source, uint8_t ase_id);
void lea_client_on_ascs_completed(bt_address_t* addr, uint32_t stream_id, uint8_t operation, uint8_t status);
void lea_client_on_audio_localtion_event(bt_address_t* addr, bool is_source, uint32_t allcation);
void lea_client_on_available_audio_contexts_event(bt_address_t* addr, uint32_t sink_ctxs, uint32_t source_ctxs);
void lea_client_on_supported_audio_contexts_event(bt_address_t* addr, uint32_t sink_ctxs, uint32_t source_ctxs);

bt_status_t lea_client_ucc_add_streams(uint32_t group_id, bt_address_t* addr);
bt_status_t lea_client_ucc_remove_streams(uint32_t group_id, bt_address_t* addr);
bt_status_t lea_client_ucc_config_codec(uint32_t group_id, bt_address_t* addr);
bt_status_t lea_client_ucc_config_qos(uint32_t group_id, bt_address_t* addr, uint32_t stream_id);
bt_status_t lea_client_ucc_enable(uint32_t group_id, bt_address_t* addr, uint32_t stream_id);
bt_status_t lea_client_ucc_disable(uint32_t group_id, bt_address_t* addr);
bt_status_t lea_client_ucc_started(uint32_t group_id);

void lea_client_on_stream_added(bt_address_t* addr, uint32_t stream_id);
void lea_client_on_stream_removed(bt_address_t* addr, uint32_t stream_id);
void lea_client_on_stream_started(lea_audio_stream_t* stream);
void lea_client_on_stream_stopped(uint32_t stream_id);
void lea_client_on_stream_suspend(uint32_t stream_id);
void lea_client_on_metedata_updated(uint32_t stream_id);
void lea_client_on_stream_recv(uint32_t stream_id, uint32_t time_stamp,
    uint16_t seq_number, uint8_t* sdu, uint16_t size);

/*
 * sal callback
 */

void lea_client_on_csip_sirk_event(bt_address_t* addr, uint8_t type, uint8_t* sirk);
void lea_client_on_csip_size_event(bt_address_t* addr, uint8_t cs_size);
void lea_client_on_csip_member_lock(bt_address_t* addr, uint8_t lock);
void lea_client_on_csip_member_rank_event(bt_address_t* addr, uint8_t rank);
void lea_client_on_csip_set_created(uint8_t* sirk);
void lea_client_on_csip_set_size_updated(uint8_t* sirk, uint8_t size);
void lea_client_on_csip_set_removed(uint8_t* sirk);
void lea_client_on_csip_set_member_discovered(bt_address_t* addr, uint8_t* sirk);
void lea_client_on_csip_set_member_added(bt_address_t* addr, uint8_t* sirk);
void lea_client_on_csip_set_member_removed(bt_address_t* addr, uint8_t* sirk);
void lea_client_on_csip_discovery_terminated(uint8_t* sirk);
void lea_client_on_csip_set_lock_changed(uint8_t* sirk, bool locked, lea_csip_lock_status result);
void lea_client_on_csip_set_ordered_access(uint8_t* sirk, lea_csip_lock_status result);

/*
 * register profile to service manager
 */
void register_lea_client_service(void);

#endif /* __HFP_HF_SERVICE_H__ */
