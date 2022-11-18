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

#include <nuttx/list.h>
<<<<<<< HEAD
#ifndef __NuttX__
#define _GNU_SOURCE
#endif
=======
>>>>>>> bluetooth framework re-implement base
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "service_loop.h"

#define LOG_TAG "service_loop"
#include "utils/log.h"

#if 1
// #ifdef CONFIG_OBELISK_LIBUV_LOOP

<<<<<<< HEAD
=======
typedef struct service_loop {
    uv_loop_t *handle;
    uv_async_t async;
    uv_thread_t thread;
    uv_mutex_t msg_lock;
    uv_sem_t ready;
    uv_sem_t exited;
    uint8_t is_running;
    struct list_node msg_queue;
    struct list_node init_queue;
    struct list_node clean_queue;
} service_loop_t;

typedef struct service_timer {
    uv_timer_t handle;
    service_timer_cb_t callback;
    void *userdata;
} service_timer_t;

typedef struct service_poll {
    uv_poll_t handle;
    service_poll_cb_t callback;
    void *userdata;
} service_poll_t;

>>>>>>> bluetooth framework re-implement base
typedef struct {
    struct list_node node;
    union {
        service_func_t func;
<<<<<<< HEAD
        service_init_t init;
=======
        service_func_t init;
>>>>>>> bluetooth framework re-implement base
        service_func_t cleanup;
    };
    void *msg;
} internel_msg_t;

typedef struct {
    service_func_t func;
    void *data;
    uv_sem_t signal;
} signal_msg_t;

<<<<<<< HEAD
static void set_stop(void *data);

static void set_ready(void *data)
{
    service_loop_t *loop = data;
    struct list_node *node;
    struct list_node *tmp;
    internel_msg_t *imsg;
    int ret;
=======
#define SERVICE_LOOP_THREAD_STACK_SIZE 8192
static service_loop_t bt_service_loop = { 0 };

static void set_ready(void *data)
{
    service_loop_t *loop = &bt_service_loop;
    struct list_node *node;
    struct list_node *tmp;
    internel_msg_t *imsg;
>>>>>>> bluetooth framework re-implement base

    loop->is_running = 1;
    uv_sem_init(&loop->exited, 0);

    list_for_every_safe(&loop->init_queue, node, tmp)
    {
        imsg = (internel_msg_t *)node;
<<<<<<< HEAD
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

=======
        imsg->init(NULL);
        list_delete(node);
        free(imsg);
    }
    uv_sem_post(&loop->ready);
>>>>>>> bluetooth framework re-implement base
    BT_LOGD("set_ready");
}

static void set_stop(void *data)
{
<<<<<<< HEAD
    service_loop_t *loop = data;

    loop->is_running = 0;
    uv_close((uv_handle_t *)&loop->async, NULL);
    uv_stop(loop->handle);
=======
    bt_service_loop.is_running = 0;
    uv_close((uv_handle_t *)&bt_service_loop.async, NULL);
    uv_stop(bt_service_loop.handle);
>>>>>>> bluetooth framework re-implement base
    BT_LOGD("set_stopped");
}

static void service_sync_callback(void *data)
{
    signal_msg_t *msg = (signal_msg_t *)data;

    msg->func(msg->data);
    uv_sem_post(&msg->signal);
}

static void service_message_callback(uv_async_t *handle)
{
<<<<<<< HEAD
    uv_loop_t *uvloop = handle->loop;
    service_loop_t *loop = uvloop->data;
    internel_msg_t *imsg;

    for (;;) {
        uv_mutex_lock(&loop->msg_lock);
        imsg = (internel_msg_t *)list_remove_head(&loop->msg_queue);
        uv_mutex_unlock(&loop->msg_lock);
=======
    internel_msg_t *imsg;

    for (;;) {
        uv_mutex_lock(&bt_service_loop.msg_lock);
        imsg = (internel_msg_t *)list_remove_head(&bt_service_loop.msg_queue);
        uv_mutex_unlock(&bt_service_loop.msg_lock);
>>>>>>> bluetooth framework re-implement base
        if (!imsg)
            return;

        imsg->func(imsg->msg);
        free(imsg);
    }
}

static void service_schedule_loop(void *data)
{
<<<<<<< HEAD
    service_loop_t *loop = data;
=======
    service_loop_t *loop = &bt_service_loop;

    loop->handle = uv_default_loop();
    if (!loop->handle) {
        BT_LOGE("%s get default loop fail", __func__);
        return;
    }
>>>>>>> bluetooth framework re-implement base

    int ret = uv_async_init(loop->handle, &loop->async, service_message_callback);
    if (ret != 0) {
        BT_LOGE("%s async error: %d", __func__, ret);
        return;
    }

    BT_LOGD("%s:%p, async:%p", __func__, loop->handle, &loop->async);
<<<<<<< HEAD
    do_in_service_loop(set_ready, loop);
    uv_run(loop->handle, UV_RUN_DEFAULT);
    loop->is_running = 0;
    uv_loop_close(loop->handle);
    uv_sem_post(&loop->exited);

    BT_LOGD("%s %s quit", loop->name, __func__);
=======
    do_in_service_loop(set_ready, NULL);
    uv_run(loop->handle, UV_RUN_DEFAULT);
    bt_service_loop.is_running = 0;
    uv_loop_close(loop->handle);
    uv_sem_post(&loop->exited);

    BT_LOGD("%s quit", __func__);
>>>>>>> bluetooth framework re-implement base
}

static void service_timer_cb(uv_timer_t *handle)
{
    service_timer_t *timer = (service_timer_t *)handle;

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

static void service_poll_cb(uv_poll_t *handle, int status, int events)
{
    service_poll_t *poll = (service_poll_t *)handle;
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

static void handle_close_cb(uv_handle_t *handle)
{
    if (handle->data)
        free(handle->data);
}

int service_loop_init(void)
{
<<<<<<< HEAD
    uv_loop_t *uvloop;
    service_loop_t *loop;
    int ret;

    uvloop = get_service_uv_loop();
    if (uvloop->data != NULL)
        return -1;

    loop = calloc(1, sizeof(service_loop_t));
    if (loop == NULL)
        return -1;

    loop->handle = uvloop;
    if (!loop->handle) {
        BT_LOGE("%s get default loop fail", __func__);
        free(loop);
        return -1;
    }

    loop->handle->data = loop;
=======
    service_loop_t *loop = &bt_service_loop;
    int ret;

>>>>>>> bluetooth framework re-implement base
    loop->is_running = 0;
    ret = uv_mutex_init(&loop->msg_lock);
    if (ret != 0) {
        BT_LOGE("%s mutex error: %d", __func__, ret);
<<<<<<< HEAD
        free(loop);
=======
>>>>>>> bluetooth framework re-implement base
        return ret;
    }

    list_initialize(&loop->msg_queue);
    list_initialize(&loop->init_queue);

    return 0;
}

<<<<<<< HEAD
int service_loop_run(bool start_thread, char *name)
{
    uv_loop_t *handle = get_service_uv_loop();
    service_loop_t *loop = handle->data;
=======
int service_loop_run(bool start_thread)
{
    service_loop_t *loop = &bt_service_loop;
>>>>>>> bluetooth framework re-implement base

    if (start_thread) {
        int ret = uv_sem_init(&loop->ready, 0);
        if (ret != 0) {
            BT_LOGE("%s sem init error: %d", __func__, ret);
            return ret;
        }

<<<<<<< HEAD
        uv_thread_options_t options = {
            UV_THREAD_HAS_STACK_SIZE | UV_THREAD_HAS_PRIORITY,
            CONFIG_BLUETOOTH_SERVICE_LOOP_THREAD_STACK_SIZE,
            CONFIG_BLUETOOTH_SERVICE_LOOP_THREAD_PRIORITY
        };
        ret = uv_thread_create_ex(&loop->thread, &options, service_schedule_loop, loop);
=======
        uv_thread_options_t options = { UV_THREAD_HAS_STACK_SIZE, SERVICE_LOOP_THREAD_STACK_SIZE };
        ret = uv_thread_create_ex(&loop->thread, &options, service_schedule_loop, NULL);
>>>>>>> bluetooth framework re-implement base
        if (ret != 0) {
            BT_LOGE("service loop thread create :%d", ret);
            return ret;
        }

<<<<<<< HEAD
        if (name != NULL && strlen(name) > 0)
            snprintf(loop->name, sizeof(loop->name), "%s_%d", name, getpid());
        else
            snprintf(loop->name, sizeof(loop->name), "loop_%d", getpid());
        pthread_setname_np(loop->thread, loop->name);
        uv_sem_wait(&loop->ready);
        uv_sem_destroy(&loop->ready);
        BT_LOGD("%s loop running now !!!", loop->name);
    } else {
        BT_LOGD("service loop running now !!!");
        service_schedule_loop(loop);
=======
        pthread_setname_np(loop->thread, "bt_service_thread");
        uv_sem_wait(&loop->ready);
        uv_sem_destroy(&loop->ready);
        BT_LOGD("service loop running now !!!");
    } else {
        BT_LOGD("service loop running now !!!");
        service_schedule_loop(NULL);
>>>>>>> bluetooth framework re-implement base
    }

    return 0;
}

void service_loop_exit(void)
{
<<<<<<< HEAD
    uv_loop_t *handle = get_service_uv_loop();
    service_loop_t *loop = handle->data;
    struct list_node *node;
    struct list_node *tmp;

    if (loop == NULL)
        return;

    if (loop->is_running) {
        do_in_service_loop(set_stop, loop);
=======
    struct list_node *node;
    struct list_node *tmp;
    service_loop_t *loop = &bt_service_loop;

    if (loop->is_running) {
        do_in_service_loop(set_stop, NULL);
>>>>>>> bluetooth framework re-implement base
        uv_sem_wait(&loop->exited);
        uv_sem_destroy(&loop->exited);
    }

    uv_mutex_lock(&loop->msg_lock);
    list_for_every_safe(&loop->msg_queue, node, tmp)
    {
        list_delete(node);
        free(node);
    }
    list_delete(&loop->msg_queue);
    uv_mutex_unlock(&loop->msg_lock);
    uv_mutex_destroy(&loop->msg_lock);
<<<<<<< HEAD
    free(loop);
=======
>>>>>>> bluetooth framework re-implement base
}

service_poll_t *service_loop_poll_fd(int fd, int pevents, service_poll_cb_t cb, void *userdata)
{
<<<<<<< HEAD
    uv_loop_t *handle = get_service_uv_loop();
    service_loop_t *loop = handle->data;

    assert(fd);
    assert(cb);
=======
    assert(fd);
    assert(cb);
    assert(bt_service_loop.is_running);
>>>>>>> bluetooth framework re-implement base

    service_poll_t *poll = (service_poll_t *)malloc(sizeof(service_poll_t));
    if (!poll)
        return NULL;

    poll->callback = cb;
    poll->userdata = userdata;
    poll->handle.data = poll;
<<<<<<< HEAD
    int ret = uv_poll_init(loop->handle, &poll->handle, fd);
=======
    int ret = uv_poll_init(bt_service_loop.handle, &poll->handle, fd);
>>>>>>> bluetooth framework re-implement base
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

int service_loop_reset_poll(service_poll_t *poll, int pevents)
{
    assert(poll);
<<<<<<< HEAD
=======
    assert(bt_service_loop.is_running);
>>>>>>> bluetooth framework re-implement base

    uv_poll_stop(&poll->handle);

    return uv_poll_start(&poll->handle, uv_events(pevents), service_poll_cb);
}

void service_loop_remove_poll(service_poll_t *poll)
{
<<<<<<< HEAD
=======
    assert(bt_service_loop.is_running);

>>>>>>> bluetooth framework re-implement base
    if (!poll)
        return;

    uv_poll_stop(&poll->handle);
    uv_close((uv_handle_t *)&poll->handle, handle_close_cb);
}

service_timer_t *service_loop_timer(uint64_t timeout, uint64_t repeat, service_timer_cb_t cb, void *userdata)
{
<<<<<<< HEAD
    uv_loop_t *handle = get_service_uv_loop();
    service_loop_t *loop = handle->data;

=======
    assert(bt_service_loop.is_running);
>>>>>>> bluetooth framework re-implement base
    if (!cb)
        return NULL;

    service_timer_t *timer = malloc(sizeof(service_timer_t));
    if (!timer)
        return NULL;

<<<<<<< HEAD
    uv_timer_init(loop->handle, &timer->handle);
=======
    uv_timer_init(bt_service_loop.handle, &timer->handle);
>>>>>>> bluetooth framework re-implement base
    timer->callback = cb;
    timer->userdata = userdata;
    timer->handle.data = timer;
    uv_timer_start(&timer->handle, service_timer_cb, timeout, repeat);

    return timer;
}

service_timer_t *service_loop_timer_no_repeating(uint64_t timeout, service_timer_cb_t cb, void *userdata)
{
    return service_loop_timer(timeout, 0, cb, userdata);
}

void service_loop_cancel_timer(service_timer_t *timer)
{
<<<<<<< HEAD
=======
    assert(bt_service_loop.is_running);
>>>>>>> bluetooth framework re-implement base
    if (!timer)
        return;

    uv_timer_stop(&timer->handle);
    uv_close((uv_handle_t *)&timer->handle, handle_close_cb);
}

<<<<<<< HEAD
static void service_work_cb(uv_work_t *req)
{
    service_work_t *work = req->data;
    assert(work);

    work->work_cb(work, work->userdata);
}

static void service_after_work_cb(uv_work_t *req, int status)
{
    service_work_t *work = req->data;
    assert(status == 0);
    assert(work);

    if (work->after_work_cb)
        work->after_work_cb(work, work->userdata);
    free(work);
}

service_work_t *service_loop_work(void *user_data, service_work_cb_t work_cb,
                                  service_after_work_cb_t after_work_cb)
{
    uv_loop_t *handle = get_service_uv_loop();

    service_work_t *work = zalloc(sizeof(*work));
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
    uv_loop_t *handle = get_service_uv_loop();
    service_loop_t *loop = handle->data;

    internel_msg_t *msg = (internel_msg_t *)malloc(sizeof(internel_msg_t));

    msg->init = func;
    uv_mutex_lock(&loop->msg_lock);
    list_add_tail(&loop->init_queue, &msg->node);
    uv_mutex_unlock(&loop->msg_lock);
=======
void add_init_process(service_func_t func)
{
    internel_msg_t *msg = (internel_msg_t *)malloc(sizeof(internel_msg_t));

    msg->init = func;
    uv_mutex_lock(&bt_service_loop.msg_lock);
    list_add_tail(&bt_service_loop.init_queue, &msg->node);
    uv_mutex_unlock(&bt_service_loop.msg_lock);
>>>>>>> bluetooth framework re-implement base
}

void do_in_service_loop(service_func_t func, void *data)
{
<<<<<<< HEAD
    uv_loop_t *handle = get_service_uv_loop();
    service_loop_t *loop = handle->data;

=======
>>>>>>> bluetooth framework re-implement base
    internel_msg_t *msg = (internel_msg_t *)malloc(sizeof(internel_msg_t));
    assert(msg);

    msg->func = func;
    msg->msg = data;

<<<<<<< HEAD
    uv_mutex_lock(&loop->msg_lock);
    list_add_tail(&loop->msg_queue, &msg->node);
    uv_mutex_unlock(&loop->msg_lock);

    uv_async_send(&loop->async);
=======
    uv_mutex_lock(&bt_service_loop.msg_lock);
    list_add_tail(&bt_service_loop.msg_queue, &msg->node);
    uv_mutex_unlock(&bt_service_loop.msg_lock);

    uv_async_send(&bt_service_loop.async);
>>>>>>> bluetooth framework re-implement base
}

void do_in_service_loop_sync(service_func_t func, void *data)
{
    signal_msg_t msg;

    msg.func = func;
    msg.data = data;
    uv_sem_init(&msg.signal, 0);
    do_in_service_loop(service_sync_callback, &msg);
    uv_sem_wait(&msg.signal);
    uv_sem_destroy(&msg.signal);
}

uv_loop_t *get_service_uv_loop(void)
{
    return uv_default_loop();
}

#else

<<<<<<< HEAD
#endif
=======
#endif
>>>>>>> bluetooth framework re-implement base
