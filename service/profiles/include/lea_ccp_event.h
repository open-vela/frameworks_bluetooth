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
#ifndef __LEA_CCP_EVENT_H__
#define __LEA_CCP_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"
#include "bt_lea_ccp.h"
#include <stdint.h>

typedef enum {
    STACK_EVENT_READ_PROVIDER_NAME,
    STACK_EVENT_READ_UCI,
    STACK_EVENT_READ_TECHNOLOGY,
    STACK_EVENT_READ_URI_SCHEMES_SUPPORT_LIST,
    STACK_EVENT_READ_SIGNAL_STRENGTH,
    STACK_EVENT_READ_SIGNAL_STRENGTH_REPORT_INTERVAL,
    STACK_EVENT_READ_CONTENT_CONTROL_ID,
    STACK_EVENT_READ_STATUS_FLAGS,
    STACK_EVENT_READ_CALL_CONTROL_OPTIONAL_OPCODES,
    STACK_EVENT_READ_INCOMING_CALL,
    STACK_EVENT_READ_INCOMING_CALL_TARGET_BEARER_URI,
    STACK_EVENT_READ_CALL_STATE,
    STACK_EVENT_READ_BEARER_LIST_CURRENT_CALL,
    STACK_EVENT_READ_CALL_FRIENDLY_NAME,
    STACK_EVENT_TERMINATION_REASON,
    STACK_EVENT_CALL_CONTROL_RESULT,
} lea_ccp_event_t;

typedef struct {
    uint32_t tbs_id;
    uint32_t valueint32;
    uint16_t valueint16;
    uint8_t valueint8_0;
    uint8_t valueint8_1;
    uint8_t valueint8_2;
    uint8_t dataarry[1];
} lea_ccp_event_data_t;

typedef struct {
    bt_address_t remote_addr;
    lea_ccp_event_t event;
    lea_ccp_event_data_t event_data;
} lea_ccp_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
lea_ccp_msg_t* lea_ccp_msg_new(lea_ccp_event_t event, bt_address_t* remote_addr, uint32_t tbs_id);
lea_ccp_msg_t* lea_ccp_msg_new_ext(lea_ccp_event_t event, bt_address_t* remote_addr, uint32_t tbs_id, size_t size);
void lea_ccp_msg_destory(lea_ccp_msg_t* ccp_msg);

#endif /* __LEA_CCP_EVENT_H__ */