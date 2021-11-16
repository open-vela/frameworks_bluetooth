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
#include <nuttx/list.h>
#define LOG_TAG "bts_gap_service"
#include "log.h"


typedef struct {
  struct list_node handle_list;
  bt_service_state bt_state;
  ble_service_state ble_state;
}bt_gap_service_t;

typedef struct {
  struct list_node node;
  void* gap_handle;
  btm_gap_callbacks_t* gap_callbacks;
}bt_if_gap_handle_t;

bt_gap_service_t* gap_service = NULL;

static void gap_if_init_done_callback()
{

}
static void gap_if_received_remote_name_callback(BD_ADDR bd_addr, char *bt_name, uint8_t length)
{

}
static void gap_if_discovery_state_changed_callback(bt_discovery_state state)
{
    BT_LOGD("%s", __func__);
    struct list_node *list = &gap_service->handle_list;
    bt_if_gap_handle_t *if_handle;
    struct list_node *node;

    list_for_every(list, node) {
        if_handle = (bt_if_gap_handle_t *)node;
        if (if_handle->gap_callbacks->discovery_state_changed_callback_cb)
            if_handle->gap_callbacks->discovery_state_changed_callback_cb(if_handle->gap_handle, state);
    }
}
static void gap_if_ssp_request_callback(bt_ssp_request_data_t *request_data)
{
    
}
static void gap_if_device_found_callback(bt_device_t* device)
{
    BT_LOGD("%s", __func__);
    struct list_node *list = &gap_service->handle_list;
    bt_if_gap_handle_t *if_handle;
    struct list_node *node;

    list_for_every(list, node) {
        if_handle = (bt_if_gap_handle_t *)node;
        if (if_handle->gap_callbacks->device_found_callback_cb)
           if_handle->gap_callbacks->device_found_callback_cb(if_handle->gap_handle, device);
    }    
}
static void gap_if_bond_state_changed_callback(bt_device_t* device, bt_bond_state state)
{
    
}
static void gap_if_connection_state_callback(bt_device_t* device, bt_connection_state state)
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
    struct list_node *list = &gap_service->handle_list;
    bt_if_gap_handle_t *if_handle;
    struct list_node *node;

    list_for_every(list, node) {
        if_handle = (bt_if_gap_handle_t *)node;
        if (if_handle->gap_callbacks->state_changed_cb){
        if_handle->gap_callbacks->state_changed_cb(if_handle->gap_handle, state);
        }
    }
}
void gap_if__smp_request_callback(ssp_request_data_t *request_data)
{

}
void gap_if__ble_phy_update_callback( bd_addr_t remote_addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy, bt_status status)
{

}
void gap_if__ble_address_callback( bd_addr_t ble_addr, ble_addr_type ble_addr_type)
{

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
        if (!gap_service){
        gap_service = (bt_gap_service_t*)malloc(sizeof(bt_gap_service_t));
        gap_init(&bts_gap_callbacks);
        list_initialize(&gap_service->handle_list);

    }
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_if_register_callbacks(void* handle, void* gap_handle, const btm_gap_callbacks_t* callbacks)
{
    BT_LOGD("%s", __func__);
    bt_if_gap_handle_t * gap_if_handle = (bt_if_gap_handle_t*)malloc(sizeof(bt_if_gap_handle_t));
    gap_if_handle->gap_callbacks = malloc(sizeof(btm_gap_callbacks_t));
    gap_if_handle->gap_callbacks = callbacks;
    gap_if_handle->gap_handle = gap_handle;
    list_add_tail(&gap_service->handle_list, &gap_if_handle->node);
}

bt_result_code bts_if_start_discovery(void* gap_handle, uint32_t timeout)
{
    bts_start_discovery(timeout);    
}
bt_result_code bts_if_set_local_name(void* gap_handle, char *bt_name, uint8_t len)
{
    bts_set_local_name(bt_name, len);
}

/*Local property*/
bt_result_code bts_if_set_local_address(void* gap_handle, bt_device_t* device)
{
    BT_LOGD("%s", __func__);
    return BT_RESULT_FAILED;
}
bt_address* bts_if_get_local_address(void* gap_handle)
{
    bt_address* addr = bts_get_local_address();
    return addr;
}
bt_result_code bts_if_set_local_io_capability(void* gap_handle, bt_io_capability io_capability)
{
    bts_set_local_io_capability(io_capability);
}
char* bts_if_get_local_name(void* gap_handle)
{
    char *name = bts_get_local_name();
    return name;
}
bt_result_code bts_if_set_local_device_class(void* gap_handle, uint32_t class_of_device)
{
    bt_result_code ret =  bts_set_local_device_class(class_of_device);
    return ret;
}
uint32_t bts_if_get_local_device_class(void* gap_handle)
{
    uint32_t class = bts_get_local_device_class();
}

/*Remote device*/
bt_result_code bts_if_get_remote_name(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret =  bts_get_remote_name(device);
    return ret;
}

bt_result_code bts_if_get_connection_state(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret =  bts_get_connection_state(device);

}

int  bts_if_get_remote_services(void* gap_handle, bt_device_t* remote_addr, bt_uuid_t *service_list, uint8_t count_in)
{
    bts_get_remote_services(remote_addr, service_list, count_in);
}

/*Bond*/
bt_bond_state bts_if_get_bond_state(void* gap_handle, bt_device_t* device)
{
    bts_get_bond_state(device);
}
bt_result_code bts_if_reply_pair_request(void* gap_handle, bt_device_t* device, bool accept)
{
    bts_reply_pair_request(device, accept);
}
bt_result_code bts_if_create_bond(void* gap_handle, bt_device_t* device)
{
    bts_create_bond(device);
}

bt_result_code bts_if_cancel_bond(void* gap_handle, bt_device_t* device)
{
    bts_cancel_bond(device);
}

bt_result_code bts_if_remove_bond(void* gap_handle, bt_device_t* device)
{
    bts_remove_bond(device);
}

bt_result_code bts_if_get_bonded_devices(void* gap_handle)
{
    bts_get_bonded_devices();
}

/*Connection*/
bt_result_code bts_if_connect_all(void* gap_handle, bt_device_t* device)
{
    bts_connect_all(device);
}
bt_result_code bts_if_disonnect_all(void* gap_handle, bt_device_t* device)
{
    bts_disonnect_all(device);
}
bt_result_code bts_if_get_connected_devices(void* gap_handle)
{
    bts_get_connected_devices();
}

/*Discovery*/
bt_result_code bts_if_set_scan_mode(void* gap_handle, bt_scan_mode scan_mode, bool bondable)
{
    bts_set_scan_mode(scan_mode, bondable);
}
bt_result_code bts_if_stop_discovery(void* gap_handle)
{
    bts_stop_discovery();
}

/*service discovery*/
bt_result_code bts_if_start_service_discovery(void* gap_handle, bt_device_t* device, bt_uuid_t uuid)
{
    bts_start_service_discovery(device, uuid);
}
bt_result_code bts_if_stop_service_discovery(void* gap_handle, bt_device_t* device)
{
    bts_stop_service_discovery(device);
}

/*VSC command*/
bt_result_code bts_if_send_hci_command_v1(void* gap_handle, bt_hci_command_t *command, bt_service_hci_command_complete_event event_type)
{
    bts_send_hci_command_v1(command, event_type);
}

/*ble interface*/
bt_result_code bts_if_set_ble_scan_parameters(void* gap_handle, scan_params_t *scan_param)
{
    bts_set_ble_scan_parameters(scan_param);
}
bt_result_code bts_if_set_ble_scan_filter(void* gap_handle, ble_scan_filter_t *scan_filter)
{
    bts_set_ble_scan_filter(scan_filter);
}
bt_result_code bts_if_start_ble_scan(void* gap_handle)
{
    bts_start_ble_scan();
}
bt_result_code bts_if_stop_ble_scan(void* gap_handle)
{
    bts_stop_ble_scan();
}
bt_result_code bts_if_start_ble_adv(void* gap_handle, advertise_param_t *scan_params)
{
    bts_start_ble_adv(scan_params);
}
bt_result_code bts_if_stop_ble_adv(void* gap_handle, uint8_t adv_id)
{
    bts_stop_ble_adv(adv_id);
}
bt_result_code bts_if_ble_set_static_identity(void* gap_handle, bt_device_t* device)
{
    bts_ble_set_static_identity(device);
}
bt_result_code bts_if_ble_get_current_irk(void* gap_handle)
{
    bts_ble_get_current_irk();
}
bt_result_code bts_if_ble_set_address(void* gap_handle, bt_device_t* device)
{
    bts_ble_set_address(device);
}
bt_result_code bts_if_ble_get_address(void* gap_handle)
{
    bts_ble_get_address();
}
bt_result_code bts_if_ble_set_bonded_devices(void* gap_handle, ble_keys_t *bonded_device_list, uint8_t count_in)
{
    return BT_RESULT_FAILED;
}
bt_result_code bts_if_ble_connect(void* gap_handle, ble_connect_params_t *conn_param)
{
    bts_ble_connect(conn_param);
}
bt_result_code bts_if_ble_disconnect(void* gap_handle, bt_device_t* device)
{
    bts_ble_disconnect(device);
}
bt_result_code bts_if_ble_smp_reply(void* gap_handle, spp_reply_data_t *reply_data)
{
    bts_ble_smp_reply(reply_data);
}
bt_result_code bts_if_ble_add_white_list(void* gap_handle, bt_device_t* device)
{
    bts_ble_add_white_list(device);
}
bt_result_code bts_if_ble_remove_white_list(void* gap_handle, bt_device_t* device)
{
    bts_ble_remove_white_list(device);
}
bt_result_code bts_if_ble_add_resolving_list(void* gap_handle, bt_device_t* device)
{
    bts_ble_add_resolving_list(device);
}
bt_result_code bts_if_ble_remove_resolving_list(void* gap_handle, bt_device_t* device)
{
    bts_ble_remove_resolving_list(device);
}
bt_result_code bts_if_ble_set_phy(void* gap_handle, bt_device_t* device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bts_ble_set_phy(device, tx_phy, rx_phy);
}
bt_result_code bts_if_ble_add_private_channel(void* gap_handle, uint16_t private_cid)
{
    bts_ble_add_private_channel(private_cid);
}
bt_result_code bts_if_ble_send_packet(void* gap_handle, bt_device_t* device, uint16_t private_cid, uint8_t *packet, uint16_t packet_size)
{
    bts_ble_send_packet(device, private_cid, packet, packet_size);
}
    
/**
 * Send HCI command for testing purpose
 * hci_cmd_packet[in] Complete HCI command packet, e.g. 01 03 0c 00
 * @return Bluetooth Error status code (0- Success)
 */
bt_result_code bts_if_send_hci_command(uint8_t *hci_cmd_packet, hci_event_callback cb)
{
    bts_send_hci_command(hci_cmd_packet, cb);
}
bt_result_code bts_if_enter_bluetooth_test_mode(bt_test_mode test_mode)
{
    bts_enter_bluetooth_test_mode(test_mode);
}

// static gap_service_interface_t gap_interface = {
//     .size = sizeof(gap_service_interface_t),
//     .register_callbacks = bts_if_register_callbacks,
//     .start_discovery = bts_if_start_discovery,
//     .set_local_address = bts_if_set_local_address,
//     .get_local_address = bts_if_get_local_address,
//     .set_local_io_capability = bts_if_set_local_io_capability,
//     .set_local_name  = bts_if_set_local_name,
//     .get_local_name = bts_if_get_local_name,
//     .set_local_device_class = bts_if_set_local_device_class,
//     .get_local_device_class = bts_if_get_local_device_class,
//     .get_remote_name = bts_if_get_remote_name,
//     .get_connection_state = bts_if_get_connection_state,
//     .get_remote_services = bts_if_get_remote_services,
//     .get_bond_state = bts_if_get_bond_state,
//     .reply_pair_request = bts_if_reply_pair_request,
//     .create_bond = bts_if_create_bond,
//     .cancel_bond = bts_if_cancel_bond,
//     .remove_bond = bts_if_remove_bond,
//     .get_bonded_devices = bts_if_get_bonded_devices,
//     .connect_all = bts_if_connect_all,
//     .disonnect_all = bts_if_disonnect_all,
//     .get_connected_devices = bts_if_get_connected_devices,
//     .set_scan_mode = bts_if_set_scan_mode,
//     .start_discovery = bts_if_start_discovery,
//     .stop_discovery = bts_if_stop_discovery,
//     .start_service_discovery = bts_if_start_service_discovery,
//     .stop_service_discovery = bts_if_stop_service_discovery,
//     .send_hci_command_v1 = bts_if_send_hci_command_v1,
//     .set_ble_scan_parameters = bts_if_set_ble_scan_parameters,
//     .set_ble_scan_filter = bts_if_set_ble_scan_filter,
//     .start_ble_scan = bts_if_start_ble_scan,
//     .stop_ble_scan = bts_if_stop_ble_scan,
//     .start_ble_adv = bts_if_start_ble_adv,
//     .stop_ble_adv = bts_if_stop_ble_adv,
//     .ble_set_static_identity = bts_if_ble_set_static_identity,
//     .ble_get_current_irk = bts_if_ble_get_current_irk,
//     .ble_set_address = bts_if_ble_set_address,
//     .ble_get_address = bts_if_ble_get_address,
//     .ble_set_bonded_devices = bts_if_ble_set_bonded_devices,
//     .ble_connect = bts_if_ble_connect,
//     .ble_disconnect = bts_if_ble_disconnect,
//     .ble_smp_reply = bts_if_ble_smp_reply,
//     .ble_add_white_list = bts_if_ble_add_white_list,
//     .ble_remove_white_list = bts_if_ble_remove_white_list,
//     .ble_add_resolving_list = bts_if_ble_add_resolving_list,
//     .ble_remove_resolving_list = bts_if_ble_remove_resolving_list,
//     .ble_set_phy = bts_if_ble_set_phy,
//     .ble_add_private_channel = bts_if_ble_add_private_channel,
//     .ble_send_packet = bts_if_ble_send_packet,
//     .send_hci_command = bts_if_send_hci_command,
//     .enter_bluetooth_test_mode = bts_if_enter_bluetooth_test_mode,
// };

static btm_gap_interface_t gap_interface = {
    .size = sizeof(btm_gap_interface_t),
    .register_callbacks = bts_if_register_callbacks,
    .start_discovery = bts_if_start_discovery,
    .set_address = bts_if_set_local_address,
    .get_address = bts_if_get_local_address,
    .set_iocapability = bts_if_set_local_io_capability,
    .set_name  = bts_if_set_local_name,
    .get_name = bts_if_get_local_name,
    .set_class = bts_if_set_local_device_class,
    .get_class = bts_if_get_local_device_class,
    .get_remote_name = bts_if_get_remote_name,
    .get_connetion_state = bts_if_get_connection_state,
    .get_remote_services = bts_if_get_remote_services,
    .get_bond_state = bts_if_get_bond_state,
    .replay_pair_request = bts_if_reply_pair_request,
    .create_bond = bts_if_create_bond,
    .cancel_bond = bts_if_cancel_bond,
    .remove_bond = bts_if_remove_bond,
    .get_bonded_devices = bts_if_get_bonded_devices,
    .connect_all = bts_if_connect_all,
    .disconnect_all = bts_if_disonnect_all,
    .get_connected_devices = bts_if_get_connected_devices,
    .set_scan_mode = bts_if_set_scan_mode,
    .start_discovery = bts_if_start_discovery,
    .stop_discovery = bts_if_stop_discovery,
    .start_service_discovery = bts_if_start_service_discovery,
    .stop_service_discovery = bts_if_stop_service_discovery,
    .send_hci_command_v1 = bts_if_send_hci_command_v1,
    .set_ble_scan_parameters = bts_if_set_ble_scan_parameters,
    .set_ble_scan_filter = bts_if_set_ble_scan_filter,
    .start_ble_scan = bts_if_start_ble_scan,
    .stop_ble_scan = bts_if_stop_ble_scan,
    .start_ble_adv = bts_if_start_ble_adv,
    .stop_ble_adv = bts_if_stop_ble_adv,
    .ble_set_static_identity = bts_if_ble_set_static_identity,
    .ble_get_current_irk = bts_if_ble_get_current_irk,
    .ble_set_address = bts_if_ble_set_address,
    .ble_get_address = bts_if_ble_get_address,
    .ble_set_bonded_devices = bts_if_ble_set_bonded_devices,
    .ble_connect = bts_if_ble_connect,
    .ble_disconnect = bts_if_ble_disconnect,
    .ble_smp_reply = bts_if_ble_smp_reply,
    .ble_add_white_list = bts_if_ble_add_white_list,
    .ble_remove_white_list = bts_if_ble_remove_white_list,
    .ble_add_resolving_list = bts_if_ble_add_resolving_list,
    .ble_remove_resolving_list = bts_if_ble_remove_resolving_list,
    .ble_set_phy = bts_if_ble_set_phy,
    .ble_add_private_channel = bts_if_ble_add_private_channel,
    .ble_send_packet = bts_if_ble_send_packet,
    .send_hci_command = bts_if_send_hci_command,
    .enter_bluetooth_test_mode = bts_if_enter_bluetooth_test_mode,
};

btm_gap_interface_t* get_gap_service_instance(void)
{
    return &gap_interface;
}