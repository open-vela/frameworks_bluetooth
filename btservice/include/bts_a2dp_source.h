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
#ifndef __BTS_A2DP_SOURCE_H__
#define __BTS_A2DP_SOURCE_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/list.h>
#include "btm_manager.h"
#include "btm_a2dp_source.h"
#include "bts_a2dp_device.h"

typedef struct {
    struct list_node device_list;
    int orb_fd;
    bool enabled;
    a2dp_peer_t *active_peer;
    const a2dp_source_callbacks_t* callbacks;
} a2dp_source_t;

void bts_a2dp_source_stream_start(void);
void bts_a2dp_source_stream_stop(void);
void bts_a2dp_source_codec_state_change(void);
bool bts_a2dp_source_stream_ready(void);
bool bts_a2dp_source_stream_started(void);
a2dp_peer_t* bts_a2dp_source_find_peer(bt_address addr);
a2dp_peer_t* bts_a2dp_source_active_peer(void);

bt_result_code bts_a2dp_source_init(const a2dp_source_callbacks_t* callbacks);
bt_result_code bts_a2dp_source_connect(bt_address addr);
bt_result_code bts_a2dp_source_disconnect(bt_address addr);
void bts_a2dp_source_cleanup(void);

void bts_a2dp_service_handle_event(bt_profile_id id, void* data, size_t size);
void bts_a2dp_source_dump(void);

extern bt_result_code a2dp_source_service_start(void);
extern void a2dp_source_service_stop(void);
extern const a2dp_source_interface_t* get_a2dp_source_service_interface(void);

#endif
