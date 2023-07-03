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
#ifndef __BT_IPC_H__
#define __BT_IPC_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "uv.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_MAX_IPC_NUM
#define CONFIG_MAX_IPC_NUM 4
#endif

#define IPC_CH_ID_AV_CTRL         0
#define IPC_CH_ID_AV_AUDIO        1
#define IPC_CH_ID_AV_SOURCE_CTRL  0
#define IPC_CH_ID_AV_SOURCE_AUDIO 1
#define IPC_CH_ID_AV_SINK_CTRL    2
#define IPC_CH_ID_AV_SINK_AUDIO   3
#define IPC_CH_NUM                CONFIG_MAX_IPC_NUM

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef enum {
    IPC_OPEN_EVT = 0x0001,
    IPC_CLOSE_EVT = 0x0002,
    IPC_RX_DATA_EVT = 0x0004,
    IPC_RX_DATA_READY_EVT = 0x0008,
    IPC_TX_DATA_READY_EVT = 0x0010
} ipc_event_t;

typedef void (*ipc_event_cb_t)(uint8_t ch_id, ipc_event_t event);
typedef void (*ipc_alloc_cb_t)(uint8_t ch_id, uint8_t **buffer, size_t *len);
typedef void (*ipc_read_cb_t)(uint8_t ch_id, uint8_t *buffer, ssize_t len);
typedef void (*ipc_write_cb_t)(uint8_t ch_id, uint8_t *buffer);

typedef enum {
    IPC_DISCONNTECTED = -1,
    IPC_CONNTECTED
} ipc_conn_state_t;

typedef struct {
    void *ipc_handle;
    uint8_t ch_id;
    uv_pipe_t *svr_pipe;
    uv_pipe_t *cli_pipe;
    ipc_conn_state_t state;
    ipc_event_cb_t event_cb;
} ipc_channel_t;

typedef struct {
    uv_write_t req;
    uint8_t *buffer;
    ipc_channel_t *ch;
    ipc_write_cb_t write_cb;
} ipc_write_t;

typedef struct {
    uint16_t read_size;
    ipc_channel_t *ch;
    ipc_alloc_cb_t alloc_cb;
    ipc_read_cb_t read_cb;
} ipc_read_t;

typedef struct {
    uv_loop_t *loop;
    ipc_channel_t ch[CONFIG_MAX_IPC_NUM];
} ipc_handle_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

ipc_handle_t *ipc_init(uv_loop_t *loop);
void ipc_cleanup(ipc_handle_t *handle);
bool ipc_open(ipc_handle_t *ipc, uint8_t ch_id, const char *path,
              ipc_event_cb_t cb);
void ipc_close(ipc_handle_t *ipc, uint8_t ch_id);
int ipc_write(ipc_handle_t *ipc, uint8_t ch_id, const uint8_t *data,
              uint16_t len, ipc_write_cb_t cb);
int ipc_read_start(ipc_handle_t *ipc, uint8_t ch_id,
                   ipc_alloc_cb_t alloc_cb, ipc_read_cb_t cb);
int ipc_read_stop(ipc_handle_t *ipc, uint8_t ch_id);
const char *dump_ipc_event(uint8_t event);

#endif