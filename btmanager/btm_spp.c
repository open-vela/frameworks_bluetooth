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
#define LOG_TAG "btm_spp"
#include <stdio.h>
#include <sys/types.h>

// internel dependent
#include "btm_manager.h"
#include "btm_spp.h"
#include "bts_service.h"
#include "bts_service_interface.h"
#include "bts_spp.h"
#include "log.h"

static spp_interface_t* get_service(void)
{
    return (spp_interface_t*)get_bluetooth_service_interface()->get_profile_interface(BT_PROFILE_SPP);
}

static BT_RESULT_CODE spp_server_start(void* handle, uint16_t port, uint16_t uuid16)
{
    spp_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    return service->server_start(handle, port, uuid16);
}

static BT_RESULT_CODE spp_server_stop(void* handle, uint16_t port)
{
    spp_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    return service->server_stop(handle, port);
}

static BT_RESULT_CODE spp_client_connect(void* handle, bt_address addr, uint16_t port, uint16_t uuid16)
{
    spp_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    return service->client_connect(handle, addr, port, uuid16);
}

static BT_RESULT_CODE spp_disconnect(void* handle, bt_address addr, uint16_t port)
{
    spp_interface_t* service = get_service();
    if (!service)
        return BT_RESULT_FAILED;
    return service->disconnect(handle, addr, port);
}

static void spp_set_callbacks(void* handle, spp_callbacks_t* callbacks)
{
    spp_interface_t* service = get_service();
    if (!service)
        return;
    service->set_callbacks(handle, callbacks);
}

static spp_interface_t sppInterface = {
    sizeof(spp_interface_t),
    spp_server_start,
    spp_server_stop,
    spp_client_connect,
    spp_disconnect,
    spp_set_callbacks,
};

spp_interface_t* get_spp_interface(void)
{
    return &sppInterface;
}
