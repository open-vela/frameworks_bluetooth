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
#ifndef __SAL_LEA_SERVER_INTERFACE_H__
#define __SAL_LEA_SERVER_INTERFACE_H__

#include <stdint.h>

#include "stack_adapter_lea_gaf.h"

#include "bt_addr.h"
#include "bt_lea_server.h"
#include "bt_status.h"
#include "lea_server_service.h"

void adpt_server_stream_state_callback(bt_address_t *addr, uint32_t stream_id, bool added);
void adpt_server_stream_start_callback(lea_audio_stream_t *lea_stream);
void adpt_server_stream_stop_callback(uint32_t stream_id);
void adpt_server_stream_recv_callback(uint32_t stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data);

bool adpt_req_pacs_info_callback(SERVICE_LEA_PACS_INFO_S *info);
bool adpt_req_ascs_info_callback(SERVICE_LEA_ASCS_INFO_S *info);
bool adpt_req_bass_info_callback(SERVICE_LEA_BASS_INFO_S *info);

bt_status_t bt_sal_lea_server_start_announce(uint8_t id, uint8_t type,
                                             uint8_t *adv_data, uint8_t adv_size,
                                             uint8_t *md_data, uint8_t md_size);
bt_status_t bt_sal_lea_server_stop_announce(uint8_t adv_id);
bt_status_t bt_sal_lea_server_request_disable(bt_address_t *addr, uint8_t ase_id);

#endif /* __SAL_LEA_SERVER_INTERFACE_H__ */