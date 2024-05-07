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
#ifndef __LEA_CLIENT_EVENT_H__
#define __LEA_CLIENT_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bt_addr.h"

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LEA_CLIENT_MSG_ADD_STR(msg, num, str, len) \
    if (str != NULL && len != 0) {                 \
        msg->data.string##num = malloc(len + 1);   \
        msg->data.string##num[len] = '\0';         \
        memcpy(msg->data.string##num, str, len);   \
    } else {                                       \
        msg->data.string##num = NULL;              \
    }

typedef enum {
    CONNECT_DEVICE = 1,
    DISCONNECT_DEVICE = 2,
    CONNECT_AUDIO = 3,
    DISCONNECT_AUDIO = 4,
    STARTUP = 10,
    SHUTDOWN = 11,
    TIMEOUT = 12,
    OFFLOAD_START_REQ,
    OFFLOAD_STOP_REQ,
    OFFLOAD_START_EVT,
    OFFLOAD_STOP_EVT,
    OFFLOAD_TIMEOUT,
    STACK_EVENT_STACK_STATE,
    STACK_EVENT_CONNECTION_STATE,
    STACK_EVENT_METADATA_UPDATED,
    STACK_EVENT_STORAGE,
    STACK_EVENT_SERVICE,
    STACK_EVENT_STREAM_ADDED,
    STACK_EVENT_STREAM_REMOVED,
    STACK_EVENT_STREAM_STARTED,
    STACK_EVENT_STREAM_STOPPED,
    STACK_EVENT_STREAM_RESUME,
    STACK_EVENT_STREAM_SUSPEND,
    STACK_EVENT_STREAN_RECV,
    STACK_EVENT_STREAN_SENT,
    STACK_EVENT_ASE_CODEC_CONFIG,
    STACK_EVENT_ASE_QOS_CONFIG,
    STACK_EVENT_ASE_ENABLING,
    STACK_EVENT_ASE_STREAMING,
    STACK_EVENT_ASE_DISABLING,
    STACK_EVENT_ASE_RELEASING,
    STACK_EVENT_ASE_IDLE,
} lea_client_event_t;

typedef struct
{
    bt_address_t addr;
    uint32_t valueint1;
    uint32_t valueint2;
    uint16_t valueint3;
    uint16_t valueint4;
    size_t size;
    void* data;
    void* cb;
} lea_client_data_t;

typedef struct
{
    lea_client_event_t event;
    lea_client_data_t data;
} lea_client_msg_t;

typedef enum {
    STACK_EVENT_CSIP_CS_SIRK = 0,
    STACK_EVENT_CSIP_CS_SIZE,
    STACK_EVENT_CSIP_CS_CREATED,
    STACK_EVENT_CSIP_CS_SIZE_UPDATED,
    STACK_EVENT_CSIP_CS_DELETED,
    STACK_EVENT_CSIP_CS_LOCKED,
    STACK_EVENT_CSIP_CS_UNLOCKED,
    STACK_EVENT_CSIP_CS_ORDERED_ACCESS,
    STACK_EVENT_CSIP_MEMBER_RANK,
    STACK_EVENT_CSIP_MEMBER_DISCOVERED,
    STACK_EVENT_CSIP_MEMBER_ADD,
    STACK_EVENT_CSIP_MEMBER_REMOVED,
    STACK_EVENT_CSIP_MEMBER_DISCOVERY_TERMINATED,
    STACK_EVENT_CSIP_MEMBER_LOCKED,
    STACK_EVENT_CSIP_MEMBER_UNLOCKED,
} lea_csip_event_t;

typedef struct {
    uint8_t valueint8;
    uint8_t dataarry[1];
} lea_csip_event_data_t;

typedef struct {
    bt_address_t addr;
    lea_csip_event_t event;
    lea_csip_event_data_t event_data;
} lea_csip_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lea_client_msg_t* lea_client_msg_new(lea_client_event_t event,
    bt_address_t* addr);

lea_client_msg_t* lea_client_msg_new_ext(lea_client_event_t event, bt_address_t* addr,
    void* data, uint32_t size);

void lea_client_msg_destory(lea_client_msg_t* msg);

lea_csip_msg_t* lea_csip_msg_new(lea_csip_event_t event, bt_address_t* remote_addr);
lea_csip_msg_t* lea_csip_msg_new_ext(lea_csip_event_t event, bt_address_t* remote_addr, size_t size);
void lea_csip_msg_destory(lea_csip_msg_t* ccp_msg);

#endif /* __LEA_CLIENT_EVENT_H__ */
