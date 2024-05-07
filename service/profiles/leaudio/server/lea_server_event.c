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

#include "lea_server_event.h"

lea_server_msg_t* lea_server_msg_new(lea_server_event_t event,
    bt_address_t* addr)
{
    return lea_server_msg_new_ext(event, addr, NULL, 0);
}

lea_server_msg_t* lea_server_msg_new_ext(lea_server_event_t event,
    bt_address_t* addr, void* data, size_t size)
{
    lea_server_msg_t* msg;

    msg = (lea_server_msg_t*)zalloc(sizeof(lea_server_msg_t));
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

void lea_server_msg_destory(lea_server_msg_t* msg)
{
    if (!msg) {
        return;
    }

    free(msg->data.data);
    free(msg);
}
