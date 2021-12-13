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
#ifndef __BTS_A2DP_CONTROL_H__
#define __BTS_A2DP_CONTROL_H__

#if 0
#define A2DP_SINK_CTRL_PATH "/data/misc/bluedroid/.sink_ctrl"
#define A2DP_SINK_DATA_PATH "/data/misc/bluedroid/.sink_data"
#define A2DP_SOURCE_CTRL_PATH "/data/misc/bluedroid/.source_ctrl"
#define A2DP_SOURCE_DATA_PATH "/data/misc/bluedroid/.source_data"
#else
#define A2DP_SINK_CTRL_PATH "sink_ctrl"
#define A2DP_SINK_DATA_PATH "sink_data"
#define A2DP_SOURCE_CTRL_PATH "source_ctrl"
#define A2DP_SOURCE_DATA_PATH "source_data"
#endif

#define A2DP_CTRL_EVT_HEADER_LEN 3

typedef enum {
    A2DP_CTRL_CMD = 1,
    A2DP_CTRL_EVT
} a2dp_ctrl_type_t;

/*
cmd:      cmd_type|cmd_code|cmd_len|cmd_data|
cmd cmpt  evt_type|evt_code|evt_len|cmd_code|cmd_status|evt_data|
evt       evt_type|evt_code|evt_len|evt_data|
*/
typedef enum {
    A2DP_CTRL_CMD_NONE,
    A2DP_CTRL_CMD_CHECK_READY,
    A2DP_CTRL_CMD_START,
    A2DP_CTRL_CMD_STOP,
    A2DP_CTRL_CMD_SUSPEND,
    A2DP_CTRL_GET_INPUT_AUDIO_CONFIG,
    A2DP_CTRL_GET_OUTPUT_AUDIO_CONFIG,
    A2DP_CTRL_SET_OUTPUT_AUDIO_CONFIG
} a2dp_ctrl_cmd_t;

typedef enum {
    A2DP_CTRL_EVT_CMD_COMPLETED,
    A2DP_CTRL_EVT_CONNECTED,
    A2DP_CTRL_EVT_DISCONNECTED
} a2dp_ctrl_evt_t;

typedef enum {
    A2DP_CTRL_STATUS_SUCCESS,
    A2DP_CTRL_STATUS_FAILURE,
    A2DP_CTRL_STATUS_INCALL_FAILURE, /* Failure when in Call*/
    A2DP_CTRL_STATUS_UNSUPPORTED,
    A2DP_CTRL_STATUS_PENDING,
    A2DP_CTRL_STATUS_DISCONNECT_IN_PROGRESS,
} a2dp_ctrl_status_t;

extern void bts_a2dp_control_init(uint8_t ctrl_id, uint8_t data_id);
extern void bts_a2dp_control_cleanup(void);
extern void bts_a2dp_ctrl_event(uint8_t ch_id, a2dp_ctrl_evt_t event);
extern void bts_a2dp_ctrl_command_ack(uint8_t ch_id, a2dp_ctrl_cmd_t cmd, a2dp_ctrl_status_t ack);
#endif
