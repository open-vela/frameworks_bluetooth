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
#ifndef __HFP_AG_STATE_MACHINE_H__
#define __HFP_AG_STATE_MACHINE_H__

#include "hfp_ag_event.h"
#include "service_loop.h"
#include "state_machine.h"

typedef enum pending_state {
    PENDING_NONE = 0x0,
    PENDING_OFFLOAD_START = 0x02,
    PENDING_OFFLOAD_STOP = 0x04,
    PENDING_DISCONNECT = 0x08,
} pending_state_t;

typedef struct _ag_state_machine ag_state_machine_t;

ag_state_machine_t* ag_state_machine_new(bt_address_t* addr, void* context);
void ag_state_machine_destory(ag_state_machine_t* agsm);
void ag_state_machine_dispatch(ag_state_machine_t* agsm, hfp_ag_msg_t* msg);
uint32_t ag_state_machine_get_state(ag_state_machine_t* agsm);
uint16_t ag_state_machine_get_sco_handle(ag_state_machine_t* agsm);
void ag_state_machine_set_sco_handle(ag_state_machine_t* agsm, uint16_t sco_hdl);
uint8_t ag_state_machine_get_codec(ag_state_machine_t* agsm);
void ag_state_machine_set_offloading(ag_state_machine_t* agsm, bool offloading);

#endif /* __HFP_AG_STATE_MACHINE_H__ */
