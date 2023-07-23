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

#include "bt_addr.h"
#include "bt_lea_client.h"
#include "bt_status.h"
#include "lea_client_service.h"

bt_status_t bt_sal_lea_client_init(void);
void bt_sal_lea_client_cleanup(void);

bt_status_t bt_sal_lea_client_connect(bt_address_t *addr);
bt_status_t bt_sal_lea_client_disconnect(bt_address_t *addr);
bt_status_t bt_sal_lea_ucc_discovery_service(bt_address_t *addr);
bt_status_t bt_sal_lea_ucc_group_create(uint32_t *group_id, uint8_t salt, lea_ase_config_codec_t *codec, lea_ase_config_qos_t *qos);
bt_status_t bt_sal_lea_alloc_stream_id(uint32_t group_id, uint8_t cis_id, uint8_t ase_id, bool is_source,
                                       uint32_t *stream_id);
bt_status_t bt_sal_lea_free_stream_id(uint32_t stream_id);

bt_status_t bt_sal_lea_ucc_group_add_stream(uint32_t group_id, lea_audio_stream_t *stream);
bt_status_t bt_sal_lea_ucc_group_remove_stream(uint32_t group_id, uint8_t number, uint32_t *stream_id);
bt_status_t bt_sal_lea_ucc_group_request_codec(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids);
bt_status_t bt_sal_lea_ucc_group_request_qos(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids);
bt_status_t bt_sal_lea_ucc_group_request_enable(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids, lea_metadata_t *metadata);
bt_status_t bt_sal_lea_ucc_group_request_disable(uint32_t group_id, uint8_t stream_num, uint32_t *stream_ids);
bt_status_t bt_sal_lea_ucc_group_request_release(uint32_t group_id);
bt_status_t bt_sal_lea_ucc_group_request_update_metadata(uint32_t group_id, uint8_t number, lea_metadata_t *data);
bt_status_t bt_sal_lea_ucc_group_request_delete(uint32_t group_id);

lea_send_iso_data_t *bt_sal_lea_alloc_send_buffer(uint16_t length, uint16_t handle);
bool bt_sal_lea_is_source_stream(uint32_t stream_id);
bt_status_t bt_sal_lea_send_iso_data(lea_send_iso_data_t *packet);

#endif /* __SAL_LEA_CLIENT_INTERFACE_H__ */