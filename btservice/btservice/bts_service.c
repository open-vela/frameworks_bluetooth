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
#include "uv.h"
#include "stack_adapter_service_base.h"
#include "stack_adapter_gap.h"

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

SERVICE_BT_STACK_STATE service_state = BT_STATE_OFF;
bt_init_state init_state = BT_STATE_NOT_INITED;

uv_loop_t* dispatch_loop;
uv_idle_t idler;

bts_process_command_func_in_service service_func_list[BT_PROFILE_MAX_ID] = {NULL};

char* package_profile_buffer_to_manager(int command_id, void* command_buffer, int command_size, int * profile_size)
{
    bt_result_code result = BT_RESULT_FAILED;
    char * profile_buff = NULL;

    return profile_buff;
}

//register process for receiving data from manager
bt_result_code  register_process_func_to_service(bt_profile_id profile_id, bts_process_command_func_in_service func)
{
    bt_result_code result = BT_RESULT_FAILED;
    if ((NULL == func) ||  ( profile_id >= BT_PROFILE_MAX_ID))
    {
        goto Exit;
    }
    service_func_list[profile_id] = func;
    result = BT_RESULT_SUCCESS;
Exit:
    return result;
}

//register process for receiving data from manager
bt_result_code  unregister_process_func_to_service(bt_profile_id profile_id)
{
    bt_result_code result = BT_RESULT_FAILED;
    bts_process_command_func_in_service func = NULL;
    if ((NULL == func) ||  ( profile_id >= BT_PROFILE_MAX_ID))
    {
        goto Exit;
    }
    service_func_list[profile_id] = NULL;
    result = BT_RESULT_SUCCESS;
Exit:
    return result;
}

void bts_uv_close_cb(uv_handle_t* handle)
{
    printf("uv_close_cb\n");
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

//all the message will be porcess first time here, there is uv process here
void execute_manager_message(uv_async_t* handle)
{
    printf("execute_manager_message\n");

    if (NULL == handle)
    {
        printf("execute_manager_message handle is NULl \n");
        return;
    }
    excute_manager_context_t *context = NULL;
    context = (excute_manager_context_t*)uv_handle_get_data((uv_handle_t*)handle);
    if (NULL == context)
    {
        return;
    }

     process_data_from_manager(context->profile_id, context->data, context->data_size);
   
    uv_close(handle, bts_uv_close_cb);
    if (NULL != context) 
    {
        free(context);
    }

}

void execute_service_callback(uv_async_t* handle)
{
    printf("execute_manager_message\n");
      bts_process_loop_data func;

    if (NULL == handle)
    {
        printf("execute_manager_message handle is NULl \n");
        return;
    }
    excute_service_context_t *context = NULL;
    context = (excute_service_context_t*)uv_handle_get_data((uv_handle_t*)handle);
    if (NULL == context)
    {
        return;
    }

    func = context->loop_func;
    func(context->data, context->data_size);
    uv_close(handle, bts_uv_close_cb);
    if (NULL != context) 
    {
        free(context);
    }

}

void process_in_loop(excute_service_context_t *context) 
{
    uv_async_t *post_function_async ;
    uv_loop_t * loop = NULL;

    printf("process_in_loop  \n");

    if (NULL == dispatch_loop)
    {
        return;
    }
    post_function_async = malloc(sizeof(uv_async_t));
    loop = dispatch_loop;
    uv_async_init(loop, post_function_async, execute_service_callback);
    uv_handle_set_data((uv_handle_t*)post_function_async, context);
    uv_async_send(post_function_async);
}

void process_data_from_manager(int profile_id, char * buff, size_t size) 
{

    bt_result_code result = BT_RESULT_FAILED;
    bool status = false;
    bts_process_command_func_in_service func = NULL;
    printf(" process_data_from_manager size is %d \n", size);


    if ((NULL == buff)  ||(0 == size)){
        goto Exit;
    }
    
    printf("receive data ,profile Id is %d!\n", profile_id);
    func = service_func_list[profile_id];
    if (NULL == func){
        goto Exit;
    }

    //profile register function
    func(buff, size);

    free(buff);
Exit:
    return ;
}


void receive_data_from_manager(void * handle, bt_profile_id profile_id, char * buff, size_t size)
{
    bool status = false;
    bts_process_command_func_in_service func = NULL;
    uv_loop_t * loop = NULL;
    uv_async_t * post_function_async = NULL;
    printf(" receive_data_from_manager size is %d \n", size);

    if ((NULL == buff)  ||(0 == size)){
        goto Exit;
    }
    char *service_buff = malloc(size);
    memcpy(service_buff, buff,size);
    excute_manager_context_t *context = (excute_manager_context_t*)malloc(sizeof(excute_manager_context_t));
    context->data = service_buff;
    context->data_size = size;
    context->profile_id = profile_id;
    context->handle = handle;

    post_function_async = malloc(sizeof(uv_async_t));
    loop = dispatch_loop;
    uv_async_init(loop, post_function_async, execute_manager_message);
    uv_handle_set_data((uv_handle_t*)post_function_async, context);
    uv_async_send(post_function_async);
    
Exit:
    return;
}

void on_rpc_client_callback()
{
    //TODO fisrt call is initializing local stack,
    //and nedd to callback init status changed to clent

}

bt_result_code send_to_manager(send_buffer, size)
{
    bt_result_code result = BT_RESULT_FAILED;

    return result;
}


bt_result_code send_pb_command_buffer_to_manager(bt_profile_id profile_id, void* profile_buff, size_t profile_size)
{
    size_t size = 0;
    char *send_buffer = NULL;
    bt_result_code result = BT_RESULT_FAILED;

    if (send_buffer != NULL) {
        result = send_to_manager(send_buffer, size);
    }
Exit:
    return result;
}

void io_process(uv_work_t *req) 
{
    printf("btservice io_process \n");

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
    printf("btservice after_io_process \n");
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
    printf("Do timer_hadler_cb\n");
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
        return NULL;
    }
    uv_timer_stop(timer);
    uv_close(timer, bts_uv_close_cb);
    return &timer;
}

void idle_process(uv_idle_t* handle, int status) {
    //printf("btservice idle_process \n");
}

void stop_idle_process() {
    uv_idle_stop(&idler);
}

void process_in_loop_timer(char * data)
{

}

int loop_init()
{
    uv_loop_t _loop;
    pthread_t message_tid;
    pthread_attr_t message_attr;

    printf("btservice loop_init \n");
    uv_loop_init(&_loop);
    dispatch_loop = uv_default_loop();

    uv_idle_init(uv_default_loop(), &idler);
    //uv_idle_start(&idler, idle_process);
    start_timer(0, 200000, process_in_loop_timer, NULL);
    printf("Idling...\n");
    uv_run(dispatch_loop, UV_RUN_DEFAULT);

    //nerver touch here only if service is down
    printf("Idling done\n");
    uv_loop_close(uv_default_loop());
}

int bts_service_get_interface(void* handle)
{
    if (NULL == handle){

    }
    manager_context_t *context = (manager_context_t *)handle;
    context->app_id = 0;
}
extern bt_result_code spp_service_start(void);
bt_result_code hf_client_service_start(void);

int bts_service_init()
{
    init_state = BT_STATE_INITING;
    int i = 0;
    printf(" bt_service_init coming \n");
    bts_common_init();
    hf_client_service_start();
    spp_service_start();
    loop_init();
    printf(" bt_service_init done \n");
    return 0;
}

int btservice_main(int argc, FAR char *argv[])
{
    printf(" main coming in \n");
#ifdef CONFIG_BLUETOOTH_LOCAL_THREAD
    
#endif
    printf(" main coming end\n");
 
    return 0;
}
