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

#ifdef CONFIG_OBELISK_LIBUV_LOOP
#include "uv.h"
#endif

enum service_poll_event {
    POLL_READABLE = 1,
    POLL_WRITABLE = 2,
    POLL_DISCONNECT = 4,
    POLL_ERROR = 8
};

typedef struct service_timer service_timer_t;
typedef struct service_poll service_poll_t;
typedef void (*service_poll_cb_t)(service_poll_t *poll, int revent, void *userdata);
typedef void (*service_timer_cb_t)(service_timer_t *timer, void *userdata);
typedef void (*service_func_t)(void *data);

int service_loop_init(void);
int service_loop_run(bool start_thread);
void service_loop_exit(void);
service_poll_t *service_loop_poll_fd(int fd, int pevents, service_poll_cb_t cb, void *userdata);
int service_loop_reset_poll(service_poll_t *poll, int pevents);
void service_loop_remove_poll(service_poll_t *poll);
service_timer_t *service_loop_timer(uint64_t timeout, uint64_t repeat, service_timer_cb_t cb, void *userdata);
service_timer_t *service_loop_timer_no_repeating(uint64_t timeout, service_timer_cb_t cb, void *userdata);
void service_loop_cancel_timer(service_timer_t *timer);
void do_in_service_loop(service_func_t func, void *data);
void do_in_service_loop_sync(service_func_t func, void *data);
void add_init_process(service_func_t func);

#ifdef CONFIG_OBELISK_LIBUV_LOOP
uv_loop_t *get_service_uv_loop(void);
#endif

#endif /* _BT_SERVICE_LOOP_H__ */