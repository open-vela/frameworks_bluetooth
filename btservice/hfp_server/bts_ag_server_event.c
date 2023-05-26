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

#include "btm_manager.h"
#include "bts_ag_server_event.h"

ag_server_msg_t* ag_server_msg_new(ag_server_event_t event, bt_address bd_addr)
{
    ag_server_msg_t* msg;

    msg = (ag_server_msg_t*)malloc(sizeof(ag_server_msg_t));
    if (msg == NULL)
        return NULL;

    msg->event = event;
    memset(&msg->data, 0, sizeof(ag_server_data_t));
    if (bd_addr != NULL)
        memcpy(&msg->data.addr, bd_addr, sizeof(bt_address));

    return msg;
}

void ag_server_msg_destory(ag_server_msg_t* msg)
{
    free(msg->data.string1);
    free(msg->data.string2);
    free(msg);
}
