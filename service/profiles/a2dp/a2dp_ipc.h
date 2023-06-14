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
#ifndef __A2DP_IPC_H__
#define __A2DP_IPC_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "stdbool.h"
#include "uv.h"

typedef enum {
    IPC_OPEN_EVT = 0x0001,
    IPC_CLOSE_EVT = 0x0002,
    IPC_RX_DATA_EVT = 0x0004,
    IPC_RX_DATA_READY_EVT = 0x0008,
    IPC_TX_DATA_READY_EVT = 0x0010
} a2dp_ipc_event_t;

typedef struct _a2dp_ipc a2dp_ipc_t;
typedef void (*ipc_event_cb_t)(uint8_t ch_id, a2dp_ipc_event_t event);
typedef void (*ipc_alloc_cb_t)(uint8_t ch_id, uint8_t **buffer, size_t *len);
typedef void (*ipc_read_cb_t)(uint8_t ch_id, uint8_t *buffer, ssize_t len);
typedef void (*ipc_write_cb_t)(uint8_t ch_id, uint8_t *buffer);

#define A2DP_IPC_CH_ID_AV_CTRL         0
#define A2DP_IPC_CH_ID_AV_AUDIO        1
#define A2DP_IPC_CH_ID_AV_SOURCE_CTRL  0
#define A2DP_IPC_CH_ID_AV_SOURCE_AUDIO 1
#define A2DP_IPC_CH_ID_AV_SINK_CTRL    2
#define A2DP_IPC_CH_ID_AV_SINK_AUDIO   3
#define A2DP_IPC_CH_NUM                4
#define A2DP_IPC_CH_ID_ALL             5 /* used to address all the ch id at once */

const char *dump_a2dp_ipc_event(uint8_t event);

a2dp_ipc_t *a2dp_ipc_init(uv_loop_t *loop);
bool a2dp_ipc_open(a2dp_ipc_t *a2dp, uint8_t ch_id, const char *path, ipc_event_cb_t cb);
void a2dp_ipc_close(a2dp_ipc_t *a2dp, uint8_t ch_id);
int a2dp_ipc_write(a2dp_ipc_t *a2dp, uint8_t ch_id, const uint8_t *data, uint16_t len, ipc_write_cb_t cb);
int a2dp_ipc_read_start(a2dp_ipc_t *a2dp, uint8_t ch_id, ipc_alloc_cb_t alloc_cb, ipc_read_cb_t read_cb);
int a2dp_ipc_read_stop(a2dp_ipc_t *a2dp, uint8_t ch_id);

#endif