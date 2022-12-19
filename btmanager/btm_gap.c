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

#define BT_GAP_INTERFACE(P_IF, MOTHOD, RET, ...)           \
    do {                                                   \
        if ((P_IF) && (P_IF)->MOTHOD) {                    \
            RET = (P_IF)->MOTHOD(__VA_ARGS__);             \
        } else {                                           \
            BT_LOGE("%s GAP interface is NULL", __func__); \
        }                                                  \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    void* manager_context;
    const btm_gap_callbacks_t* gap_callbacks;
    btm_gap_interface_t* service_interface;
} gap_context_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static char* bond_state_to_str(bt_bond_state state)
{
    static char* bond_state = NULL;
    switch (state) {
    case BT_BOND_STATE_NONE:
        bond_state = "BT_BOND_STATE_NONE";
        break;
    case BT_BOND_STATE_BONDING:
        bond_state = "BT_BOND_STATE_BONDING";
        break;
    case BT_BOND_STATE_BONDED:
        bond_state = "BT_BOND_STATE_BONDED";
        break;
    case BT_BOND_STATE_SDP_DONE:
        bond_state = "BT_BOND_STATE_SDP_DONE";
        break;
    case BT_BOND_STATE_BLE_NONE:
        bond_state = "BT_BOND_STATE_BLE_NONE";
        break;
    case BT_BOND_STATE_BLE_BONDING:
        bond_state = "BT_BOND_STATE_BLE_BONDING";
        break;
    case BT_BOND_STATE_BLE_BONDED:
        bond_state = "BT_BOND_STATE_BLE_BONDED";
        break;
    default:
        break;
    }
    return bond_state;
}

BTM_CB_X(received_remote_name_cb, bt_address bd_addr, char* bt_name, uint8_t length)
{
    BTM_CBX(received_remote_name_cb, bd_addr, bt_name, length);
}

BTM_CB_X(discovery_state_changed_cb, bt_discovery_state state)
{
    BTM_CBX(discovery_state_changed_cb, state);
}

BTM_CB_X(ssp_request_cb, ssp_request_data_t* request_data)
{
    BTM_CBX(ssp_request_cb, request_data);
}

BTM_CB_X(device_found_cb, bt_device_t* device)
{
    BT_LOGD("%s: PERFORMANCE-GAP-BTM-DISCOVERY-FOUND", __func__);
    BTM_CBX(device_found_cb, device);
}

BTM_CB_X(bond_state_changed_cb, bt_device_t* device, bt_bond_state state)
{
    BT_LOGD("%s: PERFORMANCE-GAP-BTM-%s", __func__, bond_state_to_str(state));
    BTM_CBX(bond_state_changed_cb, device, state);
}

BTM_CB_X(connection_state_cb, bt_device_t* device, bt_connection_state state)
{
    BTM_CBX(connection_state_cb, device, state);
}

BTM_CB_X(hci_event_cb, hci_event_t* hci_event)
{
    BTM_CBX(hci_event_cb, hci_event);
}

BTM_CB_X(local_name_cb, char* bt_name, uint8_t length)
{
    BTM_CBX(local_name_cb, bt_name, length);
}

BTM_CB_X(local_device_class_cb, uint32_t device_class)
{
    BTM_CBX(local_device_class_cb, device_class);
}

BTM_CB_X(local_address_cb, bt_device_t* device)
{
    BTM_CBX(local_address_cb, device);
}

BTM_CB_X(smp_request_cb, ssp_request_data_t* request_data)
{
    BTM_CBX(smp_request_cb, request_data);
}

BTM_CB_X(ble_phy_update_cb, bt_address remote_addr, ble_phy_type tx_phy, ble_phy_type rx_phy, bt_status status)
{
    BTM_CBX(ble_phy_update_cb, remote_addr, tx_phy, rx_phy, status);
}

BTM_CB_X(ble_address_cb, bt_address bd_addr, ble_addr_type addr_type)
{
    BTM_CBX(ble_address_cb, bd_addr, addr_type);
}

BTM_CB_X(pairing_request_cb, bt_address remote_addr, bool local_initiate, bool is_bondable)
{
    BTM_CBX(pairing_request_cb, remote_addr, local_initiate, is_bondable);
}

BTM_CB_X(ble_irk_cb, bt_common_key irk, bt_address ble_addr, ble_addr_type addr_type)
{
    BTM_CBX(ble_irk_cb, irk, ble_addr, addr_type);
}

BTM_CB_X(delete_linkey_cb, bt_address remote_addr, bt_status reason)
{
    BTM_CBX(delete_linkey_cb, remote_addr, reason);
}

BTM_IF_X(bt_reply_link_request, bt_address remote_addr, bool accept)
{
    BTM_IFX(bt_reply_link_request, remote_addr, accept);
}

BTM_CB_X(link_connect_request_cb, bt_address remote_addr)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if (NULL == context->gap_callbacks)
        return;

    if (NULL == context->gap_callbacks->link_connect_request_cb) {
        btm_bt_reply_link_request(gap_handle, remote_addr, true);
        return;
    }
    context->gap_callbacks->link_connect_request_cb(gap_handle, remote_addr);
}

BTM_CB_X(ble_adv_started_cb, uint8_t adv_id)
{
    BTM_CBX(ble_adv_started_cb, adv_id);
}

BTM_CB_X(ble_adv_stopped_cb, uint8_t adv_id)
{
    BTM_CBX(ble_adv_stopped_cb, adv_id);
}

static void btm_link_mode_change_callback(void* gap_handle, bt_address remote_addr, bool active, uint16_t interval)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->link_mode_change_cb))
        return;
    context->gap_callbacks->link_mode_change_cb(gap_handle, remote_addr, active, interval);
}

static void btm_scan_mode_change_callback(void* gap_handle, bt_scan_mode scan_mode)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->scan_mode_change_cb))
        return;
    context->gap_callbacks->scan_mode_change_cb(gap_handle, scan_mode);
}

static void btm_link_encryption_change_callback(void* gap_handle, bt_address remote_addr, bool br_link, bool encryption_on)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->link_encryption_change_cb))
        return;
    context->gap_callbacks->link_encryption_change_cb(gap_handle, remote_addr, br_link, encryption_on);
}

static void btm_ble_connection_updated_callback(void* gap_handle, bt_address remote_addr, bt_status status,
    uint16_t connection_interval, uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    BTM_CBX(ble_connection_updated_cb, remote_addr, status, connection_interval,
        peripheral_latency, supervision_timeout);
}

static void btm_update_ble_bonede_device_callback(void* gap_handle, ble_keys_t* bonded_device_list, uint8_t count_in)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->update_ble_bonede_device_cb))
        return;
    context->gap_callbacks->update_ble_bonede_device_cb(gap_handle, bonded_device_list, count_in);
}

static void btm_ble_l2cap_connection_state_callback(void* gap_handle, bt_address remote_addr, ble_l2cap_state state, uint16_t psm, uint16_t cid, uint16_t mtu, uint16_t mps)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_l2cap_connection_state_cb))
        return;
    context->gap_callbacks->ble_l2cap_connection_state_cb(gap_handle, remote_addr, state, psm, cid, mtu, mps);
}

static void btm_ble_packet_received_callback(void* gap_handle, bt_address remote_addr, uint16_t cid, uint8_t* packet, uint16_t packet_size)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_packet_received_cb))
        return;
    context->gap_callbacks->ble_packet_received_cb(gap_handle, remote_addr, cid, packet, packet_size);
}

static void btm_ble_packet_sent_callback(void* gap_handle, bt_address remote_addr, uint16_t cid)
{
    if (!gap_handle)
        return;
    gap_context_t* context = (gap_context_t*)gap_handle;
    if ((NULL == context->gap_callbacks) || (NULL == context->gap_callbacks->ble_packet_received_cb))
        return;
    context->gap_callbacks->ble_packet_sent_cb(gap_handle, remote_addr, cid);
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
    .delete_linkey_cb = btm_delete_linkey_callback,
    .link_connect_request_cb = btm_link_connect_request_callback,
    .ble_adv_started_cb = btm_ble_adv_started_callback,
    .ble_adv_stopped_cb = btm_ble_adv_stopped_callback,
    .link_mode_change_cb = btm_link_mode_change_callback,
    .scan_mode_change_cb = btm_scan_mode_change_callback,
    .link_encryption_change_cb = btm_link_encryption_change_callback,
    .ble_connection_updated_cb = btm_ble_connection_updated_callback,
    .update_ble_bonede_device_cb = btm_update_ble_bonede_device_callback,
    .ble_l2cap_connection_state_cb = btm_ble_l2cap_connection_state_callback,
    .ble_packet_received_cb = btm_ble_packet_received_callback,
    .ble_packet_sent_cb = btm_ble_packet_sent_callback,
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

BTM_IF_X(bt_set_local_address, bt_device_t* device)
{
    BTM_IFX(bt_set_local_address, device);
}

BTM_IF_X(bt_get_local_address, bt_address addr)
{
    BTM_IFX(bt_get_local_address, addr);
}

BTM_IF_X(bt_set_local_io_capability, bt_io_capability io_capability)
{
    BTM_IFX(bt_set_local_io_capability, io_capability);
}

BTM_IF_X(bt_set_local_name, char* bt_name, uint8_t len)
{
    BTM_IFX(bt_set_local_name, bt_name, len);
}

BTM_IF_X(bt_set_local_device_class, uint32_t class_of_device)
{
    BTM_IFX(bt_set_local_device_class, class_of_device);
}

BTM_IF_X(bt_get_remote_name, bt_device_t* device)
{
    BTM_IFX(bt_get_remote_name, device);
}

BTM_IF_X(bt_reply_pair_request, bt_device_t* device, int accept)
{
    BTM_IFX(bt_reply_pair_request, device, accept);
}

BTM_IF_X(bt_ssp_reply, spp_reply_data_t* reply_data)
{
    BTM_IFX(bt_ssp_reply, reply_data);
}

BTM_IF_X(bt_create_bond, bt_device_t* device)
{
    BT_LOGD("%s: PERFORMANCE-GAP-BTM-BOND-START", __func__);
    BTM_IFX(bt_create_bond, device);
}

BTM_IF_X(bt_cancel_bond, bt_device_t* device)
{
    BTM_IFX(bt_cancel_bond, device);
}

BTM_IF_X(bt_remove_bond, bt_device_t* device)
{
    BTM_IFX(bt_remove_bond, device);
}

BTM_IF_X(bt_set_scan_mode, bt_scan_mode scan_mode, bool bondable)
{
    BTM_IFX(bt_set_scan_mode, scan_mode, bondable);
}

BTM_IF_X(bt_start_discovery, uint32_t timeout)
{
    BT_LOGD("%s: PERFORMANCE-GAP-BTM-DISCOVERY-START ", __func__);
    BTM_IFX(bt_start_discovery, timeout);
}

BTM_IF_X(bt_stop_discovery)
{
    BTM_IFX(bt_stop_discovery);
}

BTM_IF_X(bt_send_hci_command, bt_hci_command_t* command, hci_command_complete_event event_type)
{
    BTM_IFX(bt_send_hci_command, command, event_type);
}

BTM_IF_X(bt_start_service_discovery, bt_device_t* device, bt_uuid_t uuid)
{
    BTM_IFX(bt_start_service_discovery, device, uuid);
}

BTM_IF_X(bt_stop_service_discovery, bt_device_t* device)
{
    BTM_IFX(bt_stop_service_discovery, device);
}

BTM_IF_X(bt_set_inquiry_scan_parameters, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    BTM_IFX(bt_set_inquiry_scan_parameters, scan_type, scan_interval, scan_window);
}

BTM_IF_X(bt_set_page_scan_parameters, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    BTM_IFX(bt_set_page_scan_parameters, scan_type, scan_interval, scan_window);
}

BTM_IF_X(bt_set_afh_channel_classification, bt_afh_radio_channel_info_t* channels, uint16_t number)
{
    BTM_IFX(bt_set_afh_channel_classification, channels, number);
}

BTM_IF_X(bt_set_link_role, bt_device_t* device, bt_link_role role)
{
    BTM_IFX(bt_set_link_role, device, role);
}

BTM_IF_XX(bt_get_local_name, char*)
{
    BTM_IFXX(bt_get_local_name, char* ret = NULL);
}

BTM_IF_XX(bt_get_local_device_class, uint32_t)
{
    BTM_IFXX(bt_get_local_device_class, uint32_t ret = 0);
}

BTM_IF_XX(bt_get_bonded_devices, int, bt_device_t* device_list, int max_out)
{
    BTM_IFXX(bt_get_bonded_devices, int ret = 0, device_list, max_out);
}

BTM_IF_XX(bt_get_connected_devices, int, bt_device_t* device_list, int max_out)
{
    BTM_IFXX(bt_get_connected_devices, int ret = 0, device_list, max_out);
}

static bt_result_code btm_disconnect_bt_link(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_disconnect_link, ret, gap_handle, device);
    return ret;
}

static bt_result_code btm_enable_ctkd_bonding(void* gap_handle, bool brkey_to_lekey, bool lekey_to_brkey)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_enable_ctkd_bonding, ret, gap_handle, brkey_to_lekey, lekey_to_brkey);
    return ret;
}

static bt_result_code btm_set_auto_sniff(void* gap_handle, bool enable, uint8_t idle_time)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, bt_set_auto_sniff, ret, gap_handle, enable, idle_time);
    return ret;
}

static bt_result_code btm_ble_set_static_identity(void* gap_handle, bt_device_t* device)
{
    BTM_IFXX(bt_get_remote_services, int ret = -1, remote_addr, service_list, count_in);
}

BTM_IF_X(ble_start_advertising, advertise_param_t* param)
{
    BTM_IFX(ble_start_advertising, param);
}

BTM_IF_X(ble_stop_advertising, uint8_t adv_id)
{
    BTM_IFX(ble_stop_advertising, adv_id);
}

BTM_IF_X(ble_set_static_identity, bt_device_t* device)
{
    BTM_IFX(ble_set_static_identity, device);
}

BTM_IF_X(ble_set_public_identity, bt_device_t* device)
{
    BTM_IFX(ble_set_public_identity, device);
}

BTM_IF_X(ble_get_current_irk)
{
    BTM_IFX(ble_get_current_irk);
}

BTM_IF_X(ble_set_address, bt_device_t* device)
{
    BTM_IFX(ble_set_address, device);
}

BTM_IF_X(ble_get_address)
{
    BTM_IFX(ble_get_address);
}

BTM_IF_X(ble_set_bonded_devices, ble_keys_t* bonded_device_list, uint8_t count_in)
{
    BTM_IFX(ble_set_bonded_devices, bonded_device_list, count_in);
}

BTM_IF_X(ble_connect, ble_connect_params_t* conn_param)
{
    BTM_IFX(ble_connect, conn_param);
}

BTM_IF_X(ble_disconnect, bt_device_t* device)
{
    BTM_IFX(ble_disconnect, device);
}

BTM_IF_X(ble_smp_reply, spp_reply_data_t* reply_data)
{
    BTM_IFX(ble_smp_reply, reply_data);
}

BTM_IF_X(ble_add_white_list, bt_device_t* device)
{
    BTM_IFX(ble_add_white_list, device);
}

BTM_IF_X(ble_remove_white_list, bt_device_t* device)
{
    BTM_IFX(ble_remove_white_list, device);
}

static bt_result_code btm_ble_listen_l2cap_channel(void* gap_handle, uint8_t psm, ble_l2cap_config_option_t* opt)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_listen_l2cap_channel, ret, gap_handle, psm, opt);
    return ret;
}

static bt_result_code btm_ble_send_packet(void* gap_handle, bt_device_t* device, uint16_t cid, uint8_t* packet, uint16_t packet_size)
{
    bt_result_code ret = BT_RESULT_FAILED;
    CHECK_PTR_RETURN(gap_handle, ret);
    gap_context_t* context = (gap_context_t*)gap_handle;
    BT_GAP_INTERFACE(context->service_interface, ble_send_packet, ret, gap_handle, device, cid, packet, packet_size);
    return ret;
}

BTM_IF_X(ble_set_phy, bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy)
{
    BTM_IFX(ble_set_phy, device, tx_phy, rx_phy);
}

BTM_IF_X(ble_add_private_channel, uint16_t cid)
{
    BTM_IFX(ble_add_private_channel, cid);
}

BTM_IF_X(ble_send_packet, bt_device_t* device, uint16_t cid, uint8_t* packet, uint16_t packet_size)
{
    BTM_IFX(ble_send_packet, device, cid, packet, packet_size);
}

BTM_IF_X(enter_bluetooth_test_mode, test_mode mode)
{
    BTM_IFX(enter_bluetooth_test_mode, mode);
}

BTM_IF_X(ble_get_bonded_devices, bt_device_t* device_list, int max_out)
{
    BTM_IFXX(ble_get_bonded_devices, int ret = 0, device_list, max_out);
}

BTM_IF_X(ble_get_connected_devices, bt_device_t* device_list, int max_out)
{
    BTM_IFXX(ble_get_connected_devices, int ret = 0, device_list, max_out);
}

BTM_IF_X(ble_get_whitelist_devices, bt_device_t* device_list, int max_out)
{
    BTM_IFXX(ble_get_whitelist_devices, int ret = 0, device_list, max_out);
}

BTM_IF_X(ble_get_resolvinglist_devices, bt_device_t* device_list, int max_out)
{
    BTM_IFXX(ble_get_resolvinglist_devices, int ret = 0, device_list, max_out);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

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
    .bt_set_link_role = btm_set_link_role,
    .bt_disconnect_link = btm_disconnect_bt_link,
    .bt_enable_ctkd_bonding = btm_enable_ctkd_bonding,
    .bt_set_auto_sniff = btm_set_auto_sniff,
#ifdef HCI_VSC_COMMAND
    .bt_send_hci_command = btm_send_hci_command,
#endif
    .ble_start_advertising = btm_ble_start_advertising,
    .ble_stop_advertising = btm_ble_stop_advertising,
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
    .ble_add_irk_to_resolving_list = btm_ble_add_irk_to_resolving_list,
    .ble_set_phy = btm_ble_set_phy,
    .ble_listen_l2cap_channel = btm_ble_listen_l2cap_channel,
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
    .bt_reply_link_request = btm_reply_link_request,
    .bt_set_afh_channel_classification = btm_set_afh_channel_classification,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

btm_gap_interface_t* get_gap_instance(void)
{
    return &gap_interface;
}
