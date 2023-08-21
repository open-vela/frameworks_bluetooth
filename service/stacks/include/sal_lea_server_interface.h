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

#include "bt_addr.h"
#include "bt_lea_server.h"
#include "bt_status.h"
#include "lea_server_service.h"

bt_status_t bt_sal_lea_server_init(void);
void bt_sal_lea_server_cleanup(void);

bt_status_t bt_sal_lea_server_start_announce(uint8_t id, uint8_t type,
                                             uint8_t *adv_data, uint8_t adv_size,
                                             uint8_t *md_data, uint8_t md_size);

bt_status_t bt_sal_lea_server_stop_announce(uint8_t adv_id);

bt_status_t bt_sal_lea_server_disconnect(bt_address_t *addr);

bt_status_t bt_sal_lea_server_request_disable(bt_address_t *addr, uint8_t ase_id);

lea_send_iso_data_t *bt_sal_leas_alloc_send_buffer(uint16_t length, uint16_t handle);

bool bt_sal_leas_is_source_stream(uint32_t stream_id);

bt_status_t bt_sal_leas_send_iso_data(lea_send_iso_data_t *packet);

#endif /* __SAL_LEA_SERVER_INTERFACE_H__ */