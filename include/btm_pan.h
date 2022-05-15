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
#ifndef __BTM_PAN_H__
#define __BTM_PAN_H__

#include "btm_manager.h"
typedef enum {
    PAN_ROLE_NONE = 0,
    PAN_ROLE_NAP,
    PAN_ROLE_PANU
} pan_role_t;

typedef enum {
  PAN_STATE_DISCONNECTED = 0,
  PAN_STATE_CONNECTING = 1,
  PAN_STATE_CONNECTED = 2,
  PAN_STATE_DISCONNECTING = 3
} pan_connection_state_t;

typedef enum {
  PAN_STATE_ENABLED = 0,
  PAN_STATE_DISABLED = 1
} pan_netif_state_t;

typedef void (*pan_connection_state_callback)(pan_connection_state_t state,
                                              bt_address bd_addr,
                                              uint8_t local_role,
                                              uint8_t remote_role);
typedef void (*pan_netif_state_callback)(pan_netif_state_t state,
                                           int local_role,
                                           const char* ifname);

typedef struct {
  size_t size;
  pan_netif_state_callback netif_state_cb;
  pan_connection_state_callback connection_state_cb;
} pan_callbacks_t;

typedef struct {
    size_t size;
    bt_result_code (*connect)(void* handle, bt_address addr, uint8_t dst_role, uint8_t src_role);
    bt_result_code (*disconnect)(void* handle, bt_address addr);
    void (*set_callbacks)(void** handle, pan_callbacks_t* callbacks);
    void (*reset_callbacks)(void** handle);
} pan_interface_t;

const pan_interface_t* get_pan_interface(void);

#endif