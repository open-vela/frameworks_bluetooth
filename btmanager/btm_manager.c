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
#define LOG_TAG "btm_manager"

#include "btm_manager.h"
#include "bts_service.h"
#include "bts_service_interface.h"
#include <stdio.h>
#include <stdlib.h>

#include "log.h"

typedef struct {
    size_t size;
    bt_mgr_callback_t* callback;
} manager_context_t;

static bluetooth_service_interface* bluetooth_service = NULL;

static manager_context_t* btm_context_init(void)
{
    manager_context_t* init_context = NULL;
    init_context = (manager_context_t*)malloc(sizeof(manager_context_t));
    if (!init_context) {
        BT_LOGE("context init malloc failed!");
        goto Exit;
    }
    init_context->callback = (bt_mgr_callback_t*)malloc(sizeof(bt_mgr_callback_t));
    if (!init_context->callback) {
        BT_LOGE("context init malloc failed!");
        free(init_context);
        init_context = NULL;
    }
Exit:
    return init_context;
}

static bt_manager_bt_state bt_get_state(void* handle)
{
    //size_t size = 0;
    //char *buffer = NULL;
    bt_result_code result = BT_RESULT_FAILED;
    if (!handle) {
        goto Exit;
    }
    //TODO
Exit:
    return result;
}

static bt_manager_ble_state ble_get_state(void* handle)
{
    bt_result_code result = BT_RESULT_FAILED;
    //TODO

    return result;
}

static void bt_mgr_adapter_state_changed_callback(void* handle, stack_state_t state)
{
    manager_context_t* context;
    if (NULL == handle)
        return ;
    context = (manager_context_t*)handle;

    bt_mgr_callback_t* bluetooth_upper_callbacks = context->callback;
    if (!bluetooth_upper_callbacks) {
        BT_LOGE("fail, bluetooth_upper_callbacks nullptr");
        return ;
    }
    bluetooth_upper_callbacks->bt_manager_state_changed_callback_cb(state);
}

static void bt_mgr_adapter_ble_state_changed_callback(void* handle, bt_manager_ble_state state)
{
    manager_context_t* context;
    if (NULL == handle)
        return;
    context = (manager_context_t*)handle;

    bt_mgr_callback_t* bluetooth_upper_callbacks = context->callback;
    if (!bluetooth_upper_callbacks) {
        BT_LOGE("fail, bluetooth_upper_callbacks nullptr");
        return ;
    }
    bluetooth_upper_callbacks->bt_manager_ble_state_changed_callback_cb(state);
}

static bt_service_if_callbacks bluetooth_lower_callbacks = {
    .size = sizeof(bluetooth_lower_callbacks),
    .adapter_state_changed_cb = bt_mgr_adapter_state_changed_callback,
    .adapter_state_ble_changed_cb = bt_mgr_adapter_ble_state_changed_callback,
};

static bt_result_code bt_mgr_init(void** handle, const bt_mgr_callback_t* callbacks)
{
    bt_result_code ret = BT_RESULT_FAILED;
    manager_context_t* context = NULL;
    *handle = btm_context_init();
    context = *handle;
    if (!*handle) {
        goto Exit;
    }

    ret = bluetooth_service->init(context, &bluetooth_lower_callbacks);
    context->callback = callbacks;
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, bluetooth_service init fail");
        goto Exit;
    }
Exit:
    return ret;
}

static bt_result_code bt_mgr_enable(void* handle)
{
    if (!handle) {
        BT_LOGE("%s, handle is NULL", __func__);
        return BT_RESULT_FAILED;
    }
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return BT_RESULT_FAILED;
    }
    return bluetooth_service->enable(handle);
}

static bt_result_code bt_mgr_disable(void* handle)
{
    if (!handle) {
        BT_LOGE("%s, handle is NULL", __func__);
        return BT_RESULT_FAILED;
    }

    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return BT_RESULT_FAILED;
    }
    return bluetooth_service->disable(handle);
}

static bt_result_code bt_mgr_enable_ble(void* handle)
{
    if (!handle) {
        BT_LOGE("%s, handle is NULL", __func__);
        return BT_RESULT_FAILED;
    }

    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return BT_RESULT_FAILED;
    }
    return bluetooth_service->enable(handle);
}

static bt_result_code bt_mgr_disable_ble(void* handle)
{
    if (!handle) {
        BT_LOGE("%s, handle is NULL", __func__);
        return BT_RESULT_FAILED;
    }
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return BT_RESULT_FAILED;
    }
    return bluetooth_service->disable(handle);
}

static void bt_mgr_cleanup(void* handle)
{
    if (!handle) {
        BT_LOGE("%s, handle is NULL", __func__);
        return ;
    }
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return ;
    }
    bluetooth_service->cleanup(handle);
}

static const void* get_profile_interface(const char* profile_id)
{
    if (!bluetooth_service) {
        BT_LOGE("fail, bluetooth_service null");
        return NULL;
    }
    return bluetooth_service->get_profile_interface(profile_id);
}

static btm_interface_t bluetooth_manager = {
    .size = sizeof(btm_interface_t),

    .init = bt_mgr_init,
    .enable = bt_mgr_enable,
    .disable = bt_mgr_disable,
    .enable_ble = bt_mgr_enable_ble,
    .disable_ble = bt_mgr_disable_ble,
    .cleanup = bt_mgr_cleanup,
    .bt_get_state = bt_get_state,
    .ble_get_state = ble_get_state,
    .get_profile_interface = get_profile_interface,
};

btm_interface_t* get_bt_manager_interface(void)
{
    if (!bluetooth_service) {
        bluetooth_service = get_bluetooth_service_interface();
    }
    return &bluetooth_manager;
}