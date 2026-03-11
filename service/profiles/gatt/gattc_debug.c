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
#define LOG_TAG "gattc_debug"

#include "gattc_debug.h"

#include "bt_addr.h"
#include "bt_list.h"
#include "bt_uuid.h"
#include "utils/log.h"

void gattc_log(const bt_address_t* addr, const char* msg)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGI("%s addr:%s", msg, addr_str);
}

void gattc_log_state(const bt_address_t* addr, const char* msg, profile_connection_state_t state)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGI("%s addr:%s, state:%d", msg, addr_str, state);
}

void gattc_log_status(const bt_address_t* addr, const char* msg, gatt_status_t status)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(addr, addr_str);
    BT_LOGI("%s addr:%s, status:%d", msg, addr_str, status);
}

void gattc_dump_services(const gattc_connection_t* connection)
{
    bt_list_node_t* snode;
    bt_list_t* slist;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    char uuid_str[BT_UUID_STR_LENGTH] = { 0 };
    int s_id = 0;

    if (!connection) {
        BT_LOGW("Invalid connection pointer");
        return;
    }

    slist = connection->services;
    if (!slist) {
        BT_LOGW("Invalid services list");
        return;
    }

    bt_addr_ba2str(&connection->remote_addr, addr_str);
    BT_LOGI("GATT Client[%d]: State:%d, Peer:%s", connection->conn_id, connection->state, addr_str);
    for (snode = bt_list_head(slist); snode != NULL; snode = bt_list_next(slist, snode)) {
        gattc_service_t* service = (gattc_service_t*)bt_list_node(snode);
        gatt_element_t* element;

        if (!service || !service->elements)
            continue;

        element = service->elements;

        BT_LOGI("\tAttribute Table[%d]: Handle:0x%04x~0x%04x, Num:%d", s_id++,
            service->start_handle, service->end_handle, service->element_size);
        for (int i = 0; i < service->element_size; i++, element++) {
            bt_uuid_to_string(&element->uuid, uuid_str, BT_UUID_STR_LENGTH);
            BT_LOGI("\t\t>[0x%04x][Type:%d][Prop:%04x][UUID:%s]", element->handle,
                element->type, element->properties, uuid_str);
        }
    }

    if (bt_list_is_empty(slist))
        BT_LOGI("\tNo service found");
}
