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
#ifndef __SAL_LEA_COMMON_INTERFACE_H__
#define __SAL_LEA_COMMON_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "sal_lea_client_interface.h"
#include "sal_lea_server_interface.h"

bt_status_t bt_sal_lea_init(void);
bt_status_t bt_sal_lea_alloc_stream_id(uint32_t group_id, uint8_t cis_id,
                                       uint8_t ase_id, bool is_source, uint32_t *stream_id);
bt_status_t bt_sal_lea_free_stream_id(uint32_t stream_id);
bt_status_t bt_sal_lea_disconnect(bt_address_t *addr);
bool bt_sal_lea_is_source_stream(uint32_t stream_id);
lea_send_iso_data_t *bt_sal_lea_alloc_send_buffer(uint16_t length, uint16_t handle);
bt_status_t bt_sal_lea_send_iso_data(lea_send_iso_data_t *packet);
void bt_sal_lea_cleanup(void);
#endif /* __SAL_LEA_COMMON_INTERFACE_H__ */