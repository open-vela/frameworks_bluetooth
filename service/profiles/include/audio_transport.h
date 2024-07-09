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
#ifndef __AUDIO_TRANSPORT_H__
#define __AUDIO_TRANSPORT_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdbool.h>

#include "uv.h"

typedef enum {
    TRANSPORT_OPEN_EVT = 0x0001,
    TRANSPORT_CLOSE_EVT = 0x0002,
    TRANSPORT_RX_DATA_EVT = 0x0004,
    TRANSPORT_RX_DATA_READY_EVT = 0x0008,
    TRANSPORT_TX_DATA_READY_EVT = 0x0010
} audio_transport_event_t;

typedef enum {
    AUDIO_CTRL_CMD_START,
    AUDIO_CTRL_CMD_STOP,
    AUDIO_CTRL_CMD_CONFIG_DONE
} audio_ctrl_cmd_t;

typedef enum {
    AUDIO_CTRL_EVT_STARTED,
    AUDIO_CTRL_EVT_START_FAIL,
    AUDIO_CTRL_EVT_STOPPED,
    AUDIO_CTRL_EVT_UPDATE_CONFIG
} audio_ctrl_evt_t;

typedef enum {
    IPC_DISCONNTECTED = -1,
    IPC_CONNTECTED
} transport_conn_state_t;

typedef struct _audio_transport audio_transport_t;
typedef void (*transport_event_cb_t)(uint8_t ch_id, audio_transport_event_t event);
typedef void (*transport_alloc_cb_t)(uint8_t ch_id, uint8_t** buffer, size_t* len);
typedef void (*transport_read_cb_t)(uint8_t ch_id, uint8_t* buffer, ssize_t len);
typedef void (*transport_write_cb_t)(uint8_t ch_id, uint8_t* buffer);

#define AUDIO_TRANS_CH_NUM 5
#define AUDIO_TRANS_CH_ID_ALL 6 /* used to address all the ch id at once */

const char* audio_transport_dump_event(uint8_t event);
audio_transport_t* audio_transport_init(uv_loop_t* loop);
bool audio_transport_open(audio_transport_t* transport, uint8_t ch_id,
    const char* path, transport_event_cb_t cb);
void audio_transport_close(audio_transport_t* transport, uint8_t ch_id);
int audio_transport_write(audio_transport_t* transport, uint8_t ch_id,
    const uint8_t* data, uint16_t len,
    transport_write_cb_t cb);
int audio_transport_read_start(audio_transport_t* transport,
    uint8_t ch_id,
    transport_alloc_cb_t alloc_cb,
    transport_read_cb_t read_cb);
int audio_transport_read_stop(audio_transport_t* transport, uint8_t ch_id);
transport_conn_state_t audio_transport_get_state(audio_transport_t* transport,
    uint8_t ch_id);

#endif