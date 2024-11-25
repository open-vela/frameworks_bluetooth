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
#ifndef _UV_THREAD_LOOP_H__
#define _UV_THREAD_LOOP_H__

#include <stdint.h>

#include "uv.h"

typedef void (*thread_func_t)(void* data);

int thread_loop_init(uv_loop_t* loop);
int thread_loop_run(uv_loop_t* loop, bool start_thread, const char* name);
void thread_loop_exit(uv_loop_t* loop);
uv_poll_t* thread_loop_poll_fd(uv_loop_t* loop, int fd, int pevents, uv_poll_cb cb, void* userdata);
int thread_loop_reset_poll(uv_poll_t* poll, int pevents, uv_poll_cb cb);
void thread_loop_remove_poll(uv_poll_t* poll);
uv_timer_t* thread_loop_timer(uv_loop_t* loop, uint64_t timeout, uint64_t repeat, uv_timer_cb cb, void* userdata);
uv_timer_t* thread_loop_timer_no_repeating(uv_loop_t* loop, uint64_t timeout, uv_timer_cb cb, void* userdata);
void thread_loop_cancel_timer(uv_timer_t* timer);
void do_in_thread_loop(uv_loop_t* loop, thread_func_t func, void* data);
void do_in_thread_loop_sync(uv_loop_t* loop, thread_func_t func, void* data);

#endif /* _UV_THREAD_LOOP_H__ */