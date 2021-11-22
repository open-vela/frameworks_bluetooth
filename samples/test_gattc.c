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

#include <stdio.h>
#include <stdlib.h>

#include "btm_gatt_client.h"
#include "btm_le_scan.h"
#include "btm_manager.h"
#include "log.h"

#define LOG_TAG "btsample_gattc"

static bd_addr_t remote_address;

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

static void on_client_connection_state_changed_callback(void* handle, bd_addr_t remote_addr, profile_state_t state)
{
    BT_LOGD("%s", __func__);
}

static void gatt_display_service(gatt_element_t* elements, uint16_t size)
{
    gatt_element_t* item = elements;
    gatt_element_t* item_end = elements + size;

    while (item < item_end) {
        switch (item->type) {
        case PRIMARY_SERVICE:
            BT_LOGD(">[%d][PRI]", item->id);
            break;
        case SECONDARY_SERVICE:
            BT_LOGD(">[%d][SND]", item->id);
            break;
        case INCLUDED_SERVICE:
            BT_LOGD(">  [%d][INC]", item->id);
            break;
        case CHARACTERISTIC:
            BT_LOGD(">  [%d][CHR]", item->id);
            break;
        case DESCRIPTOR:
            BT_LOGD(">    [%d][DES]", item->id);
            break;
        }
        BT_LOGD("[PROP:%d", item->properties);
        if (item->properties) {
            BT_LOGD(",");
            if (item->properties & GATT_PROPERTY_READ) {
                BT_LOGD("R");
            }
            if (item->properties & GATT_PROPERTY_WRITE_NO_RESPONSE) {
                BT_LOGD("Wn");
            }
            if (item->properties & GATT_PROPERTY_WRITE) {
                BT_LOGD("W");
            }
            if (item->properties & GATT_PROPERTY_NOTIFY) {
                BT_LOGD("N");
            }
            if (item->properties & GATT_PROPERTY_INDICATE) {
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

static void on_client_service_discovered_callback(void* handle, bd_addr_t remote_addr, gatt_element_t* element, uint16_t size)
{
    BT_LOGD("%s", __func__);
    gatt_display_service(element, size);
}

static void on_client_read_result_callback(void* handle, bd_addr_t remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_status_t status)
{
    BT_LOGD("%s, size:%d", __func__, size);
    for (int i = 0; i < size; i++) {
        BT_LOGD("value[%d]:%d", i, value[i]);
    }
}

static void on_client_write_result_callback(void* handle, bd_addr_t remote_addr, gatt_element_t* element, gatt_status_t status)
{
    BT_LOGD("%s", __func__);
}

static void on_client_nofity_request_callback(void* handle, bd_addr_t remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size)
{
    BT_LOGD("%s", __func__);
}

static void on_client_rssi_read_callback(void* handle, bd_addr_t remote_addr, int32_t rssi, gatt_status_t status)
{
    BT_LOGD("%s rssi:%d", __func__, rssi);
}

static void on_client_phy_read_callback(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s", __func__);
}
static void on_client_phy_update_callback(void* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s", __func__);
}

static void on_client_mtu_changed_callback(void* handle, bd_addr_t remote_addr, uint32_t mtu)
{
    BT_LOGD("%s, mtu:%d", __func__, mtu);
}

static void manager_init_status_changed_callback(bt_result_code status)
{
    BT_LOGD("%s, state:%d", __func__, status);
}

static void manager_state_changed_callback(bt_manager_bt_state state)
{
    BT_LOGD("%s, state:%d", __func__, state);
}

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
    //.init_status_changed_callback_cb = manager_init_status_changed_callback,
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

    btm_le_scan_interface_t* scan_interface = get_btm_lescan_interface(manager);
    if (!scan_interface) {
        BT_LOGE("fail, get_btm_lescan_interface");
        return -1;
    }

    btm_le_scan_callbacks scan_cb = {
        .le_scan_started_cb = on_scan_started_callback,
        .le_scan_failed_cb = on_scan_failed_callback,
        .le_scan_result_cb = on_scan_result_callback,
        .le_scan_stopped_cb = on_scan_stopped_callback,
    };

    void* scan_handle;
    bool exit = false;
    while (!exit) {
        char ch = ch = getchar();
        switch (ch) {
        case 'a': {
            scan_params_t filter = {
                .scan_interval = 300,
                .scan_window = 500,
                .scan_phy = BLE_1M_PHY,
            };
            scan_interface->start_scan(&scan_handle, &filter, NULL, &scan_cb);
            break;
        }
        case 'b': {
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
    BT_LOGD("### quit start/stop scan, come into gatt client");

    btm_gatt_client_callbacks client_cb = {
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

    btm_gatt_client_interface_t* client_interface = get_btm_gattc_interface(manager);
    if (!client_interface) {
        BT_LOGE("fail, get_btm_gattc_interface");
        return -1;
    }

    void* client_handle;
    exit = false;
    while (!exit) {
        char ch = getchar();
        switch (ch) {
        case 'a': {
            bd_addr_t remote_address;
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
            client_interface->update_mtu(client_handle, 30);
            break;
        }
        case 'f': {
            client_interface->update_phy(client_handle, BLE_1M_PHY, BLE_1M_PHY);
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
            element.properties = GATT_PROPERTY_WRITE;
            BT_LOGD("input id:%ld", element.id);
            static uint8_t value[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
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
            element.type = CHARACTERISTIC;
            element.properties = GATT_PROPERTY_NOTIFY;
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
            element.type = CHARACTERISTIC;
            element.properties = GATT_PROPERTY_NOTIFY;
            BT_LOGD("input id:%ld", element.id);
            client_interface->register_notification(client_handle, &element, false);
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