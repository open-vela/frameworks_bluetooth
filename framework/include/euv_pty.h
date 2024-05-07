/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#ifndef __EUV_PTY_H__
#define __EUV_PTY_H__
#include "uv.h"

typedef struct _euv_pty euv_pty_t;
typedef void (*euv_read_cb)(euv_pty_t* handle,
    const uint8_t* buf, ssize_t size);
typedef void (*euv_write_cb)(euv_pty_t* handle, uint8_t* buf, int status);
typedef void (*euv_alloc_cb)(euv_pty_t* handle, uint8_t** buf, size_t* len);

euv_pty_t* euv_pty_init(uv_loop_t* loop, int fd, uv_tty_mode_t mode);
void euv_pty_close(euv_pty_t* hdl);
int euv_pty_write(euv_pty_t* handle, uint8_t* buffer, int length, euv_write_cb cb);
int euv_pty_read_start(euv_pty_t* handle, uint16_t read_size, euv_read_cb cb);
int euv_pty_read_start2(euv_pty_t* handle, uint16_t read_size, euv_read_cb read_cb, euv_alloc_cb alloc_cb);
int euv_pty_read_stop(euv_pty_t* handle);
#endif