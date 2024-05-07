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
#include "hfp_hf_event.h"

hfp_hf_msg_t* hfp_hf_msg_new(hfp_hf_event_t event, bt_address_t* addr)
{
    return hfp_hf_msg_new_ext(event, addr, NULL, 0);
}

hfp_hf_msg_t* hfp_hf_msg_new_ext(hfp_hf_event_t event, bt_address_t* addr,
    void* data, size_t size)
{
    hfp_hf_msg_t* msg;

    msg = (hfp_hf_msg_t*)zalloc(sizeof(hfp_hf_msg_t));
    if (msg == NULL)
        return NULL;

    msg->event = event;
    if (addr != NULL)
        memcpy(&msg->data.addr, addr, sizeof(bt_address_t));

    if (size > 0) {
        msg->data.size = size;
        msg->data.data = malloc(size);
        memcpy(msg->data.data, data, size);
    }

    return msg;
}

void hfp_hf_msg_destroy(hfp_hf_msg_t* msg)
{
    if (!msg) {
        return;
    }

    free(msg->data.string1);
    free(msg->data.string2);
    free(msg->data.data);
    free(msg);
}
