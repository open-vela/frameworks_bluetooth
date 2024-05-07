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
#ifndef __LEA_SERVER_STATE_MACHINE_H__
#define __LEA_SERVER_STATE_MACHINE_H__

#include "lea_server_event.h"
#include "state_machine.h"

typedef struct _lea_server_state_machine lea_server_state_machine_t;

lea_server_state_machine_t* lea_server_state_machine_new(bt_address_t* addr, void* context);
void lea_server_state_machine_destory(lea_server_state_machine_t* leas_sm);
void lea_server_state_machine_dispatch(lea_server_state_machine_t* leas_sm, lea_server_msg_t* msg);
uint32_t lea_server_state_machine_get_state(lea_server_state_machine_t* leas_sm);
void lea_server_state_machine_set_offloading(lea_server_state_machine_t* leas_sm, bool offloading);

#endif /* __LEA_SERVER_STATE_MACHINE_H__ */
