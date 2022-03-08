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
#define LOG_TAG "btm_a2dp_sink"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <sys/types.h>

#include "btm_a2dp_sink.h"
#include "btm_manager.h"
#include "bts_service.h"
#include "bts_service_interface.h"

#include "log.h"

static a2dp_sink_interface_t* get_service(void)
{
    return (a2dp_sink_interface_t*)get_bluetooth_service_interface()->get_profile_interface(BT_PROFILE_ADVANCED_AUDIO_SINK);
}

static bt_result_code a2dp_sink_connect(void* handle, bt_address addr)
{
    a2dp_sink_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    BT_LOGD("PERFORMANCE-A2DP-SNK-BTM-CONNECT-START");
    return service->connect(handle, addr);
}

static bt_result_code a2dp_sink_disconnect(void* handle, bt_address addr)
{
    a2dp_sink_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->disconnect(handle, addr);
}

static bt_result_code a2dp_sink_set_active_device(void* handle, bt_address addr)
{
    a2dp_sink_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->set_active_device(handle, addr);
}

static void a2dp_sink_set_callbacks(void* handle, a2dp_sink_callbacks_t* callbacks)
{
    a2dp_sink_interface_t* service = get_service();
    if (!service)
        return;

    service->set_callbacks(handle, callbacks);
}

static const a2dp_sink_interface_t a2dpSinkInterface = {
    sizeof(a2dp_sink_interface_t),
    a2dp_sink_connect,
    a2dp_sink_disconnect,
    a2dp_sink_set_active_device,
    a2dp_sink_set_callbacks
};

const a2dp_sink_interface_t* get_a2dp_sink_interface(void)
{
    return &a2dpSinkInterface;
}