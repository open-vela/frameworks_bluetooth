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
#include "lea_mcp_event.h"

mcp_event_t* mcp_event_new(mcp_event_type_t event, bt_address_t* remote_addr, uint32_t mcs_id)
{
    return mcp_event_new_ext(event, remote_addr, mcs_id, 0);
}

mcp_event_t* mcp_event_new_ext(mcp_event_type_t event, bt_address_t* remote_addr, uint32_t mcs_id, size_t size)
{
    mcp_event_t* mcp_event;

    mcp_event = (mcp_event_t*)malloc(sizeof(mcp_event_t) + size);
    if (mcp_event == NULL)
        return NULL;

    mcp_event->event = event;
    memset(&mcp_event->event_data, 0, sizeof(mcp_event->event_data));
    if (remote_addr != NULL)
        memcpy(&mcp_event->remote_addr, remote_addr, sizeof(bt_address_t));

    mcp_event->event_data.mcs_id = mcs_id;
    return mcp_event;
}

void mcp_event_destory(mcp_event_t* mcp_event)
{
    free(mcp_event);
}
