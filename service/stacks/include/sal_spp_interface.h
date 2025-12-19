/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef __SAL_SPP_INTERFACE_H__
#define __SAL_SPP_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "spp_service.h"

bt_status_t bt_sal_spp_init(void);
void bt_sal_spp_cleanup(void);
bt_status_t bt_sal_spp_server_start(uint16_t svr_port, bt_uuid_t* uuid128, uint8_t max_conn_cnt);
bt_status_t bt_sal_spp_server_stop(uint16_t svr_port);
bt_status_t bt_sal_spp_connect(bt_address_t* addr, uint16_t conn_port, bt_uuid_t* uuid128);
bt_status_t bt_sal_spp_disconnect(uint16_t conn_port);
bt_status_t bt_sal_spp_write(uint16_t conn_port, uint8_t* buffer, uint16_t length);
bt_status_t bt_sal_spp_add_credits(uint16_t conn_port, uint8_t credits);
bt_status_t bt_sal_spp_data_received_response(uint16_t conn_port, uint8_t* buffer);
bt_status_t bt_sal_spp_connect_request_reply(bt_address_t* addr, uint16_t conn_port, bool accept);
bt_status_t bt_sal_spp_connect_with_option(bt_address_t* addr, uint16_t conn_port, bt_uuid_t* uuid128, uint8_t insecure);

#endif /* __SAL_SPP_INTERFACE_H__ */