/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef _PBAP_PCE_STATE_MACHINE_H_
#define _PBAP_PCE_STATE_MACHINE_H_

#include "bt_device.h"
#include "pbap_pce_event.h"

typedef enum {
    PBAP_PCE_STATE_DISCONNECTED,
    PBAP_PCE_STATE_CONNECTING,
    PBAP_PCE_STATE_CONNECTED,
    PBAP_PCE_STATE_DISCONNECTING,
} pbap_pce_state_t;

typedef enum {
    PBAP_PCE_VCARD_VERSION_2_1,
    PBAP_PCE_VCARD_VERSION_3_0,
} pbap_pce_vcard_version_t;

typedef struct __pbap_pce_state_machine pce_state_machine_t;

pce_state_machine_t* pce_state_machine_new(void* context, bt_address_t* bd_addr);
void pce_state_machine_destory(pce_state_machine_t* pce_sm);
void pce_state_machine_handle_event(pce_state_machine_t* sm, pbap_pce_msg_t* pce_event);
pbap_pce_state_t pce_state_machine_get_state(pce_state_machine_t* sm);
const char* pce_state_machine_current_state(pce_state_machine_t* sm);
profile_connection_state_t pce_state_machine_get_connection_state(pce_state_machine_t* sm);
bt_status_t do_in_pbap_pce_service(bt_address_t* addr, pbap_pce_event_t event, void* ext_data);

#endif