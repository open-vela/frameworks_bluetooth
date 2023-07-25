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
#ifndef __LEA_SERVER_SERVICE_H__
#define __LEA_SERVER_SERVICE_H__

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
    lea_metadata_t metadata_value[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_METADATA_MAX_NUMBER];
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
    void *(*register_callbacks)(void *remote,
                                const lea_client_callbacks_t *callbacks);
    bool (*unregister_callbacks)(void **remote, void *cookie);

    bt_status_t (*connect)(bt_address_t *addr);
    bt_status_t (*connect_audio)(bt_address_t *addr, uint8_t context);
    bt_status_t (*disconnect)(bt_address_t *addr);
    bt_status_t (*disconnect_audio)(bt_address_t *addr);
    profile_connection_state_t (*get_connection_state)(bt_address_t *addr);
} lea_client_interface_t;

lea_audio_stream_t *lea_client_add_stream(uint32_t stream_id, bt_address_t *remote_addr);
lea_audio_stream_t *lea_client_find_stream(uint32_t stream_id);
lea_audio_stream_t *lea_client_find_update_stream(lea_audio_stream_t *stream);
void lea_client_remove_stream(uint32_t stream_id);
void lea_client_remove_streams(void);

void lea_client_notify_stack_state_changed(lea_client_stack_state_t enabled);
void lea_client_notify_connection_state_changed(bt_address_t *addr, profile_connection_state_t state);

void lea_client_on_stack_state_changed(lea_client_stack_state_t state);
void lea_client_on_connection_state_changed(bt_address_t *addr,
                                            profile_connection_state_t state);
void lea_client_on_storage_changed(void *value, uint32_t size);

void lea_client_on_pac_event(bt_address_t *addr, lea_client_capability_t *cap);
void lea_client_on_ascs_event(bt_address_t *addr, uint8_t ase_state, bool is_source, uint8_t ase_id);
void lea_client_on_ascs_completed(bt_address_t *addr, uint32_t stream_id, uint8_t operation, uint8_t status);
void lea_client_on_audio_localtion_event(bt_address_t *addr, bool is_source, uint32_t allcation);
void lea_client_on_available_audio_contexts_event(bt_address_t *addr, uint32_t sink_ctxs, uint32_t source_ctxs);
void lea_client_on_supported_audio_contexts_event(bt_address_t *addr, uint32_t sink_ctxs, uint32_t source_ctxs);

bt_status_t lea_client_ucc_add_streams(bt_address_t *addr);
bt_status_t lea_client_ucc_remove_streams(bt_address_t *addr);
bt_status_t lea_client_ucc_config_codec(bt_address_t *addr);
bt_status_t lea_client_ucc_config_qos(bt_address_t *addr);
bt_status_t lea_client_ucc_enable(bt_address_t *addr);
bt_status_t lea_client_ucc_disable(bt_address_t *addr);

void lea_client_on_stream_added(bt_address_t *addr, uint32_t stream_id);
void lea_client_on_stream_removed(bt_address_t *addr, uint32_t stream_id);
void lea_client_on_stream_started(lea_audio_stream_t *stream);
void lea_client_on_stream_stopped(uint32_t stream_id);
void lea_client_on_stream_suspend(uint32_t stream_id);
void lea_client_on_metedata_updated(uint32_t stream_id);
void lea_client_on_stream_recv(uint32_t stream_id, uint32_t time_stamp,
                               uint16_t seq_number, uint8_t *sdu, uint16_t size);
void lea_client_on_stream_send(uint32_t stream_id);

/*
 * register profile to service manager
 */
void register_lea_client_service(void);

#endif /* __HFP_HF_SERVICE_H__ */
