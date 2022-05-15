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

#include "btm_pan.h"
#include "bts_panu.h"

#define LOG_TAG "pan_service"
#include "log.h"

static pan_callbacks_t* panCallbacks = NULL;
static void pan_svr_netif_state_callback(pan_netif_state_t state,
                                           int local_role,
                                           const char* ifname)
{
    if (panCallbacks)
        panCallbacks->netif_state_cb(state, local_role, ifname);
}

static void pan_svr_connection_state_callback(pan_connection_state_t state,
                                              bt_address bd_addr,
                                              uint8_t local_role,
                                              uint8_t remote_role)
{
    if (panCallbacks)
        panCallbacks->connection_state_cb(state, bd_addr, local_role, remote_role);
}

static bt_result_code pan_connect(void* handle, bt_address addr, uint8_t dst_role, uint8_t src_role)
{
    return bts_pan_connect(addr, dst_role, src_role);
}

static bt_result_code pan_disconnect(void* handle, bt_address addr)
{
    return bts_pan_disconnect(addr);
}

static void pan_set_callbacks(void** handle, pan_callbacks_t* callbacks)
{
    panCallbacks = callbacks;
}

static void pan_reset_callbacks(void** handle)
{
    panCallbacks = NULL;
}

static pan_callbacks_t pan_cbs = {
    sizeof(pan_callbacks_t),
    pan_svr_netif_state_callback,
    pan_svr_connection_state_callback,
};

static pan_interface_t panInterface = {
    sizeof(pan_interface_t),
    pan_connect,
    pan_disconnect,
    pan_set_callbacks,
    pan_reset_callbacks
};

bt_result_code pan_service_start(void)
{
    bt_result_code ret;

    ret = bts_pan_init(&pan_cbs);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("PAN Service start failed");
        return ret;
    }

    BT_LOGD("PAN Service Started");
    return ret;
}

void pan_service_stop(void)
{
    bts_pan_cleanup();
    BT_LOGD("PAN Service Stoped");
}

pan_interface_t* get_pan_service_interface(void)
{
    return &panInterface;
}