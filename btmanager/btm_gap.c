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

#define BT_GAP_INTERFACE(P_IF, MOTHOD,RET, ...)                           \
    do {                                                       \
        if ((P_IF) && (P_IF)->MOTHOD) {                       \
            BT_LOGD("%s: GAP %s->%s", __func__, #P_IF, #MOTHOD); \
            RET = (P_IF)->MOTHOD(__VA_ARGS__);                      \
        } else {                                               \
            BT_LOGE("%s GAP interface is NULL", __func__);            \
        }                                                      \
    } while (0)

typedef struct {
    void *manager_context;
    btm_gap_callbacks_t *gap_callbacks;
    btm_gap_interface_t* service_interface;
}gap_context_t;


static void adapter_state_changed_callback(void* gap_handle, stack_state_t state)
{
    bt_result_code ret = BT_RESULT_FAILED;
    BT_LOGD("%s", __func__);

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
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->connection_state_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connection_state_callback_cb(gap_handle, device, state);
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
    if((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->connection_state_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connection_state_callback_cb(gap_handle, device, state);   
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
    .connection_state_callback_cb = btm_connected_state_callback,
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
    context->service_interface->gap_register_callbacks(manager_handle, context, &service_callbacks);
}

bt_result_code btm_gap_cleanup(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, gap_cleanup, ret, gap_handle);
    return ret;
}

bt_result_code btm_set_local_address(void * gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_address, ret, gap_handle, device);
    return ret;
}
bt_address* btm_get_local_address(void * gap_handle)
{
    bt_address* addr;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_local_address, addr, gap_handle);
    return addr;
}

bt_result_code btm_set_local_io_capability(void * gap_handle, bt_io_capability io_capability){
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_io_capability, ret, gap_handle, io_capability);
    return ret;
}
bt_result_code btm_set_local_name(void * gap_handle, char *bt_name, uint8_t len)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_name, ret, gap_handle, bt_name, len);
    return ret;
}
char* btm_get_local_name(void * gap_handle)
{
    char* ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_local_name, ret, gap_handle);
    return ret;
}
bt_result_code btm_set_local_device_class(void * gap_handle, uint32_t class_of_device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_device_class, ret, gap_handle, class_of_device);
    return ret;
}
bt_result_code btm_get_local_device_class(void * gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_local_device_class, ret, gap_handle);
    return ret;
}

/*Remote device*/
bt_result_code btm_get_remote_name(void * gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_remote_name, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_get_connection_state(void * gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_connection_state, ret, gap_handle, device);
    return ret;
}

/*Bond*/
bt_bond_state btm_get_bond_state(void * gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_bond_state, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_reply_pair_request(void * gap_handle, bt_device_t* device, bool accept)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_reply_pair_request, ret, gap_handle, device, accept);
    return ret;
}
bt_result_code btm_create_bond(void * gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_create_bond, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_cancel_bond(void * gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_cancel_bond, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_remove_bond(void * gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_remove_bond, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_get_bonded_devices(void * gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_bonded_devices, ret, gap_handle);
    return ret;
}

/*Connection*/
// bt_result_code btm_connect_all(void * gap_handle, bt_device_t* device)
// {
//     bt_result_code ret = BT_RESULT_FAILED;
//     CHECK_PTR(gap_handle);
//     gap_context_t * context = (gap_context_t*)gap_handle;
//     BT_GAP_INTERFACE(context->service_interface, bt_connect_all, ret, gap_handle, device);
//     return ret;
// }
// bt_result_code btm_disconnect_all(void * gap_handle, bt_device_t* device)
// {
//     bt_result_code ret = BT_RESULT_FAILED;
//     CHECK_PTR(gap_handle);
//     gap_context_t * context = (gap_context_t*)gap_handle;
//     BT_GAP_INTERFACE(context->service_interface, bt_disconnect_all, ret, gap_handle, device);
//     return ret;
// }
bt_result_code btm_get_connected_devices(void * gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_connected_devices, ret, gap_handle);
    return ret;
}

/*Discovery*/
bt_result_code btm_set_scan_mode(void * gap_handle, bt_scan_mode scan_mode, bool bondable)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_scan_mode, ret, gap_handle, scan_mode, bondable);
    return ret;
}
//bt_result_code btGapSetLinkMode(bt_address remote_addr, BTLinkMode link_mode);
bt_result_code btm_start_discovery(void * gap_handle, uint32_t timeout)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_start_discovery, ret, gap_handle, timeout);
    return ret;
}
bt_result_code btm_stop_discovery(void * gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_stop_discovery, ret, gap_handle);
    return ret;
}

/*VSC command*/
bt_result_code btm_send_hci_command_v1(void * gap_handle, bt_hci_command_t *command, bt_service_hci_command_complete_event event_type)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_send_hci_command_v1, ret, gap_handle, command, event_type);
    return ret;
}

/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code btm_send_hci_command(void * gap_handle, uint8_t *hci_cmd_packet, hci_event_callback cb)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR(gap_handle);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_send_hci_command, ret, gap_handle, hci_cmd_packet, cb);
    return ret;
}

int  btm_get_remote_services(void* gap_handle, bt_device_t* remote_addr, bt_uuid_t *service_list, uint8_t count_in)
{
    int ret = -1;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_remote_services, ret, gap_handle, remote_addr, service_list, count_in);
    return ret;
}

/*service discovery*/
bt_result_code btm_start_service_discovery(void* gap_handle, bt_device_t* device, bt_uuid_t uuid)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_start_service_discovery, ret, gap_handle, device, uuid);
    return ret;
}

bt_result_code btm_stop_service_discovery(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_stop_service_discovery, ret, gap_handle, device);
    return ret;
}


bt_result_code btm_ble_set_static_identity(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_static_identity, ret, gap_handle, device);
    return ret;
}

bt_result_code btm_ble_get_current_irk(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_current_irk, ret, gap_handle);
    return ret;
}

bt_result_code btm_ble_set_address(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_address, ret, gap_handle, device);
    return ret;
}

bt_result_code btm_ble_get_address(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_address, ret, gap_handle);
    return ret;
}
bt_result_code btm_ble_set_bonded_devices(void* gap_handle, ble_keys_t *bonded_device_list, uint8_t count_in)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_bonded_devices, ret, gap_handle, bonded_device_list, count_in);
    return ret;
}
bt_result_code btm_ble_connect(void* gap_handle, ble_connect_params_t *conn_param)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_connect, ret, gap_handle, conn_param);
    return ret;
}
bt_result_code btm_ble_disconnect(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_disconnect, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_ble_smp_reply(void* gap_handle, spp_reply_data_t *reply_data)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_smp_reply, ret, gap_handle, reply_data);
    return ret;
}
bt_result_code btm_ble_add_white_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_add_white_list, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_ble_remove_white_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_remove_white_list, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_ble_add_resolving_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_add_resolving_list, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_ble_remove_resolving_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_remove_resolving_list, ret, gap_handle, device);
    return ret;
}
bt_result_code btm_ble_set_phy(void* gap_handle, bt_device_t* device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_phy, ret, gap_handle, device, tx_phy, rx_phy);
    return ret;
}
bt_result_code btm_ble_add_private_channel(void* gap_handle, uint16_t private_cid)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_add_private_channel, ret, gap_handle, private_cid);
    return ret;
}
bt_result_code btm_ble_send_packet(void* gap_handle, bt_device_t* device, uint16_t private_cid, uint8_t *packet, uint16_t packet_size)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_send_packet, ret, gap_handle, device, private_cid, packet, packet_size);
    return ret;
}
    
/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code btm_enter_bluetooth_test_mode(void* gap_handle, bt_test_mode test_mode)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t * context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, enter_bluetooth_test_mode, ret, gap_handle, test_mode);
    return ret;
}

const btm_gap_interface_t gap_interface = {
.size = sizeof(btm_gap_interface_t),
    .gap_register_callbacks = btm_gap_register_callbacks,
    .gap_cleanup = btm_gap_cleanup,
    .bt_start_discovery = btm_start_discovery,
    .bt_set_local_address = btm_set_local_address,
    .bt_get_local_address = btm_get_local_address,
    .bt_set_local_io_capability = btm_set_local_io_capability,
    .bt_set_local_name  = btm_set_local_name,
    .bt_get_local_name = btm_get_local_name,
    .bt_set_local_device_class = btm_set_local_device_class,
    .bt_get_local_device_class = btm_get_local_device_class,
    .bt_get_remote_name = btm_get_remote_name,
    .bt_get_connection_state = btm_get_connection_state,
    .bt_get_remote_services = btm_get_remote_services,
    .bt_get_bond_state = btm_get_bond_state,
    .bt_reply_pair_request = btm_reply_pair_request,
    .bt_create_bond = btm_create_bond,
    .bt_cancel_bond = btm_cancel_bond,
    .bt_remove_bond = btm_remove_bond,
    .bt_get_bonded_devices = btm_get_bonded_devices,
    // .bt_connect_all = btm_connect_all,
    // .bt_disconnect_all = btm_disconnect_all,
    .bt_get_connected_devices = btm_get_connected_devices,
    .bt_set_scan_mode = btm_set_scan_mode,
    .bt_start_discovery = btm_start_discovery,
    .bt_stop_discovery = btm_stop_discovery,
    .bt_start_service_discovery = btm_start_service_discovery,
    .bt_stop_service_discovery = btm_stop_service_discovery,
    .bt_send_hci_command_v1 = btm_send_hci_command_v1,
    .ble_set_static_identity = btm_ble_set_static_identity,
    .ble_get_current_irk = btm_ble_get_current_irk,
    .ble_set_address = btm_ble_set_address,
    .ble_get_address = btm_ble_get_address,
    .ble_set_bonded_devices = btm_ble_set_bonded_devices,
    .ble_connect = btm_ble_connect,
    .ble_disconnect = btm_ble_disconnect,
    .ble_smp_reply = btm_ble_smp_reply,
    .ble_add_white_list = btm_ble_add_white_list,
    .ble_remove_white_list = btm_ble_remove_white_list,
    .ble_add_resolving_list = btm_ble_add_resolving_list,
    .ble_remove_resolving_list = btm_ble_remove_resolving_list,
    .ble_set_phy = btm_ble_set_phy,
    .ble_add_private_channel = btm_ble_add_private_channel,
    .ble_send_packet = btm_ble_send_packet,
    .bt_send_hci_command = btm_send_hci_command,
    .enter_bluetooth_test_mode = btm_enter_bluetooth_test_mode,
};

btm_gap_interface_t* get_gap_instance(void)
{
    return &gap_interface;
}
