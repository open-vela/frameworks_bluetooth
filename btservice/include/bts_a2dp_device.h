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
#ifndef __BTS_A2DP_DEVICE_H__
#define __BTS_A2DP_DEVICE_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/list.h>
#include "btm_manager.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_event.h"
#include "bts_a2dp_state_machine.h"

#define SEP_SRC     0     /* Source SEP */
#define SEP_SNK     1     /* Sink SEP */
#define SEP_INVALID 3     /* Invalid SEP */

#define SVR_SOURCE  0
#define SVR_SINK    1
typedef struct {
    bt_address bd_addr;
    uint8_t is_sink;
    a2dp_codec_config_t codec_config;
    uint16_t mtu;
} a2dp_peer_t;

typedef struct {
    struct list_node node;
    a2dp_state_machine_t* a2dp_sm;
    bt_address bd_addr;
    a2dp_peer_t peer;
    uint8_t peer_sep;
} a2dp_device_t;

a2dp_device_t* find_a2dp_device_by_addr(struct list_node *list, bt_address bd_addr);
a2dp_device_t* a2dp_device_new(void *ctx, uint8_t peer_sep, bt_address bd_addr);
void a2dp_device_delete(a2dp_device_t* device);
#endif
