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
#define LOG_TAG "lea_ipc"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "audio_ipc.h"
#include "bt_utils.h"
#include "utils/log.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void ipc_chnl_close_cb(uv_handle_t *handle)
{
    free(handle);
}

static void ipc_connection_close(ipc_channel_t *ch)
{
    if (ch->state == IPC_CONNTECTED) {
        ch->state = IPC_DISCONNTECTED;
        free(ch->cli_pipe->data);
        ch->cli_pipe->data = NULL;
        uv_close((uv_handle_t *)ch->cli_pipe, ipc_chnl_close_cb);
        if (ch->event_cb)
            ch->event_cb(ch->ch_id, IPC_CLOSE_EVT);
    }
}

static void ipc_chnl_listen_cb(uv_stream_t *stream, int status)
{
    ipc_channel_t *ch = stream->data;
    int ret;

    if (status != 0) {
        BT_LOGE("%s, status = %d", __func__, status);
        return;
    }

    ch->cli_pipe = malloc(sizeof(uv_pipe_t));
    ret = uv_pipe_init(stream->loop, ch->cli_pipe, 0);
    if (ret != 0) {
        free(ch->cli_pipe);
        BT_LOGE("client pipe init error %s", uv_strerror(ret));
        return;
    }

    ret = uv_accept(stream, (uv_stream_t *)ch->cli_pipe);
    if (ret != 0) {
        BT_LOGE("accept error %s", uv_strerror(ret));
        uv_close((uv_handle_t *)ch->cli_pipe, ipc_chnl_close_cb);
        return;
    }

    ch->cli_pipe->data = NULL;
    ch->state = IPC_CONNTECTED;
    if (ch->event_cb)
        ch->event_cb(ch->ch_id, IPC_OPEN_EVT);
}

static void ipc_chnl_read_alloc_cb(uv_handle_t *handle,
                                   size_t suggested_size, uv_buf_t *buf)
{
    ipc_read_t *rreq = (ipc_read_t *)handle->data;
    (void)suggested_size;

    rreq->alloc_cb(rreq->ch->ch_id, (uint8_t **)&buf->base, &buf->len);
}

static void ipc_chnl_write_cb(uv_write_t *req, int status)
{
    ipc_write_t *wreq = (ipc_write_t *)req->data;
    ipc_channel_t *ch = wreq->ch;
    uint8_t need_close = 0;

    if (status != 0) {
        need_close = 1;
        BT_LOGE("%s status:%d", __func__, status);
    }
    if (wreq->write_cb)
        wreq->write_cb(ch->ch_id, wreq->buffer);

    free(wreq->buffer);
    free(wreq);

    if (need_close)
        ipc_connection_close(ch);
}

static void ipc_chnl_read_cb(uv_stream_t *stream, ssize_t nread,
                             const uv_buf_t *buf)
{
    ipc_read_t *rreq = (ipc_read_t *)stream->data;
    ipc_channel_t *ch = rreq->ch;
    uint8_t need_close = 0;

    if (nread < 0) {
        need_close = 1;
        BT_LOGE("%s nread:%d", __func__, nread);
    }

    if (nread == 0) {
        BT_LOGW("%s, nread:%d", __func__, nread);
    }

    if (rreq->read_cb)
        rreq->read_cb(ch->ch_id, (uint8_t *)buf->base, nread);

    if (need_close)
        ipc_connection_close(ch);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

ipc_handle_t *ipc_init(uv_loop_t *loop)
{
    ipc_handle_t *handle;

    if (!loop)
        return NULL;

    handle = (ipc_handle_t *)malloc(sizeof(ipc_handle_t));
    if (!handle) {
        BT_LOGE("%s malloc failed", __func__);
        return NULL;
    }

    handle->loop = loop;
    for (uint8_t i = 0; i < CONFIG_MAX_IPC_NUM; i++) {
        handle->ch[i].state = IPC_DISCONNTECTED;
        handle->ch[i].event_cb = NULL;
    }

    return handle;
}

void ipc_cleanup(ipc_handle_t *handle)
{
    free(handle);
}

bool ipc_open(ipc_handle_t *handle, uint8_t ch_id,
              const char *path, ipc_event_cb_t cb)
{
    ipc_channel_t *ch;
    uv_fs_t fs;
    int ret;

    if (ch_id >= CONFIG_MAX_IPC_NUM || !handle)
        return false;

    ch = &handle->ch[ch_id];

    if (ch->state == IPC_CONNTECTED)
        return true;

    ch->svr_pipe = malloc(sizeof(uv_pipe_t));
    ret = uv_pipe_init(handle->loop, ch->svr_pipe, 0);
    if (ret != 0) {
        free(ch->svr_pipe);
        BT_LOGE("server pipe init error %s", uv_strerror(ret));
        return false;
    }

    ret = uv_fs_unlink(handle->loop, &fs, path, NULL);
    if (ret != 0 && ret != UV_ENOENT) {
        BT_LOGE("unlink error: %s", uv_strerror(ret));
        goto error;
    }

#ifndef CONFIG_BLUETOOTH_A2DP_IPC_RPSMG_SERVER
    ret = uv_pipe_bind(ch->svr_pipe, path);
#else
    ret = uv_pipe_rpmsg_bind(ch->svr_pipe, path, "");
#endif
    if (ret != 0) {
        BT_LOGE("bind error: %s", uv_strerror(ret));
        goto error;
    }

    ret = uv_listen((uv_stream_t *)ch->svr_pipe, 128, ipc_chnl_listen_cb);
    if (ret != 0) {
        BT_LOGE("listen error: %s", uv_strerror(ret));
        goto error;
    }
    ch->ch_id = ch_id;
    ch->event_cb = cb;
    ch->ipc_handle = (void *)handle;
    ch->svr_pipe->data = ch;

    BT_LOGD("%s path{%d}[%s] success", __func__, ch_id, path);

    return true;

error:
    uv_close((uv_handle_t *)ch->svr_pipe, ipc_chnl_close_cb);
    return false;
}

void ipc_close(ipc_handle_t *handle, uint8_t ch_id)
{
    ipc_channel_t *ch;

    if (!handle)
        return;

    for (int i = 0; i < CONFIG_MAX_IPC_NUM; i++) {
        ch = &handle->ch[i];
        ipc_connection_close(ch);
        uv_close((uv_handle_t *)ch->svr_pipe, ipc_chnl_close_cb);
    }
    free(handle);
}

int ipc_write(ipc_handle_t *handle, uint8_t ch_id, const uint8_t *data,
              uint16_t len, ipc_write_cb_t cb)
{
    ipc_write_t *wreq;
    ipc_channel_t *ch;
    uv_buf_t uv_buf;
    int ret;

    if (ch_id >= CONFIG_MAX_IPC_NUM || !handle)
        return -EINVAL;

    ch = &handle->ch[ch_id];
    if (ch->state != IPC_CONNTECTED) {
        return -1;
    }
    wreq = (ipc_write_t *)malloc(sizeof(ipc_write_t));
    if (!wreq) {
        BT_LOGE("write req alloc failed");
        return -ENOMEM;
    }
    uint8_t *tmpbuf = (uint8_t *)malloc(len);
    if (!tmpbuf) {
        free(wreq);
        return -ENOMEM;
    }

    memcpy(tmpbuf, data, len);
    wreq->write_cb = cb;
    wreq->ch = ch;
    wreq->buffer = tmpbuf;
    wreq->req.data = (void *)wreq;

    uv_buf = uv_buf_init((char *)tmpbuf, len);
    ret = uv_write(&wreq->req, (uv_stream_t *)ch->cli_pipe, &uv_buf, 1, ipc_chnl_write_cb);
    if (ret != 0) {
        BT_LOGE("write error: %s", uv_strerror(ret));
        free(wreq);
        free(tmpbuf);
        ipc_connection_close(ch);
        return ret;
    }

    return 0;
}

int ipc_read_start(ipc_handle_t *handle, uint8_t ch_id,
                   ipc_alloc_cb_t alloc_cb, ipc_read_cb_t read_cb)
{
    ipc_channel_t *ch;
    ipc_read_t *rreq;
    int ret;

    if (ch_id >= CONFIG_MAX_IPC_NUM || !handle)
        return -EINVAL;

    ch = &handle->ch[ch_id];
    if (ch->state != IPC_CONNTECTED) {
        return -1;
    }
    rreq = (ipc_read_t *)malloc(sizeof(ipc_read_t));
    if (!rreq) {
        BT_LOGE("read req alloc failed");
        return -ENOMEM;
    }

    // rreq->read_size = read_size;
    rreq->read_cb = read_cb;
    rreq->alloc_cb = alloc_cb;
    rreq->ch = ch;
    ch->cli_pipe->data = rreq;
    ret = uv_read_start((uv_stream_t *)ch->cli_pipe, ipc_chnl_read_alloc_cb, ipc_chnl_read_cb);
    if (ret != 0 && ret != UV_EALREADY) {
        BT_LOGE("read start error :%s", uv_strerror(ret));
        free(rreq);
        ipc_connection_close(ch);
        return ret;
    }

    return 0;
}

int ipc_read_stop(ipc_handle_t *handle, uint8_t ch_id)
{
    ipc_channel_t *ch;
    int ret;

    if (ch_id >= CONFIG_MAX_IPC_NUM || !handle)
        return -EINVAL;

    ch = &handle->ch[ch_id];
    if (ch->state != IPC_CONNTECTED) {
        return -1;
    }

    ret = uv_read_stop((uv_stream_t *)ch->cli_pipe);

    // free read request
    free(ch->cli_pipe->data);
    ch->cli_pipe->data = NULL;
    if (ret != 0) {
        BT_LOGE("read stop error :%s", uv_strerror(ret));
        return ret;
    }

    return 0;
}

const char *dump_ipc_event(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(IPC_OPEN_EVT)
        CASE_RETURN_STR(IPC_CLOSE_EVT)
        CASE_RETURN_STR(IPC_RX_DATA_EVT)
        CASE_RETURN_STR(IPC_RX_DATA_READY_EVT)
        CASE_RETURN_STR(IPC_TX_DATA_READY_EVT)
    default:
        return "UNKNOWN MSG ID";
    }
}
