/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#include "lea_vmicps_event.h"

lea_vmicps_msg_t *lea_vmicps_msg_new(lea_vmicps_event_t event)
{
    lea_vmicps_msg_t *msg;

    msg = (lea_vmicps_msg_t *)malloc(sizeof(lea_vmicps_msg_t));
    if (!msg)
        return NULL;

    msg->event = event;
    memset(&msg->data, 0, sizeof(msg->data));
    return msg;
}

void lea_vmicps_msg_destory(lea_vmicps_msg_t *msg)
{
    free(msg);
}
