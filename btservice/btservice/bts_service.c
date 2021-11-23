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

typedef struct
{
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

bt_service_state service_state = BT_MANAGER_STATE_OFF;
static bt_service_callbacks* bluetooth_upper_callbacks = NULL;
static uv_loop_t* bt_dispatch_loop;
static uv_mutex_t msg_mutex;
static uv_thread_t thread_handle[2];
static uv_async_t async_handle[1];
static bts_profile_callbacks profiles_callbacks[BT_PROFILE_MAX_ID];
static struct list_node bts_msg_list = LIST_INITIAL_VALUE(bts_msg_list);

extern void InitTransportLayer(void);
extern void ScheduleLoop(void);

static void bts_uv_close_cb(uv_handle_t* handle)
{
    BT_LOGD("bts_uv_close_cb");
    free(handle);
}

uv_poll_t* bts_uv_poll_start(int fd, int pevents, uv_poll_cb cb, void* userdata)
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
    return handle;
}

void bts_uv_poll_stop(uv_poll_t* handle)
{
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

uv_timer_t* start_timer(int timeout, int repeat, process_in_timer timer_callback, void* data)
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
    uv_timer_stop(timer);
    uv_close((uv_handle_t*)timer, bts_uv_close_cb);
}

uv_loop_t* get_service_loop(void)
{
    return bt_dispatch_loop;
}

static void stack_schedule_loop(void* data)
{
    BT_LOGD("%s", __func__);
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
    BT_LOGD("%s", __func__);
    uv_async_init(bt_dispatch_loop, &async_handle[THREAD_ID_SERVICE], bts_handle_uv_msg);
    uv_run(bt_dispatch_loop, UV_RUN_DEFAULT);
}

bt_result_code bts_service_init(bt_service_callbacks* callbacks)
{
    bluetooth_upper_callbacks = callbacks;

    uv_mutex_init(&msg_mutex);
    if (service_state != BT_MANAGER_STATE_OFF)
        return BT_RESULT_FAILED;
    InitTransportLayer();
    gap_service_init();
    bt_dispatch_loop = uv_loop_new();
    int ret = uv_thread_create(&thread_handle[THREAD_ID_STACK], stack_schedule_loop, NULL);
    if (ret != 0) {
        BT_LOGE("fail uv_thread_create, ret:%d", ret);
        return BT_RESULT_FAILED;
    }

#if defined(CONFIG_BLUETOOTH_HIDDEV)
    const hid_interface_t* hid_if = hid_get_interface();
    if (hid_if) {
        BT_LOGD("hid init");
        hid_if->init();
    }
#endif
    ret = uv_thread_create(&thread_handle[THREAD_ID_SERVICE], service_schedule_loop, NULL);
    if (ret != 0) {
        BT_LOGE("fail uv_thread_create, ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    service_state = BT_MANAGER_STATE_TURNING_ON;
    return BT_RESULT_SUCCESS;
}

void bts_service_cleanup(void)
{
}

void stack_state_change(bt_service_state state)
{
    service_state = state;
    if (BT_MANAGER_STATE_ON == state) {
        gap_create_factory_info(false);
        gap_read_device_info();
        gap_read_data_storage();
    }
    bluetooth_upper_callbacks->adapter_state_changed_cb(service_state);
}
