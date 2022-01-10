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

#ifndef __EUV_PTY_H__
#define __EUV_PTY_H__
#include "uv.h"

typedef struct _euv_pty euv_pty_t;
typedef void (*euv_read_cb)(euv_pty_t* handle,
    const uint8_t* buf, ssize_t size);
typedef void (*euv_write_cb)(euv_pty_t* handle, uint8_t* buf, int status);
typedef void (*euv_alloc_cb)(euv_pty_t* handle, uint8_t** buf, size_t *len);

euv_pty_t* euv_pty_init(uv_loop_t* loop, int fd, uv_tty_mode_t mode);
void euv_pty_close(euv_pty_t* hdl);
int euv_pty_write(euv_pty_t* handle, uint8_t* buffer, int length, euv_write_cb cb);
int euv_pty_read_start(euv_pty_t* handle, uint16_t read_size, euv_read_cb cb);
int euv_pty_read_start2(euv_pty_t* handle, uint16_t read_size, euv_read_cb read_cb, euv_alloc_cb alloc_cb);
int euv_pty_read_stop(euv_pty_t* handle);
#endif