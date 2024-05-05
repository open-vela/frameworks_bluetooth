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
#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct _state_machine state_machine_t;

typedef struct _state {
    const char* state_name;
    const uint16_t state_value;
    void (*enter)(state_machine_t* sm);
    void (*exit)(state_machine_t* sm);
    bool (*process_event)(state_machine_t* sm, uint32_t event, void* data);
} state_t;

typedef struct _state_machine {
    const state_t* initial_state;
    const state_t* previous_state;
    const state_t* current_state;
    const state_t* parent_state;
} state_machine_t;

void hsm_ctor(state_machine_t* sm, const state_t* initial_state);
void hsm_dtor(state_machine_t* sm);
void hsm_transition_to(state_machine_t* sm, const state_t* state);
const state_t* hsm_get_current_state(state_machine_t* sm);
const state_t* hsm_get_previous_state(state_machine_t* sm);
const char* hsm_get_state_name(const state_t* state);
uint16_t hsm_get_state_value(const state_t* state);
const char* hsm_get_current_state_name(state_machine_t* sm);
uint16_t hsm_get_current_state_value(state_machine_t* sm);
bool hsm_dispatch_event(state_machine_t* sm, uint32_t event, void* p_data);

#ifdef __cplusplus
}
#endif
#endif
