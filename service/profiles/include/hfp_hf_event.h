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
    CONNECT = 1,
    DISCONNECT = 2,
    CONNECT_AUDIO = 3,
    DISCONNECT_AUDIO = 4,
    VOICE_RECOGNITION_START = 5,
    VOICE_RECOGNITION_STOP = 6,
    SET_MIC_VOLUME = 7,
    SET_SPEAKER_VOLUME = 8,
    DIAL_NUMBER = 10,
    DIAL_MEMORY = 11,
    DIAL_LAST = 12,
    ACCEPT_CALL = 13,
    REJECT_CALL = 14,
    HOLD_CALL = 15,
    TERMINATE_CALL = 16,
    QUERY_CURRENT_CALLS = 17,
    SEND_AT_COMMAND = 18,
    UPDATE_BATTERY_LEVEL = 19,
    CONTROL_CALL = 20,
    STARTUP = 28,
    SHUTDOWN = 29,
    TIMEOUT = 30,
    STACK_EVENT = 32,
    STACK_EVENT_AUDIO_REQ,
    STACK_EVENT_CONNECTION_STATE_CHANGED,
    STACK_EVENT_AUDIO_STATE_CHANGED,
    STACK_EVENT_VR_STATE_CHANGED,
    STACK_EVENT_CALL,
    STACK_EVENT_CALLSETUP,
    STACK_EVENT_CALLHELD,
    STACK_EVENT_CLIP,
    STACK_EVENT_CALL_WAITING,
    STACK_EVENT_CURRENT_CALLS,
    STACK_EVENT_VOLUME_CHANGED,
    STACK_EVENT_CMD_RESPONSE,
    STACK_EVENT_CMD_RESULT,
    STACK_EVENT_RING_INDICATION,
    STACK_EVENT_CODEC_CHANGED,
} hfp_hf_event_t;

typedef struct
{
    bt_address_t addr;
    uint32_t valueint1;
    uint32_t valueint2;
    uint32_t valueint3;
    uint32_t valueint4;
    char *string1;
    char *string2;
} hfp_hf_data_t;

typedef struct
{
    hfp_hf_event_t event;
    hfp_hf_data_t data;
} hfp_hf_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
hfp_hf_msg_t *hfp_hf_msg_new(hfp_hf_event_t event, bt_address_t *addr);
void hfp_hf_msg_destory(hfp_hf_msg_t *msg);

#endif /* __HFP_HF_EVENT_H__ */
