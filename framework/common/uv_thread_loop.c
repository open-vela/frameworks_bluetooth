/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef __NuttX__
#define _GNU_SOURCE
#endif

#define LOG_TAG "thread_loop"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "bt_config.h"
#include "bt_list.h"
#include "uv_thread_loop.h"

typedef struct thread_loop {
    char name[64];
    uv_async_t async;
    uv_thread_t thread;
    uv_mutex_t msg_lock;
    uv_sem_t ready;
    uv_sem_t exited;
    uint8_t is_running;
    struct list_node msg_queue;
} loop_priv_t;

typedef struct {
    struct list_node node;
    union {
        thread_func_t func;
    };
    void* msg;
} internel_msg_t;

typedef struct {
    thread_func_t func;
    void* data;
    uv_sem_t signal;
} signal_msg_t;

#if defined(ANDROID)
#define LOOP_THREAD_STACK_SIZE 40960
#else
#define LOOP_THREAD_STACK_SIZE 4096
#endif

static void set_ready(void* data)
{
    loop_priv_t* priv = data;

    priv->is_running = 1;
    uv_sem_init(&priv->exited, 0);
    uv_sem_post(&priv->ready);
    syslog(LOG_DEBUG, "set_ready");
}

static void set_stop(void* data)
{
    uv_loop_t* loop = data;
    loop_priv_t* priv = loop->data;

    priv->is_running = 0;
    uv_close((uv_handle_t*)&priv->async, NULL);
    uv_stop(loop);
    syslog(LOG_DEBUG, "set_stopped");
}

static void thread_sync_callback(void* data)
{
    signal_msg_t* msg = (signal_msg_t*)data;

    msg->func(msg->data);
    uv_sem_post(&msg->signal);
}

static void thread_message_callback(uv_async_t* handle)
{
    internel_msg_t* imsg;
    loop_priv_t* priv = handle->data;

    for (;;) {
        uv_mutex_lock(&priv->msg_lock);
        imsg = (internel_msg_t*)list_remove_head(&priv->msg_queue);
        uv_mutex_unlock(&priv->msg_lock);
        if (!imsg)
            return;

        imsg->func(imsg->msg);
        free(imsg);
    }
}

static void thread_schedule_loop(void* data)
{
    uv_loop_t* loop = data;
    loop_priv_t* priv = loop->data;

    int ret = uv_async_init(loop, &priv->async, thread_message_callback);
    if (ret != 0) {
        syslog(LOG_ERR, "%s async error: %d", __func__, ret);
        return;
    }

    priv->async.data = priv;
    syslog(LOG_DEBUG, "%s:%p, async:%p", __func__, loop, &priv->async);
    do_in_thread_loop(loop, set_ready, priv);
    uv_run(loop, UV_RUN_DEFAULT);
    priv->is_running = 0;
    (void)uv_loop_close(loop);

    syslog(LOG_DEBUG, "%s %s quit", priv->name, __func__);

    uv_sem_post(&priv->exited);
}

static void handle_close_cb(uv_handle_t* handle)
{
    free(handle);
}

int thread_loop_init(uv_loop_t* loop)
{
    int ret;
    loop_priv_t* priv = malloc(sizeof(loop_priv_t));
    if (!priv)
        return -ENOMEM;

    priv->is_running = 0;
    ret = uv_mutex_init(&priv->msg_lock);
    if (ret != 0) {
        free(priv);
        syslog(LOG_ERR, "%s mutex error: %d", __func__, ret);
        return ret;
    }

    list_initialize(&priv->msg_queue);
    ret = uv_loop_init(loop);
    if (ret != 0) {
        list_delete(&priv->msg_queue);
        uv_mutex_destroy(&priv->msg_lock);
        free(priv);
        return ret;
    }

    loop->data = priv;

    return 0;
}

int thread_loop_run(uv_loop_t* loop, bool start_thread, const char* name)
{
    loop_priv_t* priv = loop->data;

    if (start_thread) {
        int ret = uv_sem_init(&priv->ready, 0);
        if (ret != 0) {
            syslog(LOG_ERR, "%s sem init error: %d", __func__, ret);
            return ret;
        }

        uv_thread_options_t options = {
            UV_THREAD_HAS_STACK_SIZE | UV_THREAD_HAS_PRIORITY,
            LOOP_THREAD_STACK_SIZE,
            CONFIG_BLUETOOTH_SERVICE_LOOP_THREAD_PRIORITY
        };
        ret = uv_thread_create_ex(&priv->thread, &options, thread_schedule_loop, (void*)loop);
        if (ret != 0) {
            syslog(LOG_ERR, "loop thread create :%d", ret);
            return ret;
        }

        if (name != NULL && strlen(name) > 0)
            snprintf(priv->name, sizeof(priv->name), "%s_%d", name, getpid());
        else
            snprintf(priv->name, sizeof(priv->name), "loop_%d", getpid());
        pthread_setname_np(priv->thread, priv->name);
        uv_sem_wait(&priv->ready);
        uv_sem_destroy(&priv->ready);
        syslog(LOG_DEBUG, "%s loop running now !!!", priv->name);
    } else {
        syslog(LOG_DEBUG, "%s loop running now !!!", name);
        thread_schedule_loop(NULL);
    }

    return 0;
}

void thread_loop_exit(uv_loop_t* loop)
{
    struct list_node* node;
    struct list_node* tmp;

    if (!loop || !loop->data)
        return;

    loop_priv_t* priv = loop->data;

    if (priv->is_running) {
        do_in_thread_loop(loop, set_stop, (void*)loop);
        uv_sem_wait(&priv->exited);
        uv_sem_destroy(&priv->exited);
    } else {
        uv_run(loop, UV_RUN_ONCE);
        (void)uv_loop_close(loop);
    }

    uv_mutex_lock(&priv->msg_lock);
    list_for_every_safe(&priv->msg_queue, node, tmp)
    {
        list_delete(node);
        free(node);
    }
    list_delete(&priv->msg_queue);
    uv_mutex_unlock(&priv->msg_lock);
    uv_mutex_destroy(&priv->msg_lock);
    free(priv);
}

uv_poll_t* thread_loop_poll_fd(uv_loop_t* loop, int fd, int pevents, uv_poll_cb cb, void* userdata)
{
    assert(fd);
    assert(cb);

    uv_poll_t* handle = (uv_poll_t*)malloc(sizeof(uv_poll_t));
    if (!handle)
        return NULL;

    handle->data = userdata;
    int ret = uv_poll_init(loop, handle, fd);
    if (ret != 0)
        goto error;

    ret = uv_poll_start(handle, pevents, cb);
    if (ret != 0)
        goto error;

    return handle;

error:
    syslog(LOG_ERR, "%s failed: %d", __func__, ret);
    free(handle);
    return NULL;
}

int thread_loop_reset_poll(uv_poll_t* poll, int pevents, uv_poll_cb cb)
{
    assert(poll);

    uv_poll_stop(poll);

    return uv_poll_start(poll, pevents, cb);
}

void thread_loop_remove_poll(uv_poll_t* poll)
{
    if (!poll)
        return;

    uv_poll_stop(poll);
    uv_close((uv_handle_t*)poll, handle_close_cb);
}

uv_timer_t* thread_loop_timer(uv_loop_t* loop, uint64_t timeout, uint64_t repeat, uv_timer_cb cb, void* userdata)
{
    if (!cb)
        return NULL;

    uv_timer_t* handle = malloc(sizeof(uv_timer_t));
    if (!handle)
        return NULL;

    uv_timer_init(loop, handle);
    handle->data = userdata;
    uv_timer_start(handle, cb, timeout, repeat);

    return handle;
}

uv_timer_t* thread_loop_timer_no_repeating(uv_loop_t* loop, uint64_t timeout, uv_timer_cb cb, void* userdata)
{
    return thread_loop_timer(loop, timeout, 0, cb, userdata);
}

void thread_loop_cancel_timer(uv_timer_t* timer)
{
    if (!timer)
        return;

    uv_timer_stop(timer);
    uv_close((uv_handle_t*)timer, handle_close_cb);
}

void do_in_thread_loop(uv_loop_t* loop, thread_func_t func, void* data)
{
    loop_priv_t* priv = loop->data;
    internel_msg_t* msg = (internel_msg_t*)malloc(sizeof(internel_msg_t));
    assert(msg);

    msg->func = func;
    msg->msg = data;

    uv_mutex_lock(&priv->msg_lock);
    list_add_tail(&priv->msg_queue, &msg->node);
    uv_mutex_unlock(&priv->msg_lock);

    uv_async_send(&priv->async);
}

void do_in_thread_loop_sync(uv_loop_t* loop, thread_func_t func, void* data)
{
    signal_msg_t msg;

    msg.func = func;
    msg.data = data;
    uv_sem_init(&msg.signal, 0);
    do_in_thread_loop(loop, thread_sync_callback, &msg);
    uv_sem_wait(&msg.signal);
    uv_sem_destroy(&msg.signal);
}
