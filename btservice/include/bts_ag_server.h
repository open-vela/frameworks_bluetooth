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
#ifndef __AG_SERVER_FROFILE_H__
#define __AG_SERVER_FROFILE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/list.h>

#include "btm_hfp_ag.h"
#include "btm_manager.h"

typedef struct
{
    bool started;
    int orb_fd;
    struct list_node device_list;
    ag_server_callbacks_t* callbacks;
} ag_server_service_t;

typedef ag_server_callbacks_t ag_server_service_callbacks_t;

extern bt_result_code bts_ag_server_init(
    const ag_server_service_callbacks_t* callbacks);
extern void bts_ag_server_cleanup(void);

extern bool bts_ag_server_is_connected(bt_address bd_addr);
extern bool bts_ag_server_is_audio_connected(bt_address bd_addr);
extern ag_server_state_t bts_ag_server_get_connection_state(bt_address bd_addr);
extern bt_result_code bts_ag_server_connect(bt_address bd_addr);
extern bt_result_code bts_ag_server_disconnect(bt_address bd_addr);
extern bt_result_code bts_ag_server_connect_audio(bt_address bd_addr);
extern bt_result_code bts_ag_server_disconnect_audio(bt_address bd_addr);
extern bt_result_code bts_ag_server_start_voice_recognition(bt_address bd_addr);
extern bt_result_code bts_ag_server_stop_voice_recognition(bt_address bd_addr);
extern bt_result_code bts_ag_server_phone_state_change(bt_address bd_addr,
    uint8_t num_active,
    uint8_t num_held,
    ag_server_call_state_t call_state,
    ag_server_call_addrtype_t type,
    const char* number,
    const char* name);
extern bt_result_code bts_ag_server_device_status_changed(
    bt_address bd_addr,
    hfp_network_state_t network,
    hfp_roaming_state_t roam,
    uint8_t signal,
    uint8_t battery);
extern bt_result_code bts_ag_server_set_inband_ring_enable(bt_address bd_addr);
extern bt_result_code bts_ag_server_send_at_command(
    bt_address bd_addr,
    char* at_command);
extern bt_result_code bts_ag_server_dial_result(bt_address bd_addr, uint8_t result);
extern bt_result_code bts_ag_server_cind_response(bt_address bd_addr,
    hfp_network_state_t service,
    uint8_t signal,
    hfp_roaming_state_t roam,
    uint8_t battery,
    hfp_call_t call,
    hfp_callsetup_t call_setup,
    hfp_callheld_t call_held);
extern bt_result_code bts_ag_server_clcc_response(bt_address bd_addr,
    uint32_t index,
    uint8_t dir,
    ag_server_call_state_t status,
    uint8_t mode,
    uint8_t mpty,
    const char* number);
extern bt_result_code bts_ag_server_cops_response(bt_address bd_addr,
    char* operator_name, uint16_t length);

extern void ag_server_cleanup(void);
extern bt_result_code ag_service_start(void);
extern bt_result_code ag_service_stop(void);
extern const ag_server_interface_t* get_ag_server_service_interface(void);
#endif
