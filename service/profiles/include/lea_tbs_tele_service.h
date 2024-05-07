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
#ifndef __LEA_TBS_TELE_SERVICE_H__
#define __LEA_TBS_TELE_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_lea_tbs.h"
#include "stddef.h"

void lea_tbs_tele_service_init(void);
lea_tbs_call_state_t* lea_tbs_find_call_by_index(uint8_t call_index);
lea_tbs_call_state_t* lea_tbs_find_call_by_state(uint8_t call_state);
bt_status_t tele_service_accept_call(char* call_id);
bt_status_t tele_service_terminate_call(char* call_id);
bt_status_t tele_service_hold_call();
bt_status_t tele_service_unhold_call();
bt_status_t tele_service_originate_call(char* uri);

#endif /* __LEA_TBS_TELE_SERVICE_H__ */