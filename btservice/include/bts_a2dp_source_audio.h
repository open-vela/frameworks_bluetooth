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
#ifndef __BTS_A2DP_SOURCE_AUDIO_H__
#define __BTS_A2DP_SOURCE_AUDIO_H__

#include <nuttx/mm/circbuf.h>
#include "a2dp_ipc.h"

typedef enum {
    STATE_OFF,
    STATE_START_UP,
    STATE_RUNNING,
} stream_state_t;

typedef struct {
    void*    buffer;
    uint16_t offset;
    uint16_t length;
} stream_buf_t;

typedef struct {
    uint32_t            last_tx_frames;
    uint32_t            total_tx_frames;
    uint16_t            frames_len;
    uint16_t            max_tx_length;
    uint16_t            mtu;
    uint32_t            sequence_number;
    uint32_t            media_timestamp;
    uint64_t            session_start_us;
    stream_state_t      stream_state;
    uint8_t             codec_info[10];
    uint32_t            interval_ms;
    uv_timer_t*         media_alarm;
    struct circbuf_s    fragmente;
    stream_buf_t        stream_buf;
} a2dp_source_stream_t;

void bts_a2dp_source_audio_init(void);
void bts_a2dp_source_audio_cleanup(void);
void bts_a2dp_source_on_connection_changed(bool connected);
void bts_a2dp_source_on_started(bool started);
void bts_a2dp_source_on_stopped(void);
void bts_a2dp_source_on_suspended(void);
bool bts_a2dp_source_is_streaming(void);
void bts_a2dp_source_set_mtu(uint16_t mtu);

#endif
