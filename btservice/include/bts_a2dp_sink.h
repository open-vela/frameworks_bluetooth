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
#ifndef __BTS_A2DP_SINK_H__
#define __BTS_A2DP_SINK_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/list.h>
#include "btm_manager.h"
#include "btm_a2dp_sink.h"
#include "bts_a2dp_device.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_event.h"

typedef struct {
    struct list_node device_list;
    int orb_fd;
    bool enabled;
    const a2dp_sink_callbacks_t* callbacks;
    a2dp_peer_t *active_peer;
} a2dp_sink_t;

bt_result_code bts_a2dp_sink_init(const a2dp_sink_callbacks_t* callbacks);
bt_result_code bts_a2dp_sink_connect(bt_address addr);
bt_result_code bts_a2dp_sink_disconnect(bt_address addr);
void bts_a2dp_sink_cleanup(void);
a2dp_peer_t* bts_a2dp_sink_find_peer(bt_address addr);
bool bts_a2dp_sink_stream_ready(void);
void bts_a2dp_sink_codec_state_change(void);
void bts_a2dp_sink_dump(void);

extern bt_result_code a2dp_sink_service_start(void);
extern void a2dp_sink_service_stop(void);
extern const a2dp_sink_interface_t* get_a2dp_sink_service_interface(void);

#endif
