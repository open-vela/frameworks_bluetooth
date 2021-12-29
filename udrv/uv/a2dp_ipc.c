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
#define LOG_TAG "a2dp_ipc"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "a2dp_ipc.h"
#include "log.h"
#include "utils/utils.h"

typedef enum {
    IPC_DISCONNTECTED = -1,
    IPC_CONNTECTED
} ipc_conn_state_t;

typedef struct {
    void* ipc_handle;
    uint8_t ch_id;
    uv_pipe_t* svr_pipe;
    uv_pipe_t* cli_pipe;
    ipc_conn_state_t state;
    ipc_event_cb_t event_cb;
} ipc_channel_t;

typedef struct {
    uv_write_t req;
    uint8_t* buffer;
    ipc_channel_t* ch;
    ipc_write_cb_t write_cb;
} ipc_write_t;

typedef struct {
    uint16_t read_size;
    ipc_channel_t* ch;
    ipc_alloc_cb_t alloc_cb;
    ipc_read_cb_t read_cb;
} ipc_read_t;

typedef struct _a2dp_ipc {
    uv_loop_t* loop;
    ipc_channel_t ch[A2DP_IPC_CH_NUM];
} a2dp_ipc_t;

const char* dump_a2dp_ipc_event(uint8_t event)
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

static void ipc_chnl_close_cb(uv_handle_t* handle)
{
    free(handle);
}

static void a2dp_ipc_connection_close(ipc_channel_t* ch)
{
    if (ch->state == IPC_CONNTECTED) {
        ch->state = IPC_DISCONNTECTED;
        free(ch->cli_pipe->data);
        uv_close((uv_handle_t*)ch->cli_pipe, ipc_chnl_close_cb);
        if (ch->event_cb)
            ch->event_cb(ch->ch_id, IPC_CLOSE_EVT);
    }
}

static void ipc_chnl_listen_cb(uv_stream_t* stream, int status)
{
    ipc_channel_t* ch = stream->data;
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

    ret = uv_accept(stream, (uv_stream_t*)ch->cli_pipe);
    if (ret != 0) {
        BT_LOGE("accept error %s", uv_strerror(ret));
        uv_close((uv_handle_t *)ch->cli_pipe, ipc_chnl_close_cb);
        return;
    }

    ch->state = IPC_CONNTECTED;
    if (ch->event_cb)
        ch->event_cb(ch->ch_id, IPC_OPEN_EVT);
}

static void ipc_chnl_read_alloc_cb(uv_handle_t* handle, size_t suggested_size,
    uv_buf_t* buf)
{
    ipc_read_t* rreq = (ipc_read_t*)handle->data;
    (void)suggested_size;

    rreq->alloc_cb(rreq->ch->ch_id, (uint8_t **)&buf->base, &buf->len);
    //buf->base = malloc(rreq->read_size);
    //buf->len = rreq->read_size;
}

static void ipc_chnl_write_cb(uv_write_t* req, int status)
{
    ipc_write_t* wreq = (ipc_write_t*)req->data;
    ipc_channel_t* ch = wreq->ch;
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
        a2dp_ipc_connection_close(ch);
}

static void ipc_chnl_read_cb(uv_stream_t* stream, ssize_t nread,
    const uv_buf_t* buf)
{
    ipc_read_t* rreq = (ipc_read_t*)stream->data;
    ipc_channel_t* ch = rreq->ch;
    uint8_t need_close = 0;

    if (nread < 0) {
        need_close = 1;
        BT_LOGE("%s nread:%d", __func__, nread);
    }

    if (nread == 0) {
        BT_LOGW("%s, nread:%d", __func__, nread);
    }

    if (rreq->read_cb)
        rreq->read_cb(ch->ch_id, (uint8_t*)buf->base, nread);

    if (need_close)
        a2dp_ipc_connection_close(ch);
}

a2dp_ipc_t* a2dp_ipc_init(uv_loop_t* loop)
{
    a2dp_ipc_t* a2dp;

    if (!loop)
        return NULL;

    a2dp = (a2dp_ipc_t*)malloc(sizeof(a2dp_ipc_t));
    if (!a2dp) {
        BT_LOGE("%s malloc failed", __func__);
        return NULL;
    }

    a2dp->loop = loop;
    for (uint8_t i = 1; i < A2DP_IPC_CH_NUM; i++) {
        a2dp->ch[i].state = IPC_DISCONNTECTED;
        a2dp->ch[i].event_cb = NULL;
    }

    BT_LOGD("%s success", __func__);

    return a2dp;
}

bool a2dp_ipc_open(a2dp_ipc_t* a2dp, uint8_t ch_id, const char* path, ipc_event_cb_t cb)
{
    ipc_channel_t* ch;
    uv_fs_t fs;
    int ret;

    if (ch_id > A2DP_IPC_CH_NUM || !a2dp)
        return false;

    ch = &a2dp->ch[ch_id];
    ch->svr_pipe = malloc(sizeof(uv_pipe_t));
    ret = uv_pipe_init(a2dp->loop, ch->svr_pipe, 0);
    if (ret != 0) {
        free(ch->svr_pipe);
        BT_LOGE("server pipe init error %s", uv_strerror(ret));
        return false;
    }

    ret = uv_fs_unlink(a2dp->loop, &fs, path, NULL);
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

    ret = uv_listen((uv_stream_t*)ch->svr_pipe, 128, ipc_chnl_listen_cb);
    if (ret != 0) {
        BT_LOGE("listen error: %s", uv_strerror(ret));
        goto error;
    }
    ch->ch_id = ch_id;
    ch->event_cb = cb;
    ch->ipc_handle = (void*)a2dp;
    ch->svr_pipe->data = ch;

    BT_LOGD("%s path[%d]: %s success", __func__, ch_id, path);

    return true;
error:
    uv_close((uv_handle_t *)ch->svr_pipe, ipc_chnl_close_cb);
    return false;
}

void a2dp_ipc_close(a2dp_ipc_t* a2dp, uint8_t ch_id)
{
    ipc_channel_t* ch;

    if (!a2dp)
        return;

    if (ch_id != A2DP_IPC_CH_ID_ALL) {
        ch = &a2dp->ch[ch_id];
        a2dp_ipc_connection_close(ch);
        uv_close((uv_handle_t*)ch->svr_pipe, ipc_chnl_close_cb);
        return;
    }

    for (int i = 0; i < A2DP_IPC_CH_NUM; i++) {
        ch = &a2dp->ch[i];
        a2dp_ipc_connection_close(ch);
        uv_close((uv_handle_t*)ch->svr_pipe, ipc_chnl_close_cb);
    }
    free(a2dp);
}

int a2dp_ipc_write(a2dp_ipc_t* a2dp, uint8_t ch_id, const uint8_t* data, uint16_t len, ipc_write_cb_t cb)
{
    ipc_write_t* wreq;
    ipc_channel_t* ch;
    uv_buf_t uv_buf;
    int ret;

    if (ch_id > A2DP_IPC_CH_NUM || !a2dp)
        return -EINVAL;

    ch = &a2dp->ch[ch_id];
    if (ch->state != IPC_CONNTECTED) {
        return -1;
    }
    wreq = (ipc_write_t*)malloc(sizeof(ipc_write_t));
    if (!wreq) {
        BT_LOGE("write req alloc failed");
        return -ENOMEM;
    }
    uint8_t* tmpbuf = (uint8_t*)malloc(len);
    if (!tmpbuf)
        return -ENOMEM;
    memcpy(tmpbuf, data, len);

    wreq->write_cb = cb;
    wreq->ch = ch;
    wreq->buffer = tmpbuf;
    wreq->req.data = (void*)wreq;

    uv_buf = uv_buf_init((char*)tmpbuf, len);
    ret = uv_write(&wreq->req, (uv_stream_t*)ch->cli_pipe,
        &uv_buf, 1,
        ipc_chnl_write_cb);
    if (ret != 0) {
        BT_LOGE("write error: %s", uv_strerror(ret));
        a2dp_ipc_connection_close(ch);
        return ret;
    }

    return 0;
}

int a2dp_ipc_read_start(a2dp_ipc_t* a2dp, uint8_t ch_id, ipc_alloc_cb_t alloc_cb, ipc_read_cb_t read_cb)
{
    ipc_channel_t* ch;
    ipc_read_t* rreq;
    int ret;

    if (ch_id > A2DP_IPC_CH_NUM || !a2dp)
        return -EINVAL;

    ch = &a2dp->ch[ch_id];
    if (ch->state != IPC_CONNTECTED) {
        return -1;
    }
    rreq = (ipc_read_t*)malloc(sizeof(ipc_read_t));
    if (!rreq) {
        BT_LOGE("read req alloc failed");
        return -ENOMEM;
    }

    //rreq->read_size = read_size;
    rreq->read_cb = read_cb;
    rreq->alloc_cb = alloc_cb;
    rreq->ch = ch;
    ch->cli_pipe->data = rreq;
    ret = uv_read_start((uv_stream_t*)ch->cli_pipe,
        ipc_chnl_read_alloc_cb,
        ipc_chnl_read_cb);
    if (ret != 0) {
        BT_LOGE("read start error :%s", uv_strerror(ret));
        a2dp_ipc_connection_close(ch);
        return ret;
    }

    return 0;
}

int a2dp_ipc_read_stop(a2dp_ipc_t* a2dp, uint8_t ch_id)
{
    ipc_channel_t* ch;
    int ret;

    if (ch_id > A2DP_IPC_CH_NUM || !a2dp)
        return -EINVAL;

    ch = &a2dp->ch[ch_id];
    if (ch->state != IPC_CONNTECTED) {
        return -1;
    }

    ret = uv_read_stop((uv_stream_t*)ch->cli_pipe);

    //free read request
    free(ch->cli_pipe->data);
    ch->cli_pipe->data = NULL;
    if (ret != 0) {
        BT_LOGE("read stop error :%s", uv_strerror(ret));
        return ret;
    }

    return 0;
}
