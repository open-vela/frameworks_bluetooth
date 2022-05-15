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
#include <stdint.h>

// internel dependent
#include "btm_manager.h"
#include "bts_service.h"
#include "bts_service_interface.h"
#include "btm_pan.h"
#include "bts_panu.h"


#define LOG_TAG "btm_pan"
#include "log.h"

static pan_interface_t* get_service(void)
{
    return (pan_interface_t*)get_bluetooth_service_interface()->get_profile_interface(BT_PROFILE_PAN);
}

static bt_result_code pan_connect(void* handle, bt_address addr, uint8_t dst_role, uint8_t src_role)
{
    pan_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->connect(handle, addr, dst_role, src_role);
}

static bt_result_code pan_disconnect(void* handle, bt_address addr)
{
    pan_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;

    return service->disconnect(handle, addr);
}

static void pan_set_callbacks(void** handle, pan_callbacks_t* callbacks)
{
    pan_interface_t* service = get_service();
    if (!service)
        return;
    service->set_callbacks(handle, callbacks);
}

static void pan_reset_callbacks(void** handle)
{
    pan_interface_t* service = get_service();
    if (!service)
        return;
    service->reset_callbacks(handle);
}

const static pan_interface_t panInterface = {
    sizeof(pan_interface_t),
    pan_connect,
    pan_disconnect,
    pan_set_callbacks,
    pan_reset_callbacks
};

const pan_interface_t* get_pan_interface(void)
{
    return &panInterface;
}
