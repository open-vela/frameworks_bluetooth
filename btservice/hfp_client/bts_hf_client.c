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
#include <stdio.h>
#include <sys/types.h>

#include "btm_manager.h"

#include "stack_adapter_service_base.h"
#include "stack_adapter_hfp.h"
#include "bts_hf_client_state_machine.h"
#include "bts_hf_client.h"

#include "list.h"

#define LOG_TAG "hfp_service"
#include "log.h"


typedef struct
{
  bt_address bd_addr;
  hf_state_machine_t *hfsm;
  list_t *current_calls;

} hf_client_device_t;

typedef struct
{
  hf_client_callbacks_t *hf_client_callbacks;
  list_t *hf_dev_list;
} hf_profile_t;

static hf_client_callbacks_t *hf_client_callbacks;
static list_t *hf_dev_list;

static hf_state_machine_t *hf_client_get_state_machine(bt_address bd_addr);

static hf_client_device_t *find_hf_device_by_addr(bt_address bd_addr)
{
  list_t *list = hf_dev_list;
  for (const list_node_t *node = list_begin(list); node != list_end(list);
       node = list_next(node))
  {
    hf_client_device_t *device = list_node(node);
    if (memcmp(device->bd_addr, bd_addr, sizeof(bt_address)) == 0)
      return device;
  }

  return NULL;
}

static hf_client_device_t *hf_device_new(hf_state_machine_t *sm, bt_address bd_addr)
{
  list_t *list = hf_dev_list;
  hf_client_device_t *device = (hf_client_device_t *)malloc(sizeof(hf_client_device_t));
  if(!device)
    return NULL;

  memcpy(device->bd_addr, bd_addr, sizeof(bt_address));
  device->hfsm = sm;

  if(!list_append(list, device)) {
    free(device);
    return NULL;
  }
    
  return device;
}

static void hf_device_delete(hf_client_device_t *device)
{
  list_t *list = hf_dev_list;
  if(!device)
    return;

  list_remove(list, device);
  free((void *)device);
}

static hf_state_machine_t *hf_client_get_state_machine(bt_address bd_addr)
{
  hf_client_device_t *device = find_hf_device_by_addr(bd_addr);
  if (device)
    return device->hfsm;

  hf_state_machine_t  *hf_sm = hf_client_state_machine_new(bd_addr);
  if (!hf_sm)
    return hf_sm;

  device = hf_device_new(hf_sm, bd_addr);
  if (!device)
  {
    hf_client_state_machine_destory(hf_sm);
    return NULL;
  }

  return hf_sm;
}

static void hfp_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_CONNECTION_STATE_CHANGED, remote_addr);
  msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_sco_connection_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_SCO_STATE state, uint16_t sco_connection_handle)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_AUDIO_STATE_CHANGED, remote_addr);
  msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_codec_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CONFIG_S *config)
{
  BT_LOGD("%s", __func__);
}

static void hfp_call_setup_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CALL_SETUP_STATE state)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_CALLSETUP, remote_addr);
  msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_call_active_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CALL_ACTIVE_STATE state)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_CALL, remote_addr);
  msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_call_held_state_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_CALL_HELD_STATE state)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_CALLHELD, remote_addr);
  msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_volume_changed_cb(BD_ADDR remote_addr, SERVICE_HFP_VOLUME_TYPE type, uint8_t volume)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_VOLUME_CHANGED, remote_addr);
  msg->event_data.valueint1 = type;
  msg->event_data.valueint2 = volume;

  hf_client_send_message(hfsm, msg);
}

static void hfp_ring_active_state_changed_cb(BD_ADDR remote_addr, bool active, bool inband_ring)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_RING_INDICATION, remote_addr);
  //msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_voice_recognition_enabled_changed_cb(BD_ADDR remote_addr, bool enabled)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_VR_STATE_CHANGED, remote_addr);
  //msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_received_at_cmd_cb(BD_ADDR remote_addr, char *response, uint16_t response_length)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  BT_LOGD("%s", __func__);
  hfsm = hf_client_get_state_machine(remote_addr);
  msg = hf_msg_new(hfsm, STACK_EVENT_CMD_RESULT, remote_addr);
  //msg->event_data.valueint1 = state;

  hf_client_send_message(hfsm, msg);
}

static void hfp_received_sco_connection_req_cb(BD_ADDR remote_addr)
{
  BT_LOGD("%s", __func__);
}

HFP_CALLBACKS_S hfp_callbacks = {
    sizeof(HFP_CALLBACKS_S),
    hfp_connection_state_changed_cb,
    hfp_sco_connection_state_changed_cb,
    hfp_codec_changed_cb,
    hfp_call_setup_state_changed_cb,
    hfp_call_active_state_changed_cb,
    hfp_call_held_state_changed_cb,
    hfp_volume_changed_cb,
    hfp_ring_active_state_changed_cb,
    hfp_voice_recognition_enabled_changed_cb,
    hfp_received_at_cmd_cb,
    hfp_received_sco_connection_req_cb};

bt_result_code hf_client_init(const hf_client_callbacks_t *callbacks)
{
  hf_client_callbacks = callbacks;
  hf_dev_list = list_new(NULL);
  service_adapter_hfp_init(0x1BF, 1, &hfp_callbacks);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_connect(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, CONNECT, bd_addr);

  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_disconnect(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, DISCONNECT, bd_addr);

  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_connect_audio(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, CONNECT_AUDIO, bd_addr);

  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_disconnect_audio(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, DISCONNECT_AUDIO, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_start_voice_recognition(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, VOICE_RECOGNITION_START, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}
bt_result_code hf_client_stop_voice_recognition(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, VOICE_RECOGNITION_STOP, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}
bt_result_code hf_client_volume_control(bt_address bd_addr, hf_client_volume_type_t type, int volume)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, SET_SPEAKER_VOLUME, bd_addr);
  msg->event_data.valueint1 = type;
  msg->event_data.valueint2 = volume;
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}
bt_result_code hf_client_dial(bt_address bd_addr, const char *number)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, DIAL_NUMBER, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_dial_memory(bt_address bd_addr, uint32_t memory)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, DIAL_MEMORY, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_accept_call(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, ACCEPT_CALL, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_reject_call(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, REJECT_CALL, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_hold_call(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, HOLD_CALL, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_terminate_call(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, TERMINATE_CALL, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_query_current_calls(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, QUERY_CURRENT_CALLS, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

bt_result_code hf_client_send_at_cmd(bt_address bd_addr)
{
  hf_state_machine_t *hfsm;
  hf_client_sm_msg_t *msg;

  hfsm = hf_client_get_state_machine(bd_addr);
  if (!hfsm)
    return BT_RESULT_FAILED;

  msg = hf_msg_new(hfsm, SEND_AT_COMMAND, bd_addr);
  hf_client_send_message(hfsm, msg);

  return BT_RESULT_SUCCESS;
}

void hf_client_cleanup(void)
{
  service_adapter_hfp_cleanup();
}

bt_result_code hf_client_service_start(void)
{
  bt_result_code ret;

  ret = hf_client_init(NULL);
  if (ret != BT_RESULT_SUCCESS)
    return ret;

  return ret;
}

void hf_client_service_stop(void)
{
  hf_client_cleanup();
}