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

#include "gatts_event.h"

gatts_msg_t* gatts_msg_new(gatts_event_t event, uint16_t playload_length)
{
    gatts_msg_t* msg;

    msg = (gatts_msg_t*)malloc(sizeof(gatts_msg_t) + playload_length);
    if (msg == NULL)
        return NULL;

    msg->event = event;

    return msg;
}

void gatts_msg_destory(gatts_msg_t* msg)
{
    free(msg);
}

gatts_op_t* gatts_op_new(gatts_request_t request)
{
    gatts_op_t* operation;

    operation = (gatts_op_t*)malloc(sizeof(gatts_op_t));
    if (operation == NULL)
        return NULL;

    operation->request = request;

    return operation;
}

void gatts_op_destory(gatts_op_t* operation)
{
    free(operation);
}
