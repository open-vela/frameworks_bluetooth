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
    HF_CONNECT,
    HF_DISCONNECT,
    HF_CONNECT_AUDIO,
    HF_DISCONNECT_AUDIO,
    HF_VOICE_RECOGNITION_START,
    HF_VOICE_RECOGNITION_STOP,
    HF_SET_VOLUME,
    HF_MEDIA_VOLUME_CHANGED,
    HF_DIAL_NUMBER,
    HF_DIAL_MEMORY,
    HF_DIAL_LAST,
    HF_ACCEPT_CALL,
    HF_REJECT_CALL,
    HF_HOLD_CALL,
    HF_TERMINATE_CALL,
    HF_CONTROL_CALL,
    HF_QUERY_CURRENT_CALLS,
    HF_QUERY_CURRENT_CALLS_WITH_CALLBACK,
    HF_SEND_AT_COMMAND,
    HF_UPDATE_BATTERY_LEVEL,
    HF_SEND_DTMF,
    HF_GET_SUBSCRIBER_NUMBER,
    HF_STARTUP,
    HF_SHUTDOWN,
    HF_TIMEOUT,
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
    HF_STACK_EVENT_CNUM,
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
