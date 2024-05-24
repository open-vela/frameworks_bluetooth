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
#ifndef __HFP_HF_EVENT_H__
#define __HFP_HF_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define HF_MSG_ADD_STR(msg, num, str, len)       \
    if (str != NULL && len != 0) {               \
        msg->data.string##num = malloc(len + 1); \
        msg->data.string##num[len] = '\0';       \
        memcpy(msg->data.string##num, str, len); \
    } else {                                     \
        msg->data.string##num = NULL;            \
    }

typedef enum {
    HF_CONNECT = 1,
    HF_DISCONNECT = 2,
    HF_CONNECT_AUDIO = 3,
    HF_DISCONNECT_AUDIO = 4,
    HF_VOICE_RECOGNITION_START = 5,
    HF_VOICE_RECOGNITION_STOP = 6,
    HF_SET_VOLUME = 7,
    HF_DIAL_NUMBER = 10,
    HF_DIAL_MEMORY = 11,
    HF_DIAL_LAST = 12,
    HF_ACCEPT_CALL = 13,
    HF_REJECT_CALL = 14,
    HF_HOLD_CALL = 15,
    HF_TERMINATE_CALL = 16,
    HF_CONTROL_CALL = 17,
    HF_QUERY_CURRENT_CALLS = 18,
    HF_SEND_AT_COMMAND = 19,
    HF_UPDATE_BATTERY_LEVEL = 20,
    HF_SEND_DTMF = 21,
    HF_STARTUP = 28,
    HF_SHUTDOWN = 29,
    HF_TIMEOUT = 30,
    HF_OFFLOAD_START_REQ,
    HF_OFFLOAD_STOP_REQ,
    HF_OFFLOAD_START_EVT,
    HF_OFFLOAD_STOP_EVT,
    HF_OFFLOAD_TIMEOUT_EVT,
    HF_STACK_EVENT,
    HF_STACK_EVENT_AUDIO_REQ,
    HF_STACK_EVENT_CONNECTION_STATE_CHANGED,
    HF_STACK_EVENT_AUDIO_STATE_CHANGED,
    HF_STACK_EVENT_VR_STATE_CHANGED,
    HF_STACK_EVENT_CALL,
    HF_STACK_EVENT_CALLSETUP,
    HF_STACK_EVENT_CALLHELD,
    HF_STACK_EVENT_CLIP,
    HF_STACK_EVENT_CALL_WAITING,
    HF_STACK_EVENT_CURRENT_CALLS,
    HF_STACK_EVENT_VOLUME_CHANGED,
    HF_STACK_EVENT_CMD_RESPONSE,
    HF_STACK_EVENT_CMD_RESULT,
    HF_STACK_EVENT_RING_INDICATION,
    HF_STACK_EVENT_CODEC_CHANGED,
} hfp_hf_event_t;

typedef struct
{
    bt_address_t addr;
    uint64_t valueint1;
    uint32_t valueint2;
    uint32_t valueint3;
    uint32_t valueint4;
    size_t size;
    char* string1;
    char* string2;
    void* data;
} hfp_hf_data_t;

typedef struct
{
    hfp_hf_event_t event;
    hfp_hf_data_t data;
} hfp_hf_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
hfp_hf_msg_t* hfp_hf_msg_new(hfp_hf_event_t event, bt_address_t* addr);
hfp_hf_msg_t* hfp_hf_msg_new_ext(hfp_hf_event_t event, bt_address_t* addr,
    void* data, size_t size);
void hfp_hf_msg_destroy(hfp_hf_msg_t* msg);

#endif /* __HFP_HF_EVENT_H__ */
