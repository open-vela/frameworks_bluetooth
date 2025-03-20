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
#ifndef __HFP_HF_STATE_MACHINE_H__
#define __HFP_HF_STATE_MACHINE_H__

#include "hfp_hf_event.h"
#include "service_loop.h"
#include "state_machine.h"

typedef enum pending_state {
    PENDING_NONE = 0x0,
    PENDING_OFFLOAD_START = 0x02,
    PENDING_OFFLOAD_STOP = 0x04,
    // PENDING_DISCONNECT = 0x08,
    PENDING_AUDIO_DISCONNECT = 0x10,
} pending_state_t;

typedef struct _hf_state_machine hf_state_machine_t;

hf_state_machine_t* hf_state_machine_new(bt_address_t* addr, void* context);
void hf_state_machine_destory(hf_state_machine_t* hfsm);
void hf_state_machine_dispatch(hf_state_machine_t* hfsm, hfp_hf_msg_t* msg);
uint32_t hf_state_machine_get_state(hf_state_machine_t* hfsm);
bt_list_t* hf_state_machine_get_calls(hf_state_machine_t* hfsm);
uint16_t hf_state_machine_get_sco_handle(hf_state_machine_t* hfsm);
void hf_state_machine_set_sco_handle(hf_state_machine_t* hfsm, uint16_t sco_hdl);
uint8_t hf_state_machine_get_codec(hf_state_machine_t* hfsm);
void hf_state_machine_set_offloading(hf_state_machine_t* hfsm, bool offloading);
void hf_state_machine_set_policy(hf_state_machine_t* hfsm, connection_policy_t policy);
// hf_client_connection_state_t hf_client_get_conn_state(hf_state_machine_t* sm);

#endif /* __HFP_HF_STATE_MACHINE_H__ */
