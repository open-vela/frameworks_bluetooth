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
#ifndef __SAL_LEA_CLIENT_INTERFACE_H__
#define __SAL_LEA_CLIENT_INTERFACE_H__

#include <stdint.h>

#include "stack_adapter_lea_gaf.h"

#include "bt_addr.h"
#include "bt_lea_client.h"
#include "bt_status.h"
#include "lea_client_service.h"

void adpt_client_stream_state_callback(bt_address_t *addr, uint32_t stream_id, bool added);
void adpt_client_stream_start_callback(lea_audio_stream_t *lea_stream);
void adpt_client_stream_stop_callback(uint32_t stream_id);
void adpt_client_stream_recv_callback(uint32_t stream_id, SERVICE_LEA_RECV_ISO_DATA_S *iso_data);

bt_status_t bt_sal_lea_client_connect(bt_address_t *addr);
bt_status_t bt_sal_lea_ucc_discovery_service(bt_address_t *addr);
bt_status_t bt_sal_lea_ucc_group_create(uint32_t *group_id, uint8_t salt, lea_ase_config_codec_t *codec, lea_ase_config_qos_t *qos);
bt_status_t bt_sal_lea_ucc_group_delete(uint32_t group_id);
bt_status_t bt_sal_lea_ucc_group_add_stream(uint32_t group_id, lea_audio_stream_t *stream);
bt_status_t bt_sal_lea_ucc_group_remove_stream(uint32_t group_id, uint8_t number, uint32_t *stream_id);
bt_status_t bt_sal_lea_ucc_group_request_codec(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids);
bt_status_t bt_sal_lea_ucc_group_request_qos(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids);
bt_status_t bt_sal_lea_ucc_group_request_enable(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids, lea_metadata_t *metadata);
bt_status_t bt_sal_lea_ucc_group_request_disable(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids);
bt_status_t bt_sal_lea_ucc_group_request_release(uint32_t group_id);
bt_status_t bt_sal_lea_ucc_group_request_update_metadata(uint32_t group_id, uint8_t number, lea_metadata_t *data);
bt_status_t bt_sal_lea_ucc_group_request_delete(uint32_t group_id);

#endif /* __SAL_LEA_CLIENT_INTERFACE_H__ */