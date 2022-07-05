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

#include "btm_spp.h"
#include "bts_spp.h"

#define LOG_TAG "spp_service"
#include "log.h"

static spp_callbacks_t* sppCallbacks = NULL;

static int spp_svr_connection_state_callback(const bt_address addr, uint16_t scn, uint16_t port, spp_connection_state_t state)
{
    if (sppCallbacks)
        return sppCallbacks->connection_state_cb(addr, scn, port, state);
    return 0;
}

static void spp_svr_pty_open_callback(const bt_address addr, uint16_t port, char* name)
{
    if (sppCallbacks)
        sppCallbacks->pty_open_cb(addr, port, name);
}

static bt_result_code spp_if_server_start(void* handle, uint16_t scn, uint16_t uuid16)
{
    return bts_spp_server_start((spp_handle_t*)handle, scn, uuid16);
}

static bt_result_code spp_if_server_stop(void* handle, uint16_t scn)
{
    return bts_spp_server_stop((spp_handle_t*)handle, scn);
}

static bt_result_code spp_if_client_connect(void* handle, bt_address addr, int16_t scn, uint16_t uuid16, uint16_t *port)
{
    return bts_spp_client_connect((spp_handle_t*)handle, addr, scn, uuid16, port);
}

static bt_result_code spp_if_disconnect(void* handle, bt_address addr, uint16_t port)
{
    return bts_spp_disconnect((spp_handle_t*)handle, addr, port);
}

static void spp_if_set_callbacks(void** handle, spp_callbacks_t* callbacks)
{
    spp_handle_t* spp_handle;

    if (!handle || *handle)
        return;

    spp_handle = bts_spp_register_app(0, callbacks);
    sppCallbacks = callbacks;
    *handle = (void *)spp_handle;
}

static void spp_if_reset_callbacks(void** handle)
{
    if (!handle)
        return;

    bts_spp_unregister_app((spp_handle_t*)*handle);
    sppCallbacks = NULL;
    *handle = NULL;
}

static spp_service_callbacks_t spp_service_cbs = {
    sizeof(spp_callbacks_t),
    spp_svr_pty_open_callback,
    spp_svr_connection_state_callback,
};

static spp_interface_t sppInterface = {
    sizeof(spp_interface_t),
    spp_if_server_start,
    spp_if_server_stop,
    spp_if_client_connect,
    spp_if_disconnect,
    spp_if_set_callbacks,
    spp_if_reset_callbacks
};

bt_result_code spp_service_start(void)
{
    bt_result_code ret;

    ret = bts_spp_init(&spp_service_cbs);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("Spp Service start failed");
        return ret;
    }

    BT_LOGD("Spp Service Started");
    return ret;
}

void spp_service_stop(void)
{
    bts_spp_cleanup();
    BT_LOGD("Spp Service Stoped");
}

spp_interface_t* get_spp_service_interface(void)
{
    return &sppInterface;
}