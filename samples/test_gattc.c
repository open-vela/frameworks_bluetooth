/****************************************************************************
 * frameworks/bluetooth/samples/test_gattc.c
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
#define LOG_TAG "btsample_gattc"

#include <stdio.h>
#include <stdlib.h>

#include "btm_gatt_client.h"
#include "btm_le_scan.h"
#include "btm_manager.h"
#include "log.h"

static bt_address remote_address;
static void* client_handle;
static btm_gatt_client_interface_t* client_interface = NULL;
static uint32_t gatt_mtu = 20;
#define THROUGHTPUT_HORIZON 5
static volatile uint16_t throughtput_cursor = 1;

static void on_scan_started_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void on_scan_stopped_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void on_scan_failed_callback(void* handle, int error)
{
    BT_LOGD("%s err:%d", __func__, error);
}

static void on_scan_result_callback(void* handle, const scan_result_t* result)
{
    BT_LOGD("%s addr:addr:[%02x:%02x:%02x:%02x:%02x:%02x], addr_type:%d, device_type:%d, evt_type:%d", __func__, result->remote_addr[0],
        result->remote_addr[1], result->remote_addr[2], result->remote_addr[3], result->remote_addr[4], result->remote_addr[5],
        result->addr_type, result->device_type, result->evt_type);
}

static void on_client_connection_state_changed_callback(void* handle, bt_address remote_addr, profile_connection_state state)
{
    BT_LOGD("%s, state:%d", __func__, state);
}

static void gatt_display_service(gatt_element_t* elements, uint16_t size)
{
    gatt_element_t* item = elements;
    gatt_element_t* item_end = elements + size;

    while (item < item_end) {
        switch (item->type) {
        case GATT_PRIMARY_SERVICE:
            BT_LOGD(">[%d][PRI]", item->id);
            break;
        case GATT_SECONDARY_SERVICE:
            BT_LOGD(">[%d][SND]", item->id);
            break;
        case GATT_INCLUDED_SERVICE:
            BT_LOGD(">  [%d][INC]", item->id);
            break;
        case GATT_CHARACTERISTIC:
            BT_LOGD(">  [%d][CHR]", item->id);
            break;
        case GATT_DESCRIPTOR:
            BT_LOGD(">    [%d][DES]", item->id);
            break;
        }
        BT_LOGD("[PROP:%d", item->properties);
        if (item->properties) {
            BT_LOGD(",");
            if (item->properties & GATT_ATT_PROPERTY_READ) {
                BT_LOGD("R");
            }
            if (item->properties & GATT_ATT_PROPERTY_WRITE_NO_RESPONSE) {
                BT_LOGD("Wn");
            }
            if (item->properties & GATT_ATT_PROPERTY_WRITE) {
                BT_LOGD("W");
            }
            if (item->properties & GATT_ATT_PROPERTY_NOTIFY) {
                BT_LOGD("N");
            }
            if (item->properties & GATT_ATT_PROPERTY_INDICATE) {
                BT_LOGD("I");
            }
        }
        BT_LOGD("]");

        BT_LOGD("[0x%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x]\r\n",
            item->uuid[15], item->uuid[14], item->uuid[13], item->uuid[12],
            item->uuid[11], item->uuid[10], item->uuid[9], item->uuid[8],
            item->uuid[7], item->uuid[6], item->uuid[5], item->uuid[4],
            item->uuid[3], item->uuid[2], item->uuid[1], item->uuid[0]);
        item++;
    }
    BT_LOGD(">");
}

static void on_client_service_discovered_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint16_t size)
{
    BT_LOGD("%s", __func__);
    gatt_display_service(element, size);
}

static void on_client_read_result_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_status status)
{
    BT_LOGD("%s, size:%d", __func__, size);
    for (int i = 0; i < size; i++) {
        BT_LOGD("value[%d]:%d", i, value[i]);
    }
}

static void on_client_write_result_callback(void* handle, bt_address remote_addr, gatt_element_t* element, gatt_status status)
{
    BT_LOGD("%s, status:%d", __func__, status);
    throughtput_cursor--;
}

static void on_client_nofity_request_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size)
{
    BT_LOGD("%s, size:%d", __func__, size);
}

static void on_client_rssi_read_callback(void* handle, bt_address remote_addr, int32_t rssi, gatt_status status)
{
    BT_LOGD("%s rssi:%d", __func__, rssi);
}

static void on_client_phy_read_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    BT_LOGD("%s, tx_phy:%d, rx_phy:%d", __func__, tx, rx);
}

static void on_client_phy_update_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    BT_LOGD("%s, tx_phy:%d, rx_phy:%d", __func__, tx, rx);
}

static void on_client_mtu_changed_callback(void* handle, bt_address remote_addr, uint32_t mtu)
{
    BT_LOGD("%s, mtu:%d", __func__, mtu);
    gatt_mtu = mtu;
}

static void test_client_throughtout_write(uint32_t times, uint16_t mtu)
{
    BT_LOGD("mtu:%d, times:%d", mtu, times);
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    unsigned long id = 0;
    BT_LOGD("please input characteristic  id");
    scanf("%ld", &id);
    element.id = id;
    element.properties = GATT_ATT_PROPERTY_WRITE;
    BT_LOGD("input id:%ld", element.id);
    int msg_counter = 1;
    uint8_t* payload = (uint8_t*)malloc(sizeof(uint8_t) * mtu);
    if (!payload) {
        BT_LOGD("malloc payload fail");
        return;
    }
    for (int i = 0; i < times; i++) {
        while (throughtput_cursor >= THROUGHTPUT_HORIZON) {
            usleep(50);
        }
        memset(payload, 1, mtu);
        payload[0] = (msg_counter >> 24) & 0xFF;
        payload[1] = (msg_counter >> 16) & 0xFF;
        payload[2] = (msg_counter >> 8) & 0xFF;
        payload[3] = msg_counter & 0xFF;
        bt_result_code code = client_interface->write_request(client_handle, &element, payload, mtu);
        throughtput_cursor++;
        BT_LOGD("write_request times:%d, throughtput_cursor:%d", i, throughtput_cursor);
        if (code != BT_RESULT_SUCCESS) {
            BT_LOGE("fail, write_request ret:%d", code);
            return;
        }
        msg_counter++;
    }
    free(payload);
    BT_LOGD("%s done", __func__);
}

static void manager_state_changed_callback(btm_bt_state state)
{
    BT_LOGD("%s, state:%d", __func__, state);
}

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
};

static btm_le_scan_callbacks scan_cb = {
    .le_scan_started_cb = on_scan_started_callback,
    .le_scan_failed_cb = on_scan_failed_callback,
    .le_scan_result_cb = on_scan_result_callback,
    .le_scan_stopped_cb = on_scan_stopped_callback,
};

static btm_gatt_client_callbacks client_cb = {
    .gattc_connection_state_changed_cb = on_client_connection_state_changed_callback,
    .gattc_service_discovered_cb = on_client_service_discovered_callback,
    .gattc_read_result_cb = on_client_read_result_callback,
    .gattc_write_result_cb = on_client_write_result_callback,
    .gattc_nofity_request_cb = on_client_nofity_request_callback,
    .gattc_rssi_read_cb = on_client_rssi_read_callback,
    .gattc_phy_read_cb = on_client_phy_read_callback,
    .gattc_phy_update_cb = on_client_phy_update_callback,
    .gattc_mtu_changed_cb = on_client_mtu_changed_callback,
};

int main(int argc, FAR char* argv[])
{
    void* manager_handle;
    btm_interface_t* manager = get_bt_manager_interface();
    if (!manager) {
        BT_LOGE("fail, get_bt_manager_interface");
        return -1;
    }
    BT_LOGD(" bt manager init ...");
    manager->init(&manager_handle, &mgt_cb);
    BT_LOGD(" bt manager inited");
    manager->enable(manager_handle);
    BT_LOGD(" bt manager enabled");

    void* scan_handle;
    bool exit = false;
    while (!exit) {
        char ch = getchar();
        switch (ch) {
        case 'a': {
            bt_address remote_address;
            BT_LOGD("please input addr ");
            scanf("%x:%x:%x:%x:%x:%x", &remote_address[0], &remote_address[1], &remote_address[2], &remote_address[3], &remote_address[4], &remote_address[5]);
            BT_LOGD("remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
            client_interface->connect(&client_handle, remote_address, &client_cb);
            break;
        }
        case 'b': {
            client_interface->disconnect(client_handle);
            break;
        }
        case 'c': {
            client_interface->read_rssi(client_handle);
            break;
        }
        case 'd': {
            client_interface->read_phy(client_handle);
            break;
        }
        case 'e': {
            uint16_t mtu = 20;
            BT_LOGD("please input mtu");
            scanf("%ld", &mtu);
            BT_LOGD("mtu:%ld", mtu);
            client_interface->update_mtu(client_handle, mtu);
            break;
        }
        case 'f': {
            client_interface->update_phy(client_handle, BLE_1M_PHY_TYPE, BLE_1M_PHY_TYPE);
            break;
        }
        case 'g': {
            uint32_t min_interval;
            uint32_t max_interval;
            uint32_t latency;
            uint32_t timeout;
            uint32_t min_connection_event_length;
            uint32_t max_connection_event_length;
            BT_LOGD("please input: min_interval max_interval latency timeout min_connection_event_length max_connection_event_length");
            scanf("%d %d %d %d %d %d", &min_interval, &max_interval, &latency, &timeout, &min_connection_event_length, &max_connection_event_length);
            BT_LOGD("min_interval: %d, max_interval: %d, latency: %d, timeout:%d, min_connection_event_length:%d, max_connection_event_length%d", min_interval, max_interval, latency, timeout, min_connection_event_length, max_connection_event_length);
            client_interface->update_connection_parameter(client_handle, min_interval, max_interval, latency, timeout, min_connection_event_length, max_connection_event_length);
            break;
        }
        case 'i': {
            bt_uuid_t uuid;
            memset(uuid, 0, sizeof(bt_uuid_t));
            client_interface->discover_services(client_handle, uuid);
            break;
        }
        case 'j': {
            gatt_element_t element;
            memset(&element, 0, sizeof(element));
            unsigned long id = 0;
            BT_LOGD("please input id");
            scanf("%ld", &id);
            element.id = id;
            BT_LOGD("input id:%ld", element.id);
            client_interface->read_request(client_handle, &element);
            break;
        }
        case 'k': {
            gatt_element_t element;
            memset(&element, 0, sizeof(element));
            unsigned long id = 0;
            BT_LOGD("please input id");
            scanf("%ld", &id);
            element.id = id;
            element.properties = GATT_ATT_PROPERTY_WRITE;
            BT_LOGD("input id:%ld", element.id);
            uint8_t value[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
            client_interface->write_request(client_handle, &element, value, sizeof(value) / sizeof(value[0]));
            break;
        }
        case 'l': {
            gatt_element_t element;
            memset(&element, 0, sizeof(element));
            unsigned long id = 0;
            BT_LOGD("please input id");
            scanf("%ld", &id);
            element.id = id;
            element.type = GATT_CHARACTERISTIC;
            element.properties = GATT_ATT_PROPERTY_NOTIFY;
            BT_LOGD("input id:%ld", element.id);
            client_interface->register_notification(client_handle, &element, true);
            break;
        }
        case 'm': {
            gatt_element_t element;
            memset(&element, 0, sizeof(element));
            unsigned long id = 0;
            BT_LOGD("please input id");
            scanf("%ld", &id);
            element.id = id;
            element.type = GATT_CHARACTERISTIC;
            element.properties = GATT_ATT_PROPERTY_NOTIFY;
            BT_LOGD("input id:%ld", element.id);
            client_interface->register_notification(client_handle, &element, false);
            break;
        }
        case 't': {
            BT_LOGD("please input times, and let's do throughtout job");
            uint32_t times;
            scanf("%d", &times);
            test_client_throughtout_write(times, gatt_mtu);
            BT_LOGD("throughtout notify ...");
            break;
        }
        case 'u': {
            btm_le_scan_interface_t* scan_interface = get_btm_lescan_interface(manager);
            if (!scan_interface) {
                BT_LOGE("fail, get_btm_lescan_interface");
                break;
            }
            scan_params_t scan_params = {
                .scan_interval = 300,
                .scan_window = 500,
                .scan_phy = BLE_1M_PHY_TYPE,
            };
            scan_interface->start_scan(&scan_handle, NULL, &scan_params, &scan_cb);
            break;
        }
        case 'v': {
            btm_le_scan_interface_t* scan_interface = get_btm_lescan_interface(manager);
            if (!scan_interface) {
                BT_LOGE("fail, get_btm_lescan_interface");
                break;
            }
            scan_interface->stop_scan(scan_handle);
            break;
        }
        case 'q': {
            BT_LOGD("exit...");
            exit = true;
            break;
        }
        default: {
            BT_LOGD("invalid %c", ch);
            break;
        }
        }
    }
    BT_LOGE("exit ...");
    return 0;
}