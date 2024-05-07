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

#include "lea_tbs_event.h"

lea_tbs_msg_t* lea_tbs_msg_new(lea_tbs_event_t event, uint32_t tbs_id)
{
    return lea_tbs_msg_new_ext(event, tbs_id, 0);
}

lea_tbs_msg_t* lea_tbs_msg_new_ext(lea_tbs_event_t event, uint32_t tbs_id, size_t size)
{
    lea_tbs_msg_t* tbs_event;

    tbs_event = (lea_tbs_msg_t*)malloc(sizeof(lea_tbs_msg_t) + size);
    if (tbs_event == NULL)
        return NULL;

    tbs_event->event = event;
    memset(&tbs_event->event_data, 0, sizeof(tbs_event->event_data) + size);
    tbs_event->event_data.tbs_id = tbs_id;

    return tbs_event;
}

void lea_tbs_msg_destory(lea_tbs_msg_t* tbs_msg)
{
    free(tbs_msg);
}
