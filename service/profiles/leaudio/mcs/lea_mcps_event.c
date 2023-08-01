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

#include "lea_mcps_event.h"

mcps_event_t *mcps_event_new(mcps_event_type_t event, uint32_t mcs_id)
{
    return mcps_event_new_ext(event, mcs_id, 0);
}

mcps_event_t *mcps_event_new_ext(mcps_event_type_t event, uint32_t mcs_id, size_t size) {
    mcps_event_t* mcps_event;

    mcps_event = (mcps_event_t*)malloc(sizeof(mcps_event_t) + size);
    if (mcps_event == NULL)
        return NULL;

    mcps_event->event = event;
    memset(&mcps_event->event_data, 0, sizeof(mcps_event->event_data) + size);
    mcps_event->event_data.mcs_id = mcs_id;
    return mcps_event;
}

void mcps_event_destory(mcps_event_t *mcps_event) {
    free(mcps_event);
}
