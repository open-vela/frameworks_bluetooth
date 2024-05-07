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
#ifndef __LEA_TBS_EVENT_H__
#define __LEA_TBS_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_lea_tbs.h"
#include <stdint.h>

typedef enum {
    STACK_EVENT_TBS_STATE_CHANGED,
    STACK_EVENT_BEARER_SET_CHANED,
    STACK_EVENT_CALL_ADDED,
    STACK_EVENT_CALL_REMOVED,
    STACK_EVENT_ACCEPT_CALL,
    STACK_EVENT_TERMINATE_CALL,
    STACK_EVENT_LOCAL_HOLD_CALL,
    STACK_EVENT_LOCAL_RETRIEVE_CALL,
    STACK_EVENT_ORIGINATE_CALL,
    STACK_EVENT_JOIN_CALL,
} lea_tbs_event_t;

typedef struct {
    uint32_t tbs_id;
    uint32_t valueint32;
    uint16_t valueint16;
    uint8_t valueint8;
    bool valuebool;
    uint8_t dataarry[1];
} lea_tbs_event_data_t;

typedef struct {
    lea_tbs_event_t event;
    lea_tbs_event_data_t event_data;
} lea_tbs_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
lea_tbs_msg_t* lea_tbs_msg_new(lea_tbs_event_t event, uint32_t tbs_id);
lea_tbs_msg_t* lea_tbs_msg_new_ext(lea_tbs_event_t event, uint32_t tbs_id, size_t size);
void lea_tbs_msg_destory(lea_tbs_msg_t* tbs_msg);

#endif /* __LEA_TBS_EVENT_H__ */