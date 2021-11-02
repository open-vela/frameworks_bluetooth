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
#include "bts_service.h"
#include "btm_hfp_hf.h"

#define LOG_TAG "hfp_hf_service"
#include "log.h"

static hf_client_interface_t* get_service(void)
{
  return (hf_client_interface_t *)get_bluetooth_service_interface()->get_profile_interface(BT_PROFILE_HANDSFREE_HF);
}

static bt_result_code hf_connect(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->connect(handle, addr);
}

static bt_result_code hf_disconnect(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->disconnect(handle, addr);
}

static bt_result_code hf_connect_audio(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->connect_audio(handle, addr);
}

static bt_result_code hf_disconnect_audio(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->disconnect_audio(handle, addr);
}

static bt_result_code hf_start_voice_recognition(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->stop_voice_recognition(handle, addr);
}

static bt_result_code hf_stop_voice_recognition(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->start_voice_recognition(handle, addr);
}

static bt_result_code hf_volume_control(void* handle, bt_address addr, hf_client_volume_type_t type, int volume)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->volume_control(handle, addr, type, volume);
}

static bt_result_code hf_dial(void* handle, bt_address addr, const char *number)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->dial(handle, addr, number);
}

static bt_result_code hf_dial_memory(void* handle, bt_address addr, uint32_t memory)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->dial_memory(handle, addr, memory);
}

static bt_result_code hf_redial(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->redial(handle, addr);
}

static bt_result_code hf_accept_call(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->accept_call(handle, addr);
}

static bt_result_code hf_reject_call(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->reject_call(handle, addr);
}

static bt_result_code hf_hold_call(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->hold_call(handle, addr);
}

static bt_result_code hf_terminate_call(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->terminate_call(handle, addr);
}

static bt_result_code hf_query_current_calls(void* handle, bt_address addr)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->query_current_calls(handle, addr);
}

static bt_result_code hf_send_at_cmd(void* handle, bt_address addr, const char *cmd)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return BT_RESULT_FAILED;

  return service->send_at_cmd(handle, addr, cmd);
}

static void set_callbacks(void* handle, hf_client_callbacks_t *callbacks)
{
  hf_client_interface_t *service = get_service();
  if (!service)
    return;

  return service->set_callbacks(handle, callbacks);
}


static const hf_client_interface_t hfInterface = {
  sizeof(hf_client_interface_t),
  hf_connect,
  hf_disconnect,
  hf_connect_audio,
  hf_disconnect_audio,
  hf_start_voice_recognition,
  hf_stop_voice_recognition,
  hf_volume_control,
  hf_dial,
  hf_dial_memory,
  hf_redial,
  hf_accept_call,
  hf_reject_call,
  hf_hold_call,
  hf_terminate_call,
  hf_query_current_calls,
  hf_send_at_cmd,
  set_callbacks,
};


const hf_client_interface_t *get_hf_client_interface(void)
{
  return &hfInterface;
}