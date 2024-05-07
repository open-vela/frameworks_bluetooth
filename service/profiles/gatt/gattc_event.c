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

#include "gattc_event.h"

gattc_msg_t* gattc_msg_new(gattc_event_t event, bt_address_t* addr, uint16_t playload_length)
{
    gattc_msg_t* msg;

    msg = (gattc_msg_t*)malloc(sizeof(gattc_msg_t) + playload_length);
    if (msg == NULL)
        return NULL;

    msg->event = event;
    memcpy(&msg->addr, addr, sizeof(bt_address_t));

    return msg;
}

void gattc_msg_destory(gattc_msg_t* msg)
{
    free(msg);
}

gattc_op_t* gattc_op_new(gattc_request_t request)
{
    gattc_op_t* operation;

    operation = (gattc_op_t*)malloc(sizeof(gattc_op_t));
    if (operation == NULL)
        return NULL;

    operation->request = request;

    return operation;
}

void gattc_op_destory(gattc_op_t* operation)
{
    free(operation);
}
