/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __BT_L2CAP_SERVICE_H_
#define __BT_L2CAP_SERVICE_H_

#include <stdint.h>

#include "bt_l2cap.h"

typedef struct {
    uint16_t mtu;
    uint16_t le_mps;
    uint16_t init_credits;
} l2cap_endpoint_param_t;

typedef struct {
    uint16_t cid;
    uint16_t psm;
    bt_transport_t transport;
    l2cap_endpoint_param_t incoming;
    l2cap_endpoint_param_t outgoing;
} l2cap_channel_param_t;

void l2cap_on_channel_connected(bt_address_t* addr, l2cap_channel_param_t* param);
void l2cap_on_channel_disconnected(bt_address_t* addr, uint16_t cid, uint32_t reason);
void l2cap_on_packet_received(bt_address_t* addr, uint16_t cid, uint8_t* packet_data, uint16_t packet_size);
void l2cap_on_packet_sent(bt_address_t* addr, uint16_t cid);

void* l2cap_register_callbacks(void* remote, const l2cap_callbacks_t* callbacks);
bool l2cap_unregister_callbacks(void** remote, void* cookie);
bt_status_t l2cap_listen_channel(l2cap_config_option_t* option);
bt_status_t l2cap_connect_channel(bt_address_t* addr, l2cap_config_option_t* option);
bt_status_t l2cap_disconnect_channel(uint16_t cid);

bt_status_t l2cap_service_init(void);
void l2cap_service_cleanup(void);

#endif /* __BT_L2CAP_SERVICE_H_ */