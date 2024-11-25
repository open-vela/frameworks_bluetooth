/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
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
#define LOG_TAG "audio_transport"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "bt_utils.h"
#include "utils/log.h"

#include "audio_transport.h"

typedef struct {
    void* ipc_handle;
    uint8_t ch_id;
    uint8_t closing;
    uv_pipe_t* svr_pipe;
    uv_pipe_t* cli_pipe;
    transport_conn_state_t state;
    transport_event_cb_t event_cb;
} transport_channel_t;

typedef struct {
    uv_write_t req;
    uint8_t* buffer;
    transport_channel_t* ch;
    transport_write_cb_t write_cb;
} transport_write_t;

typedef struct {
    uint16_t read_size;
    transport_channel_t* ch;
    transport_alloc_cb_t alloc_cb;
    transport_read_cb_t read_cb;
} transport_read_t;

typedef struct _audio_transport {
    uv_loop_t* loop;
    uint8_t closing;
    transport_channel_t ch[AUDIO_TRANS_CH_NUM];
} audio_transport_t;

const char* audio_transport_dump_event(uint8_t event)
{
    switch (event) {
        CASE_RETURN_STR(TRANSPORT_OPEN_EVT)
        CASE_RETURN_STR(TRANSPORT_CLOSE_EVT)
        CASE_RETURN_STR(TRANSPORT_RX_DATA_EVT)
        CASE_RETURN_STR(TRANSPORT_RX_DATA_READY_EVT)
        CASE_RETURN_STR(TRANSPORT_TX_DATA_READY_EVT)
    default:
        return "UNKNOWN MSG ID";
    }
}

static void transport_connection_close_cb(uv_handle_t* handle)
{
    transport_channel_t* ch = handle->data;

    free(handle);
    if (ch->state == IPC_CONNTECTED) {
        ch->state = IPC_DISCONNTECTED;
        if (ch->event_cb)
            ch->event_cb(ch->ch_id, TRANSPORT_CLOSE_EVT);

        if (!ch->closing && ch->ipc_handle) {
            audio_transport_t* transport = ch->ipc_handle;
            if (!transport->closing)
                return;

            for (int i = 0; i < AUDIO_TRANS_CH_NUM; i++) {
                ch = &transport->ch[i];
                if (ch->closing || ch->state != IPC_DISCONNTECTED)
                    return;
            }

            BT_LOGI("%s transport freed:0x%p", __func__, transport);
            free(transport);
        }
    }
}

static void audio_transport_connection_close(transport_channel_t* ch)
{
    if (ch->cli_pipe) {
        /* check client is reading before disconnect */
        if (ch->state == IPC_CONNTECTED && ch->cli_pipe->data)
            audio_transport_read_stop(ch->ipc_handle, ch->ch_id);

        ch->cli_pipe->data = ch;
        uv_close((uv_handle_t*)ch->cli_pipe, transport_connection_close_cb);
        ch->cli_pipe = NULL;
    }
}

static void transport_chnl_close_cb(uv_handle_t* handle)
{
    transport_channel_t* ch = handle->data;
    audio_transport_t* transport = NULL;

    free(handle);
    ch->closing = 0;
    if (ch->ipc_handle && ch->state == IPC_DISCONNTECTED) {
        transport = ch->ipc_handle;
        if (!transport->closing)
            return;

        for (int i = 0; i < AUDIO_TRANS_CH_NUM; i++) {
            ch = &transport->ch[i];
            if (ch->closing || ch->state != IPC_DISCONNTECTED)
                return;
        }

        BT_LOGI("%s transport freed:0x%p", __func__, transport);
        free(transport);
    }
}

static void audio_transport_channel_close(transport_channel_t* ch)
{
    audio_transport_connection_close(ch);

    if (ch->svr_pipe) {
        ch->closing = 1;
        uv_close((uv_handle_t*)ch->svr_pipe, transport_chnl_close_cb);
        ch->svr_pipe = NULL;
    }
}

static void transport_chnl_listen_cb(uv_stream_t* stream, int status)
{
    transport_channel_t* ch = stream->data;
    int ret;

    if (status != 0) {
        BT_LOGE("%s, status = %d", __func__, status);
        return;
    }

    ch->cli_pipe = malloc(sizeof(uv_pipe_t));
    ret = uv_pipe_init(stream->loop, ch->cli_pipe, 0);
    if (ret != 0) {
        free(ch->cli_pipe);
        ch->cli_pipe = NULL;
        BT_LOGE("client pipe init error %s", uv_strerror(ret));
        return;
    }

    ret = uv_accept(stream, (uv_stream_t*)ch->cli_pipe);
    if (ret != 0) {
        BT_LOGE("accept error %s", uv_strerror(ret));
        audio_transport_connection_close(ch);
        return;
    }

    ch->cli_pipe->data = NULL;
    ch->state = IPC_CONNTECTED;
    if (ch->event_cb)
        ch->event_cb(ch->ch_id, TRANSPORT_OPEN_EVT);
}

static void transport_chnl_read_alloc_cb(uv_handle_t* handle, size_t suggested_size,
    uv_buf_t* buf)
{
    transport_read_t* rreq = (transport_read_t*)handle->data;
    (void)suggested_size;

    rreq->alloc_cb(rreq->ch->ch_id, (uint8_t**)&buf->base, &buf->len);
    // buf->base = malloc(rreq->read_size);
    // buf->len = rreq->read_size;
}

static void transport_chnl_write_cb(uv_write_t* req, int status)
{
    transport_write_t* wreq = (transport_write_t*)req->data;
    transport_channel_t* ch = wreq->ch;
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
        audio_transport_connection_close(ch);
}

static void transport_chnl_read_cb(uv_stream_t* stream, ssize_t nread,
    const uv_buf_t* buf)
{
    transport_read_t* rreq = (transport_read_t*)stream->data;
    transport_channel_t* ch = rreq->ch;
    uint8_t need_close = 0;

    if (nread < 0) {
        need_close = 1;
        BT_LOGE("%s nread:%" PRIuPTR, __func__, nread);
    }

    if (rreq->read_cb)
        rreq->read_cb(ch->ch_id, (uint8_t*)buf->base, nread);

    if (need_close)
        audio_transport_connection_close(ch);
}

audio_transport_t* audio_transport_init(uv_loop_t* loop)
{
    audio_transport_t* transport;

    if (!loop)
        return NULL;

    transport = (audio_transport_t*)zalloc(sizeof(audio_transport_t));
    if (!transport) {
        BT_LOGE("%s malloc failed", __func__);
        return NULL;
    }

    transport->loop = loop;
    for (uint8_t i = 0; i < AUDIO_TRANS_CH_NUM; i++)
        transport->ch[i].state = IPC_DISCONNTECTED;

    return transport;
}

bool audio_transport_open(audio_transport_t* transport, uint8_t ch_id,
    const char* path, transport_event_cb_t cb)
{
    transport_channel_t* ch;
    uv_fs_t fs;
    int ret;

    if (ch_id >= AUDIO_TRANS_CH_NUM || !transport)
        return false;

    ch = &transport->ch[ch_id];

    if (ch->state == IPC_CONNTECTED)
        return true;

    ch->cli_pipe = NULL;
    ch->svr_pipe = malloc(sizeof(uv_pipe_t));
    ret = uv_pipe_init(transport->loop, ch->svr_pipe, 0);
    if (ret != 0) {
        free(ch->svr_pipe);
        ch->svr_pipe = NULL;
        BT_LOGE("server pipe init error %s", uv_strerror(ret));
        return false;
    }

    ch->svr_pipe->data = ch;
    ret = uv_fs_unlink(transport->loop, &fs, path, NULL);
    if (ret != 0 && ret != UV_ENOENT) {
        BT_LOGE("unlink error: %s", uv_strerror(ret));
        goto error;
    }

#ifndef CONFIG_BLUETOOTH_AUDIO_TRANS_RPSMG_SERVER
    ret = uv_pipe_bind(ch->svr_pipe, path);
#else
    ret = uv_pipe_rpmsg_bind(ch->svr_pipe, path, "");
#endif
    if (ret != 0) {
        BT_LOGE("bind error: %s", uv_strerror(ret));
        goto error;
    }

    ret = uv_listen((uv_stream_t*)ch->svr_pipe, 128, transport_chnl_listen_cb);
    if (ret != 0) {
        BT_LOGE("listen error: %s", uv_strerror(ret));
        goto error;
    }
    ch->ch_id = ch_id;
    ch->event_cb = cb;
    ch->ipc_handle = (void*)transport;

    BT_LOGD("%s path{%d}[%s] success", __func__, ch_id, path);

    return true;
error:
    audio_transport_channel_close(ch);
    return false;
}

void audio_transport_close(audio_transport_t* transport, uint8_t ch_id)
{
    transport_channel_t* ch;

    if (!transport)
        return;

    if (ch_id != AUDIO_TRANS_CH_ID_ALL) {
        ch = &transport->ch[ch_id];
        audio_transport_channel_close(ch);
        return;
    }

    for (int i = 0; i < AUDIO_TRANS_CH_NUM; i++) {
        ch = &transport->ch[i];
        audio_transport_channel_close(ch);
        if (ch->closing || ch->state != IPC_DISCONNTECTED)
            transport->closing = 1;
    }

    if (!transport->closing)
        free(transport);
}

int audio_transport_write(audio_transport_t* transport, uint8_t ch_id,
    const uint8_t* data, uint16_t len,
    transport_write_cb_t cb)
{
    transport_write_t* wreq;
    transport_channel_t* ch;
    uv_buf_t uv_buf;
    int ret;

    if (ch_id >= AUDIO_TRANS_CH_NUM || !transport)
        return -EINVAL;

    ch = &transport->ch[ch_id];
    if (ch->state != IPC_CONNTECTED || !ch->cli_pipe) {
        return -1;
    }
    wreq = (transport_write_t*)malloc(sizeof(transport_write_t));
    if (!wreq) {
        BT_LOGE("write req alloc failed");
        return -ENOMEM;
    }
    uint8_t* tmpbuf = (uint8_t*)malloc(len);
    if (!tmpbuf) {
        free(wreq);
        return -ENOMEM;
    }

    memcpy(tmpbuf, data, len);
    wreq->write_cb = cb;
    wreq->ch = ch;
    wreq->buffer = tmpbuf;
    wreq->req.data = (void*)wreq;

    uv_buf = uv_buf_init((char*)tmpbuf, len);
    ret = uv_write(&wreq->req, (uv_stream_t*)ch->cli_pipe,
        &uv_buf, 1,
        transport_chnl_write_cb);
    if (ret != 0) {
        BT_LOGE("write error: %s", uv_strerror(ret));
        free(wreq);
        free(tmpbuf);
        audio_transport_connection_close(ch);
        return ret;
    }

    return 0;
}

int audio_transport_read_start(audio_transport_t* transport,
    uint8_t ch_id,
    transport_alloc_cb_t alloc_cb,
    transport_read_cb_t read_cb)
{
    transport_channel_t* ch;
    transport_read_t* rreq;
    int ret;

    if (ch_id >= AUDIO_TRANS_CH_NUM || !transport)
        return -EINVAL;

    ch = &transport->ch[ch_id];
    if (ch->state != IPC_CONNTECTED || !ch->cli_pipe) {
        return -1;
    }
    rreq = (transport_read_t*)malloc(sizeof(transport_read_t));
    if (!rreq) {
        BT_LOGE("read req alloc failed");
        return -ENOMEM;
    }

    // rreq->read_size = read_size;
    rreq->read_cb = read_cb;
    rreq->alloc_cb = alloc_cb;
    rreq->ch = ch;
    ch->cli_pipe->data = rreq;
    ret = uv_read_start((uv_stream_t*)ch->cli_pipe,
        transport_chnl_read_alloc_cb,
        transport_chnl_read_cb);
    if (ret != 0 && ret != UV_EALREADY) {
        BT_LOGE("read start error :%s", uv_strerror(ret));
        free(rreq);
        audio_transport_connection_close(ch);
        return ret;
    }

    return 0;
}

int audio_transport_read_stop(audio_transport_t* transport, uint8_t ch_id)
{
    transport_channel_t* ch;
    int ret;

    if (ch_id >= AUDIO_TRANS_CH_NUM || !transport)
        return -EINVAL;

    ch = &transport->ch[ch_id];
    if (ch->state != IPC_CONNTECTED) {
        return -1;
    }

    ret = uv_read_stop((uv_stream_t*)ch->cli_pipe);

    // free read request
    free(ch->cli_pipe->data);
    ch->cli_pipe->data = NULL;
    if (ret != 0) {
        BT_LOGE("read stop error :%s", uv_strerror(ret));
        return ret;
    }

    return 0;
}

transport_conn_state_t audio_transport_get_state(audio_transport_t* transport, uint8_t ch_id)
{
    if (!transport) {
        return IPC_DISCONNTECTED;
    }

    return transport->ch[ch_id].state;
}