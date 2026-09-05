/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#ifndef __BT_TELE_SERVICE_H__
#define __BT_TELE_SERVICE_H__

#include "bt_status.h"

void tele_service_init(void);
void tele_service_cleanup(void);
bt_status_t tele_service_dial_number(char* number);
bt_status_t tele_service_answer_call(void);
bt_status_t tele_service_reject_call(void);
bt_status_t tele_service_hangup_call(void);
bt_status_t tele_service_call_control(uint8_t chld);
void tele_service_get_phone_state(uint8_t* num_active, uint8_t* num_held,
    uint8_t* call_state);
void tele_service_query_current_call(bt_address_t* addr);
char* tele_service_get_operator(void);
bt_status_t tele_service_get_network_info(hfp_network_state_t* network,
    hfp_roaming_state_t* roam,
    uint8_t* signal);
#endif /* __BT_TELE_SERVICE_H__ */