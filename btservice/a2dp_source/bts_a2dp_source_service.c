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
#include <sys/types.h>

#include "btm_manager.h"
#include "bts_a2dp_source.h"
#include "bts_service.h"

#define LOG_TAG "a2dp_service"
#include "log.h"

static a2dp_source_callbacks_t* a2dpSourceCbs = NULL;

static void a2dp_connection_state_cb(bt_address addr,
    a2dp_connection_state_t state)
{
    if (a2dpSourceCbs)
        a2dpSourceCbs->connection_state_cb(addr, state);
}

static void a2dp_audio_state_cb(bt_address addr,
    a2dp_audio_state_t state)
{
    if (a2dpSourceCbs)
        a2dpSourceCbs->audio_state_cb(addr, state);
}

static void a2dp_audio_source_config_cb(bt_address addr)
{
    if (a2dpSourceCbs)
        a2dpSourceCbs->audio_source_config_cb(addr);
}

static const a2dp_source_callbacks_t a2dp_callbacks = {
    sizeof(a2dp_source_callbacks_t),
    a2dp_connection_state_cb,
    a2dp_audio_state_cb,
    a2dp_audio_source_config_cb
};

static bt_result_code a2dp_source_connect(void* handle, bt_address addr)
{
    return bts_a2dp_source_connect(addr);
}

static bt_result_code a2dp_source_disconnect(void* handle, bt_address addr)
{
    return bts_a2dp_source_disconnect(addr);
}

static bt_result_code a2dp_source_set_silence_device(void* handle, bt_address addr, bool silence)
{
    return BT_RESULT_SUCCESS;
}

static bt_result_code a2dp_source_set_active_device(void* handle, bt_address addr)
{
    return BT_RESULT_SUCCESS;
}

static void a2dp_source_set_callbacks(void* handle, a2dp_source_callbacks_t* callbacks)
{
    a2dpSourceCbs = callbacks;
}

static const a2dp_source_interface_t a2dpSourceSvrInterface = {
    sizeof(a2dp_source_interface_t),
    a2dp_source_connect,
    a2dp_source_disconnect,
    a2dp_source_set_silence_device,
    a2dp_source_set_active_device,
    a2dp_source_set_callbacks
};

bt_result_code a2dp_source_service_start(void)
{
    BT_LOGD("%s", __func__);
    return bts_a2dp_source_init(&a2dp_callbacks);
}

void a2dp_source_service_stop(void)
{
    bts_a2dp_source_cleanup();
}

const a2dp_source_interface_t* get_a2dp_source_service_interface(void)
{
    return &a2dpSourceSvrInterface;
}