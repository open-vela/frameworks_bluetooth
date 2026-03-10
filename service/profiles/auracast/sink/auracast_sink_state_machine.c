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

#define AURACAST_SINK_STM_DEBUG 1
#ifndef AURACAST_SINK_STM_DEBUG
#define AURACAST_SINK_DBG_ENTER(__sm)
#define AURACAST_SINK_DBG_EXIT(__sm)
#define AURACAST_SINK_DBG_EVENT(__sm, __event)
#else
#define AURACAST_SINK_TRANS_DBG(_sm, _action)                             \
    do {                                                                  \
        BT_LOGD("%s State=%s", _action, hsm_get_current_state_name(_sm)); \
    } while (0)

#define AURACAST_SINK_DBG_ENTER(__sm) AURACAST_SINK_TRANS_DBG(__sm, "Enter")
#define AURACAST_SINK_DBG_EXIT(__sm) AURACAST_SINK_TRANS_DBG(__sm, "Exit ")
#define AURACAST_SINK_DBG_EVENT(__sm, __event)                                       \
    do {                                                                             \
        BT_LOGD("ProcessEvent, State=%s, Event=%s", hsm_get_current_state_name(_sm), \
            auracast_sink_event_to_string(__event));                                   \
    } while (0)

static const char* auracast_sink_event_to_string(auracast_sink_event_t event)
{
    switch (event) {
        CASE_RETURN_STR(AURACAST_SINK_CREATE_SYNC)
        CASE_RETURN_STR(AURACAST_SINK_TERMINATE_SYNC)
        CASE_RETURN_STR(AURACAST_SINK_SYNC_ESTABLISHED)
        CASE_RETURN_STR(AURACAST_SINK_SYNC_TERMINATED)
        CASE_RETURN_STR(AURACAST_SINK_CONFIG_DONE)
        CASE_RETURN_STR(AURACAST_SINK_DATA_IN)
        CASE_RETURN_STR(AURACAST_SINK_DUMP)
        DEFAULT_BREAK()
    }

    return "Unknown";
}
#endif

static void dump(const auracast_sink_state_machine_t* stm)
{
    BT_LOGD("%s", __func__);
}

static void idle_enter(state_machine_t* sm)
{
    AURACAST_SINK_DBG_ENTER(sm);
}

static void idle_exit(state_machine_t* sm)
{
    AURACAST_SINK_DBG_EXIT(sm);
}

static bool idle_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    auracast_sink_state_machine_t* stm = (auracast_sink_state_machine_t*)sm;

    AURACAST_SINK_DBG_EVENT(sm, event);

    switch (event) {
    case AURACAST_SINK_CREATE_SYNC:
        hsm_transition_to(sm, &enabling_state);
        break;
    case AURACAST_SINK_DUMP:
        dump(stm);
        break;
    default:
        /** Unexpected */
        break;
    }

    return true;
}

static void enabling_enter(state_machine_t* sm)
{
    AURACAST_SINK_DBG_ENTER(sm);
}

static void enabling_exit(state_machine_t* sm)
{
    AURACAST_SINK_DBG_EXIT(sm);
}

static bool enabling_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    auracast_sink_state_machine_t* stm = (auracast_sink_state_machine_t*)sm;

    AURACAST_SINK_DBG_EVENT(sm, event);

    switch (event) {
    case AURACAST_SINK_SYNC_ESTABLISHED:
        hsm_transition_to(sm, &streaming_state);
        break;
    case AURACAST_SINK_SYNC_TERMINATED:
        hsm_transition_to(sm, &idle_state);
        break;
    case AURACAST_SINK_DUMP:
        dump(stm);
        break;
    default:
        /** Unexpected */
        break;
    }

    return true;
}

static void streaming_enter(state_machine_t* sm)
{

    AURACAST_SINK_DBG_ENTER(sm);

}

static void streaming_exit(state_machine_t* sm)
{
    AURACAST_SINK_DBG_EXIT(sm);
}

static bool streaming_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    auracast_sink_state_machine_t* stm = (auracast_sink_state_machine_t*)sm;

    AURACAST_SINK_DBG_EVENT(sm, event);

    switch (event) {
    case AURACAST_SINK_TERMINATE_SYNC:
        hsm_transition_to(sm, &releasing_state);
        break;
    case AURACAST_SINK_SYNC_TERMINATED:
        hsm_transition_to(sm, &idle_state);
        break;
    case AURACAST_SINK_DUMP:
        dump(stm);
        break;
    default:
        /** Unexpected */
        break;
    }

    return true;
}

static void releasing_enter(state_machine_t* sm)
{
    AURACAST_SINK_DBG_ENTER(sm);
}

static void releasing_exit(state_machine_t* sm)
{
    AURACAST_SINK_DBG_EXIT(sm);
}

static bool releasing_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    auracast_sink_state_machine_t* stm = (auracast_sink_state_machine_t*)sm;

    AURACAST_SINK_DBG_EVENT(sm, event);

    switch (event) {
    case AURACAST_SINK_SYNC_TERMINATED:
        hsm_transition_to(sm, &idle_state);
        break;
    case AURACAST_SINK_DUMP:
        dump(stm);
        break;
    default:
        /** Unexpected */
        break;
    }

    return true;
}

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