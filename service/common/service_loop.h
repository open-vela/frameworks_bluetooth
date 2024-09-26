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
#ifndef _BT_SERVICE_LOOP_H__
#define _BT_SERVICE_LOOP_H__

#include <stdint.h>

#include "bt_list.h"
#include "uv.h"

enum service_poll_event {
    POLL_READABLE = 1,
    POLL_WRITABLE = 2,
    POLL_DISCONNECT = 4,
    POLL_ERROR = 8
};

typedef struct service_timer service_timer_t;
typedef struct service_poll service_poll_t;
typedef struct service_work service_work_t;
typedef void (*service_poll_cb_t)(service_poll_t* poll, int revent, void* userdata);
typedef void (*service_timer_cb_t)(service_timer_t* timer, void* userdata);
typedef void (*service_func_t)(void* data);
typedef int (*service_init_t)(void* data);
typedef void (*service_work_cb_t)(service_work_t* work, void* userdata);
typedef void (*service_after_work_cb_t)(service_work_t* work, void* userdata);

typedef struct service_loop {
    char name[64];
    uv_loop_t* handle;
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
    void* userdata;
} service_timer_t;

typedef struct service_poll {
    uv_poll_t handle;
    service_poll_cb_t callback;
    void* userdata;
} service_poll_t;

typedef struct service_work {
    uv_work_t work;
    service_work_cb_t work_cb;
    service_after_work_cb_t after_work_cb;
    void* userdata;
} service_work_t;

int service_loop_init(void);
int service_loop_run(bool start_thread, char* name);
void service_loop_exit(void);
service_poll_t* service_loop_poll_fd(int fd, int pevents, service_poll_cb_t cb, void* userdata);
int service_loop_reset_poll(service_poll_t* poll, int pevents);
void service_loop_remove_poll(service_poll_t* poll);
service_timer_t* service_loop_timer(uint64_t timeout, uint64_t repeat, service_timer_cb_t cb, void* userdata);
service_timer_t* service_loop_timer_no_repeating(uint64_t timeout, service_timer_cb_t cb, void* userdata);
void service_loop_cancel_timer(service_timer_t* timer);
service_work_t* service_loop_work(void* user_data, service_work_cb_t work_cb,
    service_after_work_cb_t after_work_cb);
void do_in_service_loop(service_func_t func, void* data);
void do_in_service_loop_sync(service_func_t func, void* data);
void add_init_process(service_init_t func);

uv_loop_t* get_service_uv_loop(void);

uint64_t get_os_timestamp_us(void);
#endif /* _BT_SERVICE_LOOP_H__ */
