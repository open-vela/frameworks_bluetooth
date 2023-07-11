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
#include "lea_ccpc_event.h"

lea_ccpc_msg_t *lea_ccpc_msg_new(lea_ccpc_event_t event, bt_address_t *remote_addr,
                                 uint32_t tbs_id)
{
    return lea_ccpc_msg_new_ext(event, remote_addr, tbs_id, 0);
}

lea_ccpc_msg_t *lea_ccpc_msg_new_ext(lea_ccpc_event_t event, bt_address_t *remote_addr,
                                     uint32_t tbs_id, size_t size)
{
    lea_ccpc_msg_t *ccpc_msg;

    ccpc_msg = (lea_ccpc_msg_t *)malloc(sizeof(lea_ccpc_msg_t) + size);
    if (ccpc_msg == NULL)
        return NULL;

    ccpc_msg->event = event;
    memset(&ccpc_msg->event_data, 0, sizeof(ccpc_msg->event_data) + size);
    if (remote_addr != NULL)
        memcpy(&ccpc_msg->remote_addr, remote_addr, sizeof(bt_address_t));

    ccpc_msg->event_data.tbs_id = tbs_id;
    return ccpc_msg;
}

void lea_ccpc_msg_destory(lea_ccpc_msg_t *ccpc_msg)
{
    free(ccpc_msg);
}
