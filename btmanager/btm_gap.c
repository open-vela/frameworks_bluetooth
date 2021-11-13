/****************************************************************************
 * frameworks/blluetooth/src/btmanager/gap.c
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
#include "btm_manager.h"
#include "bts_gap_service.h"
#include "btm_gap.h"
#include "bts_service.h"

#define LOG_TAG "btm_gap"
#include "log.h"

typedef struct {
    void *manager_context;
    btm_gap_callbacks_t *gap_callbacks;
    gap_service_interface_t* service_interface;
}gap_context_t;


void btm_discovery_state_changed_callback(void* gap_handle, bt_discovery_state state)
{

}

void btm_remote_name_callback(void* gap_handle, bt_address bd_addr, char *bt_name, uint8_t length)
{

}

static void adapter_state_changed_callback(void* gap_handle, stack_state_t state)
{
    bt_result_code ret = BT_RESULT_FAILED;
    BT_LOGD("%s", __func__);

    if (!gap_handle)
        return ret;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->state_changed_cb))
        return ret;
    BT_LOGD("%s", __func__);
    BT_CBACK(context->gap_callbacks, state_changed_cb, state);
    //context->gap_callbacks->state_changed_cb(state);
}

const bts_service_gap_callbacks_t service_callbacks = {
    .size = sizeof(bts_service_gap_callbacks_t),
    .adapter_state_changed = adapter_state_changed_callback,
};


bt_result_code btm_gap_register_callbacks(void * manager_handle, void ** gap_handle, const btm_gap_callbacks_t* callbacks)
{
    bt_result_code ret = BT_RESULT_FAILED;
    gap_context_t * context = malloc(sizeof(gap_context_t));
    if (!context)
      return ret;
    *gap_handle = context;
    context->gap_callbacks = callbacks;
    context->manager_context = manager_handle;
    context->service_interface = get_gap_service_instance();
    context->service_interface->register_callbacks(context, &service_callbacks);
}

bt_result_code btm_start_discovery(void * handle, uint32_t timeout)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!handle)
        return ret;
    gap_context_t * context = (gap_context_t*)handle;
    if(!context->service_interface)
        return ret;
    ret = context->service_interface->start_discovery(handle, timeout);
    return ret;
}

bt_result_code btm_set_local_name(void * handle, char *bt_name, uint8_t len)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!handle)
        return ret;
    gap_context_t * context = (gap_context_t*)handle;
    if(!context->service_interface)
        return ret;
    ret = context->service_interface->set_local_name(handle, bt_name, len);
    return ret;
}


const btm_gap_interface_t gap_interface = {
    .register_callbacks = btm_gap_register_callbacks,
    .start_discovery = btm_start_discovery,
    .set_name = btm_set_local_name,

};

btm_gap_interface_t* get_gap_instance(void)
{
    return &gap_interface;
}
