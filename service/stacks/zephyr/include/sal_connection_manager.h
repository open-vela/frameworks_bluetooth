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
#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_profile.h"

typedef struct {
    bt_address_t addr;
    uint8_t profile_id;
} cm_data_t;

typedef bt_status_t (*bt_profile_conn_handler_t)(
    bt_controller_id_t id, bt_address_t* addr);

typedef struct {
    bt_profile_conn_handler_t handler;
    uint8_t profile_id;
    bt_controller_id_t id;
} bt_profile_conn_handler_node_t;

bt_status_t bt_sal_profile_connect_request(bt_address_t* addr,
    uint8_t profile_id, bt_controller_id_t id,
    bt_profile_conn_handler_t handler);
bt_status_t bt_sal_profile_disconnect_register(bt_address_t* addr,
    uint8_t profile_id, bt_controller_id_t id,
    bt_profile_conn_handler_t handler);
bt_status_t bt_sal_profile_disconnect_request(bt_address_t* addr,
    uint8_t profile_id, bt_controller_id_t id,
    bt_profile_conn_handler_t handler);
bt_status_t bt_sal_remove_bond_internal(bt_controller_id_t id,
    bt_address_t* addr);
bt_status_t bt_sal_disconnect_internal(bt_controller_id_t id,
    bt_address_t* addr, uint8_t reason);

cm_data_t* cm_data_new(bt_address_t* addr, uint8_t profile_id);
void bt_sal_cm_profile_connected_callback(cm_data_t* data);
void bt_sal_cm_profile_disconnected_callback(cm_data_t* data);
void bt_sal_cm_acl_connected_callback(cm_data_t* data);
void bt_sal_cm_acl_disconnected_callback(cm_data_t* data);

void bt_sal_cm_conn_init(void);
bt_status_t bt_sal_cm_try_disconnect_profiles(bt_address_t* addr, bool is_unpair);
void bt_sal_cm_conn_cleanup(void);