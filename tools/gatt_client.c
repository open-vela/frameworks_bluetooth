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

#define THROUGHTPUT_HORIZON 2

typedef struct {
    struct list_node node;
    void* handle;
    uint16_t gatt_mtu;
    bt_address remote_address;
} gattc_device_t;

typedef struct {
    struct list_node node;
    bt_address remote_addr;
} gattc_scan_result_t;

static btm_interface_t* manager = NULL;
static btm_gatt_client_interface_t* gattc_interface = NULL;
static volatile uint16_t throughtput_cursor = 1;
static struct list_node gattc_device_list = LIST_INITIAL_VALUE(gattc_device_list);
static struct list_node gattc_scan_result_list = LIST_INITIAL_VALUE(gattc_scan_result_list);
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
    if (!device) {
        return false;
    }
    list_delete(&device->node);
    free(device);
    return true;
}

static bool find_scan_device(const bt_address remote_address)
{
    gattc_scan_result_t* device;
    list_for_every_entry(&gattc_scan_result_list, device, gattc_scan_result_t, node)
    {
        if (!memcmp(device->remote_addr, remote_address, sizeof(bt_address))) {
            return true;
        }
    }
    return false;
}

static void add_scan_device(const bt_address remote_address)
{
    gattc_scan_result_t* device = (gattc_scan_result_t*)malloc(sizeof(gattc_scan_result_t));
    if (!device) {
        BT_LOGE("malloc gattc_scan_result_t fail");
        return;
    }

    memset(device, 0, sizeof(gattc_scan_result_t));
    memcpy(device->remote_addr, remote_address, sizeof(bt_address));
    list_add_tail(&gattc_scan_result_list, &device->node);
}

static void clear_scan_devices(void)
{
    gattc_scan_result_t* device;
    gattc_scan_result_t* device_next;
    list_for_every_entry_safe(&gattc_scan_result_list, device, device_next, gattc_scan_result_t, node)
    {
        list_delete(&device->node);
        free(device);
    }
}

static void on_scan_started_callback(void* handle)
{
    BT_LOGD("%s", __func__);
}

static void on_scan_stopped_callback(void* handle)
{
    BT_LOGD("%s", __func__);
    clear_scan_devices();
}

static void on_scan_failed_callback(void* handle, int error)
{
    BT_LOGD("%s err:%d", __func__, error);
    clear_scan_devices();
}

static void on_scan_result_callback(void* handle, const scan_result_t* result)
{
    if (find_scan_device(result->remote_addr)) {
        return;
    }
    add_scan_device(result->remote_addr);
    BT_LOGD("%s addr:%s, addr_type:%d, device_type:%d, evt_type:%d, rssi:%d", __func__, addr_str(((scan_result_t*)result)->remote_addr), result->addr_type, result->device_type, result->evt_type, result->rssi);
    // BT_HEXDUMP(result->adv_data, result->length);
}

static char* profile_state_to_str(profile_connection_state state)
{
    switch (state) {
    case PROFILE_DISCONNECTED: {
        return "profile disconnected";
    }
    case PROFILE_CONNECTING: {
        return "profile connecting";
    }
    case PROFILE_CONNECTED: {
        return "profile connected";
    }
    case PROFILE_DISCONNECTING: {
        return "profile disconnecting";
    }
    default: {
        return "unknown state";
    }
    }
}

static void on_client_connection_state_changed_callback(void* handle, bt_address remote_addr, profile_connection_state state)
{
    BT_LOGD("%s, addr:%s, state:%s", __func__, addr_str(remote_addr), profile_state_to_str(state));
    if (state == PROFILE_DISCONNECTED) {
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
        case GATT_PRIMARY_SERVICE:
            BT_LOGD(">[%lu][PRI]", item->id);
            break;
        case GATT_SECONDARY_SERVICE:
            BT_LOGD(">[%lu][SND]", item->id);
            break;
        case GATT_INCLUDED_SERVICE:
            BT_LOGD(">  [%lu][INC]", item->id);
            break;
        case GATT_CHARACTERISTIC:
            BT_LOGD(">  [%lu][CHR]", item->id);
            break;
        case GATT_DESCRIPTOR:
            BT_LOGD(">    [%lu][DES]", item->id);
            break;
        }
        BT_LOGD("[PROP:%lu", item->properties);
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
    BT_LOGD("%s, addr:%s", __func__, addr_str(remote_addr));
    gatt_display_service(element, size);
}

static void on_client_read_result_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size, gatt_status status)
{
    BT_LOGD("%s,status:%d, addr:%s, size:%d, value:", __func__, status, addr_str(remote_addr), size);
    BT_HEXDUMP(value, size);
    gatt_display_service(element, 1);
}

static void on_client_write_result_callback(void* handle, bt_address remote_addr, gatt_element_t* element, gatt_status status)
{
    BT_LOGD("%s,status:%d, addr:%s", __func__, status, addr_str(remote_addr));
    gatt_display_service(element, 1);
    throughtput_cursor--;
}

static void on_client_nofity_request_callback(void* handle, bt_address remote_addr, gatt_element_t* element, uint8_t* value, uint16_t size)
{
    BT_LOGD("%s,addr:%s, size:%d, value:", __func__, addr_str(remote_addr), size);
    BT_HEXDUMP(value, size);
    gatt_display_service(element, 1);
}

static void on_client_rssi_read_callback(void* handle, bt_address remote_addr, int32_t rssi, gatt_status status)
{
    BT_LOGD("%s,status:%d, addr:%s, rssi:%lu", __func__, status, addr_str(remote_addr), rssi);
}

static void on_client_phy_read_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    BT_LOGD("%s,  addr:%s, tx:%d, rx:%d", __func__, addr_str(remote_addr), tx, rx);
}

static void on_client_phy_update_callback(void* handle, bt_address remote_addr, ble_phy_type tx, ble_phy_type rx)
{
    BT_LOGD("%s,  addr:%s, tx:%d, rx:%d", __func__, addr_str(remote_addr), tx, rx);
}

static void on_client_mtu_changed_callback(void* handle, bt_address remote_addr, uint32_t mtu)
{
    BT_LOGD("%s, addr:%s, mtu:%lu", __func__, addr_str(remote_addr), mtu);
    gattc_device_t* device = find_gattc_device(remote_addr);
    if (!device) {
        BT_LOGD("device not found");
        return;
    }
    device->gatt_mtu = mtu;
}

static void test_client_throughtout_write(void* client_handle, uint32_t id, uint32_t times, uint16_t mtu)
{
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    element.id = id;
    element.properties = GATT_ATT_PROPERTY_WRITE;

    int msg_counter = 1;
    throughtput_cursor = 1;
    uint8_t* payload = (uint8_t*)malloc(sizeof(uint8_t) * mtu);
    if (!payload) {
        BT_LOGD("malloc payload fail");
        return;
    }
    for (int i = 0; i < times; i++) {
        while (throughtput_cursor >= THROUGHTPUT_HORIZON) {
            usleep(500);
        }
        memset(payload, 1, mtu);
        payload[0] = (msg_counter >> 24) & 0xFF;
        payload[1] = (msg_counter >> 16) & 0xFF;
        payload[2] = (msg_counter >> 8) & 0xFF;
        payload[3] = msg_counter & 0xFF;
        bt_result_code code = gattc_interface->write_request(client_handle, &element, payload, mtu);
        throughtput_cursor++;
        BT_LOGD("write_request times:%d, throughtput_cursor:%u", i, throughtput_cursor);
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
    BT_LOGD("%s, remote_addr:%s", __func__, addr_str(remote_address));
    gattc_device_t* device = find_gattc_device(remote_address);
    if (device) {
        BT_LOGE("fail,  please disconnect device, try again");
        return 0;
    }
    device = add_gattc_device(remote_address);
    bt_result_code ret = gattc_interface->connect(&device->handle, device->remote_address, &client_cb);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, connect  ret: %d", ret);
        remove_gattc_device(device);
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
    BT_LOGD("%s, remote_addr:%s", __func__, addr_str(remote_address));
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }
    bt_result_code ret = gattc_interface->disconnect(device->handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, disconnect  ret: %d", ret);
        remove_gattc_device(device);
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
    BT_LOGD("%s, remote_addr:%s", __func__, addr_str(remote_address));
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
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
    BT_LOGD("%s, remote_addr:%s", __func__, addr_str(remote_address));
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
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
    BT_LOGD("%s, remote_addr:%s, mtu:%d", __func__, addr_str(remote_address), mtu);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
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
    BT_LOGD("%s, remote_addr:%s, tx:%d, rx:%d", __func__, addr_str(remote_address), tx, rx);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }
    bt_result_code ret = gattc_interface->update_phy(device->handle, tx, rx);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, update_phy  ret: %d", ret);
    }
    return 0;
}

static int gattc_update_connection_parameter(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 7) {
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
    BT_LOGD("min_interval: %lu, max_interval: %lu, latency: %lu, timeout:%lu, min_connection_event_length:%lu, max_connection_event_length%lu", min_interval, max_interval, latency, timeout, min_connection_event_length, max_connection_event_length);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
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
    BT_LOGD("%s, remote_addr:%s", __func__, addr_str(remote_address));
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
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
    uint32_t id = atoi(argv[1]);
    BT_LOGD("%s, remote_addr:%s, id:%lu", __func__, addr_str(remote_address), id);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
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
    if (!gattc_interface || argc < 3) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    uint32_t id = atoi(argv[1]);

    size_t size = strlen(argv[2]);
    uint8_t* payload = (uint8_t*)malloc(size);
    memcpy(payload, argv[2], size);

    BT_LOGD("%s, remote_addr:%s, id:%lu, size:%d, value", __func__, addr_str(remote_address), id, size);
    BT_HEXDUMP(payload, size);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        free(payload);
        return 0;
    }
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    element.id = id;
    bt_result_code ret = gattc_interface->write_request(device->handle, &element, payload, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, write_request  ret: %d", ret);
    }
    return 0;
}

static int gattc_enable_cccd(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    uint32_t id = atoi(argv[1]);
    BT_LOGD("%s, remote_addr:%s, id:%lu", __func__, addr_str(remote_address), id);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    element.id = id;
    element.type = GATT_CHARACTERISTIC;
    element.properties = GATT_ATT_PROPERTY_NOTIFY;
    bt_result_code ret = gattc_interface->register_notification(device->handle, &element, true);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read_request  ret: %d", ret);
    }
    return 0;
}

static int gattc_disable_cccd(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 2) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    uint32_t id = atoi(argv[1]);
    BT_LOGD("%s, remote_addr:%s, id:%lu", __func__, addr_str(remote_address), id);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }
    gatt_element_t element;
    memset(&element, 0, sizeof(element));
    element.id = id;
    element.type = GATT_CHARACTERISTIC;
    element.properties = GATT_ATT_PROPERTY_NOTIFY;
    bt_result_code ret = gattc_interface->register_notification(device->handle, &element, false);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, read_request  ret: %d", ret);
    }
    return 0;
}

static int gattc_throughtout_write(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 3) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    uint32_t times = atoi(argv[1]);
    uint32_t id = atoi(argv[2]);
    BT_LOGD("throughtout_write, characteristic id:%lu, remote_addr:[%s], times:%lu", id, addr_str(remote_address), times);
    gattc_device_t* device = find_gattc_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }

    test_client_throughtout_write(device->handle, id, times, device->gatt_mtu);
    return 0;
}

static int gattc_start_scan(void* handle, int argc, char** argv)
{
    if (!gattc_interface || argc < 6) {
        return -1;
    }

    btm_le_scan_interface_t* scan_interface = get_btm_lescan_interface(manager);
    if (!scan_interface) {
        BT_LOGE("fail, get_btm_lescan_interface");
        return 0;
    }

    uint8_t enable_filter = atoi(argv[0]);
    bt_address remote_address;
    str2ba(argv[1], remote_address);
    size_t size = strlen(argv[2]);
    ble_scan_filter_t* filter_params = NULL;
    if (enable_filter) {
        filter_params = malloc(sizeof(ble_scan_filter_t) + size);
        memset(filter_params, 0, sizeof(ble_scan_filter_t) + size);
        memcpy(filter_params->bd_addr, remote_address, sizeof(bt_address));
        filter_params->length = size;
        memcpy(filter_params->adv_data_mask, argv[2], size);
        BT_LOGD("%s, enable filter params remote_addr:%s, mask:", __func__, addr_str(remote_address));
        BT_HEXDUMP(filter_params->adv_data_mask, size);
    } else {
        BT_LOGD(" filter  disabled");
    }

    scan_params_t scan_params;
    memset(&scan_params, 0, sizeof(scan_params_t));
    int interval = atoi(argv[3]);
    int window = atoi(argv[4]);
    uint8_t phy = atoi(argv[5]);
    scan_params.scan_interval = interval;
    scan_params.scan_window = window;
    scan_params.scan_phy = phy;
    BT_LOGD("%s, scan params interval:%d, widow:%d, phy:%d", __func__, interval, window, phy);
    if (phy > 2) {
        BT_LOGE("fail, invalid phy:%d", phy);
        return 0;
    }

    bt_result_code ret = scan_interface->start_scan(&scan_handle, filter_params, &scan_params, &scan_cb);

    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, start_scan  ret: %d", ret);
        return 0;
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
        return 0;
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
    { "read_request", gattc_read_request, "\"gatt client read request:<address> <charateristic id>\"" },
    { "write_request", gattc_write_request, "\"gatt client write request :<address> <charateristic id> <payload>\"" },
    { "enable_cccd", gattc_enable_cccd, "\"gatt client enable cccd :<address> <charateristic id>\"" },
    { "disable_cccd", gattc_disable_cccd, "\"gatt client disable cccd:<address> <charateristic id>\"" },
    { "throughtout_write", gattc_throughtout_write, "\"gatt client throughtout write  :<address> <times> <write charateristic id>\"" },
    { "start_scan", gattc_start_scan, "\"gatt start le scan  {phy(0: 1M, 1: 2M, 2: LE_Coded)}: <enable filter 0:disable 1:enable ><filter_addr> <filter_mask>  <scan_interval> <scan_windows> <scan_phy> \"" },
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
