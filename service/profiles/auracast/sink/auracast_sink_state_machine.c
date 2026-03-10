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
#define LOG_TAG "auracast_snk_stm"

#include "auracast_sink_state_machine.h"

#include "auracast_sink_service.h"
#include "bt_utils.h"
#include "pa_sync_service.h"
#include "sal_auracast_sink_interface.h"
#include "service_loop.h"
#include "utils/log.h"

typedef struct auracast_sink_state_machine {
    state_machine_t sm;
} auracast_sink_state_machine_t;

auracast_sink_state_machine_t* auracast_sink_state_machine_new(void* context)
{
    return NULL;
}

void auracast_sink_state_machine_destroy(auracast_sink_state_machine_t* stm)
{
}

void auracast_sink_state_machine_handle_event(auracast_sink_state_machine_t* stm,
    const auracast_sink_msg_t* msg)
{
}