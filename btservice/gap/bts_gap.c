/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_gap.c
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

#define LOG_TAG "bts_gap"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"

#include "btm_manager.h"
#include "bts_gap.h"
#include "bts_le_advertise.h"
#include "bts_le_scan.h"
#include "bts_service.h"
#include "bts_service_interface.h"

#include "log.h"
#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define KEEP_ALIVE_INTERVAL 10000

#define BTS_ADPT(name) .name = bts_adapt_##name

#define BTS_ADPTXX(name, initializer_list, ...)         \
    do {                                                \
        initializer_list;                               \
        ret = service_adapter_gap_##name(__VA_ARGS__);  \
        if (ret != BT_RESULT_SUCCESS) {                 \
            BT_LOGE("%s, ret:%" PRIu32, __func__, ret); \
            return ret;                                 \
        }                                               \
        return ret;                                     \
    } while (0)

#define BTS_ADPTX(name, ...) BTS_ADPTXX(name, bt_result_code ret = BT_RESULT_FAILED, ##__VA_ARGS__)

#define BTS_CB_X(name, ...) static BTS_ADPT_XX(name, void, ##__VA_ARGS__)

#define BTS_CB(callback, ...)                                   \
    do {                                                        \
        if (bts_gap_callbacks && bts_gap_callbacks->callback) { \
            bts_gap_callbacks->callback(__VA_ARGS__);           \
        } else {                                                \
            BT_LOGE("gap_callbacks->" #callback "is NULL");     \
        }                                                       \
    } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum {
    GAP_STACK_STATE_CHANGED = 1,
    GAP_DEVICE_FOUND,
    GAP_REMOTE_NAME,
    GAP_DISCOVERY_STATE_CHANGED,
    GAP_PIN_CODE_REQUEST,
    GAP_SSP_REQUEST,
    GAP_BOND_STATE_CHANGED,
    GAP_ACL_STATE_CHANGED,
    GAP_BLE_SCAN_REQUEST,
    GAP_BLE_ADV_STARTED,
    GAP_BLE_ADV_STOPPED,
    GAP_LINK_ROLE_CHANGED,
    GAP_LINK_MODE_CHANGED,
    GAP_SCAN_MODE_CHANGED,
    GAP_LINK_CONNECT_REQUEST,
    GAP_LINK_POLICY_CHANGED,
    GAP_HCI_EVENT,
    GAP_UPDATE_BR_LINKKEY,
    GAP_DELETE_BR_LINKKEY,
    GAP_PAIR_REQUEST,
    GAP_CONNECT_REQUEST,
    GAP_SERVICE_DISCOVERED,
    GAP_LINK_ENCRYPTION_STATE_CHANGED,
    GAP_SMP_REQUEST,
    GAP_UPDATE_BLE_BONDED_DEVICES,
    GAP_BLE_ADD_WHITE_LIST,
    GAP_BLE_REMOVE_WHITE_LIST,
    GAP_ADD_BLE_RESOlVING_LIST,
    GAP_REMOVE_BLE_RESOlVING_LIST,
    GAP_BLE_ADDRESS,
    GAP_BLE_PHY_UPDATE,
    GAP_BLE_IRK,
    GAP_BLE_L2CAP_CONN_STATE,
    GAP_BLE_PACKET_RECEIVED,
    GAP_BLE_PACKET_SENT,
    GAP_BR_LINK_KEY_CHANGED,
    GAP_DELETE_LINK_KEY_CHANGED,
    GAP_EVENT_MAX_ID,
    GAP_BLE_CONNECTION_UPDATE,
} gap_event_t;

typedef SERVICE_REMOTE_DEVICE_S remote_device_t;

typedef struct {
    char* bt_name;
    uint8_t length;
} remote_name_info_t;

typedef struct {
    bool local_initiate;
    bool is_bondable;
} pair_request_t;

typedef struct {
    br_service_t* services;
    uint16_t size;
} discovery_service_t;

typedef struct {
    bool br_link;
    bool encryption_on;
} link_encryption_t;

typedef struct {
    ble_keys_t* bonded_device_list;
    uint8_t count_in;
} ble_bonded_update_t;

typedef struct {
    ble_phy_type tx_phy;
    ble_phy_type rx_phy;
} phy_update_changed_t;

typedef struct {
    uint16_t cid;
    uint8_t* packet;
    uint16_t packet_size;
} ble_packet_receive_t;

typedef struct {
    uint16_t connection_interval;
    uint16_t peripheral_latency;
    uint16_t supervision_timeout;
} ble_connection_update_t;

typedef struct {
    ble_l2cap_state state;
    uint16_t psm;
    uint16_t cid;
    uint16_t mtu;
    uint16_t mps;
} ble_l2cap_conn_state_t;

typedef struct {
    bt_address bd_addr;
    ble_addr_type addr_type;
    uint32_t valueint1;
    bt_status status;
    union {
        remote_device_t* found_result;
        bt_discovery_state discovery_state;
        pin_request_data_t pin_request_data;
        ssp_request_data_t ssp_request_data;
        acl_state_params_t acl_state_params;
        scan_result_t scan_result;
        bt_link_role link_role;
        bt_link_mode link_mode;
        bt_link_policy link_policy;
        bt_service_state stack_state;
        hci_event_t hci_event;
        remote_name_info_t remote_name;
        bt_bond_state bond_state;
        bt_scan_mode scan_mode;
        remote_device_t bonded_device;
        pair_request_t pair_request;
        discovery_service_t discovery_service;
        link_encryption_t link_encryption;
        ble_bonded_update_t ble_bonded_update;
        phy_update_changed_t phy_update;
        bt_common_key irk;
        ble_packet_receive_t ble_packet_receive;
        bt_device_t device;
        ble_connection_update_t ble_conn_update;
        ble_l2cap_conn_state_t ble_l2cap_conn;
    } data;
} gap_event_data_t;

typedef struct {
    struct list_node node;
    gap_event_t event;
    gap_event_data_t event_data;
} gap_msg_t;

typedef enum {
    BT_ROLE_UNKOWN,
    BT_ROLE_BR_MASTER,
    BT_ROLE_BR_SLVAER,
    BT_ROLE_BLE_MASTER,
    BT_ROLE_BLE_SLVAER,
} bt_link_role_t;

typedef struct
{
    struct list_node node;
    bt_address remote_address;
    bt_link_role_t role;
    uv_timer_t* timer;
    bt_acl_state pre_state;
    bt_acl_state current_state;
} bluetooth_device_t;

struct list_node* g_msg_list;
static bts_gap_callback_t* g_bts_gap_callbacks = NULL;

/*process callback from stack */

extern void InitTransportLayer(void);

static bluetooth_device_t* find_bt_device(bt_address remote_address)
{
    bluetooth_device_t* device;
    list_for_every_entry(&bt_device_list, device, bluetooth_device_t, node)
    {
        if (!memcmp(device->remote_address, remote_address, sizeof(bt_address))) {
            return device;
        }
    }
    return NULL;
}

static bluetooth_device_t* add_bt_device(bt_address remote_address)
{
    bluetooth_device_t* device = (bluetooth_device_t*)malloc(sizeof(bluetooth_device_t));
    if (!device) {
        BT_LOGE("malloc device fail");
        return NULL;
    }

    memset(device, 0, sizeof(bluetooth_device_t));
    memcpy(device->remote_address, remote_address, sizeof(bt_address));
    device->current_state = BT_ACL_STATE_DISCONNECTED;
    device->pre_state = BT_ACL_STATE_DISCONNECTED;
    list_add_tail(&bt_device_list, &device->node);
    return device;
}

static bool remove_bt_device(bluetooth_device_t* device)
{
    if (!device) {
        return false;
    }
    list_delete(&device->node);
    free(device);
    return true;
}

static gap_msg_t* gap_msg_new(gap_event_t event)
{
    gap_msg_t* msg;
    if (!g_msg_list)
        return NULL;
    msg = (gap_msg_t*)malloc(sizeof(gap_msg_t));
    if (!msg)
        return NULL;

    msg->event = event;
    memset(&msg->event_data, 0, sizeof(msg->event_data));
    return msg;
}

static bts_gap_callback_t* bts_gap_callbacks = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void bluetooth_state_change_callback(bluetooth_device_t* device)
{
    bt_device_t new_device = { 0 };

    memcpy(new_device.addr, device->remote_address, BT_ADDR_LENGTH);
    g_bts_gap_callbacks->connection_state_changed_cb(&new_device, device->current_state);

    if (device->current_state == BT_ACL_STATE_LE_DISCONNECTED
        || device->current_state == BT_ACL_STATE_DISCONNECTED) {
        remove_bt_device(device);
    }
}

static void bt_connection_state_timeout(char* data)
{
    bluetooth_device_t* device = (bluetooth_device_t*)data;

    BT_LOGD("%s addr:%s", __func__, addr_str(device->remote_address));

    stop_timer(device->timer);
    device->timer = NULL;

    if (device->pre_state != device->current_state) {
        bluetooth_state_change_callback(device);
    }
}

static void process_loop_in_gap(void* data, size_t data_size)
{
    gap_msg_t* gap_msg = (gap_msg_t*)(data);
    if (!gap_msg) {
        BT_LOGE("%s fail, gap_msg null", __func__);
        return;
    }
    switch (gap_msg->event) {
    case GAP_STACK_STATE_CHANGED: {
        get_bluetooth_service_interface()->stack_state_change(gap_msg->event_data.data.stack_state);
        break;
    }
    case GAP_DISCOVERY_STATE_CHANGED: {
        BTS_CB(discovery_state_changed_cb, gap_msg->event_data.data.discovery_state);
        break;
    }
    case GAP_REMOTE_NAME: {
        BTS_CB(received_remote_name_cb, gap_msg->event_data.bd_addr,
            gap_msg->event_data.data.remote_name.bt_name,
            gap_msg->event_data.data.remote_name.length);
        if (gap_msg->event_data.data.remote_name.bt_name)
            free(gap_msg->event_data.data.remote_name.bt_name);
        gap_bt_bond_store();
        break;
    }
    case GAP_SSP_REQUEST: {
        BTS_CB(ssp_request_cb, &gap_msg->event_data.data.ssp_request_data);
        break;
    }
    case GAP_PAIR_REQUEST: {
        BTS_CB(pairing_request_cb, gap_msg->event_data.bd_addr, gap_msg->event_data.data.pair_request.local_initiate,
            gap_msg->event_data.data.pair_request.is_bondable);
        break;
    }

    case GAP_DEVICE_FOUND: {
        BTS_CB(device_found_cb, &gap_msg->event_data.data.device);
        break;
    }
    case GAP_BOND_STATE_CHANGED: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->bond_state_changed_cb)) {
            bt_device_t new_device = { 0 };

            memcpy(new_device.addr, gap_msg->event_data.bd_addr, BT_ADDR_LENGTH);
            g_bts_gap_callbacks->bond_state_changed_cb(&new_device, gap_msg->event_data.data.bond_state);
        }
        break;
    }
    case GAP_ACL_STATE_CHANGED: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->connection_state_changed_cb)) {
            bt_address bt_addr;
            bluetooth_device_t* device;
            bt_acl_state acl_state;
            int timeout;

            acl_state = gap_msg->event_data.data.acl_state_params.state;
            memcpy(bt_addr, gap_msg->event_data.data.acl_state_params.remote_addr, BT_ADDR_LENGTH);

            device = find_bt_device(bt_addr);
            if (!device) {
                device = add_bt_device(bt_addr);
            }
            device->pre_state = device->current_state;
            device->current_state = acl_state;

            if (device->role == BT_ROLE_BLE_MASTER && device->timer) {
                BT_LOGD("delay remote le master(%s) state :%d", addr_str(device->remote_address),
                    device->current_state);
                return;
            }

            if (device->current_state == BT_ACL_STATE_LE_CONNECTING) {
                BT_LOGD("start_timer remote le master(%s) connecting", addr_str(bt_addr));
                device->role = BT_ROLE_BLE_MASTER;
                device->pre_state = BT_ACL_STATE_LE_CONNECTING;
                timeout = CONFIG_BLUETOOTH_TGAP_CONN_INTERVAL_MAX * 6 + BLUETOOTH_SCHEDULE_DELAY_MS;
                device->timer = start_timer(timeout, 0, bt_connection_state_timeout, device);
            }

            bluetooth_state_change_callback(device);
        }
        break;
    }
    case GAP_HCI_EVENT: {
        BTS_CB(hci_event_cb, &gap_msg->event_data.data.hci_event);
        if (gap_msg->event_data.data.hci_event.length != 0)
            free(gap_msg->event_data.data.hci_event.params);
        break;
    }
    case GAP_BLE_ADV_STARTED: {
        BTS_CB(ble_adv_started_cb, gap_msg->event_data.valueint1);
        break;
    }
    case GAP_BLE_ADV_STOPPED: {
        BTS_CB(ble_adv_stopped_cb, gap_msg->event_data.valueint1);
        break;
    }
    case GAP_LINK_MODE_CHANGED: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->link_mode_change_cb)) {
            g_bts_gap_callbacks->link_mode_change_cb(gap_msg->event_data.bd_addr, (gap_msg->event_data.data.link_mode == BT_MODE_ACTIVE) ? true : false, gap_msg->event_data.valueint1);
        }
        break;
    }
    case GAP_SCAN_MODE_CHANGED: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->scan_mode_change_cb)) {
            g_bts_gap_callbacks->scan_mode_change_cb(gap_msg->event_data.data.scan_mode);
        }
        break;
    }
    case GAP_UPDATE_BLE_BONDED_DEVICES: {
        gap_ble_bond_store(gap_msg->event_data.data.ble_bonded_update.bonded_device_list, gap_msg->event_data.data.ble_bonded_update.count_in);
        if (gap_msg->event_data.data.ble_bonded_update.count_in > 0) {
            free(gap_msg->event_data.data.ble_bonded_update.bonded_device_list);
        }
        break;
    }
    case GAP_SMP_REQUEST: {
        BTS_CB(smp_request_cb, &gap_msg->event_data.data.ssp_request_data);
        break;
    }
    case GAP_BLE_PHY_UPDATE: {
        BTS_CB(ble_phy_update_cb, gap_msg->event_data.bd_addr,
            gap_msg->event_data.data.phy_update.tx_phy, gap_msg->event_data.data.phy_update.rx_phy,
            gap_msg->event_data.status);
        break;
    }
    case GAP_BLE_IRK: {
        BTS_CB(ble_irk_cb, gap_msg->event_data.data.irk, gap_msg->event_data.bd_addr, gap_msg->event_data.addr_type);
        break;
    }
    case GAP_BLE_ADDRESS: {
        BTS_CB(ble_address_cb, gap_msg->event_data.bd_addr, gap_msg->event_data.addr_type);
        break;
    }
    case GAP_BR_LINK_KEY_CHANGED: {
        gap_bt_bond_store();
        break;
    }
    case GAP_DELETE_LINK_KEY_CHANGED: {
        gap_bt_bond_store();
        BTS_CB(delete_linkey_cb, gap_msg->event_data.bd_addr, gap_msg->event_data.status);
        break;
    }
    case GAP_SERVICE_DISCOVERED: {
        if (!gap_msg->event_data.data.discovery_service.services)
            free(gap_msg->event_data.data.discovery_service.services);
        break;
    }
    case GAP_LINK_ENCRYPTION_STATE_CHANGED: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->link_encryption_change_cb)) {
            g_bts_gap_callbacks->link_encryption_change_cb(gap_msg->event_data.bd_addr, gap_msg->event_data.data.link_encryption.br_link,
                gap_msg->event_data.data.link_encryption.encryption_on);
        }
        break;
    }
    case GAP_BLE_ADD_WHITE_LIST: {
        gap_ble_whitelist_store_update(true, gap_msg->event_data.bd_addr);
        break;
    }
    case GAP_BLE_REMOVE_WHITE_LIST: {
        gap_ble_whitelist_store_update(false, gap_msg->event_data.bd_addr);
        break;
    }
    case GAP_CONNECT_REQUEST: {
        BTS_CB(link_connect_request_cb, gap_msg->event_data.bd_addr);
        break;
    }
    case GAP_BLE_CONNECTION_UPDATE: {
        BTS_CB(ble_connection_updated_cb, gap_msg->event_data.bd_addr, gap_msg->event_data.status, gap_msg->event_data.data.ble_conn_update.connection_interval,
            gap_msg->event_data.data.ble_conn_update.peripheral_latency, gap_msg->event_data.data.ble_conn_update.supervision_timeout);
        break;
    }
    case GAP_BLE_L2CAP_CONN_STATE: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->ble_l2cap_connection_state_cb)) {
            g_bts_gap_callbacks->ble_l2cap_connection_state_cb(gap_msg->event_data.bd_addr,
                gap_msg->event_data.data.ble_l2cap_conn.state, gap_msg->event_data.data.ble_l2cap_conn.psm, gap_msg->event_data.data.ble_l2cap_conn.cid,
                gap_msg->event_data.data.ble_l2cap_conn.mtu, gap_msg->event_data.data.ble_l2cap_conn.mps);
        }
        break;
    }
    case GAP_BLE_PACKET_RECEIVED: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->ble_packet_received_cb)) {
            g_bts_gap_callbacks->ble_packet_received_cb(
                gap_msg->event_data.bd_addr, gap_msg->event_data.data.ble_packet_receive.cid,
                gap_msg->event_data.data.ble_packet_receive.packet, gap_msg->event_data.data.ble_packet_receive.packet_size);
            free(gap_msg->event_data.data.ble_packet_receive.packet);
        }
        break;
    }
    case GAP_BLE_PACKET_SENT: {
        if ((g_bts_gap_callbacks) && (g_bts_gap_callbacks->ble_packet_sent_cb)) {
            g_bts_gap_callbacks->ble_packet_sent_cb(gap_msg->event_data.bd_addr, (uint16_t)(gap_msg->event_data.valueint1));
        }
        break;
    }
    default: {
        // BT_LOGD("%s, %d not handle", __func__, gap_msg->event);
        break;
    }
    }
}

static gap_msg_t* gap_msg_new(gap_event_t event)
{
    gap_msg_t* msg = (gap_msg_t*)malloc(sizeof(gap_msg_t));
    if (!msg)
        return NULL;

    msg->event = event;
    memset(&msg->event_data, 0, sizeof(msg->event_data));
    return msg;
}

static void gap_msg_destory(gap_msg_t* msg)
{
    free(msg);
}

static void handle_msg_received(bt_profile_id id, void* data, size_t size)
{
    process_loop_in_gap(data, size);
    gap_msg_destory((gap_msg_t*)data);
}

static void gap_send_message(gap_msg_t* msg)
{
    bts_send_uv_msg(BT_PROFILE_GAP_ID, msg, sizeof(gap_msg_t));
}

BTS_CB_X(gap_device_found_cb, remote_device_t* device)
{
    BT_LOGD("%s: PERFORMANCE-GAP-BLUELET-DISCOVERY-START", __func__);
    SERVICE_BT_STATUS ret = service_adapter_gap_start_device_discovery(timeout);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void adapter_device_found_callback(remote_device_t* device)
{
    // BT_LOGD("%s: PERFORMANCE-GAP-BLUELET-DISCOVERY-END", __func__);
    if ((!g_bts_gap_callbacks) || (!g_bts_gap_callbacks->device_found_cb))
        return;
    gap_msg_t* msg = gap_msg_new(GAP_DEVICE_FOUND);
    if (!msg)
        return;

    bt_device_t* new_device = &(msg->event_data.data.device);
    memcpy(new_device->addr, device->bd_addr, BT_ADDR_LENGTH);
    new_device->addr_type = device->addr_type;
    new_device->device_type = device->device_type;
    new_device->cod = device->cod;
    new_device->rssi = device->rssi;
    memcpy(new_device->uuids, device->uuids, MAX_UUID_NUM * sizeof(bt_uuid_t));
    memcpy(new_device->name, device->bt_name, DEVICE_NAME_MAX_LEN + 1);
    gap_send_message(msg);
}

BTS_CB_X(gap_received_remote_name_cb, bt_address bd_addr, char* bt_name, uint8_t length)
{
    gap_msg_t* msg = gap_msg_new(GAP_REMOTE_NAME);

    msg->event_data.data.remote_name.bt_name = (char*)malloc(length + 1);
    memcpy(msg->event_data.data.remote_name.bt_name, bt_name, length);
    msg->event_data.data.remote_name.bt_name[length] = '\0';
    memcpy(msg->event_data.bd_addr, bd_addr, BT_ADDR_LENGTH);
    msg->event_data.data.remote_name.length = length;
    BT_LOGD("%s, addr :%s name:%s", __func__, addr_str(bd_addr), msg->event_data.data.remote_name.bt_name);
    gap_send_message(msg);
}

BTS_CB_X(gap_discovery_state_changed_cb, SERVICE_BT_DISCOVERY_STATE state)
{
    BT_LOGD("%s, state:%d", __func__, state);
    gap_msg_t* msg = gap_msg_new(GAP_DISCOVERY_STATE_CHANGED);
    msg->event_data.data.discovery_state = state;
    gap_send_message(msg);
}

BTS_CB_X(gap_pin_request_cb, SERVICE_PIN_REQUEST_DATA_S* request_data)
{
    gap_msg_t* msg = gap_msg_new(GAP_PIN_CODE_REQUEST);
    memcpy(&msg->event_data.data.pin_request_data, request_data, sizeof(SERVICE_PIN_REQUEST_DATA_S));
    memcpy(msg->event_data.data.pin_request_data.bt_name, request_data->bt_name, sizeof(request_data->bt_name));
    gap_send_message(msg);
}

BTS_CB_X(gap_ssp_request_cb, SERVICE_SSP_REQUEST_DATA_S* request_data)
{
    gap_msg_t* msg = gap_msg_new(GAP_SSP_REQUEST);
    memcpy(&msg->event_data.data.ssp_request_data, request_data, sizeof(SERVICE_SSP_REQUEST_DATA_S));
    memcpy(msg->event_data.data.ssp_request_data.bt_name, request_data->bt_name, sizeof(request_data->bt_name));
    gap_send_message(msg);
}

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

static const char* acl_state_to_str(SERVICE_BT_ACL_STATE state)
{
    switch (state) {
    case SERVICE_BT_ACL_STATE_CONNECTED:
        return "CONNECTED";
    case SERVICE_BT_ACL_STATE_CONNECTING:
        return "CONNECTING";
    case SERVICE_BT_ACL_STATE_CONNECT_REQUEST:
        return "CONNECT_REQUEST";
    case SERVICE_BT_ACL_STATE_DISCONNECTED:
        return "DISCONNECTED";
    case SERVICE_BT_ACL_STATE_LE_CONNECTED:
        return "LE_CONNECTED";
    case SERVICE_BT_ACL_STATE_LE_CONNECTING:
        return "LE_CONNECTING";
    case SERVICE_BT_ACL_STATE_LE_DISCONNECTED:
        return "LE_DISCONNECTED";
    case SERVICE_BT_ACL_STATE_BR_BONDED_FULL:
        return "BR_BONDED_FULL";
    case SERVICE_BT_ACL_STATE_LE_BONDED_FULL:
        return "LE_BONDED_FULL";
    default:
        return "UNKONW";
    }
}

BTS_CB_X(gap_bond_state_changed_cb, BD_ADDR remote_addr, SERVICE_BT_BOND_STATE state)
{
    BT_LOGD("%s:PERFORMANCE-GAP-BLUELET-%s", __func__, bond_state_to_str(state));
    gap_msg_t* msg = gap_msg_new(GAP_BOND_STATE_CHANGED);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.bond_state = state;
    gap_send_message(msg);
}

BTS_CB_X(gap_acl_state_changed_cb, SERVICE_ACL_STATE_PARAM_S* acl_state_param)
{
    BT_LOGD("%s status:%" PRIu32 ", state:%s, reasonCode:0x%" PRIX32 ", device[%s]", __func__,
        acl_state_param->status, acl_state_to_str(acl_state_param->state),
        acl_state_param->reasonCode, addr_str(acl_state_param->remote_addr));
#ifdef CONFIG_BLUETOOTH_A2DP_I2S_OFFLOAD
    g_conn_handle = acl_state_param->connection_handle;
#endif
    gap_msg_t* msg = gap_msg_new(GAP_ACL_STATE_CHANGED);
    memcpy(&msg->event_data.data.acl_state_params, acl_state_param, sizeof(SERVICE_ACL_STATE_PARAM_S));
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_scan_result_cb, SERVICE_SCAN_RESULT_DATA_S* scan_result_data)
{
#if defined(CONFIG_BLUETOOTH_LE_SCAN)
    const bts_le_scan_interface_t* scan_ift = get_bts_lescan_instance();
    BT_CBACK(scan_ift->callbacks, ble_scan_result, (scan_result_t*)scan_result_data);
#endif
}

BTS_CB_X(gap_ble_adv_started_cb, uint8_t adv_id)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_ADV_STARTED);
    msg->event_data.valueint1 = adv_id;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_adv_stopped_cb, uint8_t adv_id)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_ADV_STOPPED);
    msg->event_data.valueint1 = adv_id;
    gap_send_message(msg);
}

BTS_CB_X(gap_bt_link_role_changed_cb, BD_ADDR remote_addr, SERVICE_BT_LINK_ROLE link_role)
{
    BT_LOGI("link role changed:%d address: %s", link_role, addr_str(remote_addr));
    gap_msg_t* msg = gap_msg_new(GAP_LINK_ROLE_CHANGED);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.link_role = link_role;
    gap_send_message(msg);
}

BTS_CB_X(gap_scan_mode_changed_cb, SERVICE_BT_SCAN_MODE scan_mode)
{
    gap_msg_t* msg = gap_msg_new(GAP_SCAN_MODE_CHANGED);
    msg->event_data.data.scan_mode = scan_mode;
    gap_send_message(msg);
}

BTS_CB_X(gap_link_mode_changed_cb, BD_ADDR remote_addr, SERVICE_BT_LINK_MODE link_mode,
    uint16_t sniff_interval)
{
    BT_LOGD("%s, addr :%s, link_mode:%d, sniff_interval:%dms", __func__, addr_str(remote_addr), link_mode, sniff_interval * 625 / 1000);
    gap_msg_t* msg = gap_msg_new(GAP_LINK_MODE_CHANGED);
    msg->event_data.data.link_mode = link_mode;
    msg->event_data.valueint1 = sniff_interval;
    gap_send_message(msg);
}

BTS_CB_X(gap_link_connect_request_cb, BD_ADDR remote_addr)
{
    BT_LOGI("acl link connect request, addr:%s", addr_str(remote_addr));
    gap_msg_t* msg = gap_msg_new(GAP_CONNECT_REQUEST);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    gap_send_message(msg);
}

BTS_CB_X(gap_link_policy_changed_cb, BD_ADDR remote_addr, SERVICE_BT_LINK_POLICY link_policy)
{
    gap_msg_t* msg = gap_msg_new(GAP_LINK_POLICY_CHANGED);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.link_policy = link_policy;
    gap_send_message(msg);
}

BTS_CB_X(gap_stack_state_changed_cb, SERVICE_BT_STACK_STATE stack_state)
{
    bt_service_state state;
    if (stack_state == BT_STATE_ON) {
        state = BTM_STATE_ON;
    } else if (stack_state == BT_STATE_OFF) {
        state = BTM_STATE_OFF;
    } else {
        BT_LOGE("unkown state: %d", stack_state);
        return;
    }
    gap_msg_t* msg = gap_msg_new(GAP_STACK_STATE_CHANGED);
    msg->event_data.data.stack_state = state;
    gap_send_message(msg);
}

BTS_CB_X(gap_hci_event_cb, SERVICE_BT_HCI_EVENT_S* hci_event)
{
    gap_msg_t* msg = gap_msg_new(GAP_HCI_EVENT);
    memcpy(&msg->event_data.data.hci_event, hci_event, sizeof(SERVICE_BT_HCI_EVENT_S));
    if (hci_event->length != 0) {
        msg->event_data.data.hci_event.params = malloc(hci_event->length);
        if (!msg->event_data.data.hci_event.params) {
            BT_LOGE("error, malloc params failed");
            return;
        }
        memcpy(msg->event_data.data.hci_event.params, hci_event->params, hci_event->length);
    }
    gap_send_message(msg);
}

extern int TL_h4_send(uint8_t* pBuf, uint32_t len);

BTS_CB_X(gap_transport_write_packet_cb, uint8_t* hci_packet, uint32_t length)
{
    TL_h4_send(hci_packet, length);
}

BTS_CB_X(gap_update_br_link_key_cb, remote_device_t* bonded_device)
{
    gap_msg_t* msg = gap_msg_new(GAP_BR_LINK_KEY_CHANGED);
    memcpy(&msg->event_data.data.bonded_device, bonded_device, sizeof(remote_device_t));
    gap_send_message(msg);
}

BTS_CB_X(gap_delete_br_link_key_cb, bt_address remote_addr, SERVICE_BT_STATUS reason)
{
    BT_LOGD("%s, addr:%s, reason:%" PRIu32, __func__, addr_str(remote_addr), reason);
    gap_msg_t* msg = gap_msg_new(GAP_DELETE_LINK_KEY_CHANGED);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = reason;
    gap_send_message(msg);
}

BTS_CB_X(gap_pairing_request_cb, bt_address remote_addr, bool local_initiate, bool is_bondable)
{
    gap_msg_t* msg = gap_msg_new(GAP_PAIR_REQUEST);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.pair_request.is_bondable = is_bondable;
    msg->event_data.data.pair_request.local_initiate = local_initiate;
    gap_send_message(msg);
}

BTS_CB_X(gap_service_discovered_cb, BD_ADDR remote_addr, SERVICE_BR_SERVICE_S* services, uint16_t size)
{
    gap_msg_t* msg = gap_msg_new(GAP_SERVICE_DISCOVERED);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.discovery_service.size = size;
    msg->event_data.data.discovery_service.services = malloc(size * sizeof(br_service_t));
    if (!msg->event_data.data.discovery_service.services) {
        BT_LOGE("error, malloc services failed");
        return;
    }
    memcpy(msg->event_data.data.discovery_service.services, services, size * sizeof(br_service_t));
    gap_send_message(msg);
}

BTS_CB_X(gap_link_encryption_state_cb, BD_ADDR remote_addr, bool br_link, bool encryption_on)
{
    gap_msg_t* msg = gap_msg_new(GAP_LINK_ENCRYPTION_STATE_CHANGED);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.link_encryption.br_link = br_link;
    msg->event_data.data.link_encryption.encryption_on = encryption_on;
    gap_send_message(msg);
}

BTS_CB_X(gap_smp_request_cb, SERVICE_SSP_REQUEST_DATA_S* request_data)
{
    gap_msg_t* msg = gap_msg_new(GAP_SMP_REQUEST);
    memcpy(&msg->event_data.data.ssp_request_data, request_data, sizeof(SERVICE_SSP_REQUEST_DATA_S));
    gap_send_message(msg);
}

BTS_CB_X(gap_update_ble_bonded_devices_cb, SERVICE_BLE_KEYS_S* bonded_device_list, uint8_t count_in)
{
    gap_msg_t* msg = gap_msg_new(GAP_UPDATE_BLE_BONDED_DEVICES);
    if (!msg)
        return;

    if (count_in < 1) {
        BT_LOGD("count (%d) of bond devices", count_in);
        msg->event_data.data.ble_bonded_update.bonded_device_list = NULL;
    } else {
        ble_keys_t* devices = (ble_keys_t*)malloc(sizeof(ble_keys_t) * count_in);
        memset(devices, 0, sizeof(ble_keys_t) * count_in);
        memcpy(devices, bonded_device_list, sizeof(ble_keys_t) * count_in);
        msg->event_data.data.ble_bonded_update.bonded_device_list = devices;
    }
    msg->event_data.data.ble_bonded_update.count_in = count_in;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_add_white_list_cb, bt_address remote_addr, SERVICE_BT_STATUS status)
{
    BT_LOGD("%s, addr :%s, status: %" PRIu32, __func__, addr_str(remote_addr), status);
    gap_msg_t* msg = gap_msg_new(GAP_BLE_ADD_WHITE_LIST);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_remove_white_list_cb, bt_address remote_addr, SERVICE_BT_STATUS status)
{
    BT_LOGD("%s, addr :%s, status: %" PRIu32, __func__, addr_str(remote_addr), status);
    gap_msg_t* msg = gap_msg_new(GAP_BLE_REMOVE_WHITE_LIST);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_add_resolving_list_cb, bt_address remote_addr, SERVICE_BT_STATUS status)
{
    BT_LOGD("%s, addr :%s, status: %" PRIu32, __func__, addr_str(remote_addr), status);
    gap_msg_t* msg = gap_msg_new(GAP_ADD_BLE_RESOlVING_LIST);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_remove_resolving_list_cb, bt_address remote_addr, SERVICE_BT_STATUS status)
{
    BT_LOGD("%s, addr :%s, status: %" PRIu32, __func__, addr_str(remote_addr), status);
    gap_msg_t* msg = gap_msg_new(GAP_REMOVE_BLE_RESOlVING_LIST);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_address_cb, bt_address ble_addr, SERVICE_BLE_ADDR_TYPE addr_type)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_ADDRESS);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, ble_addr, BT_ADDR_LENGTH);
    msg->event_data.addr_type = addr_type;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_phy_update_cb, bt_address remote_addr, SERVICE_BLE_PHY_TYPE tx_phy,
    SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_BT_STATUS status)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_PHY_UPDATE);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.phy_update.tx_phy = tx_phy;
    msg->event_data.data.phy_update.rx_phy = rx_phy;
    msg->event_data.status = status;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_irk_cb, bt_common_key irk, bt_address ble_addr, SERVICE_BLE_ADDR_TYPE addr_type)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_IRK);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, ble_addr, BT_ADDR_LENGTH);
    msg->event_data.addr_type = addr_type;
    memcpy(msg->event_data.data.irk, irk, BT_COMMON_KEY_SIZE);
    gap_send_message(msg);
}

static void adapter_ble_l2cap_connected_callback(bt_address remote_addr, struct SERVICE_BLE_L2CAP_CONN* conn)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_L2CAP_CONN_STATE);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.ble_l2cap_conn.state = BLE_LECAP_STATE_CONNECTED;
    msg->event_data.data.ble_l2cap_conn.cid = conn->cid;
    msg->event_data.data.ble_l2cap_conn.psm = conn->psm;
    msg->event_data.data.ble_l2cap_conn.mtu = conn->incoming.mtu;
    msg->event_data.data.ble_l2cap_conn.mps = conn->incoming.le_mps;
    gap_send_message(msg);
}

static void adapter_ble_l2cap_disconnected_callback(bt_address remote_addr, uint16_t cid)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_L2CAP_CONN_STATE);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.ble_l2cap_conn.state = BLE_LECAP_STATE_DISCONNECTED;
    msg->event_data.data.ble_l2cap_conn.cid = cid;
    gap_send_message(msg);
}

static void adapter_ble_packet_received_callback(bt_address remote_addr, uint16_t cid, uint8_t* packet, uint16_t packet_size)
{
    if (packet_size == 0) {
        return;
    }
    gap_msg_t* msg = gap_msg_new(GAP_BLE_PACKET_RECEIVED);
    if (!msg)
        return;

    uint8_t* payload = (uint8_t*)malloc(packet_size);
    memcpy(payload, packet, packet_size);
    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.data.ble_packet_receive.packet = payload;
    msg->event_data.data.ble_packet_receive.packet_size = packet_size;
    msg->event_data.data.ble_packet_receive.cid = cid;
    gap_send_message(msg);
}

static void adapter_ble_packet_sent_callback(bt_address remote_addr, uint16_t cid)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_PACKET_SENT);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.valueint1 = cid;
    gap_send_message(msg);
}

BTS_CB_X(gap_ble_connection_updated_cb, bt_address remote_addr, bt_status status, uint16_t connection_interval,
    uint16_t peripheral_latency, uint16_t supervision_timeout)
{
    gap_msg_t* msg = gap_msg_new(GAP_BLE_CONNECTION_UPDATE);
    if (!msg)
        return;

    memcpy(msg->event_data.bd_addr, remote_addr, BT_ADDR_LENGTH);
    msg->event_data.status = status;
    msg->event_data.data.ble_conn_update.connection_interval = connection_interval;
    msg->event_data.data.ble_conn_update.peripheral_latency = peripheral_latency;
    msg->event_data.data.ble_conn_update.supervision_timeout = supervision_timeout;
    gap_send_message(msg);
}

static GAP_CALLBACKS_S gap_callback = {
    .size = sizeof(GAP_CALLBACKS_S),
    .gap_stack_state_changed_cb = adapter_stack_state_changed_callback,
    .gap_received_remote_name_cb = adapter_received_remote_name_callback,
    .gap_device_found_cb = adapter_device_found_callback,
    .gap_discovery_state_changed_cb = adapter_discovery_state_changed_callback,
    .gap_pin_request_cb = adapter_pin_request_callback,
    .gap_ssp_request_cb = adapter_ssp_request_callback,
    .gap_bond_state_changed_cb = adapter_bond_state_changed_callback,
    .gap_acl_state_changed_cb = adapter_acl_state_changed_callback,
    .gap_ble_scan_result_cb = adapter_ble_scan_result_callback,
    .gap_ble_adv_started_cb = adapter_ble_adv_started_callback,
    .gap_ble_adv_stopped_cb = adapter_ble_adv_stopped_callback,
    .gap_link_connect_request_cb = adapter_link_connect_request_callback,
    .gap_bt_link_role_changed_cb = adapter_bt_link_role_changed_callback,
    .gap_scan_mode_changed_cb = adapter_scan_mode_changed_callback,
    .gap_link_mode_changed_cb = adapter_link_mode_changed_callback,
    .gap_link_policy_changed_cb = adapter_link_policy_changed_callback,
    .gap_hci_event_cb = adapter_hci_event_callback,
    .gap_transport_write_packet_cb = adapter_transport_write_packet_callback,
    .gap_update_br_link_key_cb = adapter_update_br_link_key_callback,
    .gap_delete_br_link_key_cb = adapter_delete_br_link_key_callback,
    .gap_pairing_request_cb = adapter_pairing_request_callback,
    .gap_service_discovered_cb = adapter_service_discovered_callback,
    .gap_link_encryption_state_cb = adapter_link_encryption_state_callback,
    .gap_smp_request_cb = adapter_smp_request_callback,
    .gap_update_ble_bonded_devices_cb = adapter_update_ble_bonded_devices_callback,
    .gap_ble_add_white_list_cb = adapter_ble_add_white_list_callback,
    .gap_ble_remove_white_list_cb = adapter_ble_remove_white_list_callback,
    .gap_ble_add_resolving_list_cb = adapter_ble_add_resolving_list_callback,
    .gap_ble_remove_resolving_list_cb = adapter_ble_remove_resolving_list_callback,
    .gap_ble_address_cb = adapter_ble_address_callback,
    .gap_ble_phy_update_cb = adapter_ble_phy_update_callback,
    .gap_ble_irk_cb = adapter_ble_irk_callback,
    .gap_ble_packet_received_cb = adapter_ble_packet_received_callback,
    .gap_ble_packet_sent_cb = adapter_ble_packet_sent_callback,
    .gap_ble_l2cap_disconnected_cb = adapter_ble_l2cap_disconnected_callback,
    .gap_ble_l2cap_connected_cb = adapter_ble_l2cap_connected_callback,
    .gap_ble_connection_updated_cb = adapter_ble_connection_updated_callback,
};

bt_result_code gap_init(bts_gap_callback_t* cb)
{
    create_config_folder();
    bts_gap_callbacks = cb;
    service_adapter_gap_init();
    service_adapter_gap_register_gap_callback(&gap_callback);
    bts_register_profile_process(BT_PROFILE_GAP_ID, &handle_msg_received);
    return BT_RESULT_SUCCESS;
}

void gap_cleanup(void)
{
    bts_gap_callbacks = NULL;
    service_adapter_gap_cleanup();
    bts_unregister_profile_process(BT_PROFILE_GAP_ID);
}

bt_result_code gap_enable()
{
    SERVICE_BT_STATUS ret = service_adapter_gap_enable();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap enable fail,ret:%" PRIu32, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_disable(bool normal_disable)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_disable(normal_disable);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap disable fail,ret:%" PRIu32, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

BTS_ADPT_X(bt_start_discovery, uint32_t timeout)
{
    BT_LOGD("%s: PERFORMANCE-GAP-BLUELET-DISCOVERY-START", __func__);
    BTS_ADPTX(start_device_discovery, timeout);
}

BTS_ADPT_X(bt_set_local_name, char* bt_name, uint8_t len)
{
    return gap_bt_update_name(bt_name, len);
}

BTS_ADPT_X(bt_set_local_address, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(set_local_address, device->addr);
}

BTS_ADPT_X(bt_get_local_address, bt_address addr)
{
    BTS_ADPTX(get_local_address, addr);
}

BTS_ADPT_X(bt_set_local_io_capability, bt_io_capability io_capability)
{
    return gap_bt_update_io_capability(io_capability);
}

BTS_ADPT_XX(bt_get_local_name, char*, void)
{
    static char name[BT_DEVICE_NAME_MAX_LEN];
    memset(name, 0, sizeof(name));
    char* ppname = name;

    SERVICE_BT_STATUS ret = service_adapter_gap_get_local_name(&ppname, BT_DEVICE_NAME_MAX_LEN);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return NULL;
    }
    return name;
}

BTS_ADPT_X(bt_set_local_device_class, uint32_t class_of_device)
{
    return gap_bt_update_device_class(class_of_device);
}

BTS_ADPT_XX(bt_get_local_device_class, uint32_t)
{
    uint32_t device_class = 0;
    SERVICE_BT_STATUS ret = service_adapter_gap_get_local_device_class(&device_class);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return 0;
    }
    return device_class;
}

BTS_ADPT_X(bt_get_remote_name, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(get_remote_name, device->addr);
}

BTS_ADPT_XX(bt_get_remote_services, int, bt_device_t* device, bt_uuid_t* service_list, uint8_t count_in)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(get_remote_services, device->addr, service_list, count_in);
}

BTS_ADPT_X(bt_reply_pair_request, bt_device_t* device, int accept)
{
    if (!device)
        return BT_RESULT_FAILED;
    service_adapter_gap_reply_pairing_request(device->addr, accept);
    return BT_RESULT_SUCCESS;
}

BTS_ADPT_X(bt_ssp_reply, spp_reply_data_t* reply_data)
{
    if (!reply_data)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ssp_reply, (SERVICE_SSP_REPLY_DATA_S*)reply_data);
}

BTS_ADPT_X(bt_create_bond, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BT_LOGD("%s: PERFORMANCE-GAP-BLUELET-BOND-START", __func__);
    BTS_ADPTX(create_bond, device->addr);
}

BTS_ADPT_X(bt_cancel_bond, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(cancel_bond, device->addr);
}

BTS_ADPT_X(bt_remove_bond, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(remove_bond, device->addr);
}

BTS_ADPT_XX(bt_get_bonded_devices, int, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    // get total number of paired devices
    if ((NULL == device_list) || (max_out == 0)) {
        ret = service_adapter_gap_get_bonded_devices(NULL, 0);
        return ret;
    }
    SERVICE_REMOTE_DEVICE_S* bonded_list = malloc(sizeof(SERVICE_REMOTE_DEVICE_S) * max_out);
    if (!bonded_list) {
        BT_LOGE("error, malloc bonded_list failed");
        return 0;
    }
    memset(bonded_list, 0, sizeof(SERVICE_REMOTE_DEVICE_S) * max_out);
    ret = service_adapter_gap_get_bonded_devices(bonded_list, max_out);
    for (int i = 0; i < ret; i++) {
        uint8_t* link_key = bonded_list[i].link_key;
        memcpy(device_list[i].addr, bonded_list[i].bd_addr, BT_ADDR_LENGTH);
        memcpy(device_list[i].name, bonded_list[i].bt_name, DEVICE_NAME_MAX_LEN + 1);
        memcpy(device_list[i].uuids, bonded_list[i].uuids, MAX_UUID_NUM);
        device_list[i].device_type = bonded_list[i].device_type;
        device_list[i].cod = bonded_list[i].cod;

        BT_LOGD("Linkkey: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", link_key[0], link_key[1], link_key[2], link_key[3], link_key[4], link_key[5],
            link_key[6], link_key[7], link_key[8], link_key[9], link_key[10], link_key[11],
            link_key[12], link_key[13], link_key[14], link_key[15]);
    }
    if (bonded_list)
        free(bonded_list);
    return ret;
}

BTS_ADPT_XX(bt_get_connected_devices, int, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    // get total number of paired devices
    if ((NULL == device_list) || (max_out == 0)) {
        ret = service_adapter_gap_get_connected_devices(NULL, 0);
        return ret;
    }
    SERVICE_REMOTE_DEVICE_S* connected_list = malloc(sizeof(SERVICE_REMOTE_DEVICE_S) * max_out);
    if (!connected_list) {
        BT_LOGE("error, malloc connected_list failed");
        return 0;
    }
    memset(connected_list, 0, sizeof(SERVICE_REMOTE_DEVICE_S) * max_out);
    ret = service_adapter_gap_get_connected_devices(connected_list, max_out);
    for (int i = 0; i < ret; i++) {
        memcpy(device_list[i].addr, connected_list[i].bd_addr, BT_ADDR_LENGTH);
        memcpy(device_list[i].name, connected_list[i].bt_name, DEVICE_NAME_MAX_LEN + 1);
    }
    if (connected_list)
        free(connected_list);
    return ret;
}

BTS_ADPT_XX(ble_get_bonded_devices, int, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    // get total number of paired devices
    if ((NULL == device_list) || (max_out == 0)) {
        ret = service_adapter_gap_ble_get_bonded_devices(NULL, 0);
        return ret;
    }
    SERVICE_REMOTE_BLE_DEVICE_S* bonded_list = malloc(sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);
    if (!bonded_list) {
        BT_LOGE("error, malloc bonded_list failed");
        return 0;
    }
    memset(bonded_list, 0, sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);
    ret = service_adapter_gap_ble_get_bonded_devices(bonded_list, MAX_PAIR_DEVICE);

    for (int i = 0; i < ret; i++) {
        memcpy(device_list[i].addr, bonded_list[i].bd_addr, BT_ADDR_LENGTH);
        device_list[i].addr_type = bonded_list[i].addr_type;
    }
    if (bonded_list)
        free(bonded_list);
    return ret;
}

BTS_ADPT_XX(ble_get_connected_devices, int, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    // get total number of paired devices
    if ((NULL == device_list) || (max_out == 0)) {
        ret = service_adapter_gap_ble_get_connected_devices(NULL, 0);
        return ret;
    }
    SERVICE_REMOTE_BLE_DEVICE_S* connected_list = malloc(sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);
    if (!connected_list) {
        BT_LOGE("error, malloc connected_list failed");
        return 0;
    }
    memset(connected_list, 0, sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);

    ret = service_adapter_gap_ble_get_connected_devices(connected_list, max_out);

    for (int i = 0; i < ret; i++) {
        memcpy(device_list[i].addr, connected_list[i].bd_addr, BT_ADDR_LENGTH);
        device_list[i].addr_type = connected_list[i].addr_type;
    }
    if (connected_list)
        free(connected_list);
    return ret;
}

BTS_ADPT_XX(ble_get_whitelist_devices, int, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    // get total number of paired devices
    if ((NULL == device_list) || (max_out == 0)) {
        ret = service_adapter_gap_ble_get_white_list_devices(NULL, 0);
        return ret;
    }
    SERVICE_REMOTE_BLE_DEVICE_S* whitelist_list = malloc(sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);
    if (!whitelist_list) {
        BT_LOGE("error, malloc whitelist_list failed");
        return 0;
    }
    memset(whitelist_list, 0, sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);

    ret = service_adapter_gap_ble_get_white_list_devices(whitelist_list, MAX_PAIR_DEVICE);

    for (int i = 0; i < ret; i++) {
        memcpy(device_list[i].addr, whitelist_list[i].bd_addr, BT_ADDR_LENGTH);
        device_list[i].addr_type = whitelist_list[i].addr_type;
    }
    if (whitelist_list)
        free(whitelist_list);
    return ret;
}

BTS_ADPT_XX(ble_get_resolvinglist_devices, int, bt_device_t* device_list, int max_out)
{
    int ret = 0;
    // get total number of paired devices
    if ((NULL == device_list) || (max_out == 0)) {
        ret = service_adapter_gap_ble_get_resolving_list_devices(NULL, 0);
        return ret;
    }
    SERVICE_REMOTE_BLE_DEVICE_S* resolvinglist_list = malloc(sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);
    if (!resolvinglist_list) {
        BT_LOGE("error, malloc resolvinglist_list failed");
        return 0;
    }
    memset(resolvinglist_list, 0, sizeof(SERVICE_REMOTE_BLE_DEVICE_S) * max_out);

    ret = service_adapter_gap_ble_get_resolving_list_devices(resolvinglist_list, MAX_PAIR_DEVICE);

    for (int i = 0; i < ret; i++) {
        memcpy(device_list[i].addr, resolvinglist_list[i].bd_addr, BT_ADDR_LENGTH);
        device_list[i].addr_type = resolvinglist_list[i].addr_type;
    }
    if (resolvinglist_list)
        free(resolvinglist_list);
    return ret;
}

BTS_ADPT_X(bt_set_scan_mode, bt_scan_mode scan_mode, bool bondable)
{
    return gap_bt_update_scan_mode(scan_mode, bondable);
}

BTS_ADPT_X(bt_stop_discovery)
{
    BTS_ADPTX(stop_device_discovery);
}

BTS_ADPT_X(bt_start_service_discovery, bt_device_t* device, bt_uuid_t uuid)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(start_service_discovery, device->addr, uuid);
}

BTS_ADPT_X(bt_stop_service_discovery, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(stop_service_discovery, device->addr);
}

BTS_ADPT_X(bt_set_link_role, bt_device_t* device, bt_link_role role)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(set_link_role, device->addr, role);
}

bt_result_code bts_disconnect_bt_link(bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    SERVICE_BT_STATUS ret = service_adapter_gap_disconnect_link(device->addr);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code bts_enable_ctkd_bonding(bool brkey_to_lekey, bool lekey_to_brkey)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_ble_enable_key_derivation(brkey_to_lekey, lekey_to_brkey);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

#ifdef HCI_VSC_COMMAND
/*VSC command*/
bt_result_code bts_send_hci_command(bt_hci_command_t* command, hci_command_complete_event event_type)
{
    BTS_ADPTX(send_hci_command_v1, (SERVICE_HCI_COMMAND_S*)command, event_type);
}

BTS_ADPT_X(ble_set_static_identity, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_set_static_identity, device->addr);
}

BTS_ADPT_X(ble_set_public_identity, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_set_public_identity, device->addr);
}

BTS_ADPT_X(ble_get_current_irk)
{
    BTS_ADPTX(ble_get_current_irk);
}

BTS_ADPT_X(ble_set_address, bt_device_t* device)
{
    BTS_ADPTX(ble_set_address, device->addr);
}

BTS_ADPT_X(ble_get_address)
{
    BTS_ADPTX(ble_get_address);
}

BTS_ADPT_X(ble_set_bonded_devices, ble_keys_t* bonded_device_list, uint8_t count_in)
{
    BTS_ADPTX(ble_set_bonded_devices, (SERVICE_BLE_KEYS_S*)bonded_device_list, count_in);
}

BTS_ADPT_X(ble_connect, ble_connect_params_t* conn_param)
{
    BTS_ADPTX(ble_connect, (SERVICE_LE_CONNECT_PARAMS_S*)conn_param);
}

BTS_ADPT_X(ble_disconnect, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_disconnect, device->addr);
}

BTS_ADPT_X(ble_smp_reply, spp_reply_data_t* reply_data)
{
    BTS_ADPTX(ble_smp_reply, (SERVICE_SSP_REPLY_DATA_S*)reply_data);
}

BTS_ADPT_X(ble_add_white_list, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_add_white_list, device->addr);
}

BTS_ADPT_X(ble_remove_white_list, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_remove_white_list, device->addr);
}

BTS_ADPT_X(ble_add_resolving_list, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_add_resolving_list, device->addr);
}

BTS_ADPT_X(ble_remove_resolving_list, bt_device_t* device)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_remove_resolving_list, device->addr);
}

BTS_ADPT_X(ble_set_phy, bt_device_t* device, ble_phy_type tx_phy, ble_phy_type rx_phy)
{
    if (!device)
        return BT_RESULT_FAILED;
    BTS_ADPTX(ble_set_phy, device->addr, tx_phy, rx_phy);
}
bt_result_code bts_ble_listen_l2cap_channel(uint8_t psm, ble_l2cap_config_option_t* opt)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_ble_add_public_channel(psm, (SERVICE_L2CAP_CONFIG_OPTION_S*)opt);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}
bt_result_code bts_ble_send_packet(bt_device_t* device, uint16_t cid, uint8_t* packet, uint16_t packet_size)
{
    if (!device)
        return BT_RESULT_FAILED;
    SERVICE_BT_STATUS ret = service_adapter_gap_ble_send_packet(device->addr, cid, packet, packet_size);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

BTS_ADPT_X(enter_bluetooth_test_mode, test_mode mode)
{
    BTS_ADPTX(enter_bluetooth_test_mode, mode);
}

BTS_ADPT_X(bt_set_page_scan_parameters, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    BTS_ADPTX(set_page_scan_parameters, (SERVICE_BT_SCAN_TYPE)scan_type, scan_interval, scan_window);
}

BTS_ADPT_X(bt_set_inquiry_scan_parameters, bt_scan_type scan_type, uint16_t scan_interval, uint16_t scan_window)
{
    BTS_ADPTX(set_inquiry_scan_parameters, (SERVICE_BT_SCAN_TYPE)scan_type, scan_interval, scan_window);
}

BTS_ADPT_X(bt_reply_link_request, BD_ADDR remote_addr, bool accept)
{
    BTS_ADPTX(reply_link_request, remote_addr, accept);
}

BTS_ADPT_X(bt_set_afh_channel_classification, bt_afh_radio_channel_info_t* channels, uint16_t number)
{
    SERVICE_BT_STATUS ret = service_adapter_set_afh_channel_classification((SERVICE_RADIO_CHANNEL_INFO_S*)channels, number);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("%s, ret:%" PRIu32, __func__, ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

#ifdef CONFIG_BLUETOOTH_A2DP_I2S_OFFLOAD
uint16_t gap_get_acl_handle(void)
{
    return g_conn_handle;
}
#endif
