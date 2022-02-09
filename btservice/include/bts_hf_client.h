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
#ifndef __HF_CLIENT_FROFILE_H__
#define __HF_CLIENT_FROFILE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/list.h>

#include "btm_hfp_hf.h"
#include "btm_manager.h"

typedef struct
{
    bool started;
    int orb_fd;
    struct list_node device_list;
    hf_client_callbacks_t* callbacks;
} hf_client_service_t;

typedef hf_client_callbacks_t hf_client_service_callbacks_t;

extern bt_result_code bts_hf_client_init(const hf_client_service_callbacks_t* callbacks);
extern bt_result_code bts_hf_client_connect(bt_address bd_addr);
extern bt_result_code bts_hf_client_disconnect(bt_address bd_addr);
extern bt_result_code bts_hf_client_connect_audio(bt_address bd_addr);
extern bt_result_code bts_hf_client_disconnect_audio(bt_address bd_addr);
extern bt_result_code bts_hf_client_start_voice_recognition(bt_address bd_addr);
extern bt_result_code bts_hf_client_stop_voice_recognition(bt_address bd_addr);
extern bt_result_code bts_hf_client_volume_control(bt_address bd_addr, hf_client_volume_type_t type, int volume);
extern bt_result_code bts_hf_client_dial(bt_address bd_addr, const char* number);
extern bt_result_code bts_hf_client_dial_memory(bt_address bd_addr, uint32_t memory);
extern bt_result_code bts_hf_client_redial(bt_address bd_addr);
extern bt_result_code bts_hf_client_accept_call(bt_address bd_addr);
extern bt_result_code bts_hf_client_reject_call(bt_address bd_addr);
extern bt_result_code bts_hf_client_hold_call(bt_address bd_addr);
extern bt_result_code bts_hf_client_terminate_call(bt_address bd_addr);
extern bt_result_code bts_hf_client_query_current_calls(bt_address bd_addr);
extern bt_result_code bts_hf_client_send_at_cmd(bt_address bd_addr, const char* cmd);
extern bt_result_code bts_hf_client_update_battery_level(bt_address bd_addr, uint8_t battery);
extern void bts_hf_client_cleanup(void);

extern bt_result_code hf_client_service_start(void);
extern void hf_client_service_stop(void);
extern const hf_client_interface_t* get_hf_client_service_interface(void);

#endif