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
    void* context; /** auracast_sink_device_t */
} auracast_sink_state_machine_t;

static void idle_enter(state_machine_t* sm);
static void idle_exit(state_machine_t* sm);
static void enabling_enter(state_machine_t* sm);
static void enabling_exit(state_machine_t* sm);
static void streaming_enter(state_machine_t* sm);
static void streaming_exit(state_machine_t* sm);
static void releasing_enter(state_machine_t* sm);
static void releasing_exit(state_machine_t* sm);

static bool idle_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool enabling_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool streaming_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool releasing_process_event(state_machine_t* sm, uint32_t event, void* p_data);

static const state_t idle_state = {
    .state_name = "Idle",
    .state_value = AURACAST_SINK_STATE_IDLE,
    .enter = idle_enter,
    .exit = idle_exit,
    .process_event = idle_process_event,
};

static const state_t enabling_state = {
    .state_name = "Enabling",
    .state_value = AURACAST_SINK_STATE_ENABLING,
    .enter = enabling_enter,
    .exit = enabling_exit,
    .process_event = enabling_process_event,
};

static const state_t streaming_state = {
    .state_name = "Streaming",
    .state_value = AURACAST_SINK_STATE_STREAMING,
    .enter = streaming_enter,
    .exit = streaming_exit,
    .process_event = streaming_process_event,
};

static const state_t releasing_state = {
    .state_name = "Releasing",
    .state_value = AURACAST_SINK_STATE_RELEASING,
    .enter = releasing_enter,
    .exit = releasing_exit,
    .process_event = releasing_process_event,
};

static auracast_sink_state_t auracast_sink_state_machine_get_state(
    const auracast_sink_state_machine_t* stm)
{
    const state_t* cur_state = stm->sm.current_state;

    if (!cur_state)
        return AURACAST_SINK_STATE_IDLE;

    return cur_state->state_value;
}

auracast_sink_state_machine_t* auracast_sink_state_machine_new(void* context)
{
    auracast_sink_state_machine_t* stm;

    stm = (auracast_sink_state_machine_t*)zalloc(sizeof(auracast_sink_state_machine_t));
    if (!stm)
    return NULL;

    stm->context = context;
    hsm_ctor(&stm->sm, (state_t*)&idle_state);

    return stm;
}

void auracast_sink_state_machine_destroy(auracast_sink_state_machine_t* stm)
{
    if (!stm)
        return;

    if (auracast_sink_state_machine_get_state(stm) != AURACAST_SINK_STATE_IDLE) {
        /* Unexpected */
    }

    hsm_dtor(&stm->sm);
    free((void*)stm);
}

void auracast_sink_state_machine_handle_event(auracast_sink_state_machine_t* stm,
    const auracast_sink_msg_t* msg)
{
    if (!msg || !stm)
        return;

    hsm_dispatch_event(&stm->sm, msg->event, (void*)msg);
}