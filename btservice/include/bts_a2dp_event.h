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
#ifndef __A2DP_EVENT_H__
#define __A2DP_EVENT_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_manager.h"

typedef enum {
    ENABLE = 1,
    CLEANUP,
    CONNECT_REQ,
    DISCONNECT_REQ,
    STREAM_START_REQ,
    STREAM_SUSPEND_REQ,
    CONNECTED_EVT,
    DISCONNECTED_EVT,
    STREAM_STARTED_EVT,
    STREAM_SUSPENDED_EVT,
    STREAM_CLOSED_EVT,
    STREAM_MTU_CONFIG_EVT,
    CODEC_CONFIG_EVT,
    DEVICE_CODEC_STATE_CHANGE_EVT,
    CONNECT_TIMEOUT,
    START_TIMEOUT,
} a2dp_event_type_t;

typedef struct
{
    bt_address bd_addr;
    uint16_t mtu;
    void* data;
} a2dp_event_data_t;

typedef struct
{
    a2dp_event_type_t event;
    a2dp_event_data_t event_data;
} a2dp_event_t;

a2dp_event_t* a2dp_event_new(a2dp_event_type_t event, bt_address bd_addr);
void a2dp_event_destory(a2dp_event_t* a2dp_event);

#endif
