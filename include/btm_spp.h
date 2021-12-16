/**@file  btm_spp.h
* @brief       bluetooth adapter for SPP service.
* @details   including get all SPP profile interface
* @date        2021-11-10
* @version     V1.0
*/
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
#ifndef __BTM_SPP_H__
#define __BTM_SPP_H__

#include "btm_manager.h"

#define BT_UUID_SERVCLASS_SERIAL_PORT 0x1101 /* Serial Port Profile (SPP) */

typedef enum {
    SPP_CONNECTION_STATE_DISCONNECTED,
    SPP_CONNECTION_STATE_CONNECTING,
    SPP_CONNECTION_STATE_CONNECTED,
    SPP_CONNECTION_STATE_DISCONNECTING
} spp_connection_state_t;

typedef enum {
    SPP_PTY_MODE_NORMAL,
    SPP_PTY_MODE_RAW
} spp_pty_mode_t;

typedef void (*spp_connection_state_callback)(const bt_address addr, uint16_t port, spp_connection_state_t state);
typedef void (*spp_pty_open_callback)(const bt_address addr, uint16_t port, char* name, int fd);

typedef struct {
    size_t size;
    spp_pty_open_callback pty_open_cb;
    spp_connection_state_callback connection_state_cb;
} spp_callbacks_t;

typedef struct {
    size_t size;
    bt_result_code (*server_start)(void* handle, uint16_t port, uint16_t uuid16);
    bt_result_code (*server_stop)(void* handle, uint16_t port);
    bt_result_code (*client_connect)(void* handle, bt_address addr, uint16_t port, uint16_t uuid16);
    bt_result_code (*disconnect)(void* handle, bt_address addr, uint16_t port);
    void (*set_callbacks)(void* handle, spp_callbacks_t* callbacks);
} spp_interface_t;

spp_interface_t* get_spp_interface(void);

#endif
