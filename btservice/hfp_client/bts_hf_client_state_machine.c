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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include "stack_adapter_service_base.h"
#include "stack_adapter_common.h"


#include "btm_hfp_hf.h"
#include "bts_hf_client_state_machine.h"

#define LOG_TAG "HF_STM"
#include "log.h"

#define HF_CONNECT_TIMEOUT 2 * 1000

static void disconnected_enter(state_machine_t *sm);
static void disconnected_exit(state_machine_t *sm);
static void connecting_enter(state_machine_t *sm);
static void connecting_exit(state_machine_t *sm);
static void connected_enter(state_machine_t *sm);
static void connected_exit(state_machine_t *sm);
static void audio_on_enter(state_machine_t *sm);
static void audio_on_exit(state_machine_t *sm);

static bool disconnected_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool connected_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool audio_on_process_event(state_machine_t *sm, uint32_t event, void *p_data);

static const state_t disconnected_state = {
    .state_name = "Disconnected",
    .enter = disconnected_enter,
    .exit = disconnected_exit,
    .process_event = disconnected_process_event,
};

static const state_t connecting_state = {
    .state_name = "Connecting",
    .enter = connecting_enter,
    .exit = connecting_exit,
    .process_event = connecting_process_event,
};

static const state_t connected_state = {
    .state_name = "Connected",
    .enter = connected_enter,
    .exit = connected_exit,
    .process_event = connected_process_event,
};

static const state_t audio_on_state = {
    .state_name = "AduioOn",
    .enter = audio_on_enter,
    .exit = audio_on_exit,
    .process_event = audio_on_process_event,
};

static void disconnected_enter(state_machine_t *sm)
{
  BT_LOGD("%s", __func__);
}
static void disconnected_exit(state_machine_t *sm)
{
  BT_LOGD("%s", __func__);
}

static bool disconnected_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
  hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
  event_data_t *data = (event_data_t *)p_data;
  SERVICE_BT_STATUS status;
  BT_LOGD("%s", __func__);
  switch (event)
  {
  case CONNECT:
    //check bonded state
    status = service_adapter_hfp_connect(hfsm->addr);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
      //callback disconnected
      break;
    }

    hsm_transition_to(sm, &connecting_state);
    break;
  case STACK_EVENT_CONNECTION_STATE_CHANGED:
  {
    //check bonded state;
    //slc connection callback
    uint32_t state = data->valueint1;
    switch (state)
    {
    case SERVICE_PROFILE_CONNECTING:
      hsm_transition_to(sm, &connecting_state);
      break;
    case SERVICE_PROFILE_CONNECTED:
      hsm_transition_to(sm, &connected_state);
      break;
    case SERVICE_PROFILE_DISCONNECTED:
    case SERVICE_PROFILE_DISCONNECTING:
    default:
      break;
    }
    break;
  }

  default:
    break;
  }

  return true;
}

static void hf_connect_timeout_callback(char *data)
{
  hf_state_machine_t *hfsm = (hf_state_machine_t *)data;

  hf_client_sm_msg_t *msg = hf_msg_new(hfsm, TIMEOUT, hfsm->addr);
  hf_client_send_message(hfsm, msg);
}

static void connecting_enter(state_machine_t *sm)
{
  hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
  BT_LOGD("%s", __func__);
  //start connecting timeout timer
  hfsm->connect_timer = start_timer(HF_CONNECT_TIMEOUT, 1, hf_connect_timeout_callback, hfsm);
}

static void connecting_exit(state_machine_t *sm)
{
  hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
  BT_LOGD("%s", __func__);
  //stop timer
  stop_timer(hfsm->connect_timer);
}

static bool connecting_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
  hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
  event_data_t *data = (event_data_t *)p_data;

  BT_LOGD("%s", __func__);
  switch (event)
  {
  case STACK_EVENT_CONNECTION_STATE_CHANGED:
  {
    uint32_t state = data->valueint1;
    switch (state)
    {
    case SERVICE_PROFILE_DISCONNECTED:
      //slc connection callback
      hsm_transition_to(sm, &disconnected_state);
      break;
    case SERVICE_PROFILE_CONNECTED:
      //slc connection callback
      hsm_transition_to(sm, &connected_state);
      break;
    case SERVICE_PROFILE_CONNECTING:
    case SERVICE_PROFILE_DISCONNECTING:
    default:
      break;
    }
    break;
  }
  case TIMEOUT:
    BT_LOGD("%s: TIMEOUT", __func__);
    //connect timeout 
    //slc connection callback
    hsm_transition_to(sm, &disconnected_state);
    break;

  default:
    break;
  }
  return true;
}

static void connected_enter(state_machine_t *sm)
{
  BT_LOGD("%s", __func__);
}
static void connected_exit(state_machine_t *sm)
{
  BT_LOGD("%s", __func__);
}
static bool connected_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
  hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
  event_data_t *data = (event_data_t *)p_data;
  SERVICE_BT_STATUS status;
  BT_LOGD("%s", __func__);

  switch (event)
  {
  case CONNECT:
    //no handle
    break;
  case DISCONNECT:
    //do disconnect
    status = service_adapter_hfp_disconnect(hfsm->addr);
    break;
  case CONNECT_AUDIO:
    status = service_adapter_hfp_create_sco(hfsm->addr);
    if (status != SERVICE_BT_STATUS_SUCCESS) {
      break;
    }
    break;
  case DISCONNECT_AUDIO:
    service_adapter_hfp_disconnect_sco(hfsm->addr);
    break;
  case VOICE_RECOGNITION_START:
    status = service_adapter_hfp_enable_voice_recognition(hfsm->addr);
    break;
  case VOICE_RECOGNITION_STOP:
    service_adapter_hfp_disable_voice_recognition(hfsm->addr);
    break;
  case SET_MIC_VOLUME:
    service_adapter_hfp_set_volume(hfsm->addr, VOLUME_MIC, 5);
    break;
  case SET_SPEAKER_VOLUME:
    service_adapter_hfp_set_volume(hfsm->addr, VOLUME_SPEAKER, 5);
    break;
  case DIAL_NUMBER:
    break;
  case DIAL_MEMORY:
    break;
  case ACCEPT_CALL:
    break;
  case REJECT_CALL:
    break;
  case HOLD_CALL:
    break;
  case TERMINATE_CALL:
    break;
  case QUERY_CURRENT_CALLS:
    break;
  case SEND_AT_COMMAND:
    break;
  case STACK_EVENT_CONNECTION_STATE_CHANGED:
  {
    uint32_t state = data->valueint1;
    switch (state)
    {
    case SERVICE_PROFILE_DISCONNECTED:
      //slc connection callback
      hsm_transition_to(sm, &disconnected_state);
      break;
    case SERVICE_PROFILE_CONNECTED:
      //service_adapter_hfp_set_volume(remote_addr, VOLUME_SPEAKER, vol);
      //slc connection callback
      hsm_transition_to(sm, &connected_state);
      break;
    case SERVICE_PROFILE_CONNECTING:
    case SERVICE_PROFILE_DISCONNECTING:
      break;
    }
    break;
  }
  case STACK_EVENT_AUDIO_STATE_CHANGED:
  {
    SERVICE_HFP_SCO_STATE state = data->valueint1;
    switch (state)
    {
    case SERVICE_HFP_SCO_CONNECTED:
      //set audio focus, route audio channel
      //audio connected callback
      hsm_transition_to(sm, &audio_on_state);
      break;
    case SERVICE_HFP_SCO_DISCONNECTED:
    case SERVICE_HFP_SCO_UNKNOWN:
    default:
      break;
    }
    break;
  }
  default:
    break;
  }
  return true;
}

static void audio_on_enter(state_machine_t *sm)
{
  BT_LOGD("%s", __func__);
}
static void audio_on_exit(state_machine_t *sm)
{
  BT_LOGD("%s", __func__);
}
static bool audio_on_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
  hf_state_machine_t *hfsm = (hf_state_machine_t *)sm;
  event_data_t *data = (event_data_t *)p_data;

  BT_LOGD("%s", __func__);
  switch (event)
  {
  case DISCONNECT:
  {
    //no handle? defer?
    break;
  }
  case DISCONNECT_AUDIO:
    //do disconnect audio
    break;
  case HOLD_CALL:
    //do hold call
    break;
  case STACK_EVENT_CONNECTION_STATE_CHANGED:
  {
    uint32_t state = data->valueint1;
    switch (state)
    {
    case SERVICE_PROFILE_DISCONNECTED:
      //audio disconnect callback
      //set audio focus, route audio
      //slc disconnect callback
      hsm_transition_to(sm, &disconnected_state);
      break;
    case SERVICE_PROFILE_CONNECTED:
    case SERVICE_PROFILE_CONNECTING:
    case SERVICE_PROFILE_DISCONNECTING:
      break;
    }
    break;
  }
  case STACK_EVENT_AUDIO_STATE_CHANGED:
  {
    SERVICE_HFP_SCO_STATE state = data->valueint1;
    switch (state)
    {
    case SERVICE_HFP_SCO_DISCONNECTED:
      //set audio focus, route audio
      //audio disconnect callback
      hsm_transition_to(sm, &connected_state);
      break;
    case SERVICE_HFP_SCO_CONNECTED:
    case SERVICE_HFP_SCO_UNKNOWN:
    default:
      break;
    }
    break;
  }
  default:
    break;
  }
}

static void hf_client_event_dispatch(char *data, size_t size)
{
  if (!data)
    return;
  hf_client_sm_msg_t *msg = (hf_client_sm_msg_t *)data;

  hsm_dispatch_event(&msg->hfsm->sm, msg->event, &msg->event_data);
  hf_msg_destory(msg);
}

hf_state_machine_t *hf_client_state_machine_new(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;

  hfsm = (hf_state_machine_t *)malloc(sizeof(hf_state_machine_t));
  if (!hfsm)
    return NULL;

  hfsm->connect_timer = NULL;
  hsm_ctor(&hfsm->sm, &disconnected_state);
  memcpy(hfsm->addr, bd_addr, sizeof(bt_address));

  return hfsm;
}

void hf_client_state_machine_destory(hf_state_machine_t *hfsm)
{
  if (!hfsm)
    return;

  hsm_dtor(&hfsm->sm);
  free((void *)hfsm);
}

hf_client_sm_msg_t *hf_msg_new(hf_state_machine_t *sm,
                               hf_client_event_t event,
                               bt_address bd_addr)
{
  hf_client_sm_msg_t *msg;
  msg = (hf_client_sm_msg_t *)malloc(sizeof(hf_client_sm_msg_t));
  if (msg == NULL)
    return NULL;

  msg->hfsm = sm;
  msg->event = event;
  memcpy(msg->bd_addr, bd_addr, sizeof(bt_address));
  memset(&msg->event_data, 0, sizeof(msg->event_data));
  return msg;
}

void hf_msg_destory(hf_client_sm_msg_t *msg)
{
  if (!msg)
    return;

  if (msg->event_data.string1)
    free(msg->event_data.string1);

  if (msg->event_data.string2)
    free(msg->event_data.string2);
  free(msg);
}

void hf_client_send_message(hf_state_machine_t *sm, hf_client_sm_msg_t *msg)
{
  excute_service_context_t *context = (excute_service_context_t *)malloc(sizeof(excute_service_context_t));
  context->loop_func = hf_client_event_dispatch;
  context->data = (void *)msg;
  context->data_size = sizeof(hf_client_sm_msg_t);
  process_in_loop(context);
}