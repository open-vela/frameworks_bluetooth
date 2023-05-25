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
#ifndef __AG_SERVER_EVENT_H__
#define __AG_SERVER_EVENT_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define AG_MSG_NEW(event, addr) ag_server_msg_new(event, addr)
#define AG_MSG_ADD_STR(msg, num, str, len)       \
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
    PHONE_STATE_CHANGE = 7,
    DEVICE_STATUS_CHANGED = 8,
    SET_VOLUME = 9,
    SET_INBAND_RING_ENABLE = 10,
    DIALING_RESULT = 11,
    SEND_AT_COMMAND = 18,
    STARTUP = 20,
    CLEANUP = 21,
    CONNECT_TIMEOUT = 25,
    AUDIO_TIMEOUT = 26,
    STACK_EVENT = 32,
    STACK_EVENT_AUDIO_REQ,
    STACK_EVENT_CONNECTION_STATE_CHANGED,
    STACK_EVENT_AUDIO_STATE_CHANGED,
    STACK_EVENT_VR_STATE_CHANGED,
    STACK_EVENT_CODEC_CHANGED,
    STACK_EVENT_VOLUME_CHANGED,
    STACK_EVENT_AT_CIND_REQUEST,
    STACK_EVENT_AT_CLCC_REQUEST,
    STACK_EVENT_AT_COPS_REQUEST,
    STACK_EVENT_BATTERY_UPDATE,
    STACK_EVENT_ANSWER_CALL,
    STACK_EVENT_REJECT_CALL,
    STACK_EVENT_HANGUP_CALL,
    STACK_EVENT_DIAL_NUMBER,
    STACK_EVENT_DIAL_MEMORY,
    STACK_EVENT_CALL_CONTROL,
    STACK_EVENT_AT_COMMAND,
    STACK_EVENT_SEND_DTMF,
    CIND_RESP,
    CLCC_RESP,
    COPS_RESP,
    DIAL_RESP
} ag_server_event_t;

typedef struct
{
    bt_address addr;
    uint32_t valueint1;
    uint32_t valueint2;
    uint32_t valueint3;
    uint32_t valueint4;
    uint8_t valueint5;
    uint8_t valueint6;
    uint8_t valueint7;

    char* string1;
    char* string2;
} ag_server_data_t;

typedef struct
{
    ag_server_event_t event;
    ag_server_data_t data;
} ag_server_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
ag_server_msg_t* ag_server_msg_new(ag_server_event_t event, bt_address addr);
void ag_server_msg_destory(ag_server_msg_t* msg);

#endif /* __HFP_HF_EVENT_H__ */
