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
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "uv.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "euv_pty.h"

typedef struct _euv_pty {
    uv_tty_t uv_tty;
} euv_pty_t;

typedef struct {
    uv_write_t req;
    uint8_t* buffer;
    euv_write_cb write_cb;
} euv_wreq_t;

typedef struct {
    euv_read_cb read_cb;
    euv_alloc_cb alloc_cb;
    uint16_t read_size;
} euv_read_t;

static void uv_close_callback(uv_handle_t* handle)
{
    if (handle->data)
        free(handle->data);
    free(handle);
}

static void uv_alloc_callback(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    if (!handle->data)
        return;

    euv_read_t* reader = (euv_read_t*)handle->data;

    if (reader->alloc_cb)
        reader->alloc_cb((euv_pty_t*)handle, (uint8_t**)&buf->base, &buf->len);
    else {
        buf->base = malloc(reader->read_size);
        buf->len = reader->read_size;
    }
}

static void uv_read_callback(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    if (!stream->data)
        return;

    euv_read_t* reader = (euv_read_t*)stream->data;
    /*if read stopped in read callback, will free reader struct*/
    bool release = !reader->alloc_cb;

    if (reader->read_cb)
        reader->read_cb((euv_pty_t*)stream, (const uint8_t*)buf->base, nread);

    if (release)
        free(buf->base);
}

static void uv_write_callback(uv_write_t* req, int status)
{
    euv_wreq_t* wreq = (euv_wreq_t*)req;

    if (wreq->write_cb)
        wreq->write_cb((euv_pty_t*)wreq->req.data, wreq->buffer, status);

    free(wreq);
}

static int euv_pty_read_start_(euv_pty_t* handle, uint16_t read_size, euv_read_cb read_cb, euv_alloc_cb alloc_cb)
{
    euv_read_t* reader;
    int ret;

    if (uv_is_active((uv_handle_t*)&handle->uv_tty))
        return 0;

    reader = malloc(sizeof(euv_read_t));
    if (reader == NULL)
        return -ENOMEM;

    reader->read_cb = read_cb;
    reader->alloc_cb = alloc_cb;
    reader->read_size = read_size;
    handle->uv_tty.data = reader;

    ret = uv_read_start((uv_stream_t*)&handle->uv_tty, uv_alloc_callback, uv_read_callback);
    if (ret != 0) {
        handle->uv_tty.data = NULL;
        free(reader);
    }

    return ret;
}

int euv_pty_read_start(euv_pty_t* handle, uint16_t read_size, euv_read_cb cb)
{
    return euv_pty_read_start_(handle, read_size, cb, NULL);
}

int euv_pty_read_start2(euv_pty_t* handle, uint16_t read_size, euv_read_cb read_cb, euv_alloc_cb alloc_cb)
{
    return euv_pty_read_start_(handle, read_size, read_cb, alloc_cb);
}

int euv_pty_read_stop(euv_pty_t* handle)
{
    if (handle == NULL)
        return -EINVAL;

    free(handle->uv_tty.data);
    handle->uv_tty.data = NULL;

    if (uv_is_active((uv_handle_t*)&handle->uv_tty))
        return uv_read_stop((uv_stream_t*)&handle->uv_tty);

    return 0;
}

int euv_pty_write(euv_pty_t* handle, uint8_t* buffer, int length, euv_write_cb cb)
{
    uv_buf_t buf;
    euv_wreq_t* wreq;
    int ret;

    if (handle == NULL)
        return -EINVAL;

    wreq = (euv_wreq_t*)malloc(sizeof(euv_wreq_t));
    if (!wreq)
        return -ENOMEM;

    wreq->req.data = (void*)handle;
    wreq->buffer = buffer;
    wreq->write_cb = cb;
    buf = uv_buf_init((char*)buffer, length);

    ret = uv_write(&wreq->req, (uv_stream_t*)&handle->uv_tty, &buf, 1, uv_write_callback);
    if (ret != 0)
        free(wreq);

    return ret;
}

euv_pty_t* euv_pty_init(uv_loop_t* loop, int fd, uv_tty_mode_t mode)
{
    euv_pty_t* handle;
    int ret;

    if (loop == NULL || fd < 0)
        return NULL;

    handle = (euv_pty_t*)malloc(sizeof(euv_pty_t));
    if (!handle)
        return NULL;

    memset(handle, 0, sizeof(euv_pty_t));
    ret = uv_tty_init(loop, &handle->uv_tty, fd, 1);
    if (ret != 0) {
        free(handle);
        return NULL;
    }
    // set mode
    ret = uv_tty_set_mode(&handle->uv_tty, mode);

    return handle;
}

void euv_pty_close(euv_pty_t* hdl)
{
    if (hdl == NULL)
        return;

    uv_close((uv_handle_t*)&hdl->uv_tty, uv_close_callback);
}
