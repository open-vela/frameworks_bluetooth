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
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_config.h"
#include "bt_debug.h"
#include "euv_pipe.h"
#include "uv.h"

#ifndef CONFIG_EUV_PIPE_MAX_CONNEXTIONS
#define CONFIG_EUV_PIPE_MAX_CONNEXTIONS 4
#endif

typedef struct {
    uv_write_t req;
    uint8_t* buffer;
    euv_write_cb write_cb;
} euv_write_t;

typedef struct {
    euv_read_cb read_cb;
    euv_alloc_cb alloc_cb;
    uint16_t read_size;
} euv_read_t;

typedef struct {
    uv_connect_t req;
    euv_connect_cb connect_cb;
    void* data;
} euv_connect_t;

static void euv_pipe_listen_callback(uv_stream_t* stream, int status)
{
    euv_pipe_t* handle;
    euv_connect_t* creq;
    int err;

    handle = stream->data;
    creq = handle->data;

    err = uv_pipe_init(stream->loop, &handle->cli_pipe, 0);
    if (err != 0) {
        BT_LOGE("%s, srv_pipe init failed: %s", __func__, uv_strerror(err));
        return;
    }

    err = uv_accept(stream, (uv_stream_t*)&handle->cli_pipe);
    if (err != 0) {
        BT_LOGE("%s, srv_pipe accept failed: %s", __func__, uv_strerror(err));
        return;
    }

    if (creq->connect_cb) {
        creq->connect_cb(handle, status, creq->data);
    }
}

static void euv_local_listen_callback(uv_stream_t* stream, int status)
{
    euv_pipe_t* handle;

    if (status < 0) {
        BT_LOGE("%s,uv listen error: %s", __func__, uv_strerror(status));
        return;
    }

    handle = stream->data;
    handle->mode = EUV_PIPE_TYPE_SERVER_LOCAL;

    euv_pipe_listen_callback(stream, status);
}

#ifdef CONFIG_NET_RPMSG
static void euv_rpmsg_listen_callback(uv_stream_t* stream, int status)
{
    euv_pipe_t* handle;

    if (status < 0) {
        BT_LOGE("%s,uv listen error: %s", __func__, uv_strerror(status));
        return;
    }

    handle = stream->data;
    handle->mode = EUV_PIPE_TYPE_SERVER_RPMSG;

    euv_pipe_listen_callback(stream, status);
}
#endif

static void euv_close_callback(uv_handle_t* hdl)
{
    euv_pipe_t* handle = hdl->data;

    if (!handle) {
        BT_LOGE("%s, handle null", __func__);
        return;
    }

    free(handle->data);
    handle->data = NULL;
}

static void euv_alloc_callback(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    euv_read_t* reader;

    if (!handle->data) {
        BT_LOGE("%s, handle data null", __func__);
        return;
    }

    reader = (euv_read_t*)handle->data;

    if (reader->alloc_cb)
        reader->alloc_cb((euv_pipe_t*)handle, (uint8_t**)&buf->base, &buf->len);
    else {
        buf->base = malloc(reader->read_size);
        buf->len = reader->read_size;
    }
}

static void euv_read_callback(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    euv_read_t* reader;
    bool release;

    if (!stream->data) {
        BT_LOGE("%s, stream data null", __func__);
        return;
    }

    reader = (euv_read_t*)stream->data;
    release = !reader->alloc_cb;

    if (reader->read_cb)
        reader->read_cb((euv_pipe_t*)stream, (const uint8_t*)buf->base, nread);

    if (release) {
        free(buf->base);
    }
}

static void euv_write_callback(uv_write_t* req, int status)
{
    euv_write_t* wreq = (euv_write_t*)req;

    if (wreq->write_cb)
        wreq->write_cb((euv_pipe_t*)wreq->req.data, wreq->buffer, status);

    free(wreq);
}

int euv_pipe_read_start(euv_pipe_t* handle, uint16_t read_size, euv_read_cb read_cb, euv_alloc_cb alloc_cb)
{
    euv_read_t* reader;
    int ret;

    if (uv_is_active((uv_handle_t*)&handle->cli_pipe)) {
        BT_LOGE("%s, client is active", __func__);
        return 0;
    }

    reader = malloc(sizeof(euv_read_t));
    if (!reader) {
        BT_LOGE("%s, reader malloc fail", __func__);
        return -ENOMEM;
    }

    reader->read_cb = read_cb;
    reader->alloc_cb = alloc_cb;
    reader->read_size = read_size;
    handle->cli_pipe.data = reader;

    ret = uv_read_start((uv_stream_t*)&handle->cli_pipe, euv_alloc_callback, euv_read_callback);
    if (ret != 0) {
        BT_LOGE("%s, read start err:%d", __func__, ret);
        handle->cli_pipe.data = NULL;
        free(reader);
    }

    return ret;
}

int euv_pipe_read_stop(euv_pipe_t* handle)
{
    if (!handle) {
        BT_LOGE("%s, handle null", __func__);
        return -EINVAL;
    }

    if (!uv_is_active((uv_handle_t*)&handle->cli_pipe)) {
        BT_LOGW("%s, cli_pipe is inactive", __func__);
        return 0;
    }

    if (uv_is_closing((uv_handle_t*)&handle->cli_pipe)) {
        BT_LOGE("%s, uv_is_closing", __func__);
        return 0;
    }

    free(handle->cli_pipe.data);
    handle->cli_pipe.data = NULL;

    return uv_read_stop((uv_stream_t*)&handle->cli_pipe);
}

int euv_pipe_write(euv_pipe_t* handle, uint8_t* buffer, int length, euv_write_cb cb)
{
    uv_buf_t buf;
    euv_write_t* wreq;
    int ret;

    if (!handle) {
        BT_LOGE("%s, handle null", __func__);
        return -EINVAL;
    }

    wreq = (euv_write_t*)malloc(sizeof(euv_write_t));
    if (!wreq)
        return -ENOMEM;

    wreq->req.data = (void*)handle;
    wreq->buffer = buffer;
    wreq->write_cb = cb;
    buf = uv_buf_init((char*)buffer, length);

    ret = uv_write(&wreq->req, (uv_stream_t*)&handle->cli_pipe, &buf, 1, euv_write_callback);
    if (ret != 0) {
        BT_LOGE("%s, write err:%d", __func__, ret);
        free(wreq);
    }

    return ret;
}

static void euv_connect_callback(uv_connect_t* req, int status)
{
    euv_connect_t* creq = (euv_connect_t*)req;

    if (creq->connect_cb)
        creq->connect_cb(creq->req.data, status, creq->data);

    free(req);
}

euv_pipe_t* euv_pipe_connect(uv_loop_t* loop, const char* server_path, euv_connect_cb cb, void* user_data)
{
    euv_pipe_t* handle;
    euv_connect_t* creq;
    int err;

    if (!loop || !server_path) {
        BT_LOGE("%s, invalid arg", __func__);
        return NULL;
    }

    handle = (euv_pipe_t*)zalloc(sizeof(euv_pipe_t));
    if (!handle) {
        BT_LOGE("%s, zalloc fail", __func__);
        return NULL;
    }

    err = uv_pipe_init(loop, &handle->cli_pipe, 0);
    if (err != 0) {
        BT_LOGE("%s, srv_pipe init failed: %s", __func__, uv_strerror(err));
        goto err_out;
    }

    creq = zalloc(sizeof(euv_connect_t));
    if (!creq) {
        BT_LOGE("%s, zalloc failed", __func__);
        goto err_out;
    }

    creq->connect_cb = cb;
    creq->data = user_data;
    creq->req.data = handle;

    uv_pipe_connect(&creq->req, &handle->cli_pipe, server_path, euv_connect_callback);
    return handle;

err_out:
    free(handle);
    return NULL;
}

#ifdef CONFIG_NET_RPMSG
euv_pipe_t* euv_rpmsg_pipe_connect(uv_loop_t* loop, const char* server_path, const char* cpu_name, euv_connect_cb cb, void* user_data)
{
    euv_pipe_t* handle;
    euv_connect_t* creq;
    int err;

    if (!loop || !server_path) {
        BT_LOGE("%s, invalid arg", __func__);
        return NULL;
    }

    handle = (euv_pipe_t*)zalloc(sizeof(euv_pipe_t));
    if (!handle) {
        BT_LOGE("%s, zalloc fail", __func__);
        return NULL;
    }

    err = uv_pipe_init(loop, &handle->cli_pipe, 0);
    if (err != 0) {
        BT_LOGE("%s, srv_pipe init failed: %s", __func__, uv_strerror(err));
        goto err_out;
    }

    creq = zalloc(sizeof(euv_connect_t));
    if (!creq) {
        BT_LOGE("%s, zalloc failed", __func__);
        goto err_out;
    }

    creq->connect_cb = cb;
    creq->data = user_data;
    creq->req.data = handle;

    uv_pipe_rpmsg_connect(&creq->req, &handle->cli_pipe, server_path, cpu_name, euv_connect_callback);
    return handle;

err_out:
    free(handle);
    return NULL;
}
#endif

euv_pipe_t* euv_pipe_open(uv_loop_t* loop, const char* server_path, euv_connect_cb cb, void* user_data)
{
    euv_pipe_t* handle;
    euv_connect_t* creq;
    int err;
    uv_fs_t fs;

    if (!loop || !server_path) {
        BT_LOGE("%s, invalid arg", __func__);
        return NULL;
    }

    handle = (euv_pipe_t*)zalloc(sizeof(euv_pipe_t));
    if (!handle) {
        BT_LOGE("%s, zalloc handle fail", __func__);
        return NULL;
    }

    handle->mode = EUV_PIPE_TYPE_UNKNOWN;

    creq = (euv_connect_t*)zalloc(sizeof(euv_connect_t));
    if (!creq) {
        BT_LOGE("%s, zalloc creq fail", __func__);
        goto errout_with_handle;
    }

    creq->data = user_data;
    creq->connect_cb = cb;
    handle->data = creq;

    err = uv_pipe_init(loop, &handle->srv_pipe[EUV_PIPE_TYPE_SERVER_LOCAL], 0);
    if (err != 0) {
        BT_LOGE("%s, srv_pipe init failed: %s", __func__, uv_strerror(err));
        goto errout_with_creq;
    }

    err = uv_fs_unlink(loop, &fs, server_path, NULL);
    if (err != 0 && err != UV_ENOENT) {
        BT_LOGE("%s, srv_pipe unlink failed: %s", __func__, uv_strerror(err));
        goto errout_with_creq;
    }

    err = uv_pipe_bind(&handle->srv_pipe[EUV_PIPE_TYPE_SERVER_LOCAL], server_path);
    if (err != 0) {
        BT_LOGE("%s, srv_pipe bind failed: %s", __func__, uv_strerror(err));
        goto errout_with_creq;
    }

    handle->srv_pipe[EUV_PIPE_TYPE_SERVER_LOCAL].data = handle;

    err = uv_listen((uv_stream_t*)&handle->srv_pipe[EUV_PIPE_TYPE_SERVER_LOCAL], CONFIG_EUV_PIPE_MAX_CONNEXTIONS, euv_local_listen_callback);
    if (err != 0) {
        BT_LOGE("%s, srv_pipe listen failed: %s", __func__, uv_strerror(err));
        goto errout_with_creq;
    }

#ifdef CONFIG_NET_RPMSG
    /* start RPMSG server */
    err = uv_pipe_init(loop, &handle->srv_pipe[EUV_PIPE_TYPE_SERVER_RPMSG], 0);
    if (err != 0) {
        BT_LOGE("%s, rpmsg srv_pipe init failed: %s", __func__, uv_strerror(err));
        goto errout_with_creq;
    }

    err = uv_pipe_rpmsg_bind(&handle->srv_pipe[EUV_PIPE_TYPE_SERVER_RPMSG], server_path, "");
    if (err != 0) {
        BT_LOGE("%s, rpmsg srv_pipe bind failed: %s", __func__, uv_strerror(err));
        goto errout_with_creq;
    }

    handle->srv_pipe[EUV_PIPE_TYPE_SERVER_RPMSG].data = handle;
    err = uv_listen((uv_stream_t*)&handle->srv_pipe[EUV_PIPE_TYPE_SERVER_RPMSG], CONFIG_EUV_PIPE_MAX_CONNEXTIONS, euv_rpmsg_listen_callback);
    if (err != 0) {
        BT_LOGE("%s, rpmsg srv_pipe listen failed: %s", __func__, uv_strerror(err));
        goto errout_with_creq;
    }
#endif

    return handle;

errout_with_creq:
    free(creq);
errout_with_handle:
    free(handle);
    return NULL;
}

void euv_pipe_close(euv_pipe_t* handle)
{
    if (!handle) {
        BT_LOGE("%s, invalid arg", __func__);
        return;
    }

    if (handle->mode == EUV_PIPE_TYPE_UNKNOWN) {
        BT_LOGE("%s, unkown mode", __func__);
        handle->srv_pipe[EUV_PIPE_TYPE_SERVER_LOCAL].data = handle;
        uv_close((uv_handle_t*)&handle->srv_pipe[EUV_PIPE_TYPE_SERVER_LOCAL], euv_close_callback);
        handle->srv_pipe[EUV_PIPE_TYPE_SERVER_RPMSG].data = NULL;
        uv_close((uv_handle_t*)&handle->srv_pipe[EUV_PIPE_TYPE_SERVER_RPMSG], euv_close_callback);
        return;
    }

    if (uv_is_closing((uv_handle_t*)&handle->srv_pipe[handle->mode])) {
        BT_LOGE("%s, uv_is_closing", __func__);
        return;
    }

    euv_pipe_disconnect(handle);

    handle->srv_pipe[handle->mode].data = handle;
    uv_close((uv_handle_t*)&handle->srv_pipe[handle->mode], euv_close_callback);
}

void euv_pipe_disconnect(euv_pipe_t* handle)
{
    if (!handle) {
        BT_LOGE("%s, invalid arg", __func__);
        return;
    }

    if (uv_is_closing((uv_handle_t*)&handle->cli_pipe)) {
        BT_LOGE("%s, uv_is_closing", __func__);
        return;
    }

    euv_pipe_read_stop(handle);

    handle->cli_pipe.data = handle;
    uv_close((uv_handle_t*)&handle->cli_pipe, euv_close_callback);
}

#ifdef CONFIG_NET_RPMSG
void euv_pipe_close2(euv_pipe_t* handle)
{
    euv_pipe_mode_t mode;

    if (!handle) {
        BT_LOGE("%s, invalid arg", __func__);
        return;
    }

    if (handle->mode == EUV_PIPE_TYPE_SERVER_LOCAL) {
        mode = EUV_PIPE_TYPE_SERVER_RPMSG;
    } else if (handle->mode == EUV_PIPE_TYPE_SERVER_RPMSG) {
        mode = EUV_PIPE_TYPE_SERVER_LOCAL;
    } else {
        BT_LOGE("%s, invalid mode", __func__);
        return;
    }

    if (uv_is_closing((uv_handle_t*)&handle->srv_pipe[mode])) {
        BT_LOGE("%s, uv_is_closing", __func__);
        return;
    }

    handle->srv_pipe[mode].data = NULL;
    uv_close((uv_handle_t*)&handle->srv_pipe[mode], euv_close_callback);
}
#endif
