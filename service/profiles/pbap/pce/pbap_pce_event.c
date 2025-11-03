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

#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"
#include "pbap_pce_event.h"

pbap_pce_msg_t* pbap_pce_msg_new(pbap_pce_event_t event, bt_address_t* addr,
    void* data)
{
    pbap_pce_msg_t* msg;

    msg = (pbap_pce_msg_t*)zalloc(sizeof(pbap_pce_msg_t));
    if (msg == NULL)
        return NULL;

    msg->event = event;
    if (addr != NULL)
        memcpy(&msg->data.addr, addr, sizeof(bt_address_t));

    msg->data.ext_data = data;

    return msg;
}

void pbap_pce_msg_destroy(pbap_pce_msg_t* msg)
{
    if (!msg) {
        return;
    }

    free(msg->data.ext_data);
    free(msg);
}

pce_pull_card_list_req_t* create_pull_cl_req(bt_pbap_search_property_t property,
    const void* pull_data)
{
    pce_pull_card_list_req_t* new_req;

    new_req = (pce_pull_card_list_req_t*)zalloc(sizeof(pce_pull_card_list_req_t)
        + BT_PBAP_PCE_PROPERTY_MAX_LEN);

    if (new_req == NULL)
        return NULL;

    new_req->property = property;

    strlcpy(new_req->value, pull_data, BT_PBAP_PCE_PROPERTY_MAX_LEN);

    return new_req;
}

pce_get_contact_req_t* create_query_contact_req(bt_pbap_search_property_t property,
    const void* value)
{
    pce_get_contact_req_t* new_req;

    new_req = (pce_get_contact_req_t*)zalloc(sizeof(pce_get_contact_req_t)
        + BT_PBAP_PCE_PROPERTY_MAX_LEN);

    if (new_req == NULL)
        return NULL;

    new_req->property = property;

    strlcpy(new_req->value, value, BT_PBAP_PCE_PROPERTY_MAX_LEN);

    return new_req;
}

pce_pull_card_req_t* create_pull_card_req(void* pull_data, uint64_t filter)
{
    pce_pull_card_req_t* new_req;

    new_req = (pce_pull_card_req_t*)zalloc(sizeof(pce_pull_card_req_t));

    if (new_req == NULL)
        return NULL;

    strlcpy(new_req->name, pull_data, sizeof(new_req->name));
    new_req->filter = filter;

    return new_req;
}
