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
#include <stdint.h>

#include "state_machine.h"

void hsm_ctor(state_machine_t* sm, const state_t* initial_state)
{
    if (sm == NULL)
        return;

    sm->initial_state = initial_state;
    sm->current_state = NULL;
    sm->previous_state = NULL;
    hsm_transition_to(sm, sm->initial_state);
}

void hsm_dtor(state_machine_t* sm)
{
    (void)sm;
}

void hsm_transition_to(state_machine_t* sm, const state_t* state)
{
    if (sm->current_state != NULL) {
        sm->current_state->exit(sm);
        sm->previous_state = sm->current_state;
    }
    sm->current_state = (state_t*)state;
    sm->current_state->enter(sm);
}

const state_t* hsm_get_current_state(state_machine_t* sm)
{
    if (!sm) {
        return NULL;
    }

    return sm->current_state;
}

const state_t* hsm_get_previous_state(state_machine_t* sm)
{
    if (!sm) {
        return NULL;
    }

    return sm->previous_state;
}

const char* hsm_get_current_state_name(state_machine_t* sm)
{
    if (!sm) {
        return NULL;
    }

    return sm->current_state->state_name;
}

const char* hsm_get_state_name(const state_t* state)
{
    if (!state) {
        return NULL;
    }

    return state->state_name;
}

uint16_t hsm_get_state_value(const state_t* state)
{
    return state->state_value;
}

uint16_t hsm_get_current_state_value(state_machine_t* sm)
{
    return sm->current_state->state_value;
}

bool hsm_dispatch_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    if (sm->current_state == NULL) {
        return false;
    }

    return sm->current_state->process_event(sm, event, p_data);
}
