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
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include "uv.h"

#include "euv_pty.h"

typedef struct _euv_pty {
  uv_tty_t      uv_tty;
  int           fd;
  euv_read_cb   read_cb;
}euv_pty_t;

typedef struct {
  uv_write_t    req;
  uint8_t       *buffer;
  euv_write_cb  write_cb;
}euv_wreq_t;

static void uv_close_callback(uv_handle_t* handle)
{
  free(handle);
}

static void uv_alloc_callback(uv_handle_t* handle, size_t size, uv_buf_t* buf) 
{
  buf->base = malloc(1024);
  buf->len = 1024;
}

static void uv_read_callback(uv_stream_t* stream,
                             ssize_t nread,
                             const uv_buf_t* buf)
{
  euv_pty_t *handle = (euv_pty_t *)stream;

  if (handle->read_cb)
    handle->read_cb(handle, (const uint8_t*)buf->base, nread);

  free(buf->base);
}

static void uv_write_callback(uv_write_t* req, int status)
{
  euv_wreq_t *wreq = (euv_wreq_t *)req;

  if(wreq->write_cb)
    wreq->write_cb((euv_pty_t *)wreq->req.data, wreq->buffer, status);

  free(wreq);
}

int euv_pty_read_start(euv_pty_t *handle, euv_read_cb cb)
{
  handle->read_cb = cb;

  return uv_read_start((uv_stream_t*)&handle->uv_tty, uv_alloc_callback, uv_read_callback);
}

int euv_pty_read_stop(euv_pty_t *handle)
{
  return uv_read_stop((uv_stream_t*)&handle->uv_tty);
}

int euv_pty_write(euv_pty_t *handle, uint8_t *buffer, int length, euv_write_cb cb)
{
  uv_buf_t buf;
  euv_wreq_t *wreq = (euv_wreq_t*)malloc(sizeof(euv_wreq_t));

  wreq->req.data = (void *)handle;
  wreq->buffer = buffer;
  wreq->write_cb = cb;
  buf = uv_buf_init((char *)buffer, length);

  return uv_write(&wreq->req, (uv_stream_t*)&handle->uv_tty, &buf, 1, uv_write_callback);
}

euv_pty_t *euv_pty_init(uv_loop_t* loop, int fd, uv_tty_mode_t mode)
{
  euv_pty_t *handle;
  int ret;

  handle = (euv_pty_t *)malloc(sizeof(euv_pty_t));
  if (!handle)
      return NULL;

  ret = uv_tty_init(loop, &handle->uv_tty, fd, 1);
  if (ret != 0) {
    free(handle);
    return NULL;
  }
  handle->read_cb = NULL;
  handle->fd = fd;
  //set mode
  ret = uv_tty_set_mode(&handle->uv_tty, mode);

  return handle;
}

void euv_pty_close(euv_pty_t *hdl)
{
  uv_close((uv_handle_t *)&hdl->uv_tty, uv_close_callback);
}
