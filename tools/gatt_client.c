/****************************************************************************
 * frameworks/bluetooth/samples/test_gatts.c
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
#define LOG_TAG "bttool_gattc"

#include <debug.h>
#include <nuttx/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bt_tools.h"
#include "btm_gatt_client.h"
#include "btm_le_scan.h"
#include "btm_manager.h"
#include "log.h"

#define THROUGHTPUT_HORIZON 5

typedef struct {
    struct list_node node;
    void* handle;
    uint16_t gatt_mtu;
    bt_address remote_address;
} gattc_device_t;

static btm_interface_t* manager = NULL;
static btm_gatt_client_interface_t* gattc_interface = NULL;
static volatile uint16_t throughtput_cursor = 1;
static struct list_node gattc_device_list = LIST_INITIAL_VALUE(gattc_device_list);
static void* scan_handle;

static gattc_device_t* find_gattc_device(bt_address remote_address)
{
    gattc_device_t* device;
    list_for_every_entry(&gattc_device_list, device, gattc_device_t, node)
    {
        if (!memcmp(device->remote_address, remote_address, sizeof(bt_address))) {
            return device;
        }
    }
    return NULL;
}

static gattc_device_t* find_gattc_device2(void* handle)
{
    gattc_device_t* device;
    list_for_every_entry(&gattc_device_list, device, gattc_device_t, node)
    {
        if (device->handle == handle) {
            return device;
        }
    }
    return NULL;
}

static gattc_device_t* add_gattc_device(bt_address remote_address)
{
    gattc_device_t* device = (gattc_device_t*)malloc(sizeof(gattc_device_t));
    if (!device) {
        BT_LOGE("malloc device fail");
        return NULL;
    }

    memset(device, 0, sizeof(gattc_device_t));
    memcpy(device->remote_address, remote_address, sizeof(bt_address));
    list_add_tail(&gattc_device_list, &device->node);
    return device;
}

static bool remove_gattc_device(gattc_device_t* device)
{
    list_delete(&device->node);
    free(device);
    return true;
}

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

static void on_client_connection_state_changed_callback(void* handle, bt_address remote_addr, profile_state_t state)
{
    BT_LOGD("%s, state:%d", __func__, state);
    if (state == SERVICE_PROFILE_DISCONNECTED) {
        gattc_device_t* device = find_gattc_device(remote_addr);
        remove_gattc_device(device);
    }
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

static void on_client_service_discovered_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint16_t size)
{
    BT_LOGD("%s", __func__);
    gatt_display_service(element, size);
}

static void on_client_read_result_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_status_t status)
{
    BT_LOGD("%s, size:%d", __func__, size);
    for (int i = 0; i < size; i++) {
        BT_LOGD("value[%d]:%d", i, value[i]);
    }
}

static void on_client_write_result_callback(void* handle, bt_address remote_addr, gatt_element_t* element, gatt_status_t status)
{
    BT_LOGD("%s, status:%d", __func__, status);
    throughtput_cursor--;
}

static void on_client_nofity_request_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size)
{
    BT_LOGD("%s, size:%d", __func__, size);
}

static void on_client_rssi_read_callback(void* handle, bt_address remote_addr, int32_t rssi, gatt_status_t status)
{
    BT_LOGD("%s rssi:%d", __func__, rssi);
}

static void on_client_phy_read_callback(void* handle, bt_address remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s, tx_phy:%d, rx_phy:%d", __func__, tx, rx);
}

static void on_client_phy_update_callback(void* handle, bt_address remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s, tx_phy:%d, rx_phy:%d", __func__, tx, rx);
}

static void on_client_mtu_changed_callback(void* handle, bt_address remote_addr, uint32_t mtu)
{
    BT_LOGD("%s, mtu:%d", __func__, mtu);
    gattc_device_t* device = find_gattc_device(remote_addr);
    if (!device) {
        BT_LOGD("device not found");
        return;
    }
    device->gatt_mtu = mtu;
}

static void test_client_throughtout_write(void* client_handle, uint32_t times, uint16_t mtu)
{
    BT_LOGD("mtu:%d, times:%d", mtu, times);
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    unsigned long id = 0;
    BT_LOGD("please input characteristic  id");
    scanf("%ld", &id);
    element.id = id;
    element.properties = GATT_PROPERTY_WRITE;
    BT_LOGD("input id:%d", element.id);

    int msg_counter = 1;
    throughtput_cursor = 1;
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
        bt_result_code code = gattc_interface->write_request(client_handle, &element, payload, mtu);
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

static int gattc_connect(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("connect remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        device = add_gattc_device(remote_address);
    }
    bt_result_code ret = gattc_interface->connect(&device->handle, device->remote_address, &client_cb);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, connect  ret: %d", ret);
    }
    return 0;
}

static int gattc_disconnect(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 1) {
        return -1;
    }
    find_gattc_device2(handle);
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("disconnect remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    bt_result_code ret = gattc_interface->disconnect(device->handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, disconnect  ret: %d", ret);
    }
    return 0;
}

static int gattc_read_rssi(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("read_rssi remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    bt_result_code ret = gattc_interface->read_rssi(device->handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read_rssi  ret: %d", ret);
    }
    return 0;
}

static int gattc_read_phy(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("read_phy remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    bt_result_code ret = gattc_interface->read_phy(device->handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read_phy  ret: %d", ret);
    }
    return 0;
}

static int gattc_update_mtu(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    int mtu = atoi(argv[1]);
    BT_LOGD("update_mtu remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], mtu:%d", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], mtu);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    bt_result_code ret = gattc_interface->update_mtu(device->handle, mtu);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, update_mtu  ret: %d", ret);
    }
    return 0;
}

static int gattc_update_phy(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 3) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    int tx = atoi(argv[1]);
    int rx = atoi(argv[2]);
    BT_LOGD("update_phy(0:1M, 1:2M, 2:1M_Coded) remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], tx:%d, rx:%d", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], tx, rx);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    bt_result_code ret = gattc_interface->update_phy(device->handle, tx, rx);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, update_phy  ret: %d", ret);
    }
    return 0;
}

static int gattc_update_connection_parameter(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 6) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    uint32_t min_interval = atoi(argv[1]);
    uint32_t max_interval = atoi(argv[2]);
    uint32_t latency = atoi(argv[3]);
    uint32_t timeout = atoi(argv[4]);
    uint32_t min_connection_event_length = atoi(argv[5]);
    uint32_t max_connection_event_length = atoi(argv[6]);
    BT_LOGD("min_interval: %d, max_interval: %d, latency: %d, timeout:%d, min_connection_event_length:%d, max_connection_event_length%d", min_interval, max_interval, latency, timeout, min_connection_event_length, max_connection_event_length);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    bt_result_code ret = gattc_interface->update_connection_parameter(device->handle, min_interval, max_interval, latency, timeout, min_connection_event_length, max_connection_event_length);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, update_connection_parameter  ret: %d", ret);
    }
    return 0;
}

static int gattc_discover_services(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("discover_services remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5]);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    bt_uuid_t uuid;
    memset(uuid, 0, sizeof(bt_uuid_t));
    bt_result_code ret = gattc_interface->discover_services(device->handle, uuid);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, discover_services  ret: %d", ret);
    }
    return 0;
}

static int gattc_read_request(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    unsigned long id = atoi(argv[1]);
    BT_LOGD("read_request remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], id:%ld", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], id);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    element.id = id;
    bt_result_code ret = gattc_interface->read_request(device->handle, &element);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read_request  ret: %d", ret);
    }
    return 0;
}

static int gattc_write_request(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    unsigned long id = atoi(argv[1]);
    BT_LOGD("write_request remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], id:%ld", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], id);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    element.id = id;
    uint8_t value[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    bt_result_code ret = gattc_interface->write_request(device->handle, &element, value, sizeof(value) / sizeof(value[0]));
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, write_request  ret: %d", ret);
    }
    return 0;
}

static int gattc_register_notification(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    unsigned long id = atoi(argv[1]);
    BT_LOGD("register_notification remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], id:%ld", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], id);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    element.id = id;
    element.type = CHARACTERISTIC;
    element.properties = GATT_PROPERTY_NOTIFY;
    bt_result_code ret = gattc_interface->register_notification(device->handle, &element, false);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read_request  ret: %d", ret);
    }
    return 0;
}

static int gattc_throughtout_write(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    uint32_t times = atoi(argv[1]);
    BT_LOGD("throughtout_write remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x], times:%d", remote_address[0], remote_address[1], remote_address[2], remote_address[3], remote_address[4], remote_address[5], times);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return -1;
    }
    test_client_throughtout_write(device, times, device->gatt_mtu);
    return 0;
}

static int gattc_start_scan(void* handle, int argc, char** argv)
{

    btm_le_scan_interface_t* scan_interface = get_btm_lescan_interface(manager);
    if (!scan_interface) {
        BT_LOGE("fail, get_btm_lescan_interface");
        return -1;
    }
    scan_params_t scan_params = {
        .scan_interval = 300,
        .scan_window = 500,
        .scan_phy = BLE_1M_PHY,
    };
    bt_result_code ret = scan_interface->start_scan(&scan_handle, NULL, &scan_params, &scan_cb);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, start_scan  ret: %d", ret);
        return -1;
    }
    return 0;
}

static int gattc_stop_scan(void* handle, int argc, char** argv)
{

    btm_le_scan_interface_t* scan_interface = get_btm_lescan_interface(manager);
    if (!scan_interface) {
        BT_LOGE("fail, get_btm_lescan_interface");
        return -1;
    }
    bt_result_code ret = scan_interface->stop_scan(scan_handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, start_scan  ret: %d", ret);
        return -1;
    }
    return 0;
}

static bt_command_t g_gattc_tables[] = {
    { "connect", gattc_connect, "\"gatt client connect  :<address>\"" },
    { "disconnect", gattc_disconnect, "\"gatt disconnec :<address>t\"" },
    { "read_rssi", gattc_read_rssi, "\"gatt client read rssi  :<address>\"" },
    { "read_phy", gattc_read_phy, "\"gatt client read phy  :<address>\"" },
    { "update_mtu", gattc_update_mtu, "\"gatt client update mtu  :<address> <mtu>\"" },
    { "update_phy", gattc_update_phy, "\"gatt client update phy(0: 1M, 1: 2M, 2: LE_Coded)  :<address> <tx phy> <rx phy>\"" },
    { "update_conn", gattc_update_connection_parameter, "\"gatt client update connect  parameter  :<address> <min_interval>  <max_interval> <latency> <timeout> <min_connection_event_length> <max_connection_event_length>  \"" },
    { "discover_services", gattc_discover_services, "\"gatt client discover all services <address> \"" },
    { "read_request", gattc_read_request, "\"gatt client read request: :<address> <charateristic id>\"" },
    { "write_request", gattc_write_request, "\"gatt client write request: :<address> <charateristic id>\"" },
    { "register_notification", gattc_register_notification, "\"gatt client register notification: :<address> <charateristic id>\"" },
    { "throughtout_write", gattc_throughtout_write, "\"gatt client throughtout write  :<address> <times>\"" },
    { "start_scan", gattc_start_scan, "\"gatt start le scan  \"" },
    { "stop_scan", gattc_stop_scan, "\"gatt stop le scan  \"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_gattc_tables); i++) {
        printf("\t%-8s\t%s\n", g_gattc_tables[i].cmd, g_gattc_tables[i].help);
    }
}

static struct option gattc_options[] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
};

int gatt_client_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (gattc_interface == NULL) {
        manager = get_bt_manager_interface();
        gattc_interface = get_btm_gattc_interface(manager);
    }

    while ((opt = getopt_long(argc, argv, "h", gattc_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_gattc_tables); i++) {
            if (strncmp(g_gattc_tables[i].cmd, argv[1], strlen(argv[1])) == 0) {
                if (g_gattc_tables[i].func) {
                    ret = g_gattc_tables[i].func(handle, argc - 2, &argv[2]);
                }
            }
        }
    }

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[1]);
        usage();
    }

    return 0;
}
