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
#define LOG_TAG "bts_gap_service"

#include <nuttx/list.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"

#include "btm_gap.h"
#include "btm_manager.h"
#include "bts_gap.h"
#include "bts_service.h"
#include "log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BTS_IF(name) .name = bts_if_##name

#define BTS_IF_XX(name, ret, ...) static ret bts_if_##name(void* gap_handle, ##__VA_ARGS__)

#define BTS_IF_X(name, ...) BTS_IF_XX(name, bt_result_code, ##__VA_ARGS__)

#define BTS_IFXX(name, initializer_list, ...) \
    do {                                      \
        initializer_list;                     \
        if (!gap_is_handle_valid(gap_handle)) \
            return ret;                       \
        ret = bts_adapt_##name(__VA_ARGS__);  \
        return ret;                           \
    } while (0)

#define BTS_IFX(name, ...) BTS_IFXX(name, bt_result_code ret = BT_RESULT_FAILED, ##__VA_ARGS__)

#define BTS_CBX2(callback, cond, ...)                                                           \
    do {                                                                                        \
        bt_if_gap_handle_t* if_handle;                                                          \
        struct list_node* handle_node;                                                          \
        if (!gap_service)                                                                       \
            break;                                                                              \
        struct list_node* list = &gap_service->handle_list;                                     \
        list_for_every(list, handle_node)                                                       \
        {                                                                                       \
            if_handle = (bt_if_gap_handle_t*)handle_node;                                       \
            if ((if_handle->gap_callbacks) && (if_handle->gap_callbacks)->callback && (cond)) { \
                (if_handle->gap_callbacks)->callback(if_handle->gap_handle, ##__VA_ARGS__);     \
                break;                                                                          \
            }                                                                                   \
        }                                                                                       \
    } while (0)

#define BTS_CBX(callback, ...) BTS_CBX2(callback, true, ##__VA_ARGS__)

#define BTS_CB_X(name, ...) static void bts_if_##name(__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    struct list_node handle_list;
    bt_service_state bt_state;
    ble_service_state ble_state;
} bt_gap_service_t;

typedef struct {
    struct list_node node;
    void* gap_handle;
    const btm_gap_callbacks_t* gap_callbacks;
} bt_if_gap_handle_t;

typedef struct
{
    struct list_node node;

    uint8_t advertiser_id;
    void* gap_handle;
} bts_leadv_hdl_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct list_node ble_advertiser_list = LIST_INITIAL_VALUE(ble_advertiser_list);
static bt_gap_service_t* gap_service = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static bool gap_is_handle_valid(void* gap_handle)
{
    struct list_node* list = &gap_service->handle_list;
    struct list_node* node;
    bt_if_gap_handle_t* if_handle;

    list_for_every(list, node)
    {
        if_handle = (bt_if_gap_handle_t*)node;
        if (if_handle->gap_handle == gap_handle)
            return true;
    }
    return false;
}

static uint8_t gen_ble_adv_id(void)
{
    static uint8_t adv_id = 0;
    bts_leadv_hdl_t* handle;
    bts_leadv_hdl_t* tmp;

    for (uint8_t i = 0; i < BLE_MAX_ADV_NUM; i++) {
        bool found = false;
        adv_id++;
        adv_id %= BLE_MAX_ADV_NUM;
        list_for_every_entry_safe(&ble_advertiser_list, handle, tmp, bts_leadv_hdl_t, node)
        {
            if (handle->advertiser_id == adv_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            return adv_id;
        }
    }
    BT_LOGE("gen_ble_adv_id overflow");
    return BLE_MAX_ADV_NUM;
}

static bts_leadv_hdl_t* add_advertise_handle(void* gap_handle)
{
    bts_leadv_hdl_t* client = (bts_leadv_hdl_t*)malloc(sizeof(bts_leadv_hdl_t));
    if (!client) {
        BT_LOGE("fail, malloc bts_leadv_hdl_t");
        return NULL;
    }
    memset(client, 0, sizeof(bts_leadv_hdl_t));

    uint8_t adv_id = gen_ble_adv_id();
    if (adv_id == BLE_MAX_ADV_NUM) {
        BT_LOGE("fail, gen_ble_adv_id");
        free(client);
        return NULL;
    }
    client->advertiser_id = adv_id;
    client->gap_handle = gap_handle;
    list_add_tail(&ble_advertiser_list, &client->node);
    return client;
}

static void remove_advertise_handle(bts_leadv_hdl_t* advertiser)
{
    list_delete(&advertiser->node);
    free(advertiser);
}

static bts_leadv_hdl_t* find_advertise_handle(uint8_t advertiser_id)
{
    bts_leadv_hdl_t* client;
    list_for_every_entry(&ble_advertiser_list, client, bts_leadv_hdl_t, node)
    {
        if (client->advertiser_id == advertiser_id) {
            return client;
        }
    }
    return NULL;
}

BTS_CB_X(received_remote_name_cb, BD_ADDR bd_addr, char* bt_name, uint8_t length)
{
    BTS_CBX(received_remote_name_cb, bd_addr, bt_name, length);
}

BTS_CB_X(discovery_state_changed_cb, bt_discovery_state state)
{
    BTS_CBX(discovery_state_changed_cb, state);
}

BTS_CB_X(ssp_request_cb, ssp_request_data_t* request_data)
{
    BTS_CBX(ssp_request_cb, request_data);
}

BTS_CB_X(pairing_request_cb, BD_ADDR remote_addr, bool local_initiate, bool is_bondable)
{
    BTS_CBX(pairing_request_cb, remote_addr, local_initiate, is_bondable);
}

BTS_CB_X(device_found_cb, bt_device_t* device)
{
    BTS_CBX(device_found_cb, device);
}

BTS_CB_X(bond_state_changed_cb, bt_device_t* device, bt_bond_state state)
{
    BTS_CBX(bond_state_changed_cb, device, state);
}

BTS_CB_X(connection_state_changed_cb, bt_device_t* device, bt_connection_state state)
{
    BTS_CBX(bt_connection_state_changed_cb, device, state);
}

BTS_CB_X(hci_event_cb, hci_event_t* hci_event)
{
    BTS_CBX(hci_event_cb, hci_event);
}

BTS_CB_X(adapter_state_changed_cb, SERVICE_BT_STACK_STATE state)
{
    BT_LOGD("%s", __func__);
}

BTS_CB_X(ble_adv_started_cb, uint8_t adv_id)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-STARTED: %d", adv_id);
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
    if (!client) {
        SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_adv(adv_id);
        BT_LOGW("stop le adv, adv id:%d, ret:%" PRIu32, adv_id, ret);
        return;
    }

    BTS_CBX2(ble_adv_started_cb, if_handle->gap_handle == client->gap_handle, adv_id);
}

BTS_CB_X(ble_adv_stopped_cb, uint8_t adv_id)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-STOPPED: %d", adv_id);
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
    if (!client) {
        SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_adv(adv_id);
        BT_LOGW("stop le adv, adv id:%d, ret:%" PRIu32, adv_id, ret);
        return;
    }

    BTS_CBX2(ble_adv_stopped_cb, if_handle->gap_handle == client->gap_handle, adv_id);
    remove_advertise_handle(client);
}

static void gap_if_link_mode_change_callback(bt_address remote_addr, bool active, uint16_t interval)
{
    BT_GAP_CB(link_mode_change_cb, remote_addr, active, interval);
}

static void gap_if_scan_mode_change_callback(bt_scan_mode scan_mode)
{
    BT_GAP_CB(scan_mode_change_cb, scan_mode);
}

static void gap_if_link_encryption_change_callback(bt_address remote_addr, bool br_link, bool encryption_on)
{
    BT_GAP_CB(link_encryption_change_cb, remote_addr, br_link, encryption_on);
}

static void gap_if_smp_request_callback(ssp_request_data_t* request_data)
{
    BTS_CBX(smp_request_cb, request_data);
}

BTS_CB_X(ble_phy_update_cb, bt_address remote_addr, ble_phy_type tx_phy, ble_phy_type rx_phy, bt_status status)
{
    BTS_CBX(ble_phy_update_cb, remote_addr, tx_phy, rx_phy, status);
}

BTS_CB_X(ble_address_cb, bt_address bd_addr, ble_addr_type addr_type)
{
    BTS_CBX(ble_address_cb, bd_addr, addr_type);
}

BTS_CB_X(ble_irk_cb, bt_common_key irk, bt_address ble_addr, ble_addr_type addr_type)
{
    BTS_CBX(ble_irk_cb, irk, ble_addr, addr_type);
}

BTS_CB_X(delete_linkey_cb, bt_address remote_addr, bt_status reason)
{
    BTS_CBX(delete_linkey_cb, remote_addr, reason);
}

BTS_CB_X(link_connect_request_cb, bt_address remote_addr)
{
    BTS_CBX(link_connect_request_cb, remote_addr);
}

BTS_CB_X(ble_connection_updated_cb, bt_address remote_addr, bt_status status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    BTS_CBX(ble_connection_updated_cb, remote_addr, status, connection_interval, peripheral_latency, supervision_timeout);
}

void gap_if_update_ble_bonded_device_callback(ble_keys_t* bonded_device_list, uint8_t count_in)
{
    BT_GAP_CB(update_ble_bonede_device_cb, bonded_device_list, count_in);
}

static void gap_if_ble_l2cap_connection_state_callback(bt_address remote_addr, ble_l2cap_state state, uint16_t psm, uint16_t cid, uint16_t mtu, uint16_t mps)
{
    BT_GAP_CB(ble_l2cap_connection_state_cb, remote_addr, state, psm, cid, mtu, mps);
}

static void gap_if_ble_packet_received_callback(bt_address remote_addr, uint16_t cid, uint8_t* packet, uint16_t packet_size)
{
    BT_GAP_CB(ble_packet_received_cb, remote_addr, cid, packet, packet_size);
}

static void gap_if_ble_packet_sent_callback(bt_address remote_addr, uint16_t cid)
{
    BT_GAP_CB(ble_packet_sent_cb, remote_addr, cid);
}

bts_gap_callback_t bts_gap_callbacks = {
    .size = sizeof(bts_gap_callback_t),
    .adapter_state_changed_cb = gap_if_adapter_state_changed_callback,
    .remote_name_cb = gap_if_received_remote_name_callback,
    .discovery_state_changed_cb = gap_if_discovery_state_changed_callback,
    .spp_request_cb = gap_if_ssp_request_callback,
    .pairing_request_cb = gap_if_pairing_request_callback,
    .device_found_cb = gap_if_device_found_callback,
    .bond_state_changed_cb = gap_if_bond_state_changed_callback,
    .connection_state_changed_cb = gap_if_connection_state_callback,
    .hci_event_cb = gap_if_hci_event_callback,
    .ble_adv_started_cb = gap_if_ble_advtise_started_callback,
    .ble_adv_stopped_cb = gap_if_ble_advtise_stopped_callback,
    .link_mode_change_cb = gap_if_link_mode_change_callback,
    .scan_mode_change_cb = gap_if_scan_mode_change_callback,
    .link_encryption_change_cb = gap_if_link_encryption_change_callback,
    .smp_request_cb = gap_if_smp_request_callback,
    .ble_phy_update_cb = gap_if_ble_phy_update_callback,
    .ble_address_cb = gap_if_ble_address_callback,
    .ble_irk_cb = gap_if_bts_ble_irk_callback,
    .delete_linkey_cb = gap_if_delete_linkey_callback,
    .link_connect_request_cb = gap_if_link_connect_request_callback,
    .ble_connection_updated_cb = gap_if_ble_connection_updated_callback,
    .update_ble_bonede_device_cb = gap_if_update_ble_bonded_device_callback,
    .ble_l2cap_connection_state_cb = gap_if_ble_l2cap_connection_state_callback,
    .ble_packet_received_cb = gap_if_ble_packet_received_callback,
    .ble_packet_sent_cb = gap_if_ble_packet_sent_callback,
};

bt_result_code gap_service_init()
{
    if (!gap_service) {
        gap_service = (bt_gap_service_t*)malloc(sizeof(bt_gap_service_t));
        gap_init(&bts_gap_callbacks);
        list_initialize(&g_gap_service->handle_list);
    } else
        BT_LOGD("%s service had already been initialized: %p", __func__, g_gap_service);

    return BT_RESULT_SUCCESS;
}

static bt_result_code bts_if_register_callbacks(void* handle, void** gap_handle, const btm_gap_callbacks_t* callbacks)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (gap_is_handle_valid(*gap_handle))
        return ret;
    if (!gap_service)
        return ret;

    bt_if_gap_handle_t* gap_if_handle = (bt_if_gap_handle_t*)malloc(sizeof(bt_if_gap_handle_t));
    gap_if_handle->gap_callbacks = callbacks;
    gap_if_handle->gap_handle = *gap_handle;
    list_add_tail(&gap_service->handle_list, &gap_if_handle->node);
    return BT_RESULT_SUCCESS;
}

BTS_IF_X(bt_start_discovery, uint32_t timeout)
{
    BTS_IFX(bt_start_discovery, timeout);
}

BTS_IF_X(bt_set_local_name, char* bt_name, uint8_t len)
{
    BTS_IFX(bt_set_local_name, bt_name, len);
}

BTS_IF_X(bt_set_local_address, bt_device_t* device)
{
    return BT_RESULT_FAILED;
}

BTS_IF_X(bt_get_local_address, bt_address addr)
{
    BTS_IFX(bt_get_local_address, addr);
}

BTS_IF_X(bt_set_local_io_capability, bt_io_capability io_capability)
{
    BTS_IFX(bt_set_local_io_capability, io_capability);
}

BTS_IF_XX(bt_get_local_name, char*)
{
    BTS_IFXX(bt_get_local_name, char* ret = NULL);
}

BTS_IF_X(bt_set_local_device_class, uint32_t class_of_device)
{
    BTS_IFX(bt_set_local_device_class, class_of_device);
}

BTS_IF_XX(bt_get_local_device_class, uint32_t)
{
    BTS_IFXX(bt_get_local_device_class, uint32_t ret = 0);
}

BTS_IF_X(bt_get_remote_name, bt_device_t* device)
{
    BTS_IFX(bt_get_remote_name, device);
}

BTS_IF_XX(bt_get_remote_services, int, bt_device_t* remote_addr, bt_uuid_t* service_list, uint8_t count_in)
{
    BTS_IFXX(bt_get_remote_services, int ret = 0, remote_addr, service_list, count_in);
}

BTS_IF_X(bt_reply_pair_request, bt_device_t* device, int accept)
{
    BTS_IFX(bt_reply_pair_request, device, accept);
}

BTS_IF_X(bt_ssp_reply, spp_reply_data_t* reply_data)
{
    BTS_IFX(bt_ssp_reply, reply_data);
}

BTS_IF_X(bt_create_bond, bt_device_t* device)
{
    BTS_IFX(bt_create_bond, device);
}

BTS_IF_X(bt_cancel_bond, bt_device_t* device)
{
    BTS_IFX(bt_cancel_bond, device);
}

BTS_IF_X(bt_remove_bond, bt_device_t* device)
{
    BTS_IFX(bt_remove_bond, device);
}

BTS_IF_XX(bt_get_bonded_devices, int, bt_device_t* device_list, int max_out)
{
    BTS_IFXX(bt_get_bonded_devices, int ret = 0, device_list, max_out);
}

BTS_IF_XX(bt_get_connected_devices, int, bt_device_t* device_list, int max_out)
{
    BTS_IFXX(bt_get_connected_devices, int ret = 0, device_list, max_out);
}

BTS_IF_XX(ble_get_bonded_devices, int, bt_device_t* device_list, int max_out)
{
    BTS_IFXX(ble_get_bonded_devices, int ret = 0, device_list, max_out);
}

BTS_IF_XX(ble_get_connected_devices, int, bt_device_t* device_list, int max_out)
{
    BTS_IFXX(ble_get_connected_devices, int ret = 0, device_list, max_out);
}

BTS_IF_XX(ble_get_whitelist_devices, int, bt_device_t* device_list, int max_out)
{
    BTS_IFXX(ble_get_whitelist_devices, int ret = 0, device_list, max_out);
}

BTS_IF_XX(ble_get_resolvinglist_devices, int, bt_device_t* device_list, int max_out)
{
    BTS_IFXX(ble_get_resolvinglist_devices, int ret = 0, device_list, max_out);
}

BTS_IF_X(bt_set_scan_mode, bt_scan_mode scan_mode, bool bondable)
{
    BTS_IFX(bt_set_scan_mode, scan_mode, bondable);
}

BTS_IF_X(bt_stop_discovery)
{
    BTS_IFX(bt_stop_discovery);
}

BTS_IF_X(bt_start_service_discovery, bt_device_t* device, bt_uuid_t uuid)
{
    BTS_IFX(bt_start_service_discovery, device, uuid);
}

BTS_IF_X(bt_stop_service_discovery, bt_device_t* device)
{
    BTS_IFX(bt_stop_service_discovery, device);
}

BTS_IF_X(bt_set_link_role, bt_device_t* device, bt_link_role role)
{
    BTS_IFX(bt_set_link_role, device, role);
}
static bt_result_code bts_if_disconnect_bt_link(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_disconnect_bt_link(device);
    return ret;
}
static bt_result_code bts_if_enable_ctkd_bonding(void* gap_handle, bool brkey_to_lekey, bool lekey_to_brkey)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_enable_ctkd_bonding(brkey_to_lekey, lekey_to_brkey);
    return ret;
}

static bt_result_code bts_if_set_auto_sniff(void* gap_handle, bool enable, uint8_t idle_time)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_set_auto_sniff(enable, idle_time);
    return ret;
}

#ifdef HCI_VSC_COMMAND
/*VSC command*/
static bt_result_code bts_if_send_hci_command(void* gap_handle, bt_hci_command_t* command, hci_command_complete_event event_type)
{
    BTS_IFX(bt_send_hci_command, command, event_type);
}

BTS_IF_X(ble_start_advertising, advertise_param_t* param)
{
    BT_ASSERT(!param, BT_RESULT_FAILED);
    bts_leadv_hdl_t* client = add_advertise_handle(gap_handle);
    BT_ASSERT(!client, BT_RESULT_FAILED);
    param->adv_id = client->advertiser_id;

    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-START: %d", client->advertiser_id);

    SERVICE_BT_STATUS ret = service_adapter_gap_start_ble_adv((SERVICE_SCAN_ADV_PARAMS_S*)(param));
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("service ble start adv fail, err:%" PRIu32, ret);
        remove_advertise_handle(client);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bool bts_check_ble_advertise_id(void* gap_handle, uint8_t adv_id)
{
    bts_leadv_hdl_t* client;
    list_for_every_entry(&ble_advertiser_list, client, bts_leadv_hdl_t, node)
    {
        if (client->gap_handle == gap_handle && client->advertiser_id == adv_id) {
            return true;
        }
    }
    return false;
}

BTS_IF_X(ble_stop_advertising, uint8_t adv_id)
{
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
    BT_ASSERT(!client, BT_RESULT_FAILED);

    if (!bts_check_ble_advertise_id(gap_handle, adv_id)) {
        BT_LOGE("fail, check_ble_advertise_id adv_id %d invalid", adv_id);
        return BT_RESULT_FAILED;
    }

    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-STOP: %d", client->advertiser_id);
    SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_adv(client->advertiser_id);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("service ble stop adv fail, err:%" PRIu32, ret);
        remove_advertise_handle(client);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

BTS_IF_X(ble_set_static_identity, bt_device_t* device)
{
    BTS_IFX(ble_set_static_identity, device);
}

BTS_IF_X(ble_set_public_identity, bt_device_t* device)
{
    BTS_IFX(ble_set_public_identity, device);
}

BTS_IF_X(ble_get_current_irk)
{
    BTS_IFX(ble_get_current_irk);
}

BTS_IF_X(ble_set_address, bt_device_t* device)
{
    BTS_IFX(ble_set_address, device);
}

BTS_IF_X(ble_get_address)
{
    BTS_IFX(ble_get_address);
}

BTS_IF_X(ble_set_bonded_devices, ble_keys_t* bonded_device_list, uint8_t count_in)
{
    return BT_RESULT_FAILED;
}

BTS_IF_X(ble_connect, ble_connect_params_t* conn_param)
{
    BTS_IFX(ble_connect, conn_param);
}

BTS_IF_X(ble_disconnect, bt_device_t* device)
{
    BTS_IFX(ble_disconnect, device);
}

BTS_IF_X(ble_smp_reply, spp_reply_data_t* reply_data)
{
    BTS_IFX(ble_smp_reply, reply_data);
}

BTS_IF_X(ble_add_white_list, bt_device_t* device)
{
    BTS_IFX(ble_add_white_list, device);
}

BTS_IF_X(ble_remove_white_list, bt_device_t* device)
{
    BTS_IFX(ble_remove_white_list, device);
}

BTS_IF_X(ble_add_resolving_list, bt_device_t* device)
{
    BTS_IFX(ble_add_resolving_list, device);
}

BTS_IF_X(ble_remove_resolving_list, bt_device_t* device)
{
    BTS_IFX(ble_remove_resolving_list, device);
}

BTS_IF_X(ble_set_phy, bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy)
{
    BTS_IFX(ble_set_phy, device, tx_phy, rx_phy);
}
static bt_result_code bts_if_ble_listen_l2cap_channel(void* gap_handle, uint8_t psm, ble_l2cap_config_option_t* opt)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_listen_l2cap_channel(psm, opt);
    return ret;
}
static bt_result_code bts_if_ble_send_packet(void* gap_handle, bt_device_t* device, uint16_t cid, uint8_t* packet, uint16_t packet_size)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_send_packet(device, cid, packet, packet_size);
    return ret;
}

BTS_IF_X(enter_bluetooth_test_mode, test_mode mode)
{
    BTS_IFX(enter_bluetooth_test_mode, mode);
}

BTS_IF_X(bt_set_page_scan_parameters, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    BTS_IFX(bt_set_page_scan_parameters, scan_type, scan_interval, scan_window);
}

BTS_IF_X(bt_set_inquiry_scan_parameters, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    BTS_IFX(bt_set_inquiry_scan_parameters, scan_type, scan_interval, scan_window);
}

BTS_IF_X(bt_reply_link_request, bt_address remote_addr, bool accept)
{
    BTS_IFX(bt_reply_link_request, remote_addr, accept);
}

BTS_IF_X(bt_set_afh_channel_classification, bt_afh_radio_channel_info_t* channels, uint16_t number)
{
    BTS_IFX(bt_set_afh_channel_classification, channels, number);
}

static bt_result_code bts_if_gap_cleanup(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    struct list_node* list = &gap_service->handle_list;
    struct list_node* node;
    bt_if_gap_handle_t* if_handle = NULL;
    list_for_every(list, node)
    {
        if_handle = (bt_if_gap_handle_t*)node;
        if (if_handle->gap_handle == gap_handle)
            break;
    }
    if (gap_handle == if_handle->gap_handle)
        list_delete(node);
    if (list_is_empty(list))
        gap_cleanup();

    ret = BT_RESULT_SUCCESS;
    return ret;
}

static btm_gap_interface_t gap_interface = {
    .size = sizeof(btm_gap_interface_t),
    .gap_register_callbacks = bts_if_register_callbacks,
    .gap_cleanup = bts_if_gap_cleanup,
    .bt_start_discovery = bts_if_start_discovery,
    .bt_set_local_address = bts_if_set_local_address,
    .bt_get_local_address = bts_if_get_local_address,
    .bt_set_local_io_capability = bts_if_set_local_io_capability,
    .bt_set_local_name = bts_if_set_local_name,
    .bt_get_local_name = bts_if_get_local_name,
    .bt_set_local_device_class = bts_if_set_local_device_class,
    .bt_get_local_device_class = bts_if_get_local_device_class,
    .bt_get_remote_name = bts_if_get_remote_name,
    .bt_get_remote_services = bts_if_get_remote_services,
    .bt_reply_pair_request = bts_if_reply_pair_request,
    .bt_ssp_reply = bts_if_ssp_reply,
    .bt_create_bond = bts_if_create_bond,
    .bt_cancel_bond = bts_if_cancel_bond,
    .bt_remove_bond = bts_if_remove_bond,
    .bt_get_bonded_devices = bts_if_get_bonded_devices,
    .bt_get_connected_devices = bts_if_get_connected_devices,
    .bt_set_scan_mode = bts_if_set_scan_mode,
    .bt_start_discovery = bts_if_start_discovery,
    .bt_stop_discovery = bts_if_stop_discovery,
    .bt_start_service_discovery = bts_if_start_service_discovery,
    .bt_stop_service_discovery = bts_if_stop_service_discovery,
    .bt_set_link_role = bts_if_set_link_role,
    .bt_disconnect_link = bts_if_disconnect_bt_link,
    .bt_enable_ctkd_bonding = bts_if_enable_ctkd_bonding,
    .bt_set_auto_sniff = bts_if_set_auto_sniff,
#ifdef HCI_VSC_COMMAND
    .bt_send_hci_command = bts_if_send_hci_command,
#endif
    .ble_start_advertising = bts_ble_start_advertising,
    .ble_stop_advertising = bts_ble_stop_advertising,
    .ble_set_static_identity = bts_if_ble_set_static_identity,
    .ble_set_public_identity = bts_if_ble_set_public_identity,
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
    .ble_add_irk_to_resolving_list = bts_if_ble_add_irk_to_resolving_list,
    .ble_set_phy = bts_if_ble_set_phy,
    .ble_listen_l2cap_channel = bts_if_ble_listen_l2cap_channel,
    .ble_send_packet = bts_if_ble_send_packet,
    .ble_get_bonded_devices = bts_if_get_ble_bonded_devices,
    .ble_get_connected_devices = bts_if_get_ble_connected_devices,
    .ble_get_whitelist_devices = bts_if_get_ble_whitelist_devices,
    .ble_get_resolvinglist_devices = bts_if_get_ble_resolvinglist_devices,
    .enter_bluetooth_test_mode = bts_if_enter_bluetooth_test_mode,
    .bt_set_inquiry_scan_parameters = bts_if_set_inquiry_scan_parameters,
    .bt_set_page_scan_parameters = bts_if_set_page_scan_parameters,
    .bt_reply_link_request = bts_if_reply_link_request,
    .bt_set_afh_channel_classification = bts_if_set_afh_channel_classification,
};

btm_gap_interface_t* get_gap_service_instance(void)
{
    return &gap_interface;
}
