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

#include "lea_client_event.h"

lea_client_msg_t* lea_client_msg_new(lea_client_event_t event,
    bt_address_t* addr)
{
    return lea_client_msg_new_ext(event, addr, NULL, 0);
}

lea_client_msg_t* lea_client_msg_new_ext(lea_client_event_t event, bt_address_t* addr,
    void* data, uint32_t size)
{
    lea_client_msg_t* msg;

    msg = (lea_client_msg_t*)zalloc(sizeof(lea_client_msg_t));
    if (!msg)
        return NULL;

    msg->event = event;

    if (addr != NULL) {
        memcpy(&msg->data.addr, addr, sizeof(bt_address_t));
    }

    if (size > 0) {
        msg->data.size = size;
        msg->data.data = malloc(size);
        memcpy(msg->data.data, data, size);
    }

    return msg;
}

void lea_client_msg_destory(lea_client_msg_t* msg)
{
    if (!msg) {
        return;
    }

    free(msg->data.data);
    free(msg);
}

lea_csip_msg_t* lea_csip_msg_new(lea_csip_event_t event, bt_address_t* remote_addr)
{
    return lea_csip_msg_new_ext(event, remote_addr, 0);
}

lea_csip_msg_t* lea_csip_msg_new_ext(lea_csip_event_t event, bt_address_t* remote_addr, size_t size)
{
    lea_csip_msg_t* csip_msg;

    csip_msg = (lea_csip_msg_t*)malloc(sizeof(lea_csip_msg_t) + size);
    if (csip_msg == NULL)
        return NULL;

    csip_msg->event = event;
    memset(&csip_msg->event_data, 0, sizeof(csip_msg->event_data) + size);
    if (remote_addr != NULL)
        memcpy(&csip_msg->addr, remote_addr, sizeof(bt_address_t));

    return csip_msg;
}

void lea_csip_msg_destory(lea_csip_msg_t* csip_msg)
{
    free(csip_msg);
}