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
#include "bt_lea_server.h"

typedef enum {
    ADPT_LEA_ASE_STATE_IDLE,
    ADPT_LEA_ASE_STATE_CODEC_CONFIG,
    ADPT_LEA_ASE_STATE_QOS_CONFIG,
    ADPT_LEA_ASE_STATE_ENABLING,
    ADPT_LEA_ASE_STATE_STREAMING,
    ADPT_LEA_ASE_STATE_DISABLING,
    ADPT_LEA_ASE_STATE_RELEASING,
} lea_adpt_ase_state_t;

typedef struct {
    uint8_t format;
    uint16_t company_id;
    uint16_t codec_id;
} lea_codec_id_t;

typedef struct {
    lea_codec_id_t codec_id;
    uint16_t mask;
    uint8_t frequency;
    uint8_t duration;
    uint32_t allocation;
    uint16_t octets;
    uint8_t blocks;
} lea_codec_config_t;

typedef struct {
    uint32_t stream_id;
    uint16_t iso_handle;
    uint16_t max_sdu;
    uint8_t channal_num;
    bt_address_t addr;
    uint16_t sdu_size;
    bool is_source;
    lea_codec_config_t codec_cfg;
} lea_audio_stream_t;

typedef struct lea_server_interface {
    size_t size;

    void *(*register_callbacks)(void *remote,
                                const lea_server_callbacks_t *callbacks);
    bool (*unregister_callbacks)(void **remote, void *cookie);

    bt_status_t (*start_announce)(int8_t adv_id, uint8_t announce_type,
                                  uint8_t *adv_data, uint16_t adv_size,
                                  uint8_t *md_data, uint16_t md_size);
    bt_status_t (*stop_announce)(int8_t adv_id);
    bt_status_t (*disconnect)(bt_address_t *addr);
    profile_connection_state_t (*get_connection_state)(bt_address_t *addr);
} lea_server_interface_t;

lea_audio_stream_t *lea_server_add_stream(uint32_t stream_id, bt_address_t *remote_addr);
lea_audio_stream_t *lea_server_find_stream(uint32_t stream_id);
lea_audio_stream_t *lea_server_find_update_stream(lea_audio_stream_t *stream);
void lea_server_remove_stream(uint32_t stream_id);
void lea_server_remove_streams(void);

void lea_server_notify_stack_state_changed(lea_server_stack_state_t enabled);
void lea_server_notify_connection_state_changed(bt_address_t *addr, profile_connection_state_t state);

void lea_server_on_stack_state_changed(lea_server_stack_state_t state);
void lea_server_on_connection_state_changed(bt_address_t *addr,
                                            profile_connection_state_t state);
void lea_server_on_storage_changed(void *value, uint32_t size);
void lea_server_on_stream_added(bt_address_t *addr, uint32_t stream_id);
void lea_server_on_stream_removed(bt_address_t *addr, uint32_t stream_id);
void lea_server_on_stream_started(lea_audio_stream_t *stream);
void lea_server_on_stream_stopped(uint32_t stream_id);
void lea_server_on_stream_suspend(uint32_t stream_id);
void lea_server_on_metedata_updated(uint32_t stream_id);
void lea_server_on_stream_recv(uint32_t stream_id, uint32_t time_stamp,
                               uint16_t seq_number, uint8_t *sdu, uint16_t size);
void lea_server_on_stream_send(uint32_t stream_id);
void lea_server_on_ascs_event(bt_address_t *addr, lea_adpt_ase_state_t event, void *data);

/*
 * register profile to service manager
 */
void register_lea_server_service(void);

#endif /* __HFP_HF_SERVICE_H__ */
