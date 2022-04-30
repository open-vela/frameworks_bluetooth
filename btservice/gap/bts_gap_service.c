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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"

#include "btm_gap.h"
#include "btm_manager.h"
#include "bts_gap.h"
#include "bts_service.h"
#include <nuttx/list.h>
#define LOG_TAG "bts_gap_service"
#include "log.h"

#define BT_GAP_CB(MOTHOD, ...)                                                          \
    do {                                                                                \
        bt_if_gap_handle_t* if_handle;                                                  \
        struct list_node* handle_node;                                                  \
        if (!g_gap_service)                                                             \
            break;                                                                      \
        struct list_node* list = &g_gap_service->handle_list;                           \
        list_for_every(list, handle_node)                                               \
        {                                                                               \
            if_handle = (bt_if_gap_handle_t*)handle_node;                               \
            if ((if_handle->gap_callbacks) && (if_handle->gap_callbacks)->MOTHOD) {     \
                (if_handle->gap_callbacks)->MOTHOD(if_handle->gap_handle, __VA_ARGS__); \
            } else {                                                                    \
                BT_LOGE("%s GAP interface is NULL", __func__);                          \
            }                                                                           \
        }                                                                               \
    } while (0)

#define BT_GAP_CB2(MOTHOD, COD, ...)                                                         \
    do {                                                                                     \
        bt_if_gap_handle_t* if_handle;                                                       \
        struct list_node* handle_node;                                                       \
        if (!g_gap_service)                                                                  \
            break;                                                                           \
        struct list_node* list = &g_gap_service->handle_list;                                \
        list_for_every(list, handle_node)                                                    \
        {                                                                                    \
            if_handle = (bt_if_gap_handle_t*)handle_node;                                    \
            if ((if_handle->gap_callbacks) && (if_handle->gap_callbacks)->MOTHOD && (COD)) { \
                (if_handle->gap_callbacks)->MOTHOD(if_handle->gap_handle, __VA_ARGS__);      \
                break;                                                                       \
            }                                                                                \
        }                                                                                    \
    } while (0)

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

static struct list_node ble_advertiser_list = LIST_INITIAL_VALUE(ble_advertiser_list);

bt_gap_service_t* g_gap_service = NULL;

static bool gap_is_handle_valid(void* gap_handle)
{
    struct list_node* list = &g_gap_service->handle_list;
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
    bts_leadv_hdl_t* handle = list_peek_tail_type(&ble_advertiser_list, bts_leadv_hdl_t, node);
    uint8_t adv_id = handle ? (handle->advertiser_id + 1) % BLE_MAX_ADV_NUM : 0;

    for (uint8_t i = 0; i < BLE_MAX_ADV_NUM; i++) {
        bool found = false;
        list_for_every_entry(&ble_advertiser_list, handle, bts_leadv_hdl_t, node)
        {
            if (handle->advertiser_id == adv_id) {
                found = true;
                break;
            }
        }
        if (!found) {
            return adv_id;
        }
        adv_id = (handle->advertiser_id + 1) % BLE_MAX_ADV_NUM;
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

static void gap_if_received_remote_name_callback(BD_ADDR bd_addr, char* bt_name, uint8_t length)
{
    BT_GAP_CB(received_remote_name_callback_cb, bd_addr, bt_name, length);
}

static void gap_if_discovery_state_changed_callback(bt_discovery_state state)
{
    BT_GAP_CB(discovery_state_changed_callback_cb, state);
}

static void gap_if_ssp_request_callback(ssp_request_data_t* request_data)
{
    BT_GAP_CB(ssp_request_callback_cb, request_data);
}

static void gap_if_pairing_request_callback(BD_ADDR remote_addr, bool local_initiate, bool is_bondable)
{
    BT_GAP_CB(pairing_request_cb, remote_addr, local_initiate, is_bondable);
}

static void gap_if_device_found_callback(bt_device_t* device)
{
    BT_GAP_CB(device_found_callback_cb, device);
}

static void gap_if_bond_state_changed_callback(bt_device_t* device, bt_bond_state state)
{
    BT_GAP_CB(bond_state_changed_callback_cb, device, state);
}

static void gap_if_connection_state_callback(bt_device_t* device, bt_connection_state state)
{
    BT_GAP_CB(connection_state_callback_cb, device, state);
}

static void gap_if_hci_event_callback(hci_event_t* hci_event)
{
    BT_GAP_CB(hci_event_callback_cb, hci_event);
}

static void gap_if_adapter_state_changed_callback(SERVICE_BT_STACK_STATE state)
{
    BT_LOGD("%s", __func__);
}

static void gap_if_ble_advtise_started_callback(uint8_t adv_id)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-STARTED");
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
    if (!client) {
        SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_adv(adv_id);
        BT_LOGW("stop le adv, adv id:%d, ret:%" PRIu32, adv_id, ret);
        return;
    }

    BT_GAP_CB2(ble_adv_started_cb, if_handle->gap_handle == client->gap_handle, adv_id);
}

static void gap_if_ble_advtise_stopped_callback(uint8_t adv_id)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-STOPPED");
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
    if (!client) {
        SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_adv(adv_id);
        BT_LOGW("stop le adv, adv id:%d, ret:%" PRIu32, adv_id, ret);
        return;
    }

    BT_GAP_CB2(ble_adv_stopped_cb, if_handle->gap_handle == client->gap_handle, adv_id);
    remove_advertise_handle(client);
}

static void gap_if_smp_request_callback(ssp_request_data_t* request_data)
{
    BT_GAP_CB(smp_requeset_cb, request_data);
}

static void gap_if_ble_phy_update_callback(bt_address remote_addr, ble_phy_type tx_phy, ble_phy_type rx_phy, bt_status status)
{
    BT_GAP_CB(ble_phy_update_cb, remote_addr, tx_phy, rx_phy, status);
}

static void gap_if_ble_address_callback(bt_address bd_addr, ble_addr_type addr_type)
{
    BT_GAP_CB(ble_address_cb, bd_addr, addr_type);
}
static void gap_if_bts_ble_irk_callback(bt_common_key irk, bt_address ble_addr, ble_addr_type addr_type)
{
    BT_GAP_CB(ble_irk_cb, irk, ble_addr, addr_type);
}

static void gap_if_delete_linkey_callback(bt_address remote_addr, bt_status reason)
{
    BT_GAP_CB(delete_linkey_cb, remote_addr, reason);
}

static void gap_if_link_connect_request_callback(bt_address remote_addr)
{
    BT_GAP_CB(link_connect_request_cb, remote_addr);
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
    .smp_request_cb = gap_if_smp_request_callback,
    .ble_phy_update_cb = gap_if_ble_phy_update_callback,
    .ble_address_cb = gap_if_ble_address_callback,
    .ble_irk_cb = gap_if_bts_ble_irk_callback,
    .delete_linkey_cb = gap_if_delete_linkey_callback,
    .link_connect_request_cb = gap_if_link_connect_request_callback,
};

bt_result_code gap_service_init()
{
    if (!g_gap_service) {
        g_gap_service = (bt_gap_service_t*)malloc(sizeof(bt_gap_service_t));
        gap_init(&bts_gap_callbacks);
        list_initialize(&g_gap_service->handle_list);
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code bts_if_register_callbacks(void* handle, void** gap_handle, const btm_gap_callbacks_t* callbacks)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (gap_is_handle_valid(*gap_handle))
        return ret;
    if (!g_gap_service)
        return ret;

    bt_if_gap_handle_t* gap_if_handle = (bt_if_gap_handle_t*)malloc(sizeof(bt_if_gap_handle_t));
    gap_if_handle->gap_callbacks = callbacks;
    gap_if_handle->gap_handle = *gap_handle;
    list_add_tail(&g_gap_service->handle_list, &gap_if_handle->node);
    return BT_RESULT_SUCCESS;
}

static bt_result_code bts_if_start_discovery(void* gap_handle, uint32_t timeout)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    return bts_start_discovery(timeout);
}
static bt_result_code bts_if_set_local_name(void* gap_handle, char* bt_name, uint8_t len)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    return bts_set_local_name(bt_name, len);
}

/*Local property*/
static bt_result_code bts_if_set_local_address(void* gap_handle, bt_device_t* device)
{
    return BT_RESULT_FAILED;
}
static bt_result_code bts_if_get_local_address(void* gap_handle, bt_address addr)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_local_address(addr);
    return ret;
}
static bt_result_code bts_if_set_local_io_capability(void* gap_handle, bt_io_capability io_capability)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    return bts_set_local_io_capability(io_capability);
}
static char* bts_if_get_local_name(void* gap_handle)
{
    char* name = NULL;
    if (!gap_is_handle_valid(gap_handle))
        return name;
    name = bts_get_local_name();
    return name;
}
static bt_result_code bts_if_set_local_device_class(void* gap_handle, uint32_t class_of_device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_set_local_device_class(class_of_device);
    return ret;
}
static uint32_t bts_if_get_local_device_class(void* gap_handle)
{
    uint32_t class = 0;
    if (!gap_is_handle_valid(gap_handle))
        return class;
    return class = bts_get_local_device_class();
}

/*Remote device*/
static bt_result_code bts_if_get_remote_name(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_remote_name(device);
    return ret;
}

static int bts_if_get_remote_services(void* gap_handle, bt_device_t* remote_addr, bt_uuid_t* service_list, uint8_t count_in)
{
    int service_num = 0;
    if (!gap_is_handle_valid(gap_handle))
        return service_num;
    service_num = bts_get_remote_services(remote_addr, service_list, count_in);
    return service_num;
}

static bt_result_code bts_if_reply_pair_request(void* gap_handle, bt_device_t* device, int accept)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_reply_pair_request(device, accept);
    return ret;
}
static bt_result_code bts_if_ssp_reply(void* gap_handle, spp_reply_data_t* reply_data)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ssp_reply(reply_data);
    return ret;
}
static bt_result_code bts_if_create_bond(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_create_bond(device);
    return ret;
}

static bt_result_code bts_if_cancel_bond(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_cancel_bond(device);
    return ret;
}

static bt_result_code bts_if_remove_bond(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_remove_bond(device);
    return ret;
}

static int bts_if_get_bonded_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_bonded_devices(device_list, max_out);
    return ret;
}

static int bts_if_get_connected_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_connected_devices(device_list, max_out);
    return ret;
}

static int bts_if_get_ble_bonded_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_ble_bonded_devices(device_list, max_out);
    return ret;
}

static int bts_if_get_ble_connected_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_ble_connected_devices(device_list, max_out);
    return ret;
}
static int bts_if_get_ble_whitelist_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_ble_whitelist_devices(device_list, max_out);
    return ret;
}
static int bts_if_get_ble_resolvinglist_devices(void* gap_handle, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_get_ble_resolvinglist_devices(device_list, max_out);
    return ret;
}
/*Discovery*/
static bt_result_code bts_if_set_scan_mode(void* gap_handle, bt_scan_mode scan_mode, bool bondable)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_set_scan_mode(scan_mode, bondable);
    return ret;
}

static bt_result_code bts_if_stop_discovery(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_stop_discovery();
    return ret;
}

/*service discovery*/
static bt_result_code bts_if_start_service_discovery(void* gap_handle, bt_device_t* device, bt_uuid_t uuid)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_start_service_discovery(device, uuid);
    return ret;
}
static bt_result_code bts_if_stop_service_discovery(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_stop_service_discovery(device);
    return ret;
}
static bt_result_code bts_if_set_link_role(void* gap_handle, bt_device_t* device, bt_link_role role)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_set_link_role(device, role);
    return ret;
}
#ifdef HCI_VSC_COMMAND
/*VSC command*/
static bt_result_code bts_if_send_hci_command(void* gap_handle, bt_hci_command_t* command, hci_command_complete_event event_type)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_send_hci_command(command, event_type);
    return ret;
}
#endif

static bt_result_code bts_ble_start_advertising(void* gap_handle, advertise_param_t* param)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-START");
    BT_ASSERT(!param, BT_RESULT_FAILED);
    bts_leadv_hdl_t* client = add_advertise_handle(gap_handle);
    BT_ASSERT(!client, BT_RESULT_FAILED);
    param->adv_id = client->advertiser_id;

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

static bt_result_code bts_ble_stop_advertising(void* gap_handle, uint8_t adv_id)
{
    BT_LOGD("PERFORMANCE-LE-GAP-PROFILE-BLUELET-ADVERTISE-STOP");
    bts_leadv_hdl_t* client = find_advertise_handle(adv_id);
    BT_ASSERT(!client, BT_RESULT_FAILED);

    if (!bts_check_ble_advertise_id(gap_handle, adv_id)) {
        BT_LOGE("fail, check_ble_advertise_id adv_id %d invalid", adv_id);
        return BT_RESULT_FAILED;
    }

    SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_adv(client->advertiser_id);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("service ble stop adv fail, err:%" PRIu32, ret);
        remove_advertise_handle(client);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code bts_if_ble_set_static_identity(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_set_static_identity(device);
    return ret;
}

static bt_result_code bts_if_ble_set_public_identity(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_set_public_identity(device);
    return ret;
}

static bt_result_code bts_if_ble_get_current_irk(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_get_current_irk();
    return ret;
}
static bt_result_code bts_if_ble_set_address(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_set_address(device);
    return ret;
}
static bt_result_code bts_if_ble_get_address(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_get_address();
    return ret;
}
static bt_result_code bts_if_ble_set_bonded_devices(void* gap_handle, ble_keys_t* bonded_device_list, uint8_t count_in)
{
    return BT_RESULT_FAILED;
}
static bt_result_code bts_if_ble_connect(void* gap_handle, ble_connect_params_t* conn_param)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_connect(conn_param);
    return ret;
}
static bt_result_code bts_if_ble_disconnect(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_disconnect(device);
    return ret;
}
static bt_result_code bts_if_ble_smp_reply(void* gap_handle, spp_reply_data_t* reply_data)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_smp_reply(reply_data);
    return ret;
}
static bt_result_code bts_if_ble_add_white_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_add_white_list(device);
    return ret;
}
static bt_result_code bts_if_ble_remove_white_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_remove_white_list(device);
    return ret;
}
static bt_result_code bts_if_ble_add_resolving_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_add_resolving_list(device);
    return ret;
}
static bt_result_code bts_if_ble_remove_resolving_list(void* gap_handle, bt_device_t* device)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_remove_resolving_list(device);
    return ret;
}
static bt_result_code bts_if_ble_set_phy(void* gap_handle, bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_set_phy(device, tx_phy, rx_phy);
    return ret;
}
static bt_result_code bts_if_ble_add_private_channel(void* gap_handle, uint16_t private_cid)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_add_private_channel(private_cid);
    return ret;
}
static bt_result_code bts_if_ble_send_packet(void* gap_handle, bt_device_t* device, uint16_t private_cid, uint8_t* packet, uint16_t packet_size)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_ble_send_packet(device, private_cid, packet, packet_size);
    return ret;
}

static bt_result_code bts_if_enter_bluetooth_test_mode(void* gap_handle, test_mode mode)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    ret = bts_enter_bluetooth_test_mode(mode);
    return ret;
}

static bt_result_code bts_if_set_page_scan_parameters(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    if (!gap_is_handle_valid(gap_handle))
        return BT_RESULT_FAILED;
    return bts_set_page_scan_parameters(scan_type, scan_interval, scan_window);
}

static bt_result_code bts_if_set_inquiry_scan_parameters(void* gap_handle, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    if (!gap_is_handle_valid(gap_handle))
        return BT_RESULT_FAILED;
    return bts_set_inquiry_scan_parameters(scan_type, scan_interval, scan_window);
}

static bt_result_code bts_if_reply_link_request(void* gap_handle, bt_address remote_addr, bool accept)
{
    if (!gap_is_handle_valid(gap_handle))
        return BT_RESULT_FAILED;
    return bts_reply_link_request(remote_addr, accept);
}

static bt_result_code bts_if_gap_cleanup(void* gap_handle)
{
    bt_result_code ret = BT_RESULT_FAILED;
    if (!gap_is_handle_valid(gap_handle))
        return ret;
    struct list_node* list = &g_gap_service->handle_list;
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
    .ble_set_phy = bts_if_ble_set_phy,
    .ble_add_private_channel = bts_if_ble_add_private_channel,
    .ble_send_packet = bts_if_ble_send_packet,
    .ble_get_bonded_devices = bts_if_get_ble_bonded_devices,
    .ble_get_connected_devices = bts_if_get_ble_connected_devices,
    .ble_get_whitelist_devices = bts_if_get_ble_whitelist_devices,
    .ble_get_resolvinglist_devices = bts_if_get_ble_resolvinglist_devices,
    .enter_bluetooth_test_mode = bts_if_enter_bluetooth_test_mode,
    .bt_set_inquiry_scan_parameters = bts_if_set_inquiry_scan_parameters,
    .bt_set_page_scan_parameters = bts_if_set_page_scan_parameters,
    .bt_reply_link_request = bts_if_reply_link_request,
};

btm_gap_interface_t* get_gap_service_instance(void)
{
    return &gap_interface;
}
