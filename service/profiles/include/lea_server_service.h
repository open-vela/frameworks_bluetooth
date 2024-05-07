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
#include "lea_audio_common.h"

typedef struct lea_server_interface {
    size_t size;

    void* (*register_callbacks)(void* remote,
        const lea_server_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** remote, void* cookie);

    bt_status_t (*start_announce)(int8_t adv_id, uint8_t announce_type,
        uint8_t* adv_data, uint16_t adv_size,
        uint8_t* md_data, uint16_t md_size);
    bt_status_t (*stop_announce)(int8_t adv_id);
    bt_status_t (*disconnect)(bt_address_t* addr);
    bt_status_t (*disconnect_audio)(bt_address_t* addr);
    profile_connection_state_t (*get_connection_state)(bt_address_t* addr);
} lea_server_interface_t;

lea_audio_stream_t* lea_server_add_stream(uint32_t stream_id, bt_address_t* remote_addr);
lea_audio_stream_t* lea_server_find_stream(uint32_t stream_id);
lea_audio_stream_t* lea_server_update_stream(lea_audio_stream_t* stream);
void lea_server_remove_stream(uint32_t stream_id);
void lea_server_remove_streams(void);

void lea_server_notify_stack_state_changed(lea_server_stack_state_t enabled);
void lea_server_notify_connection_state_changed(bt_address_t* addr, profile_connection_state_t state);

void lea_server_on_stack_state_changed(lea_server_stack_state_t state);
void lea_server_on_connection_state_changed(bt_address_t* addr,
    profile_connection_state_t state);
void lea_server_on_storage_changed(void* value, uint32_t size);
void lea_server_on_stream_added(bt_address_t* addr, uint32_t stream_id);
void lea_server_on_stream_removed(bt_address_t* addr, uint32_t stream_id);
void lea_server_on_stream_started(lea_audio_stream_t* stream);
void lea_server_on_stream_stopped(uint32_t stream_id);
void lea_server_on_stream_suspend(uint32_t stream_id);
void lea_server_on_metedata_updated(uint32_t stream_id);
void lea_server_on_stream_recv(uint32_t stream_id, uint32_t time_stamp,
    uint16_t seq_number, uint8_t* sdu, uint16_t size);
bt_status_t lea_server_streams_started(bt_address_t* addr);
void lea_server_on_ascs_event(bt_address_t* addr, uint8_t id, uint8_t state, uint16_t type);

void lea_server_on_csis_lock_state_changed(uint32_t csis_id, bt_address_t* addr, uint8_t lock);

bool lea_server_on_pacs_info_request(lea_pacs_info_t* pacs_info);
bool lea_server_on_ascs_info_request(lea_ascs_info_t* ascs_info);
bool lea_server_on_bass_info_request(lea_bass_info_t* bass_info);
bool lea_server_on_csis_info_request(lea_csis_infos_t* csis_info);

/*
 * register profile to service manager
 */
void register_lea_server_service(void);

#endif /* __HFP_HF_SERVICE_H__ */
