/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
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
#ifndef __A2DP_STATE_MACHINE_H__
#define __A2DP_STATE_MACHINE_H__

#include "a2dp_event.h"
#include "bt_device.h"

typedef enum {
    A2DP_STATE_IDLE,
    A2DP_STATE_OPENING,
    A2DP_STATE_OPENED,
    A2DP_STATE_STARTED,
    A2DP_STATE_CLOSING
} a2dp_state_t;

typedef struct _a2dp_state_machine a2dp_state_machine_t;

a2dp_state_machine_t* a2dp_state_machine_new(void* context, uint8_t peer_sep, bt_address_t* bd_addr);
void a2dp_state_machine_destory(a2dp_state_machine_t* a2dp_sm);
void a2dp_state_machine_handle_event(a2dp_state_machine_t* sm, a2dp_event_t* a2dp_event);
a2dp_state_t a2dp_state_machine_get_state(a2dp_state_machine_t* sm);
const char* a2dp_state_machine_current_state(a2dp_state_machine_t* sm);
profile_connection_state_t a2dp_state_machine_get_connection_state(a2dp_state_machine_t* sm);
bool a2dp_state_machine_is_pending_stop(a2dp_state_machine_t* sm);
#endif
