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

#ifndef __BT_CONNECTION_MANAGER_H__
#define __BT_CONNECTION_MANAGER_H__

#include "bt_addr.h"
#include "bt_status.h"

void bt_cm_init(void);
void bt_cm_cleanup(void);

bt_status_t bt_cm_enable_enhanced_mode(bt_address_t* peer_addr, uint8_t mode);
bt_status_t bt_cm_disable_enhanced_mode(bt_address_t* peer_addr, uint8_t mode);

void bt_cm_process_disconnect_event(bt_address_t* addr, uint8_t transport, uint32_t hci_reason_code);
void bt_cm_disconnected(bt_address_t* addr, uint8_t profile_id);
void bt_cm_connected(bt_address_t* addr, uint8_t profile_id);
bt_status_t bt_cm_device_connect(bt_address_t* addr, uint8_t transport);
bt_status_t bt_cm_device_disconnect(bt_address_t* addr, uint8_t transport);

#endif /*__BT_CONNECTION_MANAGER_H__*/