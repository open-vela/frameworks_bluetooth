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
#ifndef __HF_CLIENT_STATE_MACHINE_H__
#define __HF_CLIENT_STATE_MACHINE_H__

#include <syslog.h>

#include "bts_service.h"
#include "btm_manager.h"
#include "state_machine.h"

typedef struct
{
  state_machine_t sm;
  bt_address addr;
  uv_timer_t *connect_timer;
} hf_state_machine_t;

typedef enum
{
  CONNECT = 1,
  DISCONNECT = 2,
  CONNECT_AUDIO = 3,
  DISCONNECT_AUDIO = 4,
  VOICE_RECOGNITION_START = 5,
  VOICE_RECOGNITION_STOP = 6,
  SET_MIC_VOLUME = 7,
  SET_SPEAKER_VOLUME = 8,
  DIAL_NUMBER = 10,
  DIAL_MEMORY = 11,
  ACCEPT_CALL = 12,
  REJECT_CALL = 13,
  HOLD_CALL = 14,
  TERMINATE_CALL = 15,
  QUERY_CURRENT_CALLS,
  SEND_AT_COMMAND = 21,
  TIMEOUT,
  STACK_EVENT,
  STACK_EVENT_CONNECTION_STATE_CHANGED,
  STACK_EVENT_AUDIO_STATE_CHANGED,
  STACK_EVENT_VR_STATE_CHANGED,
  STACK_EVENT_CALL,
  STACK_EVENT_CALLSETUP,
  STACK_EVENT_CALLHELD,
  STACK_EVENT_RESP_AND_HOLD,
  STACK_EVENT_CLIP,
  STACK_EVENT_CALL_WAITING,
  STACK_EVENT_CURRENT_CALLS,
  STACK_EVENT_VOLUME_CHANGED,
  STACK_EVENT_CMD_RESULT,
  STACK_EVENT_IN_BAND_RINGTONE,
  STACK_EVENT_RING_INDICATION,
} hf_client_event_t;

typedef struct
{
  uint32_t valueint1;
  uint32_t valueint2;
  uint32_t valueint3;
  uint32_t valueint4;
  char *string1;
  char *string2;
} event_data_t;

typedef struct
{
  hf_state_machine_t *hfsm;
  hf_client_event_t event;
  bt_address bd_addr;
  event_data_t event_data;
} hf_client_sm_msg_t;

hf_state_machine_t *hf_client_state_machine_new(bt_address bd_addr);
void hf_client_state_machine_destory(hf_state_machine_t *hfsm);
void hf_msg_destory(hf_client_sm_msg_t *msg);
hf_client_sm_msg_t *hf_msg_new(hf_state_machine_t *sm, hf_client_event_t event, bt_address bd_addr);
void hf_client_send_message(hf_state_machine_t *sm, hf_client_sm_msg_t *msg);


#endif