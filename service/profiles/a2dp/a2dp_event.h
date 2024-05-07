/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
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
#include "a2dp_sink_audio.h"

typedef enum {
    A2DP_STARTUP = 1,
    A2DP_SHUTDOWN,
    CONNECT_REQ,
    DISCONNECT_REQ,
    STREAM_START_REQ,
    DELAY_STREAM_START_REQ,
    STREAM_SUSPEND_REQ,
    PEER_STREAM_START_REQ,
    CONNECTED_EVT,
    DISCONNECTED_EVT,
    STREAM_STARTED_EVT,
    STREAM_SUSPENDED_EVT,
    STREAM_CLOSED_EVT,
    STREAM_MTU_CONFIG_EVT,
#ifdef CONFIG_BLUETOOTH_A2DP_PEER_PARTIAL_RECONN
    PEER_PARTIAL_RECONN_EVT,
#endif
    CODEC_CONFIG_EVT,
    DEVICE_CODEC_STATE_CHANGE_EVT,
    DATA_IND_EVT,
    CONNECT_TIMEOUT,
    START_TIMEOUT,
    STREAM_SUSPEND_DELAY,
    OFFLOAD_START_REQ,
    OFFLOAD_STOP_REQ,
    OFFLOAD_START_EVT,
    OFFLOAD_STOP_EVT,
    OFFLOAD_TIMEOUT,
} a2dp_event_type_t;

typedef struct
{
    bt_address_t bd_addr;
    uint8_t peer_sep;
    uint16_t mtu;
    uint16_t acl_hdl;
    uint16_t l2c_rcid;
    size_t size;
    void* data;
    void* cb;
    a2dp_sink_packet_t* packet;
} a2dp_event_data_t;

typedef struct
{
    a2dp_event_type_t event;
    a2dp_event_data_t event_data;
} a2dp_event_t;

a2dp_event_t* a2dp_event_new(a2dp_event_type_t event, bt_address_t* bd_addr);
a2dp_event_t* a2dp_event_new_ext(a2dp_event_type_t event, bt_address_t* bd_addr,
    void* data, size_t size);
void a2dp_event_destory(a2dp_event_t* a2dp_event);

#endif
