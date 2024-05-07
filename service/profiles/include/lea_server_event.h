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
#ifndef __LEA_SERVER_EVENT_H__
#define __LEA_SERVER_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "bt_addr.h"

#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LEA_SERVER_MSG_ADD_STR(msg, num, str, len) \
    if (str != NULL && len != 0) {                 \
        msg->data.string##num = malloc(len + 1);   \
        msg->data.string##num[len] = '\0';         \
        memcpy(msg->data.string##num, str, len);   \
    } else {                                       \
        msg->data.string##num = NULL;              \
    }

typedef enum {
    DISCONNECT = 1,
    CONFIG_CODEC = 2,
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
    STACK_EVENT_INIT,
    STACK_EVENT_ANNOUNCE,
    STACK_EVENT_DISCONNECT,
    STACK_EVENT_CLEANUP,
} lea_server_event_t;

typedef struct
{
    bt_address_t addr;
    uint32_t valueint1;
    uint32_t valueint2;
    uint16_t valueint3;
    uint16_t valueint4;
    size_t size;
    void* data;
} lea_server_data_t;

typedef struct
{
    lea_server_event_t event;
    lea_server_data_t data;
} lea_server_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lea_server_msg_t* lea_server_msg_new(lea_server_event_t event,
    bt_address_t* addr);

lea_server_msg_t* lea_server_msg_new_ext(lea_server_event_t event,
    bt_address_t* addr, void* data, size_t size);

void lea_server_msg_destory(lea_server_msg_t* msg);

#endif /* __LEA_SERVER_EVENT_H__ */
