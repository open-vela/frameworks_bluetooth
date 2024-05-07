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
#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"
#include "lea_ccp_event.h"

lea_ccp_msg_t* lea_ccp_msg_new(lea_ccp_event_t event, bt_address_t* remote_addr,
    uint32_t tbs_id)
{
    return lea_ccp_msg_new_ext(event, remote_addr, tbs_id, 0);
}

lea_ccp_msg_t* lea_ccp_msg_new_ext(lea_ccp_event_t event, bt_address_t* remote_addr,
    uint32_t tbs_id, size_t size)
{
    lea_ccp_msg_t* ccp_msg;

    ccp_msg = (lea_ccp_msg_t*)malloc(sizeof(lea_ccp_msg_t) + size);
    if (ccp_msg == NULL)
        return NULL;

    ccp_msg->event = event;
    memset(&ccp_msg->event_data, 0, sizeof(ccp_msg->event_data) + size);
    if (remote_addr != NULL)
        memcpy(&ccp_msg->remote_addr, remote_addr, sizeof(bt_address_t));

    ccp_msg->event_data.tbs_id = tbs_id;
    return ccp_msg;
}

void lea_ccp_msg_destory(lea_ccp_msg_t* ccp_msg)
{
    free(ccp_msg);
}
