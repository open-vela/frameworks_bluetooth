/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#ifndef __EUV_PIPE_H__
#define __EUV_PIPE_H__
#include "uv.h"

typedef enum {
    EUV_PIPE_TYPE_UNKNOWN = -1,
    EUV_PIPE_TYPE_SERVER_LOCAL,
    EUV_PIPE_TYPE_SERVER_RPMSG,
    EUV_PIPE_TYPE_CLIENT_LOCAL,
    EUV_PIPE_TYPE_CLIENT_RPMSG,
} euv_pipe_mode_t;

typedef struct euv_pipe {
    uv_pipe_t cli_pipe;
    uv_pipe_t srv_pipe[2];
    euv_pipe_mode_t mode;
    void* data;
} euv_pipe_t;

typedef void (*euv_read_cb)(euv_pipe_t* handle, const uint8_t* buf, ssize_t size);
typedef void (*euv_write_cb)(euv_pipe_t* handle, uint8_t* buf, int status);
typedef void (*euv_alloc_cb)(euv_pipe_t* handle, uint8_t** buf, size_t* len);
typedef void (*euv_connect_cb)(euv_pipe_t* handle, int status, void* user_data);

euv_pipe_t* euv_pipe_open(uv_loop_t* loop, const char* path, euv_connect_cb cb, void* user_data);
void euv_pipe_close(euv_pipe_t* handle);
euv_pipe_t* euv_pipe_connect(uv_loop_t* loop, const char* path, euv_connect_cb cb, void* user_data);
#ifdef CONFIG_NET_RPMSG
euv_pipe_t* euv_rpmsg_pipe_connect(uv_loop_t* loop, const char* path, const char* cpu_name, euv_connect_cb cb, void* user_data);
void euv_pipe_close2(euv_pipe_t* handle);
#endif
void euv_pipe_disconnect(euv_pipe_t* handle);
int euv_pipe_write(euv_pipe_t* handle, uint8_t* buffer, int length, euv_write_cb cb);
int euv_pipe_read_start(euv_pipe_t* handle, uint16_t read_size, euv_read_cb read_cb, euv_alloc_cb alloc_cb);
int euv_pipe_read_stop(euv_pipe_t* handle);
#endif