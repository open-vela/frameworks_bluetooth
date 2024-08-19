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
#include "a2dp_control.h"
#include "a2dp_device.h"
#include "a2dp_sink_audio.h"
#include "a2dp_source_audio.h"
#include "bt_utils.h"
#include <stdio.h>
#include <stdlib.h>
#define LOG_TAG "a2dp_audio"
#include "utils/log.h"

static int g_audio_flag = 0;

bool a2dp_audio_on_connection_changed(uint8_t peer_sep, bool connected)
{
    BT_LOGD("%s, %d", __func__, connected);

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (peer_sep == SEP_SNK)
        return a2dp_source_on_connection_changed(connected);
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        return a2dp_sink_on_connection_changed(connected);
#endif
    return false;
}

void a2dp_audio_on_started(uint8_t peer_sep, bool started)
{
    BT_LOGD("%s: %d", __func__, started);
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (peer_sep == SEP_SNK)
        a2dp_source_on_started(started);
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        a2dp_sink_on_started(started);
#endif
}

void a2dp_audio_on_stopped(uint8_t peer_sep)
{
    BT_LOGD("%s", __func__);
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (peer_sep == SEP_SNK)
        a2dp_source_on_stopped();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        a2dp_sink_on_stopped();
#endif
}

void a2dp_audio_prepare_suspend(uint8_t peer_sep)
{
    BT_LOGD("%s", __func__);
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (peer_sep == SEP_SNK)
        a2dp_source_prepare_suspend();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (peer_sep == SEP_SRC)
        a2dp_sink_prepare_suspend();
#endif
}

void a2dp_audio_setup_codec(uint8_t peer_sep, bt_address_t* bd_addr)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (peer_sep == SEP_SNK)
        a2dp_source_setup_codec(bd_addr);
    else
#endif
    {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        a2dp_sink_setup_codec(bd_addr);
#endif
    }
}

void a2dp_audio_init(uint8_t svr_class, bool offloading)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (svr_class == SVR_SOURCE) {
        a2dp_source_audio_init(offloading);
        g_audio_flag |= 1 << SVR_SOURCE;
    } else
#endif
    {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        a2dp_sink_audio_init();
        g_audio_flag |= 1 << SVR_SINK;
#endif
    }
}

void a2dp_audio_cleanup(uint8_t svr_class)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (svr_class == SVR_SOURCE) {
        a2dp_source_audio_cleanup();
        g_audio_flag &= ~(1 << SVR_SOURCE);
    } else
#endif
    {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        a2dp_sink_audio_cleanup();
        g_audio_flag &= ~(1 << SVR_SINK);
#endif
    }

    if (!g_audio_flag)
        a2dp_control_cleanup();
}
