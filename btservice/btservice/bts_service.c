/****************************************************************************
 * frameworks/bluetooth/src/btservice/btservice.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/list.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <sys/stat.h>

#include "btm_gap.h"
#include "btm_manager.h"
#include "bts_gap.h"
#include "bts_gatt_service.h"
#include "bts_hf_client.h"
#include "bts_hid_service.h"
#include "bts_service.h"
#include "bts_spp.h"
#include "stack_adapter_gap.h"
#include "stack_adapter_service_base.h"
#include "uv.h"

#define LOG_TAG "bts_service"
#include "log.h"

typedef struct {
    process_in_timer func_in_timer;
    void* data;
} timer_process_data_t;

typedef enum {
    THREAD_ID_SERVICE,
    THREAD_ID_STACK,
} thread_id_t;

typedef struct {
    struct list_node node;
    bt_profile_id id;
    void* data;
    size_t size;
} bts_profile_msg_t;

typedef struct {
    bts_service_event_poll_cb cb;
    void* priv;
} bts_event_poll_t;

bt_service_state service_state = BTM_STATE_OFF;
static bt_service_callbacks* bluetooth_upper_callbacks = NULL;
static uv_loop_t* bt_dispatch_loop;
static uv_mutex_t msg_mutex;
static uv_thread_t thread_handle[2];
static uv_async_t async_handle[1];
static uv_sem_t wait_stack;
static uv_sem_t wait_service;
#ifdef CONFIG_EVENT_FD
static uv_poll_t* bt_event_poll;
#endif

static bts_profile_callbacks profiles_callbacks[BT_PROFILE_MAX_ID];
static struct list_node bts_msg_list = LIST_INITIAL_VALUE(bts_msg_list);

extern void InitTransportLayer(void);
extern void DeinitTransportLayer(void);
extern void ScheduleLoop(void);
extern void TransportRecvData(void);
extern int GetTransportHandler(void);
static void bts_service_transport_recv_loop(void);

#ifdef CONFIG_EVENT_FD
int bts_service_event_poll_signal(uv_poll_t* handle)
{
    return eventfd_write(handle->io_watcher.fd, 1ULL);
}

static void bts_service_event_poll_handler(uv_poll_t* handle, int status,
    int events)
{
    bts_event_poll_t* event = handle->data;
    eventfd_t value;

    if (eventfd_read(handle->io_watcher.fd, &value) < 0) {
        BT_LOGE("%s fail to read event fd: %d", __func__, handle->io_watcher.fd);
    }

    if (event->cb)
        event->cb(event->priv);
}

void bts_service_event_poll_stop(uv_poll_t* handle)
{
    bts_event_poll_t* event = handle->data;

    handle->data = NULL;
    bts_uv_poll_stop(handle);
    free(event);
}

uv_poll_t* bts_service_event_poll_start(bts_service_event_poll_cb cb,
    void* userdata)
{
    bts_event_poll_t* event;
    uv_poll_t* handle = NULL;
    int fd;

    fd = eventfd(0, EFD_SEMAPHORE | EFD_CLOEXEC);
    if (fd >= 0) {

        event = malloc(sizeof(bts_event_poll_t));
        if (!event) {
            close(fd);
            return NULL;
        }

        event->cb = cb;
        event->priv = userdata;

        handle = bts_uv_poll_start(fd, UV_READABLE, bts_service_event_poll_handler,
            event);
        if (!handle) {
            free(event);
            close(fd);
        }
    }

    return handle;
}
#endif

static void bts_uv_close_cb(uv_handle_t* handle)
{
    free(handle);
}

uv_poll_t* bts_uv_poll_start(int fd, int pevents, uv_poll_cb cb,
    void* userdata)
{
    uv_poll_t* handle = (uv_poll_t*)malloc(sizeof(uv_poll_t));
    if (!handle) {
        BT_LOGE("malloc failed");
        return NULL;
    }
    handle->data = userdata;

    int ret = uv_poll_init(bt_dispatch_loop, handle, fd);
    if (ret < 0) {
        BT_LOGE("uv_poll_init failed: %d", ret);
        free(handle);
        return NULL;
    }

    ret = uv_poll_start(handle, pevents, cb);
    if (ret < 0) {
        BT_LOGE("uv_poll_start failed: %d", ret);
        free(handle);
        return NULL;
    }

#ifdef CONFIG_EVENT_FD
    if (service_state >= BTM_STATE_TURNING_ON && bt_event_poll) {
        ret = bts_service_event_poll_signal(bt_event_poll);
        if (ret < 0)
            BT_LOGE("eventfd_write failed: %d", ret);
    }
#endif

    return handle;
}

void bts_uv_poll_stop(uv_poll_t* handle)
{
    if (!handle) {
        BT_LOGE("uv_poll_stop failed, handle NULL");
        return;
    }
    uv_poll_stop(handle);
    uv_close((uv_handle_t*)handle, bts_uv_close_cb);
}

static void timer_hadler_cb(uv_timer_t* timer)
{
    if (NULL == timer) {
        return;
    }

    if (NULL == timer->data) {
        return;
    }
    timer_process_data_t* process_data = (timer_process_data_t*)timer->data;
    process_in_timer timer_callback = process_data->func_in_timer;
    if (NULL != timer_callback) {
        timer_callback(process_data->data);
    }

    return;
}

uv_timer_t* start_timer(int timeout, int repeat,
    process_in_timer timer_callback, void* data)
{
    uv_timer_t* timer;
    if (NULL == timer_callback) {
        return NULL;
    }
    timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(bt_dispatch_loop, timer);

    timer_process_data_t* process_data = malloc(sizeof(timer_process_data_t));
    process_data->func_in_timer = timer_callback;
    process_data->data = data;
    timer->data = process_data;

    uv_timer_start(timer, (uv_timer_cb)timer_hadler_cb, timeout, repeat);
    return timer;
}

void stop_timer(uv_timer_t* timer)
{
    if (NULL == timer) {
        return;
    }
    if (timer->data)
        free(timer->data);
    timer->data = NULL;
    uv_timer_stop(timer);
    uv_close((uv_handle_t*)timer, bts_uv_close_cb);
}

uv_loop_t* get_service_loop(void) { return bt_dispatch_loop; }

static void stack_schedule_loop(void* data)
{
    uv_sem_post(&wait_stack);
    ScheduleLoop();
}

bool bts_register_profile_process(bt_profile_id id, bts_profile_callbacks cb)
{
    profiles_callbacks[id] = cb;
    return true;
}

bool bts_unregister_profile_process(bt_profile_id id)
{
    profiles_callbacks[id] = NULL;
    return true;
}

bool bts_send_uv_msg(bt_profile_id id, void* data, size_t size)
{
    bts_profile_msg_t* msg = (bts_profile_msg_t*)malloc(sizeof(bts_profile_msg_t));
    if (!msg) {
        BT_LOGE("malloc msg fail");
        return false;
    }
    msg->id = id;
    msg->data = data;
    msg->size = size;
    uv_mutex_lock(&msg_mutex);
    list_add_tail(&bts_msg_list, &msg->node);
    uv_mutex_unlock(&msg_mutex);
    uv_async_send(&async_handle[THREAD_ID_SERVICE]);
    return true;
}

static void bts_handle_uv_msg(uv_async_t* handle)
{
    if (!handle) {
        BT_LOGE("fail, handle null");
        return;
    }

    bts_profile_msg_t* msg = NULL;
    do {
        uv_mutex_lock(&msg_mutex);
        msg = list_remove_head_type(&bts_msg_list, bts_profile_msg_t, node);
        uv_mutex_unlock(&msg_mutex);
        if (!msg) {
            break;
        }
        if (!profiles_callbacks[msg->id]) {
            BT_LOGW("not handle, profile %d not registered", msg->id);
            free(msg);
            continue;
        }
        profiles_callbacks[msg->id](msg->id, msg->data, msg->size);
        free(msg);
    } while (true);
}

static void service_schedule_loop(void* data)
{
    uv_async_init(bt_dispatch_loop, &async_handle[THREAD_ID_SERVICE],
        bts_handle_uv_msg);
    bts_service_transport_recv_loop();
#ifdef CONFIG_EVENT_FD
    bt_event_poll = bts_service_event_poll_start(NULL, NULL);
#endif
    uv_sem_post(&wait_service);
    uv_run(bt_dispatch_loop, UV_RUN_DEFAULT);
}

void create_config_folder(void)
{
    char folder_misc[] = "/data/misc";
    char folder_bt[] = "/data/misc/bt";
#ifdef CONFIG_BLUELET_HCI_SNOOP_LOG_PATH
    char folder_snoop[] = CONFIG_BLUELET_HCI_SNOOP_LOG_PATH;
#else
    char folder_snoop[] = "/data/misc/bt/snoop";
#endif

    if (access(folder_misc, 0) != 0) {
        if (mkdir(folder_misc, 0777) != 0)
            syslog(LOG_WARNING, "%s, create misc folder failed\n", __func__);
    }
    if (access(folder_bt, 0) != 0) {
        if (mkdir(folder_bt, 0777) != 0)
            syslog(LOG_WARNING, "%s, create bt folder failed\n", __func__);
    }

    if (access(folder_snoop, 0) != 0) {
        if (mkdir(folder_snoop, 0777) != 0)
            syslog(LOG_WARNING, "%s, create snoop folder failed", __func__);
    }
}

static void bts_service_h4_recv_handler(uv_poll_t* handle, int status,
    int events)
{
    if (status < 0) {
        BT_LOGE("fail, %s status:%d", __func__, status);
        return;
    }

    if (events & UV_READABLE) {
        TransportRecvData();
        return;
    }

    BT_LOGE("error, %s unexpected events:%d", __func__, events);
}

static void bts_service_transport_recv_loop(void)
{
    int fd = GetTransportHandler();
    if (fd < 0) {
        BT_LOGE("fail, %s invlaid fd:%d", __func__, fd);
        return;
    }

    uv_poll_t* poll = bts_uv_poll_start(fd, UV_READABLE, bts_service_h4_recv_handler, NULL);
    if (!poll) {
        BT_LOGE("fail, %s", __func__);
        return;
    }
}

bt_result_code bts_service_init(bt_service_callbacks* callbacks)
{
    bluetooth_upper_callbacks = callbacks;
    create_config_folder();
    int rc = uv_mutex_init(&msg_mutex);
    if (rc) {
        BT_LOGE("fail, uv_mutex_init err:%d", rc);
        return BT_RESULT_FAILED;
    }
    int ret = uv_sem_init(&wait_stack, 0);
    if (ret < 0)
        return BT_RESULT_FAILED;

    ret = uv_sem_init(&wait_service, 0);
    if (ret < 0)
        return BT_RESULT_FAILED;

    if (service_state != BTM_STATE_OFF)
        return BT_RESULT_FAILED;
    utils_log_init();
    InitTransportLayer();
    gap_service_init();
    bt_dispatch_loop = uv_loop_new();
    uv_thread_options_t options = { UV_THREAD_HAS_STACK_SIZE | UV_THREAD_HAS_PRIORITY,
        CONFIG_BTSTACK_THREAD_STACK_SIZE };
    options.priority = CONFIG_BLUETOOTH_THREAD_PRIORITY;
    ret = uv_thread_create_ex(&thread_handle[THREAD_ID_STACK], &options,
        stack_schedule_loop, NULL);
    if (ret != 0) {
        BT_LOGE("fail uv_thread_create, ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    pthread_setname_np(thread_handle[THREAD_ID_STACK], "bluelet_thread");

    options.stack_size = CONFIG_BTSERVICE_THREAD_STACK_SIZE;
    options.priority = CONFIG_BLUETOOTH_THREAD_PRIORITY;
    ret = uv_thread_create_ex(&thread_handle[THREAD_ID_SERVICE], &options,
        service_schedule_loop, NULL);
    if (ret != 0) {
        BT_LOGE("fail uv_thread_create, ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    pthread_setname_np(thread_handle[THREAD_ID_SERVICE], "btservice_thread");
    service_state = BTM_STATE_TURNING_ON;

    uv_sem_wait(&wait_service);
    uv_sem_wait(&wait_stack);
    uv_sem_destroy(&wait_service);
    uv_sem_destroy(&wait_stack);

    return BT_RESULT_SUCCESS;
}

void bts_service_cleanup(void) { DeinitTransportLayer(); }

void stack_state_change(bt_service_state state)
{
    service_state = state;
    if (BTM_STATE_ON == state) {
        gap_bt_config_init(bluetooth_upper_callbacks->adapter_state_changed_cb);
    } else {
        bluetooth_upper_callbacks->adapter_state_changed_cb(state);
    }
}
