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

#include <stdio.h>
#include<stdlib.h>
#include <pthread.h>
#include "bts_service.h"
#include "btm_manager.h"
#include "btm_gap.h"
#include "uv.h"
#include "stack_adapter_service_base.h"
#include "stack_adapter_gap.h"

#include "bts_gap.h"
#include "bts_gatt_service.h"
#include "bts_spp.h"
#include "bts_hf_client.h"

#define LOG_TAG "bts_service"
#include "log.h"

#define UV_TIMEOUT  (32767)
#define UV_TIMEOUT_REPEAT  (32767)

typedef struct 
{
    process_in_io func_in_io;
    void * data;
}io_process_data_t;

typedef struct 
{
    process_in_timer func_in_timer; 
    void * data;
}timer_process_data_t;


bt_service_state service_state = BT_MANAGER_STATE_OFF;
static bt_service_callbacks* bluetooth_upper_callbacks = NULL;
uv_loop_t* dispatch_loop;

void bts_uv_close_cb(uv_handle_t* handle)
{
    //BT_LOGD("uv_close_cb\n");
    if (NULL == handle) 
    {
        return;
    }
    free(handle);
}

uv_poll_t* bts_uv_poll_start(int fd, int pevents, uv_poll_cb cb)
{
    int ret;
    uv_poll_t *handle;

    handle = (uv_poll_t *)malloc(sizeof(uv_poll_t));
    if (!handle)
        return NULL;

    ret = uv_poll_init(dispatch_loop, handle, fd);
    if (ret < 0) {
        free(handle);
        return NULL;
    }

    ret = uv_poll_start(handle, pevents, cb);
    if (ret < 0) {
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

void execute_service_callback(uv_async_t* handle)
{
    //BT_LOGD("execute_service_callback\n");
      bts_process_loop_data func;

    if (NULL == handle) {
        BT_LOGE("execute_service_callback handle is NULl");
        return;
    }
    excute_service_context_t *context = NULL;
    context = (excute_service_context_t*)uv_handle_get_data((uv_handle_t*)handle);
    if (NULL == context) {
        return;
    }

    func = context->loop_func;
    func(context->data, context->data_size);
    uv_close((uv_handle_t*)handle, bts_uv_close_cb);
    if (NULL != context) {
        free(context);
    }

}

void process_in_loop(excute_service_context_t *context) 
{
    uv_async_t *post_function_async ;
    uv_loop_t * loop = NULL;

    if (NULL == dispatch_loop) {
        return;
    }
    post_function_async = malloc(sizeof(uv_async_t));
    loop = dispatch_loop;
    uv_async_init(loop, post_function_async, execute_service_callback);
    uv_handle_set_data((uv_handle_t*)post_function_async, context);
    uv_async_send(post_function_async);
}

void io_process(uv_work_t *req) 
{
    BT_LOGD("btservice io_process");

    io_process_data_t *process_data = (io_process_data_t *)req->data;
    if (NULL == process_data){
        return;
    }

    if(NULL == process_data->func_in_io){
        return;
    }
    process_data->func_in_io(process_data->data);
    return;
}

void after_io_process(uv_work_t *req, int status)
{
    BT_LOGD("btservice after_io_process");
    //free();
    //uv_close(req, NULL);
}

void process_in_work_thread(process_in_io func_in_io, void * data)
{
    uv_work_t *req = malloc(sizeof(uv_work_t));
    io_process_data_t *process_data = malloc(sizeof(io_process_data_t));
    process_data->func_in_io = func_in_io;
    process_data->data = data;
    req->data = process_data;
    uv_queue_work(dispatch_loop, req, io_process, after_io_process);
}

void timer_hadler_cb(uv_timer_t * timer)
{
    BT_LOGD("Do timer_hadler_cb");
    if (NULL == timer){
        return;
    }

    if(NULL == timer->data){
        return;
    }
     timer_process_data_t *process_data = (timer_process_data_t *)timer->data;
     process_in_timer timer_callback = process_data->func_in_timer;
     if (NULL != timer_callback){
        timer_callback(process_data->data);
     }
     
    return;
}

uv_timer_t *start_timer(int timeout, int repeat, process_in_timer timer_callback, void * data)
{
    uv_timer_t *timer;
    if (NULL == timer_callback){
        return NULL;
    }
    timer = malloc(sizeof(uv_timer_t));
    uv_timer_init(dispatch_loop, timer);

    timer_process_data_t *process_data = malloc(sizeof(timer_process_data_t));
    process_data->func_in_timer = timer_callback;
    process_data->data = data;
    timer->data = process_data;

    uv_timer_start(timer, (uv_timer_cb)timer_hadler_cb, timeout, repeat);
    return timer;
}

void stop_timer(uv_timer_t * timer)
{
    if (NULL == timer){
        return;
    }
    uv_timer_stop(timer);
    uv_close((uv_handle_t *)timer, bts_uv_close_cb);
}

void process_in_loop_timer(char * data)
{

}

int service_loop_init(void)
{
    uv_loop_t _loop;
    BT_LOGD("btservice loop_init");
    dispatch_loop = uv_default_loop();

    start_timer(0, 200000, process_in_loop_timer, NULL);
    BT_LOGD("Idling...");
    uv_run(dispatch_loop, UV_RUN_DEFAULT);

    //nerver touch here only if service is down
    BT_LOGD("Idling done");
    uv_loop_close(uv_default_loop());
    return 0;
}



static bool interface_ready(void) { return bluetooth_upper_callbacks != NULL; }

static const void* get_profile_interface(const char* profile_id)
{
    BT_LOGD("%s: id = %s", __func__, profile_id);

//     /* sanity check */
//     if (!interface_ready())
//         return NULL;
// #if defined(CONFIG_BLUETOOTH_LE_SCAN) || (CONFIG_BLUETOOTH_LE_ADVERTISE) || (CONFIG_BLUETOOTH_GATT_CLIENT) || (CONFIG_BLUETOOTH_GATT_SERVER)
//     if (is_profile(profile_id, BT_PROFILE_GATT))
//         return gatt_get_interface();
// #endif
// #ifdef CONFIG_BLUETOOTH_HFP_HF
//     if (is_profile(profile_id, BT_PROFILE_HANDSFREE_HF))
//         return (const void *)get_hf_client_service_interface();
// #endif
// #ifdef CONFIG_BLUETOOTH_SPP
//     if (is_profile(profile_id, BT_PROFILE_SPP))
//         return (const void *)get_spp_service_interface();
// #endif
    return NULL;
}

typedef enum {
    THREAD_ID_SERVICE,
    THREAD_ID_STACK,
} thread_id_t;

static uv_thread_t thread_handle[2];

static void* stack_schedule_loop(void* data)
{
    BT_LOGD("%s", __func__);
    ScheduleLoop();
    BT_LOGD("%s", __func__);

}

static void* service_schedule_loop(void* data)
{
    BT_LOGD("%s", __func__);
    service_loop_init();
}

bt_result_code bts_service_init(bt_service_callbacks* callbacks)
{
    bluetooth_upper_callbacks = callbacks;

    if(service_state != BT_MANAGER_STATE_OFF) 
        return BT_RESULT_FAILED;
    InitTransportLayer();
    gap_service_init();
    int ret = uv_thread_create(&thread_handle[THREAD_ID_STACK], stack_schedule_loop, NULL);
    if (ret != 0) {
        BT_LOGE("fail uv_thread_create, ret:%d", ret);
        return BT_RESULT_FAILED;
    }

    ret = uv_thread_create(&thread_handle[THREAD_ID_SERVICE], service_schedule_loop, NULL);
    if (ret != 0) {
        BT_LOGE("fail uv_thread_create, ret:%d", ret);
        return BT_RESULT_FAILED;
    }
#if defined(CONFIG_BLUETOOTH_LE_SCAN) || (CONFIG_BLUETOOTH_LE_ADVERTISE) || (CONFIG_BLUETOOTH_GATT_CLIENT) || (CONFIG_BLUETOOTH_GATT_SERVER)
    gap_service_init();
    gatt_interface_t* gatt_if = gatt_get_interface();
    if (gatt_if) {
        BT_LOGD("gatt init");
        gatt_if->init();
    }
#endif
    service_state = BT_MANAGER_STATE_TURNING_ON;
    return BT_RESULT_SUCCESS;
}



bt_result_code bts_service_enable()
{
    gap_enable();
#ifdef CONFIG_BLUETOOTH_HFP_HF
    hf_client_service_start();
#endif
#ifdef CONFIG_BLUETOOTH_SPP
    spp_service_start();
#endif
    return BT_RESULT_SUCCESS;
}


bt_result_code bts_service_disable()
{
    return BT_RESULT_SUCCESS;
}

void bts_service_cleanup(void)
{
#if defined(CONFIG_BLUETOOTH_LE_SCAN) || (CONFIG_BLUETOOTH_LE_ADVERTISE) || (CONFIG_BLUETOOTH_GATT_CLIENT) || (CONFIG_BLUETOOTH_GATT_SERVER)
    gatt_interface_t* gatt_if = gatt_get_interface();
    if (!gatt_if) {
        BT_LOGD("gatt cleanup");
        gatt_if->cleanup();
    }
#endif
}


void stack_state_change(stack_state_t state)
{
    bt_service_state service_state = BT_MANAGER_STATE_OFF;
    if (BT_STATE_ON == state){
        service_state = BT_MANAGER_STATE_ON;
    }
    bluetooth_upper_callbacks->adapter_state_changed_cb(service_state);
}