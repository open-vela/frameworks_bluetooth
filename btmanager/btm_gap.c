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
#define LOG_TAG "btm_gap"

#include <stdio.h>
#include <stdlib.h>

#include "btm_gap.h"
#include "btm_manager.h"
#include "bts_gap_service.h"
#include "bts_service.h"

#include "log.h"

#define BT_GAP_INTERFACE(P_IF, MOTHOD, RET, ...)                 \
    do {                                                         \
        if ((P_IF) && (P_IF)->MOTHOD) {                          \
            BT_LOGD("%s: GAP %s->%s", __func__, #P_IF, #MOTHOD); \
            RET = (P_IF)->MOTHOD(__VA_ARGS__);                   \
        } else {                                                 \
            BT_LOGE("%s GAP interface is NULL", __func__);       \
        }                                                        \
    } while (0)

typedef struct {
    void* manager_context;
    const btm_gap_callbacks_t* gap_callbacks;
    btm_gap_interface_t* service_interface;
} gap_context_t;

static void btm_remote_name_callback(void* gap_handle, bt_address bd_addr, char* bt_name, uint8_t length)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->received_remote_name_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->received_remote_name_callback_cb(gap_handle, bd_addr, bt_name, length);
}

static void btm_discovery_state_changed_callback(void* gap_handle, bt_discovery_state state)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->discovery_state_changed_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->discovery_state_changed_callback_cb(gap_handle, state);
}

static void btm_ssp_request_callback(void* gap_handle, ssp_request_data_t* request_data)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ssp_request_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->ssp_request_callback_cb(gap_handle, request_data);
}

static void btm_device_found_callback(void* gap_handle, bt_device_t* device)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->device_found_callback_cb))
        return;
    BT_LOGD("%s: PERFORMANCE-GAP-BTM-DISCOVERY-FOUND", __func__);
    context->gap_callbacks->device_found_callback_cb(gap_handle, device);
}

static void btm_bond_state_changed_callback(void* gap_handle, bt_device_t* device, bt_bond_state state)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->bond_state_changed_callback_cb))
        return;
    if ((BT_BOND_STATE_BONDED == state) || (BT_BOND_STATE_BLE_BONDED == state))
        BT_LOGD("%s: PERFORMANCE-GAP-BTM-BOND-END", __func__);
    context->gap_callbacks->bond_state_changed_callback_cb(gap_handle, device, state);
}

static void btm_connected_state_callback(void* gap_handle, bt_device_t* device, bt_connection_state state)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->connection_state_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connection_state_callback_cb(gap_handle, device, state);
}

static void btm_hci_event_callback(void* gap_handle, hci_event_t* hci_event)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->hci_event_callback_cb))
        return;
    //BT_LOGD("%s", __func__);
    context->gap_callbacks->hci_event_callback_cb(gap_handle, hci_event);
}

static void btm_connection_state_changed_callback(void* gap_handle, bt_device_t* device, bt_connection_state state)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->connection_state_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->connection_state_callback_cb(gap_handle, device, state);
}

static void btm_local_name_callback(void* gap_handle, char* bt_name, uint8_t length)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->local_name_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->local_name_callback_cb(gap_handle, bt_name, length);
}

static void btm_local_device_class_callback(void* gap_handle, uint32_t device_class)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->local_device_class_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->local_device_class_callback_cb(gap_handle, device_class);
}

static void btm_local_address_callback(void* gap_handle, bt_device_t* device)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->local_address_callback_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->local_address_callback_cb(gap_handle, device);
}

static void btm_smp_request_callback(void* gap_handle, ssp_request_data_t* request_data)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->smp_requeset_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->smp_requeset_cb(gap_handle, request_data);
}

static void btm_ble_phy_update_callback(void* gap_handle, bt_address remote_addr, ble_phy_type tx_phy, ble_phy_type rx_phy, bt_status status)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_phy_update_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->ble_phy_update_cb(gap_handle, remote_addr, tx_phy, rx_phy, status);
}

static void btm_ble_address_callback(void* gap_handle, bt_address bd_addr, ble_addr_type addr_type)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_address_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->ble_address_cb(gap_handle, bd_addr, addr_type);
}

static void btm_pairing_request_callback(void* gap_handle, bt_address remote_addr, bool local_initiate, bool is_bondable)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->pairing_request_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->pairing_request_cb(gap_handle, remote_addr, local_initiate, is_bondable);
}

void btm_ble_irk_callback(void* gap_handle, bt_common_key irk, bt_address ble_addr, ble_addr_type addr_type)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_irk_cb))
        return;
    BT_LOGD("%s", __func__);
    context->gap_callbacks->ble_irk_cb(gap_handle, irk, ble_addr, addr_type);
}

static const btm_gap_callbacks_t service_callbacks = {
    .size = sizeof(btm_gap_callbacks_t),
    .bt_connection_state_changed_callback_cb = btm_connection_state_changed_callback,
    .received_remote_name_callback_cb = btm_remote_name_callback,
    .discovery_state_changed_callback_cb = btm_discovery_state_changed_callback,
    .ssp_request_callback_cb = btm_ssp_request_callback,
    .bond_state_changed_callback_cb = btm_bond_state_changed_callback,
    .hci_event_callback_cb = btm_hci_event_callback,
    .local_name_callback_cb = btm_local_name_callback,
    .local_device_class_callback_cb = btm_local_device_class_callback,
    .connection_state_callback_cb = btm_connected_state_callback,
    .local_address_callback_cb = btm_local_address_callback,
    .device_found_callback_cb = btm_device_found_callback,
    .smp_requeset_cb = btm_smp_request_callback,
    .ble_phy_update_cb = btm_ble_phy_update_callback,
    .ble_address_cb = btm_ble_address_callback,
    .pairing_request_cb = btm_pairing_request_callback,
    .ble_irk_cb = btm_ble_irk_callback,

};

static bt_result_code btm_gap_register_callbacks(void* manager_handle, void** gap_handle, const btm_gap_callbacks_t* callbacks)
{
    bt_result_code ret = BT_RESULT_FAILED;
    gap_context_t* context = (gap_context_t*)malloc(sizeof(gap_context_t));
    if (!context)
        return ret;
    *gap_handle = context;
    context->gap_callbacks = callbacks;
    context->manager_context = manager_handle;
    context->service_interface = get_gap_service_instance();
    ret = context->service_interface->gap_register_callbacks(manager_handle, (void**)&context, &service_callbacks);
    return ret;
}

static bt_result_code btm_gap_cleanup(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, gap_cleanup, ret, gap_handle);
    free(gap_handle);
    return ret;
}

static bt_result_code btm_set_local_address(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_address, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_get_local_address(void* gap_handle, bt_address addr)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_local_address, ret, gap_handle, addr);
    return ret;
}

static bt_result_code btm_set_local_io_capability(void* gap_handle, bt_io_capability io_capability)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_io_capability, ret, gap_handle, io_capability);
    return ret;
}

static bt_result_code btm_set_local_name(void* gap_handle, char* bt_name, uint8_t len)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_name, ret, gap_handle, bt_name, len);
    return ret;
}

static char* btm_get_local_name(void* gap_handle)
{
    char* ret = NULL;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_local_name, ret, gap_handle);
    return ret;
}

static bt_result_code btm_set_local_device_class(void* gap_handle, uint32_t class_of_device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_local_device_class, ret, gap_handle, class_of_device);
    return ret;
}

static uint32_t btm_get_local_device_class(void* gap_handle)
{
    uint32_t ret = 0;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_local_device_class, ret, gap_handle);
    return ret;
}

/*Remote device*/
static bt_result_code btm_get_remote_name(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_remote_name, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_reply_pair_request(void* gap_handle, bt_device_t* device, int accept)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_reply_pair_request, ret, gap_handle, device, accept);
    return ret;
}
static bt_result_code bm_ssp_reply(void* gap_handle, spp_reply_data_t* reply_data)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_ssp_reply, ret, gap_handle, reply_data);
    return ret;
}
static bt_result_code btm_create_bond(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_LOGD("%s: PERFORMANCE-GAP-BTM-BOND-START", __func__);
    BT_GAP_INTERFACE(context->service_interface, bt_create_bond, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_cancel_bond(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_cancel_bond, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_remove_bond(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_remove_bond, ret, gap_handle, device);
    return ret;
}

static int btm_get_bonded_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_bonded_devices, ret, gap_handle, device_list, max_out);
    return ret;
}

static int btm_get_connected_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_connected_devices, ret, gap_handle, device_list, max_out);
    return ret;
}

/*Discovery*/
static bt_result_code btm_set_scan_mode(void* gap_handle, bt_scan_mode scan_mode, bool bondable)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_scan_mode, ret, gap_handle, scan_mode, bondable);
    return ret;
}

//bt_result_code btGapSetLinkMode(bt_address remote_addr, BTLinkMode link_mode);
static bt_result_code btm_start_discovery(void* gap_handle, uint32_t timeout)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_LOGD("%s: PERFORMANCE-GAP-BTM-DISCOVERY-START ", __func__);
    BT_GAP_INTERFACE(context->service_interface, bt_start_discovery, ret, gap_handle, timeout);
    return ret;
}

static bt_result_code btm_stop_discovery(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_stop_discovery, ret, gap_handle);
    return ret;
}

/*VSC command*/
#ifdef HCI_VSC_COMMAND
static bt_result_code btm_send_hci_command(void* gap_handle, bt_hci_command_t* command, hci_command_complete_event event_type)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_send_hci_command, ret, gap_handle, command, event_type);
    return ret;
}
#endif

static int btm_get_remote_services(void* gap_handle, bt_device_t* remote_addr, bt_uuid_t* service_list, uint8_t count_in)
{
    int ret = -1;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_get_remote_services, ret, gap_handle, remote_addr, service_list, count_in);
    return ret;
}

/*service discovery*/
static bt_result_code btm_start_service_discovery(void* gap_handle, bt_device_t* device, bt_uuid_t uuid)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_start_service_discovery, ret, gap_handle, device, uuid);
    return ret;
}

static bt_result_code btm_stop_service_discovery(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_stop_service_discovery, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_set_static_identity(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_static_identity, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_set_public_identity(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_public_identity, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_get_current_irk(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_current_irk, ret, gap_handle);
    return ret;
}

static bt_result_code btm_ble_set_address(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_address, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_get_address(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_address, ret, gap_handle);
    return ret;
}

static bt_result_code btm_ble_set_bonded_devices(void* gap_handle, ble_keys_t* bonded_device_list, uint8_t count_in)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_bonded_devices, ret, gap_handle, bonded_device_list, count_in);
    return ret;
}

static bt_result_code btm_ble_connect(void* gap_handle, ble_connect_params_t* conn_param)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_connect, ret, gap_handle, conn_param);
    return ret;
}

static bt_result_code btm_ble_disconnect(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_disconnect, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_smp_reply(void* gap_handle, spp_reply_data_t* reply_data)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_smp_reply, ret, gap_handle, reply_data);
    return ret;
}

static bt_result_code btm_ble_add_white_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_add_white_list, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_remove_white_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_remove_white_list, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_add_resolving_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_add_resolving_list, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_remove_resolving_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_remove_resolving_list, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_ble_set_phy(void* gap_handle, bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_set_phy, ret, gap_handle, device, tx_phy, rx_phy);
    return ret;
}

static bt_result_code btm_ble_add_private_channel(void* gap_handle, uint16_t private_cid)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_add_private_channel, ret, gap_handle, private_cid);
    return ret;
}

static bt_result_code btm_ble_send_packet(void* gap_handle, bt_device_t* device, uint16_t private_cid, uint8_t* packet, uint16_t packet_size)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_send_packet, ret, gap_handle, device, private_cid, packet, packet_size);
    return ret;
}
static int btm_ble_get_bonded_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_bonded_devices, ret, gap_handle, device_list, max_out);
    return ret;
}

static int btm_ble_get_connected_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_connected_devices, ret, gap_handle, device_list, max_out);
    return ret;
}
static int btm_ble_get_whitelist_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_whitelist_devices, ret, gap_handle, device_list, max_out);
    return ret;
}
static int btm_ble_get_resolvinglist_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_get_resolvinglist_devices, ret, gap_handle, device_list, max_out);
    return ret;
}
/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
static bt_result_code btm_enter_bluetooth_test_mode(void* gap_handle, test_mode mode)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, enter_bluetooth_test_mode, ret, gap_handle, mode);
    return ret;
}

static bt_result_code btm_set_inquiry_scan_parameter(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_inquiry_scan_parameters, ret, gap_handle, scan_type, scan_interval, scan_window);
    return ret;
}

static bt_result_code btm_set_page_scan_parameters(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_page_scan_parameters, ret, gap_handle, scan_type, scan_interval, scan_window);
    return ret;
}

static btm_gap_interface_t gap_interface = {
    .size = sizeof(btm_gap_interface_t),
    .gap_register_callbacks = btm_gap_register_callbacks,
    .gap_cleanup = btm_gap_cleanup,
    .bt_start_discovery = btm_start_discovery,
    .bt_get_local_address = btm_get_local_address,
    .bt_set_local_io_capability = btm_set_local_io_capability,
    .bt_set_local_name = btm_set_local_name,
    .bt_get_local_name = btm_get_local_name,
    .bt_get_remote_name = btm_get_remote_name,
    .bt_get_remote_services = btm_get_remote_services,
    .bt_reply_pair_request = btm_reply_pair_request,
    .bt_ssp_reply = bm_ssp_reply,
    .bt_create_bond = btm_create_bond,
    .bt_cancel_bond = btm_cancel_bond,
    .bt_remove_bond = btm_remove_bond,
    .bt_get_bonded_devices = btm_get_bonded_devices,
    .bt_get_connected_devices = btm_get_connected_devices,
    .bt_set_scan_mode = btm_set_scan_mode,
    .bt_start_discovery = btm_start_discovery,
    .bt_stop_discovery = btm_stop_discovery,
    .bt_start_service_discovery = btm_start_service_discovery,
    .bt_stop_service_discovery = btm_stop_service_discovery,
#ifdef HCI_VSC_COMMAND
    .bt_send_hci_command = btm_send_hci_command,
#endif
    .ble_set_static_identity = btm_ble_set_static_identity,
    .ble_set_public_identity = btm_ble_set_public_identity,
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
    .enter_bluetooth_test_mode = btm_enter_bluetooth_test_mode,
    .bt_set_local_address = btm_set_local_address,
    .bt_set_local_device_class = btm_set_local_device_class,
    .bt_get_local_device_class = btm_get_local_device_class,
    .ble_get_bonded_devices = btm_ble_get_bonded_devices,
    .ble_get_connected_devices = btm_ble_get_connected_devices,
    .ble_get_whitelist_devices = btm_ble_get_whitelist_devices,
    .ble_get_resolvinglist_devices = btm_ble_get_resolvinglist_devices,
    .bt_set_inquiry_scan_parameters = btm_set_inquiry_scan_parameter,
    .bt_set_page_scan_parameters = btm_set_page_scan_parameters,
};

btm_gap_interface_t* get_gap_instance(void)
{
    return &gap_interface;
}
