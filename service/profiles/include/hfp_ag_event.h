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
#ifndef __HFP_AG_EVENT_H__
#define __HFP_AG_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"
#include <stdint.h>
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define AG_MSG_ADD_STR(msg, num, str, len)       \
    if (str != NULL && len != 0) {               \
        msg->data.string##num = malloc(len + 1); \
        msg->data.string##num[len] = '\0';       \
        memcpy(msg->data.string##num, str, len); \
    } else {                                     \
        msg->data.string##num = NULL;            \
    }

typedef enum {
    AG_CONNECT = 1,
    AG_DISCONNECT = 2,
    AG_CONNECT_AUDIO = 3,
    AG_DISCONNECT_AUDIO = 4,
    AG_VOICE_RECOGNITION_START = 5,
    AG_VOICE_RECOGNITION_STOP = 6,
    AG_PHONE_STATE_CHANGE = 7,
    AG_DEVICE_STATUS_CHANGED = 8,
    AG_SET_VOLUME = 9,
    AG_SET_INBAND_RING_ENABLE = 10,
    AG_DIALING_RESULT = 11,
    AG_CLCC_RESPONSE = 12,
    AG_START_VIRTUAL_CALL = 13,
    AG_STOP_VIRTUAL_CALL = 14,
    AG_SEND_AT_COMMAND = 18,
    AG_SEND_VENDOR_SPECIFIC_AT_COMMAND,
    AG_STARTUP = 20,
    AG_SHUTDOWN = 21,
    AG_CONNECT_TIMEOUT = 25,
    AG_AUDIO_TIMEOUT = 26,
    AG_OFFLOAD_START_REQ,
    AG_OFFLOAD_STOP_REQ,
    AG_OFFLOAD_START_EVT,
    AG_OFFLOAD_STOP_EVT,
    AG_OFFLOAD_TIMEOUT_EVT,
    AG_STACK_EVENT = 32,
    AG_STACK_EVENT_AUDIO_REQ,
    AG_STACK_EVENT_CONNECTION_STATE_CHANGED,
    AG_STACK_EVENT_AUDIO_STATE_CHANGED,
    AG_STACK_EVENT_VR_STATE_CHANGED,
    AG_STACK_EVENT_CODEC_CHANGED,
    AG_STACK_EVENT_VOLUME_CHANGED,
    AG_STACK_EVENT_AT_CIND_REQUEST,
    AG_STACK_EVENT_AT_CLCC_REQUEST,
    AG_STACK_EVENT_AT_COPS_REQUEST,
    AG_STACK_EVENT_BATTERY_UPDATE,
    AG_STACK_EVENT_ANSWER_CALL,
    AG_STACK_EVENT_REJECT_CALL,
    AG_STACK_EVENT_HANGUP_CALL,
    AG_STACK_EVENT_DIAL_NUMBER,
    AG_STACK_EVENT_DIAL_MEMORY,
    AG_STACK_EVENT_CALL_CONTROL,
    AG_STACK_EVENT_AT_COMMAND,
    AG_STACK_EVENT_SEND_DTMF,
    AG_STACK_EVENT_NREC_REQ
} hfp_ag_event_t;

typedef struct
{
    bt_address_t addr;
    uint32_t valueint1;
    uint32_t valueint2;
    uint32_t valueint3;
    uint32_t valueint4;
    size_t size;
    char* string1;
    char* string2;
    void* data;
} hfp_ag_data_t;

typedef struct
{
    hfp_ag_event_t event;
    hfp_ag_data_t data;
} hfp_ag_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
hfp_ag_msg_t* hfp_ag_msg_new(hfp_ag_event_t event, bt_address_t* addr);
hfp_ag_msg_t* hfp_ag_event_new_ext(hfp_ag_event_t event, bt_address_t* addr,
    void* data, size_t size);
void hfp_ag_msg_destory(hfp_ag_msg_t* msg);

#endif /* __HFP_HF_EVENT_H__ */
