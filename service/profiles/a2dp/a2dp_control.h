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
#ifndef __A2DP_CONTROL_H__
#define __A2DP_CONTROL_H__

#include "audio_transport.h"

#define A2DP_CTRL_EVT_HEADER_LEN 1

#define AUDIO_TRANS_CH_ID_AV_SOURCE_CTRL 0
#define AUDIO_TRANS_CH_ID_AV_SOURCE_AUDIO 1
#define AUDIO_TRANS_CH_ID_AV_SINK_CTRL 2
#define AUDIO_TRANS_CH_ID_AV_SINK_AUDIO 3
#define AUDIO_TRANS_CH_ID_AV_INVALID 0xFF

typedef enum {
    A2DP_CTRL_CMD_START,
    A2DP_CTRL_CMD_STOP,
    A2DP_CTRL_CMD_CONFIG_DONE
} a2dp_ctrl_cmd_t;

typedef enum {
    A2DP_CTRL_EVT_STARTED,
    A2DP_CTRL_EVT_START_FAIL,
    A2DP_CTRL_EVT_STOPPED,
    A2DP_CTRL_EVT_UPDATE_CONFIG
} a2dp_ctrl_evt_t;

extern void a2dp_control_init(uint8_t ctrl_id, uint8_t data_id);
extern void a2dp_control_ch_close(uint8_t ctrl_id, uint8_t data_id);
extern void a2dp_control_cleanup(void);
extern void a2dp_control_event(uint8_t ch_id, a2dp_ctrl_evt_t evt);
extern void a2dp_control_update_audio_config(uint8_t ch_id, uint8_t isvalid);
extern transport_conn_state_t a2dp_control_get_state(uint8_t ch_id);

#endif
