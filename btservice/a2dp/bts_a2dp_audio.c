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
#include <stdio.h>
#include <stdlib.h>
#include "bts_service.h"
#include "bts_a2dp_device.h"
#include "bts_a2dp_sink_audio.h"
#include "bts_a2dp_source_audio.h"
#include "utils/utils.h"
#define LOG_TAG "a2dp_audio"
#include "log.h"

void bts_a2dp_audio_on_connection_changed(uint8_t peer_sep, bool connected)
{
    BT_LOGD("%s, %d", __func__, connected);
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (peer_sep == SEP_SNK)
        bts_a2dp_source_on_connection_changed(connected);
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        bts_a2dp_sink_on_connection_changed(connected);
#endif
}

void bts_a2dp_audio_on_started(uint8_t peer_sep, bool started)
{
    BT_LOGD("%s: %d", __func__, started);
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (peer_sep == SEP_SNK)
        bts_a2dp_source_on_started(started);
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        bts_a2dp_sink_on_started(started);
#endif
}

void bts_a2dp_audio_on_stopped(uint8_t peer_sep)
{
    BT_LOGD("%s", __func__);
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (peer_sep == SEP_SNK)
        bts_a2dp_source_on_stopped();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        bts_a2dp_sink_on_stopped();
#endif
}

void bts_a2dp_audio_on_suspended(uint8_t peer_sep)
{
    BT_LOGD("%s", __func__);
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (peer_sep == SEP_SNK)
        bts_a2dp_source_on_suspended();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        bts_a2dp_sink_on_suspended();
#endif
}

void bts_a2dp_audio_setup_codec(uint8_t peer_sep, bt_address bd_addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (peer_sep == SEP_SNK)
        bts_a2dp_source_setup_codec(bd_addr);
    else
#endif
    {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bts_a2dp_sink_setup_codec(bd_addr);
#endif
    }
}

void bts_a2dp_audio_init(uint8_t svr_class)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (svr_class == SVR_SOURCE)
        bts_a2dp_source_audio_init();
    else
#endif
    {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bts_a2dp_sink_audio_init();
#endif
    }
}

void bts_a2dp_audio_cleanup(uint8_t svr_class)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (svr_class == SVR_SOURCE)
        bts_a2dp_source_audio_cleanup();
    else
#endif
    {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        bts_a2dp_sink_audio_cleanup();
#endif
    }
}
