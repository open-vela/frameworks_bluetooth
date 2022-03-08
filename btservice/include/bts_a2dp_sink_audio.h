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
#ifndef __BTS_A2DP_SINK_AUDIO_H__
#define __BTS_A2DP_SINK_AUDIO_H__

#include <nuttx/mm/circbuf.h>
#include <nuttx/list.h>
#include "btm_common_define.h"
#include "a2dp_ipc.h"

typedef struct {
    struct list_node node;
    uint32_t time_stamp;
    uint16_t seq;
    uint16_t length;
    uint8_t data[0];
}a2dp_sink_packet_t;

typedef struct {
    a2dp_sink_packet_t* (*repackage)(uint8_t *data, uint16_t length);
    void (*packet_send_done)(a2dp_sink_packet_t* packet);
} a2dp_sink_stream_interface_t;

a2dp_sink_packet_t* bts_a2dp_sink_new_packet(uint32_t timestamp, 
                        uint16_t seq, uint8_t *data, uint16_t length);
void bts_a2dp_sink_packet_recieve(a2dp_sink_packet_t *packet);
void bts_a2dp_sink_on_connection_changed(bool connected);
void bts_a2dp_sink_on_started(bool started);
void bts_a2dp_sink_on_stopped(void);
void bts_a2dp_sink_on_suspended(void);
void bts_a2dp_sink_setup_codec(bt_address bd_addr);
void bts_a2dp_sink_audio_init(void);
void bts_a2dp_sink_audio_cleanup(void);
extern const a2dp_sink_stream_interface_t *get_a2dp_sink_sbc_stream_interface(void);

#endif
