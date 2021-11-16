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
    btm_gap_interface_t* service_interface;
}gap_context_t;


static void adapter_state_changed_callback(void* gap_handle, stack_state_t state)
{
    bt_result_code ret = BT_RESULT_FAILED;
    BT_LOGD("%s", __func__);

    // if (!gap_handle)
    //     return ret;
    // gap_context_t * context = (gap_context_t*)gap_handle;
    // if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->state_changed_cb))
    //     return ret;
    // BT_LOGD("%s", __func__);
    // BT_CBACK(context->gap_callbacks, state_changed_cb, state);
    //context->gap_callbacks->state_changed_cb(state);
}
void init_done_callback(void* gap_handle)
{
    BT_LOGD("%s", __func__);

}
void btm_remote_name_callback(void* gap_handle, bt_address bd_addr, char *bt_name, uint8_t length)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->received_remote_name_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->received_remote_name_callback_cb(gap_handle, bd_addr, bt_name, length);
}

void btm_discovery_state_changed_callback(void* gap_handle, bt_discovery_state state)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->discovery_state_changed_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->discovery_state_changed_callback_cb(gap_handle, state);
}

void btm_ssp_request_callback(void* gap_handle, bt_ssp_request_data_t *request_data)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ssp_request_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->ssp_request_callback_cb(gap_handle, request_data);
}
void btm_device_found_callback(void* gap_handle, bt_device_t* device)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->device_found_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->device_found_callback_cb(gap_handle, device);
}
void btm_bond_state_changed_callback(void* gap_handle, bt_device_t* device, bt_bond_state state)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->bond_state_changed_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->bond_state_changed_callback_cb(gap_handle, device, state);
}
void btm_connected_state_callback(void* gap_handle, bt_device_t* device, bt_connection_state state)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->connected_state_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connected_state_callback_cb(gap_handle, device, state);
}
void btm_get_bonded_device_list_callback(void* gap_handle, bt_address*bonded_device_list, uint8_t number)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->get_bonded_device_list_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connected_device_list_callback_cb(gap_handle, bonded_device_list, number);
}
void btm_connected_device_list_callback(void* gap_handle, bt_address*connected_device_list, uint8_t number)
{
    if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->connected_device_list_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connected_device_list_callback_cb(gap_handle, connected_device_list, number);
}
void btm_hci_event_callback(void* gap_handle, bt_hci_event_t *hci_event)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->hci_event_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->hci_event_callback_cb(gap_handle, hci_event);   
}
void btm_connection_state_changed_callback(void* gap_handle, bt_device_t* device, bt_connection_state state)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->connected_state_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connected_state_callback_cb(gap_handle, device, state);   
}
void btm_local_name_callback(void* gap_handle, char *bt_name, uint8_t length)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->local_name_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->local_name_callback_cb(gap_handle, bt_name, length);   
}
void btm_local_device_class_callback(void* gap_handle, uint32_t device_class)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->local_device_class_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->local_device_class_callback_cb(gap_handle, device_class);   
}
void btm_local_address_callback(void* gap_handle, bt_device_t* device)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->local_address_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->local_address_callback_cb(gap_handle, device);   
}

void btm_smp_request_callback(void* gap_handle, ssp_request_data_t *request_data)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->smp_requeset_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->smp_requeset_cb(gap_handle, request_data);   
}

void btm_ble_phy_update_callback(void* gap_handle, bd_addr_t remote_addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, bt_status status)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_phy_update_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->ble_phy_update_cb(gap_handle, remote_addr, tx_phy, rx_phy, status);   
}

void btm_ble_address_callback(void* gap_handle, bd_addr_t ble_addr, ble_addr_type ble_addr_type)
{
     if (!gap_handle)
        return;
    gap_context_t * context = (gap_context_t*)gap_handle;
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_address_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->ble_address_cb(gap_handle,ble_addr, ble_addr_type);   
}

const btm_gap_callbacks_t service_callbacks = {
    .size = sizeof(btm_gap_callbacks_t),
    .state_changed_cb = adapter_state_changed_callback,
    .bt_connection_state_changed_callback_cb  = btm_connection_state_changed_callback,
    .received_remote_name_callback_cb = btm_remote_name_callback,
    .discovery_state_changed_callback_cb = btm_discovery_state_changed_callback,
    .ssp_request_callback_cb = btm_ssp_request_callback,
    .bond_state_changed_callback_cb = btm_bond_state_changed_callback,
    .hci_event_callback_cb = btm_hci_event_callback,
    .get_bonded_device_list_callback_cb = btm_get_bonded_device_list_callback,
    .local_name_callback_cb = btm_local_name_callback,
    .local_device_class_callback_cb = btm_local_device_class_callback,
    .connected_state_callback_cb = btm_connected_state_callback,
    .connected_device_list_callback_cb = btm_connected_device_list_callback,
    .local_address_callback_cb = btm_local_address_callback,
    .device_found_callback_cb = btm_device_found_callback,
    .smp_requeset_cb = btm_smp_request_callback,
    .ble_phy_update_cb = btm_ble_phy_update_callback,
    .ble_address_cb = btm_ble_address_callback,
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
    context->service_interface->register_callbacks(manager_handle, context, &service_callbacks);
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
    ret = context->service_interface->set_name(handle, bt_name, len);
    return ret;
}

bt_result_code btm_stop_discovery(void * handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!handle)
        return ret;
    gap_context_t * context = (gap_context_t*)handle;
    if(!context->service_interface)
        return ret;
    ret = context->service_interface->stop_discovery(handle);
    return ret;
}


const btm_gap_interface_t gap_interface = {
    .register_callbacks = btm_gap_register_callbacks,
    .start_discovery = btm_start_discovery,
    .set_name = btm_set_local_name,
    .stop_discovery = btm_stop_discovery,
};

btm_gap_interface_t* get_gap_instance(void)
{
    return &gap_interface;
}
