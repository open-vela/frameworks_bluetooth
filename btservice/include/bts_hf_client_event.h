/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#ifndef __BTS_HF_CLIENT_EVNET_H__
#define __BTS_HF_CLIENT_EVNET_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define HF_MSG_NEW(event, addr) hf_client_msg_new(event, addr)
#define HF_MSG_ADD_STR(msg, num, str, len)             \
    if (str != NULL && len != 0) {                     \
        msg->event_data.string##num = malloc(len + 1); \
        msg->event_data.string##num[len] = '\0';       \
        memcpy(msg->event_data.string##num, str, len); \
    } else {                                           \
        msg->event_data.string##num = NULL;            \
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
    TIMEOUT = 20,
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
    STACK_EVENT_CMD_RESULT,
    STACK_EVENT_RING_INDICATION,
} hf_client_event_t;

typedef struct
{
    bt_address bd_addr;
    uint32_t valueint1;
    uint32_t valueint2;
    uint32_t valueint3;
    uint32_t valueint4;
    char* string1;
    char* string2;
} hf_event_data_t;

typedef struct
{
    hf_client_event_t event;
    hf_event_data_t event_data;
} hf_client_msg_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
hf_client_msg_t* hf_client_msg_new(hf_client_event_t event, bt_address bd_addr);
void hf_client_msg_destory(hf_client_msg_t* msg);
#endif
