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

// internel dependent
#include "btm_manager.h"
#include "bts_service.h"

#include "bts_spp.h"
#include "btm_spp.h"

#define LOG_TAG "spp_service"
#include "log.h"

spp_callbacks_t *sppCallbacks = NULL;

static void spp_svr_connection_state_callback(const bt_address addr, uint16_t port, spp_connection_state_t state)
{
  BT_LOGD("%s", __func__);
  if (sppCallbacks)
    sppCallbacks->connection_state_cb(addr, port, state);
}

static void spp_svr_pty_open_callback(const bt_address addr, uint16_t port, char *name, int fd)
{
  BT_LOGD("%s", __func__);
  if (sppCallbacks)
    sppCallbacks->pty_open_cb(addr, port, name, fd);
}

static bt_result_code spp_if_server_start(void * handle, uint16_t port, uint16_t uuid16)
{
  (void)handle;
  return bts_spp_server_start(port, uuid16);
}

static bt_result_code spp_if_server_stop(void * handle, uint16_t port)
{
  (void)handle;
  return bts_spp_server_stop(port);
}

static bt_result_code spp_if_client_connect(void * handle, bt_address addr, uint16_t port, uint16_t uuid16)
{
  (void)handle;
  return bts_spp_client_connect(addr, port, uuid16);
}

static bt_result_code spp_if_disconnect(void * handle, bt_address addr, uint16_t port)
{
  (void)handle;
  return bts_spp_disconnect(addr, port);
}

static void spp_if_set_callbacks(void * handle, spp_callbacks_t *callbacks)
{
  (void)handle;
  sppCallbacks = callbacks;
}

spp_service_callbacks_t spp_service_cbs = {
  sizeof(spp_callbacks_t),
  spp_svr_pty_open_callback,
  spp_svr_connection_state_callback,
};

spp_interface_t sppInterface = {
  sizeof(spp_interface_t),
  spp_if_server_start,
  spp_if_server_stop,
  spp_if_client_connect,
  spp_if_disconnect,
  spp_if_set_callbacks,
};

bt_result_code spp_service_start(void)
{
  bt_result_code ret;

  ret = bts_spp_init(&spp_service_cbs);
  bts_spp_server_start(5, BT_UUID_SERVCLASS_SERIAL_PORT);
  if (ret != BT_RESULT_SUCCESS)
    return ret;

  return ret;
}

void spp_service_stop(void)
{
  bts_spp_cleanup();
}

spp_interface_t *get_spp_service_interface(void)
{
  return &sppInterface;
}