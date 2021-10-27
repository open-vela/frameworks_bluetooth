/****************************************************************************
 * frameworks/bluetooth/src/btmanager/btmanager.c
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
#include <stdlib.h>
#include "btm_manager.h"
#include "bts_service.h"


typedef struct{
    int response_id;
}common_response_t;

typedef struct{
    int resquest_id;
}common_request_t;

typedef struct{
    int message_type;
    union {
        common_response_t res;
        common_request_t req;
    }message;
}common_context_t;

//register process function for receiving data from service
bt_result_code  register_process_func_to_manager(void* handle, bt_profile_id profile_id, 
                                                btm_process_func func)
{
    bt_result_code result = BT_RESULT_FAILED;
    manager_context_t *context;
    if ((NULL == func) ||  ( profile_id >= BT_PROFILE_MAX_ID)){
        goto Exit;
    }
     
    if (NULL == handle){
        goto Exit;
    }
    context = (manager_context_t *)handle;
    context->manager_func_list[profile_id] = func;
    result = BT_RESULT_SUCCESS;
Exit:
    return result;
}

bt_result_code unregister_process_func_to_manager(void* handle, bt_profile_id profile_id)
{
    bt_result_code result = BT_RESULT_FAILED;
    manager_context_t *context;
    if (profile_id >= BT_PROFILE_MAX_ID)
    {
        goto Exit;
    } 
    if (NULL == handle){
        goto Exit;
    }
    context = (manager_context_t *)handle;
    context->manager_func_list[profile_id] = NULL;
    result = BT_RESULT_SUCCESS;
Exit:
    return result;
}

manager_context_t * pcontext;
manager_context_t* btm_context_init(manager_context_t * context)
{
    //TODO init context for RPC 
    return NULL;
}

bt_result_code bt_manager_init(void *p)
{
    bt_result_code result = BT_RESULT_FAILED;

    pcontext = malloc(sizeof(manager_context_t));
    pcontext->callback = malloc(sizeof(bt_callbacks_t));
#ifdef CONFIG_BLUETOOTH_LOCAL_THREAD
    //call service init
    bts_service_init();
    bts_service_get_interface(pcontext);
#else 

#endif

    result = BT_RESULT_WAITING_FOR_INIT_STATUS_CHANGED;
    return result;
}

void *bt_manager_thread_start(void *arg)
{
    bt_manager_init(NULL);

    return NULL;
}

bt_result_code bt_manager_deinit(void * handle)
{
    bt_result_code result = BT_RESULT_FAILED;
    
    return result;
}
//regeister callback to app for state changed event
bt_result_code  bt_register_callback(void * handle, bt_callbacks_t *bt_callbacks_cb)
{
    bt_result_code result = BT_RESULT_FAILED;
    manager_context_t *context;
    if (NULL != handle){
        context = (manager_context_t *)handle;
    }
    if (NULL != context->callback){
        result = BT_RESULT_CALLBACK_ALREADY_EXSIT;
        goto Exit;
    }
    if (NULL == bt_callbacks_cb){
        result = BT_RESULT_PARAMETER_ERROR;
        goto Exit;
    }
    context->callback = bt_callbacks_cb; 
    result = BT_RESULT_SUCCESS;
Exit:
    return result;
}

bt_result_code bt_manager_enable(void * handle)
{
    bt_result_code result = BT_RESULT_FAILED;
    manager_context_t *context;
    if (NULL == handle){
        goto Exit;
    }
    context = (manager_context_t *)handle;
Exit:
    return result;
}

bt_result_code bt_manager_disable(void * handle)
{
    //size_t size = 0;
    //char *buffer = NULL;
    bt_result_code result = BT_RESULT_FAILED;
    manager_context_t *context;
    if (NULL == handle){
        goto Exit;
    }
    context = (manager_context_t *)handle;

Exit:
    return result;
}

bool bt_manager_is_enable(void * handle)
{
    bool isEnable = false;

    return isEnable;
}

bt_manager_bt_state bt_get_state(void * handle)
{
    //size_t size = 0;
    //char *buffer = NULL;
    bt_result_code result = BT_RESULT_FAILED;
    manager_context_t *context;
    if (NULL == handle){
        goto Exit;
    }
    context = (manager_context_t *)handle;
Exit:
    return result;
}

bt_manager_ble_state ble_get_state(void * handle)
{
    bt_result_code result = BT_RESULT_FAILED;

    return result;
}

void * bt_get_profile_interface(void * handle, const bt_profile_id profile_id)
{
    void *result = NULL;

    switch (profile_id)
    {
    case BT_PROFILE_ADVANCED_AUDIO_SOURCE_ID:
        {
            #ifndef BTMANAGER_A2DP_SRC
            result = NULL;
            #endif
            //TODO add A2DP SRC interface
        }
        break;
    case BT_PROFILE_GATT_ID: {

        break;
    }
    default:
        break;
    }

    return result;
}


int main1(int argc, FAR char *argv[])
{
    //char  input;
    printf("btmanager main  coming in  \n");

#ifdef CONFIG_BLUETOOTH_LOCAL_THREAD
    pthread_t message_tid;
    pthread_attr_t message_attr;
    pthread_attr_init(&message_attr);
    pthread_create(&message_tid, &message_attr, bt_manager_thread_start, NULL);
#endif

    //test code
    while(1)
    {
        sleep(10000);
        #if 0
        printf("please input test inferface\n");
        scanf("%c", &input);
        printf("input is %c\n", input);

        switch (input)
        {
        case 'g':
            bt_get_state(pcontext);
            break;
        case 'e':
            return ;
        default:
            break;
        }
        #endif
    /* code */
    }
 
    return 0;
}

static bluetooth_service_interface* bluetooth_service = NULL;

static bt_callbacks_t* bluetooth_upper_callbacks = NULL;

static void bt_mgr_adapter_state_changed_callback(bt_state_t state)
{
    if (!bluetooth_upper_callbacks) {
        BT_LOGE("fail, bluetooth_upper_callbacks nullptr");
        return;
    }
    bluetooth_upper_callbacks->bt_manager_state_changed_callback_cb(state);
}

static const bt_callbacks bluetooth_lower_callbacks = {
    .size = sizeof(bluetooth_lower_callbacks),
    .adapter_state_changed_cb = bt_mgr_adapter_state_changed_callback,
};

static bt_result_code init(const bt_callbacks_t* callbacks)
{
    bluetooth_upper_callbacks = callbacks;
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service nullptr");
        return BT_RESULT_FAILED;
    }
    bt_result_code ret = bluetooth_service->init(&bluetooth_lower_callbacks);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, bluetooth_service init fail");
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code enable()
{
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return BT_RESULT_FAILED;
    }
    return bluetooth_service->enable();
}

static bt_result_code disable()
{
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return BT_RESULT_FAILED;
    }
    return bluetooth_service->disable();
}

static void cleanup(void)
{
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return;
    }
    bluetooth_service->cleanup();
}

static const void* get_profile_interface(const char* profile_id)
{
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service nullptr");
        return NULL;
    }
    return bluetooth_service->get_profile_interface(profile_id);
}

static btm_interface_t bluetooth_manager = {
    .size = sizeof(btm_interface_t),

    .init = init,
    .enable = enable,
    .disable = disable,
    .cleanup = cleanup,

    .get_profile_interface = get_profile_interface,
};

btm_interface_t* get_bt_manager_interface(void)
{
    bluetooth_service = get_bluetooth_service_interface();
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return NULL;
    }
    return &bluetooth_manager;
}