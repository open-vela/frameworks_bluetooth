/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

typedef struct _state_machine state_machine_t;

typedef struct _state
{
  const char *state_name;
  void (*enter)(state_machine_t *sm);
  void (*exit)(state_machine_t *sm);
  bool (*process_event)(state_machine_t *sm, uint32_t event, void *data);
} state_t;

typedef struct _state_machine
{
  state_t *initial_state;
  state_t *previous_state;
  state_t *current_state;
} state_machine_t;

void hsm_ctor(state_machine_t *sm, state_t *initial_state);
void hsm_dtor(state_machine_t *sm);
void hsm_transition_to(state_machine_t *sm, const state_t *state);
state_t *hsm_get_current_state(state_machine_t *sm);
state_t *hsm_get_previous_state(state_machine_t *sm);
bool hsm_dispatch_event(state_machine_t *sm, uint32_t event, void *p_data);





#endif