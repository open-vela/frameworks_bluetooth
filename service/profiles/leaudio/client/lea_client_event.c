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

lea_client_msg_t *lea_client_msg_new(lea_client_event_t event,
                                     bt_address_t *addr)
{
    return lea_client_msg_new_ext(event, addr, 0);
}

lea_client_msg_t *lea_client_msg_new_ext(lea_client_event_t event,
                                         bt_address_t *addr, uint32_t size)
{
    lea_client_msg_t *msg;

    msg = (lea_client_msg_t *)malloc(sizeof(lea_client_msg_t) + size);
    if (!msg)
        return NULL;

    msg->event = event;
    memset(&msg->data, 0, sizeof(lea_client_data_t));
    if (addr != NULL)
        memcpy(&msg->data.addr, addr, sizeof(bt_address_t));

    return msg;
}

void lea_client_msg_destory(lea_client_msg_t *msg)
{
    // dataptr would free by caller at anytime
    free(msg);
}
