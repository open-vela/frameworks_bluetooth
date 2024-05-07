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

#include "lea_mcs_event.h"

mcs_event_t* mcs_event_new(mcs_event_type_t event, uint32_t mcs_id)
{
    return mcs_event_new_ext(event, mcs_id, 0);
}

mcs_event_t* mcs_event_new_ext(mcs_event_type_t event, uint32_t mcs_id, size_t size)
{
    mcs_event_t* mcs_event;

    mcs_event = (mcs_event_t*)malloc(sizeof(mcs_event_t) + size);
    if (mcs_event == NULL)
        return NULL;

    mcs_event->event = event;
    memset(&mcs_event->event_data, 0, sizeof(mcs_event->event_data) + size);
    mcs_event->event_data.mcs_id = mcs_id;
    return mcs_event;
}

void mcs_event_destory(mcs_event_t* mcs_event)
{
    free(mcs_event);
}
