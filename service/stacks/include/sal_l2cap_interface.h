/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#ifndef __SAL_L2CAP_INTERFACE_H__
#define __SAL_L2CAP_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_l2cap.h"
#include "bt_status.h"

bt_status_t bt_sal_l2cap_init(void);
void bt_sal_l2cap_cleanup(void);
bt_status_t bt_sal_l2cap_listen_channel(l2cap_config_option_t* option);
bt_status_t bt_sal_l2cap_stop_listen_channel(uint16_t psm);
bt_status_t bt_sal_l2cap_connect_channel(bt_address_t* addr, l2cap_config_option_t* option);
bt_status_t bt_sal_l2cap_disconnect_channel(uint16_t local_cid);
bt_status_t bt_sal_l2cap_send_packet(uint16_t local_cid, const uint8_t* data, uint16_t size);
bt_status_t bt_sal_l2cap_give_incoming_credits(bt_address_t* addr, uint16_t local_cid, uint16_t credits);

#endif /* __SAL_L2CAP_INTERFACE_H__ */
