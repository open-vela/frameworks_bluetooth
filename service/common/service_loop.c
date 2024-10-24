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

#define LOG_TAG "service_loop"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef CONFIG_NET_SOCKOPTS
#include <sys/socket.h>
#endif

#include "bt_debug.h"
#include "bt_list.h"
#include "service_loop.h"
#include "utils/log.h"

BT_DEBUG_MKTIMEVAL_S(service_message_callback);

typedef struct {
    struct list_node node;
    union {
        service_func_t func;
        service_init_t init;
        service_func_t cleanup;
    };
    void* msg;
} internel_msg_t;

typedef struct {
    service_func_t func;
    void* data;
    uv_sem_t signal;
} signal_msg_t;

static void set_stop(void* data);

static void set_ready(void* data)
{
    service_loop_t* loop = data;
    struct list_node* node;
    struct list_node* tmp;
    internel_msg_t* imsg;
    int ret;

    loop->is_running = 1;
    uv_sem_init(&loop->exited, 0);

    list_for_every_safe(&loop->init_queue, node, tmp)
    {
        imsg = (internel_msg_t*)node;
        ret = imsg->init(NULL);
        list_delete(node);
        free(imsg);
        if (ret != 0) {
            BT_LOGE("%s init process fail: %d", __func__, ret);
            set_stop(data);
            return;
        }
    }

    if (uv_thread_self() == loop->thread)
        uv_sem_post(&loop->ready);

    BT_LOGD("set_ready");
}

static void set_stop(void* data)
{
    service_loop_t* loop = data;

    loop->is_running = 0;
    uv_close((uv_handle_t*)&loop->async, NULL);
    uv_stop(loop->handle);
    BT_LOGD("set_stopped");
}

static void service_sync_callback(void* data)
{
    signal_msg_t* msg = (signal_msg_t*)data;

    msg->func(msg->data);
    uv_sem_post(&msg->signal);
}

static void service_message_callback(uv_async_t* handle)
{
    uv_loop_t* uvloop = handle->loop;
    service_loop_t* loop = uvloop->data;
    internel_msg_t* imsg;

    BT_DEBUG_ENTER_TIME_SECTION(service_message_callback);

    for (;;) {
        uv_mutex_lock(&loop->msg_lock);
        imsg = (internel_msg_t*)list_remove_head(&loop->msg_queue);
        uv_mutex_unlock(&loop->msg_lock);
        if (!imsg) {
            BT_DEBUG_EXIT_TIME_SECTION(service_message_callback);
            return;
        }

        imsg->func(imsg->msg);
        free(imsg);
    }
}

static void service_schedule_loop(void* data)
{
    service_loop_t* loop = data;

    int ret = uv_async_init(loop->handle, &loop->async, service_message_callback);
    if (ret != 0) {
        BT_LOGE("%s async error: %d", __func__, ret);
        return;
    }

    BT_LOGD("%s:%p, async:%p", __func__, loop->handle, &loop->async);
    do_in_service_loop(set_ready, loop);
    uv_run(loop->handle, UV_RUN_DEFAULT);
    loop->is_running = 0;
    (void)uv_loop_close(loop->handle);

    BT_LOGD("%s %s quit", loop->name, __func__);
    uv_sem_post(&loop->exited);
}

static void service_timer_cb(uv_timer_t* handle)
{
    service_timer_t* timer = (service_timer_t*)handle;

    if (timer->callback)
        timer->callback(timer, timer->userdata);
}

static int uv_events(int events)
{
    int pevents = 0;

    if (events & POLL_READABLE)
        pevents |= UV_READABLE;
    if (events & POLL_WRITABLE)
        pevents |= UV_WRITABLE;
    if (events & POLL_DISCONNECT)
        pevents |= UV_DISCONNECT;

    return pevents;
}

static void service_poll_cb(uv_poll_t* handle, int status, int events)
{
    service_poll_t* poll = (service_poll_t*)handle;
    int revents = 0;

    if (poll->callback) {
        if (status != 0)
            revents |= POLL_ERROR;
        if (events & UV_READABLE)
            revents |= POLL_READABLE;
        if (events & UV_WRITABLE)
            revents |= POLL_WRITABLE;
        if (events & UV_DISCONNECT)
            revents |= POLL_DISCONNECT;
        poll->callback(poll, revents, poll->userdata);
    }
}

static void handle_close_cb(uv_handle_t* handle)
{
    if (handle->data)
        free(handle->data);
}

int service_loop_init(void)
{
    uv_loop_t* uvloop;
    service_loop_t* loop;
    int ret;

    uvloop = get_service_uv_loop();
    if (uvloop->data != NULL)
        return -1;

    loop = calloc(1, sizeof(service_loop_t));
    if (loop == NULL) {
        ret = -ENOMEM;
        goto fail;
    }

    loop->handle = uvloop;
    loop->handle->data = loop;
    loop->is_running = 0;
    ret = uv_mutex_init(&loop->msg_lock);
    if (ret != 0) {
        BT_LOGE("%s mutex error: %d", __func__, ret);
        goto fail;
    }

    list_initialize(&loop->msg_queue);
    list_initialize(&loop->init_queue);

    return 0;

fail:
    (void)uv_loop_close(uvloop);
    free(loop);
    return ret;
}

int service_loop_run(bool start_thread, char* name)
{
    uv_loop_t* handle = get_service_uv_loop();
    service_loop_t* loop = handle->data;

    if (start_thread) {
        int ret = uv_sem_init(&loop->ready, 0);
        if (ret != 0) {
            BT_LOGE("%s sem init error: %d", __func__, ret);
            return ret;
        }

        uv_thread_options_t options = {
            UV_THREAD_HAS_STACK_SIZE | UV_THREAD_HAS_PRIORITY,
            CONFIG_BLUETOOTH_SERVICE_LOOP_THREAD_STACK_SIZE,
            CONFIG_BLUETOOTH_SERVICE_LOOP_THREAD_PRIORITY
        };
        ret = uv_thread_create_ex(&loop->thread, &options, service_schedule_loop, loop);
        if (ret != 0) {
            BT_LOGE("service loop thread create :%d", ret);
            return ret;
        }

        if (name != NULL && strlen(name) > 0)
            snprintf(loop->name, sizeof(loop->name), "%s_%d", name, getpid());
        else
            snprintf(loop->name, sizeof(loop->name), "loop_%d", getpid());
        pthread_setname_np(loop->thread, loop->name);
        uv_sem_wait(&loop->ready);
        uv_sem_destroy(&loop->ready);
        BT_LOGD("%s loop running now !!!", loop->name);
    } else {
        pthread_setschedprio(loop->thread, CONFIG_BLUETOOTH_SERVICE_LOOP_THREAD_PRIORITY);

        BT_LOGD("service loop running now !!!");
        service_schedule_loop(loop);
    }

    return 0;
}

void service_loop_exit(void)
{
    uv_loop_t* handle = get_service_uv_loop();
    service_loop_t* loop = handle->data;
    struct list_node* node;
    struct list_node* tmp;

    if (loop == NULL) {
        (void)uv_loop_close(handle);
        return;
    }

    if (loop->is_running) {
        do_in_service_loop(set_stop, loop);
        uv_sem_wait(&loop->exited);
        uv_sem_destroy(&loop->exited);
    } else {
        uv_run(handle, UV_RUN_ONCE);
        (void)uv_loop_close(handle);
    }

    uv_mutex_lock(&loop->msg_lock);
    list_for_every_safe(&loop->init_queue, node, tmp)
    {
        list_delete(node);
        free(node);
    }

    list_for_every_safe(&loop->msg_queue, node, tmp)
    {
        list_delete(node);
        free(node);
    }
    list_delete(&loop->msg_queue);
    uv_mutex_unlock(&loop->msg_lock);
    uv_mutex_destroy(&loop->msg_lock);
    free(loop);
}

service_poll_t* service_loop_poll_fd(int fd, int pevents, service_poll_cb_t cb, void* userdata)
{
    uv_loop_t* handle = get_service_uv_loop();
    service_loop_t* loop = handle->data;

    assert(fd);
    assert(cb);

    service_poll_t* poll = (service_poll_t*)malloc(sizeof(service_poll_t));
    if (!poll)
        return NULL;

    poll->callback = cb;
    poll->userdata = userdata;
    poll->handle.data = poll;
    int ret = uv_poll_init(loop->handle, &poll->handle, fd);
    if (ret != 0)
        goto error;

    ret = uv_poll_start(&poll->handle, uv_events(pevents), service_poll_cb);
    if (ret != 0)
        goto error;

    return poll;

error:
    BT_LOGE("%s failed: %d", __func__, ret);
    free(poll);
    return NULL;
}

int service_loop_reset_poll(service_poll_t* poll, int pevents)
{
    assert(poll);

    uv_poll_stop(&poll->handle);

    return uv_poll_start(&poll->handle, uv_events(pevents), service_poll_cb);
}

void service_loop_remove_poll(service_poll_t* poll)
{
    if (!poll)
        return;

    uv_poll_stop(&poll->handle);
    uv_close((uv_handle_t*)&poll->handle, handle_close_cb);
}

service_timer_t* service_loop_timer(uint64_t timeout, uint64_t repeat, service_timer_cb_t cb, void* userdata)
{
    uv_loop_t* handle = get_service_uv_loop();
    service_loop_t* loop = handle->data;

    if (!cb)
        return NULL;

    service_timer_t* timer = malloc(sizeof(service_timer_t));
    if (!timer)
        return NULL;

    uv_timer_init(loop->handle, &timer->handle);
    timer->callback = cb;
    timer->userdata = userdata;
    timer->handle.data = timer;
    uv_timer_start(&timer->handle, service_timer_cb, timeout, repeat);

    return timer;
}

service_timer_t* service_loop_timer_no_repeating(uint64_t timeout, service_timer_cb_t cb, void* userdata)
{
    return service_loop_timer(timeout, 0, cb, userdata);
}

void service_loop_cancel_timer(service_timer_t* timer)
{
    if (!timer)
        return;

    uv_timer_stop(&timer->handle);
    uv_close((uv_handle_t*)&timer->handle, handle_close_cb);
}

static void service_work_cb(uv_work_t* req)
{
    service_work_t* work = req->data;
    assert(work);

    work->work_cb(work, work->userdata);
}

static void service_after_work_cb(uv_work_t* req, int status)
{
    service_work_t* work = req->data;
    assert(status == 0);
    assert(work);

    if (work->after_work_cb)
        work->after_work_cb(work, work->userdata);
    free(work);
}

service_work_t* service_loop_work(void* user_data, service_work_cb_t work_cb,
    service_after_work_cb_t after_work_cb)
{
    uv_loop_t* handle = get_service_uv_loop();

    service_work_t* work = zalloc(sizeof(*work));
    if (work == NULL)
        return work;

    work->userdata = user_data;
    work->work_cb = work_cb;
    work->after_work_cb = after_work_cb;
    work->work.data = work;

    if (uv_queue_work(handle, &work->work, service_work_cb, service_after_work_cb) != 0) {
        free(work);
        return NULL;
    }

    return work;
}

void add_init_process(service_init_t func)
{
    uv_loop_t* handle = get_service_uv_loop();
    service_loop_t* loop = handle->data;

    internel_msg_t* msg = (internel_msg_t*)malloc(sizeof(internel_msg_t));

    msg->init = func;
    uv_mutex_lock(&loop->msg_lock);
    list_add_tail(&loop->init_queue, &msg->node);
    uv_mutex_unlock(&loop->msg_lock);
}

void do_in_service_loop(service_func_t func, void* data)
{
    uv_loop_t* handle = get_service_uv_loop();
    service_loop_t* loop = handle->data;

    internel_msg_t* msg = (internel_msg_t*)malloc(sizeof(internel_msg_t));
    assert(msg);

    msg->func = func;
    msg->msg = data;

    uv_mutex_lock(&loop->msg_lock);
    list_add_tail(&loop->msg_queue, &msg->node);
    uv_mutex_unlock(&loop->msg_lock);

    uv_async_send(&loop->async);
}

void do_in_service_loop_sync(service_func_t func, void* data)
{
    signal_msg_t msg;

    msg.func = func;
    msg.data = data;
    uv_sem_init(&msg.signal, 0);
    do_in_service_loop(service_sync_callback, &msg);
    uv_sem_wait(&msg.signal);
    uv_sem_destroy(&msg.signal);
}

uv_loop_t* get_service_uv_loop(void)
{
    return uv_default_loop();
}
