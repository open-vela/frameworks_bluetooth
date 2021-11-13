/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_gap_service.c
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
#include <stdint.h>
#include <stdlib.h>

#include "btdatatype.h"
#include "global.h"
#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"

#include "btm_manager.h"
#include "bts_service.h"
#include "bts_gap.h"
#include "btm_gap.h"

#define LOG_TAG "bts_gap_service"
#include "log.h"


bts_service_gap_callbacks_t* btm_callbacks;
void *test_handle = NULL;
static void gap_if_init_done_callback()
{

}
static void gap_if_received_remote_name_callback(BD_ADDR bd_addr, char *bt_name, uint8_t length)
{

}
static void gap_if_discovery_state_changed_callback(bt_discovery_state state)
{
    
}
static void gap_if_ssp_request_callback(bt_ssp_request_data_t *request_data)
{
    
}
static void gap_if_device_found_callback(bt_device_t device)
{
    
}
static void gap_if_bond_state_changed_callback(bt_device_t device, bt_bond_state state)
{
    
}
static void gap_if_connection_state_callback(bt_device_t device, bt_connection_state state)
{
    
}
static void gap_if_get_bonded_device_list_callback(bt_address*bonded_device_list, uint8_t umber)
{
    
}

static void gap_if_connected_device_list_callback(bt_address*connected_device_list, uint8_t umber)
{
    
}

static void gap_if_hci_event_callback(bt_hci_event_t *hci_event)
{
    
}

void gap_if_adapter_state_changed_callback(stack_state_t state)
{
    BT_LOGD("%s", __func__);
    if ((NULL == btm_callbacks) || (NULL == test_handle)){
        BT_LOGE("%s, callback is NULL", __func__);
        return;
    }
    btm_callbacks->adapter_state_changed(test_handle, state);
}

bts_gap_callback_t bts_gap_callbacks = {
    .adapter_state_changed_cb = gap_if_adapter_state_changed_callback,
    .gap_init_done_cb = gap_if_init_done_callback,
    .remote_name_cb = gap_if_received_remote_name_callback,
    .discovery_state_changed_cb = gap_if_discovery_state_changed_callback,
    .spp_request_cb = gap_if_ssp_request_callback,
    .device_found_cb = gap_if_device_found_callback,
    .bond_state_changed_cb = gap_if_bond_state_changed_callback,
    .connection_state_changed_cb = gap_if_connection_state_callback,
    .bond_state_changed_cb = gap_if_bond_state_changed_callback,
    .connected_list_cb = gap_if_connected_device_list_callback,
    .hci_event_cb = gap_if_hci_event_callback,
};

bt_result_code gap_service_init()
{
    gap_init(&bts_gap_callbacks);
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_if_register_callbacks(void* gap_handle, const bts_service_gap_callbacks_t* callbacks)

{
    BT_LOGD("%s", __func__);
    test_handle = gap_handle;
    btm_callbacks = callbacks;
}

bt_result_code bts_if_start_discovery(void* gap_handle, uint32_t timeout)
{
    bts_start_discovery(timeout);    
}
bt_result_code bts_if_set_local_name(void* gap_handle, char *bt_name, uint8_t len)
{
    bts_set_local_name(bt_name, len);
}


static gap_service_interface_t gap_interface = {
    .size = sizeof(gap_service_interface_t),
    .register_callbacks = bts_if_register_callbacks,
    .start_discovery = bts_if_start_discovery,
    .set_local_name  = bts_if_set_local_name,
};

gap_service_interface_t* get_gap_service_instance(void)
{
    return &gap_interface;
}