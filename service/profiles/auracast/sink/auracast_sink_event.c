/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#include "auracast_sink_event.h"

#include "bt_utils.h"

auracast_sink_msg_t* auracast_sink_msg_new(auracast_sink_event_t event, bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid)
{
    return auracast_sink_msg_new_ext(event, id, addr, sid, 0);
}

auracast_sink_msg_t* auracast_sink_msg_new_ext(auracast_sink_event_t event, bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid, size_t size)
{
    auracast_sink_msg_t* msg;

    msg = (auracast_sink_msg_t*)zalloc(sizeof(auracast_sink_msg_t) + size);
    if (msg == NULL)
        return NULL;

    if (addr) {
        memcpy(&msg->addr.addr, &addr->addr, BT_ADDR_LENGTH);
        msg->addr.addr_type = addr->addr_type;
    }
    msg->event = event;
    msg->id = id;
    msg->sid = sid;
    msg->data.size = size;

    return msg;
}

void auracast_sink_msg_destory(auracast_sink_msg_t* msg)
{
    free(msg->context);
    free(msg);
}
